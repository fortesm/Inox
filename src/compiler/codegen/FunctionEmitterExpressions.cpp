// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: expression types and the lowering of expressions.

#include "FunctionEmitter.h"
#include "../support/CharLiteral.h"

#include "AstQueries.h"
#include "ExceptionAbi.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "MathBuiltins.h"
#include "Operators.h"
#include "../exceptions/ExceptionTypes.h"
#include "../support/Platform.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace inox::compiler::codegen::detail {

// P-C stage 1: a module Const is read from the semantic result, already
// resolved, never re-parsed here.
const semantic::ConstantValue* FunctionEmitter::resolvedConstant(const ast::IdentifierExpression& identifier) const
{
    const semantic::Symbol* symbol = semantics_.symbolOf(identifier);
    if (symbol == nullptr || symbol->kind != semantic::SymbolKind::Constant) {
        return nullptr;
    }
    return semantics_.constantValueOf(*symbol);
}

// The LLVM type of an expression's value: the LLVM type of the type the
// analysis gave it (a Char is its code point, an i64); Free and the file
// calls without a result are void; None is a null pointer.
std::string FunctionEmitter::expressionLlvmType(const ast::Expression& expression) const
{
    if (expression.kind() == ast::AstNodeKind::CallExpression) {
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (isPointerBuiltin(call) &&
            equalsIgnoreCase(static_cast<const ast::IdentifierExpression&>(call.callee()).name(), "Free")) {
            return "void";
        }
        if (isFileCall(call)) {
            return fileCallLlvmType(call);
        }
    }
    const TypeRef type = typeOf(expression);
    if (type != nullptr && type->isNone()) {
        return "ptr";
    }
    return valueLlvmType(type);
}

// Natural(X), Integer(X) or Int64(X) applied to one i64 value.
bool FunctionEmitter::isNaturalConversion(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 1) {
        return false;
    }
    const TypeRef target = conversionTarget(call);
    if (target == nullptr) {
        return false;
    }
    return expressionLlvmType(*call.arguments().front()) == "i64";
}

// ADR-0013: `if C then A else B`. Only the chosen branch is evaluated, so
// this is control flow, never an LLVM `select`. A branch may end in a block
// other than the one it started in (an `invoke` continuation, a nested
// conditional's merge), so each branch jumps to its own tail block and the
// phi names the tails, whose predecessors are known.
std::string FunctionEmitter::emitConditional(const ast::ConditionalExpression& expression)
{
    const std::string label = std::to_string(nextLabel_++);
    const std::string type = expressionLlvmType(expression);
    const std::string condition = emitExpression(expression.condition());
    output_ << "  br i1 " << condition << ", label %condthen" << label
            << ", label %condelse" << label << "\n\n";

    output_ << "condthen" << label << ":\n";
    const std::string thenValue = emitExpression(expression.thenValue());
    output_ << "  br label %condthentail" << label << "\n\n";
    output_ << "condthentail" << label << ":\n";
    output_ << "  br label %condend" << label << "\n\n";

    output_ << "condelse" << label << ":\n";
    const std::string elseValue = emitExpression(expression.elseValue());
    output_ << "  br label %condelsetail" << label << "\n\n";
    output_ << "condelsetail" << label << ":\n";
    output_ << "  br label %condend" << label << "\n\n";

    output_ << "condend" << label << ":\n";
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << result << " = phi " << type << " [ " << thenValue
            << ", %condthentail" << label << " ], [ " << elseValue
            << ", %condelsetail" << label << " ]\n";
    return result;
}

// Change uint64: a conversion into or out of UInt64 checks that the value is
// one of the target. A UInt64 above Int64.Max is a negative i64, so the
// signed range check with a low bound of at least 0 rejects it too.
std::string FunctionEmitter::emitUnsignedConversion(const std::string& value, TypeRef source, TypeRef target)
{
    if (isUInt64Type(source) && isUInt64Type(target)) {
        return value;
    }
    if (isUInt64Type(target)) {
        // From a signed type: a negative value is out of range; Natural and
        // the narrow unsigned types are never negative.
        if (!source->isSigned() || source == semantics_.types().naturalType()) {
            return value;
        }
        return emitRangeCheck(value, IntegerRange{0, std::numeric_limits<std::int64_t>::max()}, 8);
    }
    const std::int64_t high = target->maxValue();
    return emitRangeCheck(value, IntegerRange{std::max<std::int64_t>(target->minValue(), 0), high}, 8);
}

TypeRef FunctionEmitter::typeOf(const ast::Expression& expression) const
{
    return semantics_.typeOf(expression);
}

// The integer type of a conversion `T(X)` lowered as a range check: Natural,
// Integer (Int64) or a fixed width narrower than 64 bits; else null.
TypeRef FunctionEmitter::conversionTarget(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 1) {
        return nullptr;
    }
    const TypeRef target =
        semantics_.types().lookup(static_cast<const ast::IdentifierExpression&>(call.callee()).name());
    const bool lowered = target != nullptr && (target == semantics_.types().naturalType() ||
                                               target == semantics_.types().int64Type() ||
                                               target->isNarrowInteger() || isUInt64Type(target) ||
                                               target->isEnum() || target->isRange() || target->isChar());
    return lowered ? target : nullptr;
}

// An arithmetic result whose semantic type is a narrow fixed-width integer
// must stay in that type's range (CANON-8: overflow is an error).
bool FunctionEmitter::isRangeCheckedArithmetic(const ast::Expression& expression)
{
    if (expression.kind() == ast::AstNodeKind::UnaryExpression) {
        return static_cast<const ast::UnaryExpression&>(expression).op() == ast::UnaryOperator::Minus;
    }
    if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
        return false;
    }
    switch (static_cast<const ast::BinaryExpression&>(expression).op()) {
    case ast::BinaryOperator::Add:
    case ast::BinaryOperator::Subtract:
    case ast::BinaryOperator::Multiply:
    case ast::BinaryOperator::Power:
    case ast::BinaryOperator::IntegerDivide:
    case ast::BinaryOperator::Modulo:
    case ast::BinaryOperator::ShiftLeft:
    case ast::BinaryOperator::ShiftRight:
    case ast::BinaryOperator::BitAnd:
    case ast::BinaryOperator::BitOr:
    case ast::BinaryOperator::BitXor:
        return true;
    default:
        return false;
    }
}

std::string FunctionEmitter::emitRangeCheck(const std::string& value, IntegerRange range, int faultKind)
{
    const std::string checked = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << checked << " = call i64 @__inox_range_i64(i64 " << value << ", i64 "
            << range.low << ", i64 " << range.high << ", i32 " << faultKind << ")\n";
    return checked;
}

std::string FunctionEmitter::emitExpression(const ast::Expression& expression)
{
    std::string value = emitExpressionValue(expression);
    if (statementRunsRoutine_ && isBorrowedRead(expression)) {
        value = protectBorrowed(expression, value);
    }
    if (isRangeCheckedArithmetic(expression)) {
        if (const auto range = narrowIntegerRange(typeOf(expression))) {
            value = emitRangeCheck(value, *range, 1);
        }
    }
    // CANON-13 Range (Q1): a value stored into a Range is checked (kind 11).
    if (const TypeRef range = semantics_.rangeCheck(expression)) {
        value = emitRangeCheck(value, IntegerRange{range->minValue(), range->maxValue()}, 11);
    }
    if (!semantics_.needsNaturalCheck(expression)) {
        return value;
    }
    const std::string checked = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << checked << " = call i64 @__inox_natural_i64(i64 " << value << ")\n";
    return checked;
}

std::string FunctionEmitter::emitExpressionValue(const ast::Expression& expression)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression: {
        const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
        if (literal.literalKind() == ast::LiteralKind::Integer) {
            return llvmIntegerLiteral(literal.value());
        }
        if (literal.literalKind() == ast::LiteralKind::Float) {
            return std::string(literal.value());
        }
        if (literal.literalKind() == ast::LiteralKind::Boolean) {
            return equalsIgnoreCase(literal.value(), "true") ? "1" : "0";
        }
        if (literal.literalKind() == ast::LiteralKind::String) {
            return emitStringValue(literal.value());
        }
        if (literal.literalKind() == ast::LiteralKind::Char) {
            return std::to_string(support::decodeCharLiteral(literal.value()).code);
        }
        break;
    }
    case ast::AstNodeKind::IdentifierExpression: {
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
        const std::string normalizedName = normalize(identifier.name());
        const auto local = locals_.find(normalizedName);
        if (local != locals_.end()) {
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            if (local->second.llvmType.empty()) {
                throw CodegenError("LLVM emission found local value with unknown LLVM type");
            }
            emitWithGuard(normalizedName);
            output_ << "  " << result << " = load " << local->second.llvmType
                    << ", ptr " << local->second.slot << '\n';
            return result;
        }
        const auto parameter = parameters_.find(normalizedName);
        if (parameter != parameters_.end()) {
            return parameter->second;
        }
        if (const FunctionSignature* routine = calledRoutine(identifier)) {
            if (!routine->parameters.empty()) {
                throw CodegenError("function requires arguments; use parentheses only when passing arguments: " + identifier.name());
            }
            if (routine->llvmReturnType == "void") {
                throw CodegenError("void function call cannot be used as an expression");
            }
            // Through emitUserCall: an `invoke` inside try (the call may
            // raise) and the ownership of a String result.
            return emitUserCall(*routine, {}, true);
        }
        // None: an empty Link or an absent owner (safe-pointers).
        if (const TypeRef type = typeOf(expression); type != nullptr && type->isNone()) {
            return "zeroinitializer";
        }
        if (const semantic::ConstantValue* constant = resolvedConstant(identifier)) {
            if (constant->kind == semantic::ConstantValue::Kind::Boolean) {
                return constant->boolean ? "1" : "0";
            }
            if (constant->kind == semantic::ConstantValue::Kind::String) {
                return emitStringValue(constant->text);
            }
            return std::to_string(constant->integer);
        }
        break;
    }
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        if (isStringConcatenation(binary)) {
            return emitConcatenation(binary);
        }
        if (binary.op() == ast::BinaryOperator::In) {
            return emitMembership(binary);
        }
        if (const TypeRef leftSet = typeOf(binary.left()); leftSet != nullptr && leftSet->isSet()) {
            return emitSetComparison(binary);
        }
        const std::string leftType = expressionLlvmType(binary.left());
        const std::string rightType = expressionLlvmType(binary.right());
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        const std::string left = emitExpression(binary.left());
        std::string right = emitExpression(binary.right());

        // CANON-8: the shift count of a fixed-width type is 0..width-1.
        if (binary.op() == ast::BinaryOperator::ShiftLeft ||
            binary.op() == ast::BinaryOperator::ShiftRight) {
            if (const auto range = narrowIntegerRange(typeOf(expression))) {
                int bits = 0;
                for (std::int64_t span = range->high; span > 0; span >>= 1) {
                    ++bits;
                }
                if (range->low < 0) {
                    ++bits;
                }
                right = emitRangeCheck(right, IntegerRange{0, bits - 1}, 3);
            }
        }

        // CANON-14: `=` and `#` compare the UTF-8 bytes of two Strings.
        if (isStringType(typeOf(binary.left())) &&
            (binary.op() == ast::BinaryOperator::Equal || binary.op() == ast::BinaryOperator::NotEqual)) {
            requireRuntime(RuntimeFunction::StringEqual);
            const std::string equal = binary.op() == ast::BinaryOperator::Equal
                                          ? result
                                          : "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << equal << " = call zeroext i1 @__inox_str_equal(ptr " << left << ", ptr " << right
                    << ")\n";
            if (binary.op() == ast::BinaryOperator::NotEqual) {
                output_ << "  " << result << " = xor i1 " << equal << ", true\n";
            }
            return result;
        }
        // CANON-14 (v3.56): `<` `>` `<=` `>=` order Strings byte by byte.
        if (isStringType(typeOf(binary.left()))) {
            if (const std::string predicate = llvmComparisonPredicate(binary.op()); !predicate.empty()) {
                requireRuntime(RuntimeFunction::StringCompare);
                const std::string order = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << order << " = call i32 @__inox_str_compare(ptr " << left << ", ptr " << right
                        << ")\n";
                output_ << "  " << result << " = icmp " << predicate << " i32 " << order << ", 0\n";
                return result;
            }
        }

        if (isFloatLlvmType(leftType) || isFloatLlvmType(rightType)) {
            if (const std::string predicate = fcmpPredicate(binary.op()); !predicate.empty()) {
                output_ << "  " << result << " = fcmp " << predicate << ' ' << leftType
                        << ' ' << left << ", " << right << '\n';
                return result;
            }
            if (binary.op() == ast::BinaryOperator::Power) {
                output_ << "  " << result << " = call double @llvm.pow.f64(double "
                        << left << ", double " << right << ")\n";
                return result;
            }
            output_ << "  " << result << " = " << llvmFloatOperation(binary.op()) << ' '
                    << leftType << ' ' << left << ", " << right << '\n';
            return result;
        }

        if (const std::string predicate = llvmComparisonPredicate(binary.op());
            !predicate.empty()) {
            // Every integer type is carried as i64 (signed compare). Bool
            // is i1 and ordered false < true (v3.57): compared unsigned,
            // since a signed i1 reads true as -1.
            const bool isBool = leftType == "i1";
            // A UInt64 is ordered by its unsigned value (change uint64).
            const bool isUnsigned =
                isBool || isUInt64Type(typeOf(binary.left())) || isUInt64Type(typeOf(binary.right()));
            std::string boolPredicate = predicate;
            if (isUnsigned && boolPredicate.front() == 's') {
                boolPredicate.front() = 'u';
            }
            output_ << "  " << result << " = icmp " << (isUnsigned ? boolPredicate : predicate) << ' '
                    << (isBool ? "i1" : "i64") << ' ' << left << ", " << right << '\n';
        } else if (isUInt64Type(typeOf(expression)) && !llvmUnsignedHelper(binary.op()).empty()) {
            requireUnsignedHelpers();
            output_ << "  " << result << " = call i64 @" << llvmUnsignedHelper(binary.op()) << "(i64 " << left
                    << ", i64 " << right << ")\n";
        } else if (const std::string operation = llvmBooleanOperation(binary.op());
                   !operation.empty()) {
            output_ << "  " << result << " = " << operation << " i1 "
                    << left << ", " << right << '\n';
        } else if (binary.op() == ast::BinaryOperator::Power) {
            output_ << "  " << result << " = call i64 @__inox_ipow_i64(i64 "
                    << left << ", i64 " << right << ")\n";
        } else if (const std::string helper = checkedIntegerHelper(binary.op()); !helper.empty()) {
            output_ << "  " << result << " = call i64 @" << helper << "(i64 "
                    << left << ", i64 " << right << ")\n";
        } else {
            output_ << "  " << result << " = " << llvmOperation(binary.op()) << " i64 "
                    << left << ", " << right << '\n';
        }
        return result;
    }
    case ast::AstNodeKind::UnaryExpression: {
        const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
        if (unary.op() == ast::UnaryOperator::Not) {
            const std::string operand = emitExpression(unary.operand());
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << result << " = xor i1 " << operand << ", true\n";
            return result;
        }
        if (unary.op() == ast::UnaryOperator::Plus) {
            return emitExpression(unary.operand());
        }
        if (unary.op() == ast::UnaryOperator::BitNot) {
            // Two's-complement NOT in the operand's width: for an unsigned
            // narrow type that is `x xor (2^n - 1)`, which stays in range.
            const std::string operand = emitExpression(unary.operand());
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            std::int64_t mask = -1;
            if (const auto range = narrowIntegerRange(typeOf(unary.operand()));
                range && range->low == 0) {
                mask = range->high;
            }
            output_ << "  " << result << " = xor i64 " << operand << ", " << mask << '\n';
            return result;
        }
        if (unary.op() == ast::UnaryOperator::Minus) {
            const std::string operandType = expressionLlvmType(unary.operand());
            if (unary.operand().kind() == ast::AstNodeKind::LiteralExpression) {
                const auto& literal = static_cast<const ast::LiteralExpression&>(unary.operand());
                if (literal.literalKind() == ast::LiteralKind::Integer) {
                    return "-" + llvmIntegerLiteral(literal.value());
                }
                if (literal.literalKind() == ast::LiteralKind::Float) {
                    return "-" + std::string(literal.value());
                }
            }
            const std::string operand = emitExpression(unary.operand());
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            if (isFloatLlvmType(operandType)) {
                output_ << "  " << result << " = fsub " << operandType << " -0.0, " << operand << '\n';
            } else {
                output_ << "  " << result << " = call i64 @__inox_neg_i64(i64 " << operand << ")\n";
            }
            return result;
        }
        break;
    }
    case ast::AstNodeKind::ConditionalExpression:
        return emitConditional(static_cast<const ast::ConditionalExpression&>(expression));
    case ast::AstNodeKind::CallExpression: {
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (isMemberAccessCall(call.callee())) {
            return emitMethodCall(call, true);
        }
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
            break;
        }

        const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
        if (const semantic::StringFunction* function = semantics_.stringCall(call)) {
            return emitStringCall(call, *function);
        }
        if (callee.name() == "__index") {
            const ElementAddress element = emitElementAddress(call);
            return loadElement(element);
        }
        if (callee.name() == "__slice") {
            return emitSlice(call);
        }
        if (isOrdinalBuiltin(call)) {
            return emitOrdinalBuiltin(call);
        }
        if (semantics_.fillUnit(call) != nullptr) {
            return emitFill(call);
        }
        if (isSetCall(call)) {
            return emitSetCall(call);
        }
        if (isPointerBuiltin(call)) {
            if (equalsIgnoreCase(callee.name(), "Free")) {
                throw CodegenError("void function call cannot be used as an expression");
            }
            return emitPointerBuiltin(call);
        }
        if (const TypeRef structType = constructedStruct(call)) {
            return emitConstruction(call, structType);
        }
        if (call.arguments().size() == 1 && isSliceType(typeOf(*call.arguments()[0])) &&
            (equalsIgnoreCase(callee.name(), "Low") || equalsIgnoreCase(callee.name(), "High") ||
             equalsIgnoreCase(callee.name(), "Length"))) {
            const SliceParts slice = extractSlice(emitExpression(*call.arguments()[0]));
            if (equalsIgnoreCase(callee.name(), "Low")) {
                return slice.low;
            }
            if (equalsIgnoreCase(callee.name(), "High")) {
                return slice.high;
            }
            const std::string span = "%tmp" + std::to_string(nextTemporary_++);
            const std::string length = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << span << " = sub i64 " << slice.high << ", " << slice.low << '\n';
            output_ << "  " << length << " = add i64 " << span << ", 1\n";
            return length;
        }
        if (const auto bound = arrayBoundCall(call)) {
            return std::to_string(*bound);
        }
        if (equalsIgnoreCase(callee.name(), "__member")) {
            try {
                const FieldAddress field = emitMemberAddress(call);
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << result << " = load " << field.llvmType
                        << ", ptr " << field.pointer << '\n';
                return result;
            } catch (const CodegenError&) {
                return emitNoArgumentMethodCall(call, true);
            }
        }

        // CANON-24: `old X` and a loop variant, evaluated where they stand.
        if (callee.name() == "__pure" && call.arguments().size() == 1) {
            return emitExpression(*call.arguments().front());
        }

        if (equalsIgnoreCase(callee.name(), "Abs") && call.arguments().size() == 1 &&
            expressionLlvmType(*call.arguments().front()) == "i64") {
            const std::string value = emitExpression(*call.arguments().front());
            if (isUInt64Type(typeOf(*call.arguments().front()))) {
                return value;  // never negative
            }
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << result << " = call i64 @__inox_abs_i64(i64 " << value << ")\n";
            return result;
        }

        if (isFileCall(call)) {
            if (fileCallLlvmType(call) == "void") {
                throw CodegenError("void function call cannot be used as an expression");
            }
            return emitFileCall(call);
        }

        if (isMathBuiltin(callee.name())) {
            return emitMathBuiltinCall(callee.name(), call.arguments());
        }

        // String(C) (change char-values): the one-character String.
        if (call.arguments().size() == 1 && equalsIgnoreCase(callee.name(), "String") &&
            calledRoutine(callee) == nullptr) {
            const TypeRef source = typeOf(*call.arguments().front());
            const std::string value = emitExpression(*call.arguments().front());
            return source != nullptr && source->isChar() ? emitCharString(value) : value;
        }

        // CANON-8: Natural(X) checks X >= 0 at run time (fault kind 7);
        // Integer(X) and Int64(X) of an integer value are the identity,
        // because Natural shares Integer's i64 representation.
        if (isNaturalConversion(call)) {
            const std::string value = emitExpression(*call.arguments().front());
            const TypeRef target = conversionTarget(call);
            const TypeRef source = typeOf(*call.arguments().front());
            if (isUInt64Type(source) || isUInt64Type(target)) {
                return emitUnsignedConversion(value, source, target);
            }
            // Char(I) (change char-values): a Unicode scalar, 0..$10FFFF
            // without the surrogates (kind 8); TLetter(X): one of its values.
            if (target->isChar()) {
                if (source == target || (source != nullptr && source->isChar() && !target->isCharRange())) {
                    return value;
                }
                return emitCharCheck(value, target);
            }
            // TSuit(I): an ordinal 0..N-1 (CANON-13 Q2: kind 8); TMonth(I):
            // one of its values.
            if (target->isEnum() || target->isRange()) {
                if (source == target) {
                    return value;
                }
                return emitRangeCheck(value, IntegerRange{target->minValue(), target->maxValue()}, 8);
            }
            if (const auto range = narrowIntegerRange(target)) {
                return emitRangeCheck(value, *range, 8);
            }
            if (target != semantics_.types().naturalType()) {
                return value;
            }
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << result << " = call i64 @__inox_natural_i64(i64 " << value << ")\n";
            return result;
        }

        const FunctionSignature* routine = calledRoutine(callee);
        if (routine == nullptr) {
            break;
        }
        if (routine->llvmReturnType == "void") {
            throw CodegenError("void function call cannot be used as an expression");
        }
        if (call.arguments().size() != routine->parameters.size()) {
            throw CodegenUnsupported("unsupported call argument count in function: " + function_.name());
        }

        const std::vector<std::string> arguments = emitArguments(*routine, call.arguments(), 0);
        const std::string result = emitUserCall(*routine, arguments, true);
        emitInvariantChecks(call, true);
        return result;
    }
    default:
        break;
    }

    throw CodegenUnsupported("unsupported expression in function: " + function_.name());
}

std::string FunctionEmitter::emitMathBuiltinCall(std::string_view name, const std::vector<ast::ExpressionPtr>& arguments)
{
    std::vector<std::string> values;
    values.reserve(arguments.size());
    for (const auto& argument : arguments) {
        values.push_back(emitExpression(*argument));
    }
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);

    const auto unaryScale = [&](double factor) {
        output_ << "  " << result << " = fmul double " << values.at(0) << ", " << std::scientific << factor << std::defaultfloat << "\n";
        return result;
    };

    if (equalsIgnoreCase(name, "RadToDeg")) return unaryScale(57.295779513082320876798154814105);
    if (equalsIgnoreCase(name, "DegToRad")) return unaryScale(0.017453292519943295769236907684886);
    if (equalsIgnoreCase(name, "RadToGrad")) return unaryScale(63.661977236758134307553505349005);
    if (equalsIgnoreCase(name, "GradToRad")) return unaryScale(0.015707963267948966192313216916398);
    if (equalsIgnoreCase(name, "RadToCycle")) return unaryScale(0.15915494309189533576888376337251);
    if (equalsIgnoreCase(name, "CycleToRad")) return unaryScale(6.283185307179586476925286766559);

    if (equalsIgnoreCase(name, "LogN")) {
        const std::string logX = "%tmp" + std::to_string(nextTemporary_++);
        const std::string logBase = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << logX << " = call double @llvm.log.f64(double " << values.at(1) << ")\n";
        output_ << "  " << logBase << " = call double @llvm.log.f64(double " << values.at(0) << ")\n";
        output_ << "  " << result << " = fdiv double " << logX << ", " << logBase << "\n";
        return result;
    }
    if (equalsIgnoreCase(name, "Hypot3")) {
        const std::string xy = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << xy << " = call double @hypot(double " << values.at(0) << ", double " << values.at(1) << ")\n";
        output_ << "  " << result << " = call double @hypot(double " << xy << ", double " << values.at(2) << ")\n";
        return result;
    }

    if (const std::string intrinsic = mathIntrinsicName(name); !intrinsic.empty()) {
        output_ << "  " << result << " = call double @" << intrinsic << "(";
    } else if (const std::string libm = mathLibmName(name); !libm.empty()) {
        output_ << "  " << result << " = call double @" << libm << "(";
    } else {
        throw CodegenUnsupported("unsupported math builtin: " + std::string(name));
    }
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) output_ << ", ";
        output_ << "double " << values[index];
    }
    output_ << ")\n";
    return result;
}

// Change char-values: `value` stops the program (kind 8) unless it is a
// Unicode scalar inside `target` (Char, or a Char Range): a surrogate is
// replaced by -1, which the range check then rejects.
std::string FunctionEmitter::emitCharCheck(const std::string& value, TypeRef target)
{
    const std::string offset = "%tmp" + std::to_string(nextTemporary_++);
    const std::string surrogate = "%tmp" + std::to_string(nextTemporary_++);
    const std::string checked = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << offset << " = sub i64 " << value << ", 55296\n";
    output_ << "  " << surrogate << " = icmp ult i64 " << offset << ", 2048\n";
    output_ << "  " << checked << " = select i1 " << surrogate << ", i64 -1, i64 " << value << '\n';
    return emitRangeCheck(checked, IntegerRange{target->minValue(), target->maxValue()}, 8);
}

// String(C) (change char-values): a new String held to the end of the
// statement.
std::string FunctionEmitter::emitCharString(const std::string& value)
{
    requireRuntime(RuntimeFunction::StringFromChar);
    requireStringRuntime();
    const std::string status = newStringStatus();
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << result << " = call ptr @__inox_str_from_char(i64 " << value << ", ptr " << status << ")\n";
    emitStringStatusCheck(status);
    holdOwnedResult(result, semantics_.types().stringType(), "ptr");
    return result;
}

} // namespace inox::compiler::codegen::detail
