// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: expression types and the lowering of expressions.

#include "FunctionEmitter.h"
#include "../support/DecimalLiteral.h"
#include "../support/CharLiteral.h"
#include "../support/RealLiteral.h"

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
    case ast::BinaryOperator::Divide:
    case ast::BinaryOperator::Modulo:
    case ast::BinaryOperator::Remainder:
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
        // Change decimals (r3.91): a literal read as a Currency or a Crypto
        // is its exact value scaled by 10^places.
        if (const TypeRef type = typeOf(expression); type != nullptr && type->isDecimal()) {
            return support::parseDecimalLiteral(literal.value(), type->decimalPlaces()).scaled;
        }
        if (literal.literalKind() == ast::LiteralKind::Integer) {
            return llvmIntegerLiteral(literal.value());
        }
        if (literal.literalKind() == ast::LiteralKind::Float) {
            const TypeRef type = typeOf(expression);
            const bool float32 = type != nullptr && type->isFloat() && type->bits() == 32;
            return llvmRealConstant(support::parseRealLiteral(literal.value(), float32), float32);
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
        if (const semantic::ModuleIntrinsic* terminal = intrinsicCallOf(expression)) {
            return emitIntrinsicCall(expression, *terminal);
        }
        // r3.89 (C1): EndOfInput.
        if (equalsIgnoreCase(identifier.name(), "EndOfInput")) {
            if (const semantic::Symbol* symbol = semantics_.symbolOf(identifier);
                symbol != nullptr && symbol->kind == semantic::SymbolKind::Builtin) {
                requireRuntime(RuntimeFunction::ConsoleAtEnd);
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << result << " = call zeroext i1 @__inox_con_at_end()\n";
                return result;
            }
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
            if (constant->kind == semantic::ConstantValue::Kind::Decimal) {
                return constant->text;
            }
            if (constant->kind == semantic::ConstantValue::Kind::Real) {
                const TypeRef type = typeOf(expression);
                const bool float32 = type != nullptr && type->bits() == 32;
                // A universal Const is read once from its digits (r3.86).
                const double value =
                    constant->universal ? support::parseRealLiteral(constant->text, float32) : constant->real;
                return llvmRealConstant(value, float32);
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

        // Change decimals (r3.91): Currency and Crypto compare as their
        // scaled integers; + - * / are exact, checked, and * / round half to
        // even (A1).
        if (const TypeRef type = typeOf(binary.left()); type != nullptr && type->isDecimal()) {
            if (const std::string predicate = llvmComparisonPredicate(binary.op()); !predicate.empty()) {
                output_ << "  " << result << " = icmp " << predicate << ' ' << leftType << ' ' << left << ", "
                        << right << '\n';
                return result;
            }
            return emitDecimalArithmetic(binary.op(), type, left, right);
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
                const TypeRef literalType = typeOf(unary.operand());
                if (literalType != nullptr && literalType->isDecimal()) {
                    const std::string scaled =
                        support::parseDecimalLiteral(literal.value(), literalType->decimalPlaces()).scaled;
                    return scaled == "0" ? scaled : "-" + scaled;
                }
                if (literal.literalKind() == ast::LiteralKind::Integer) {
                    return "-" + llvmIntegerLiteral(literal.value());
                }
                if (literal.literalKind() == ast::LiteralKind::Float) {
                    // The literal read in its type, then negated (a Float32
                    // `-0.1` is the negation of the Float32 nearest 0.1).
                    const bool float32 = literalType != nullptr && literalType->isFloat() && literalType->bits() == 32;
                    return llvmRealConstant(-support::parseRealLiteral(literal.value(), float32), float32);
                }
            }
            const std::string operand = emitExpression(unary.operand());
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            if (const TypeRef type = typeOf(unary.operand()); type != nullptr && type->isDecimal()) {
                return emitDecimalArithmetic(ast::BinaryOperator::Subtract, type, "0", operand);
            }
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

        if (const semantic::ModuleIntrinsic* terminal = intrinsicCallOf(call)) {
            return emitIntrinsicCall(call, *terminal);
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

        // Change decimals (r3.91): Currency(X), Crypto(X), and T(C) of a
        // decimal into a number; Round and Trunc of Std.Math.
        if (call.arguments().size() == 1 && calledRoutine(callee) == nullptr) {
            const TypeRef target = typeOf(call);
            const TypeRef source = typeOf(*call.arguments().front());
            const bool conversion = semantics_.types().lookup(callee.name()) != nullptr;
            if (conversion && target != nullptr && source != nullptr &&
                (target->isDecimal() || (source->isDecimal() && (target->isInteger() || target->isFloat())))) {
                return emitDecimalConversion(call, target);
            }
            if (!conversion && (equalsIgnoreCase(callee.name(), "Round") || equalsIgnoreCase(callee.name(), "Trunc"))) {
                if (const semantic::Symbol* symbol = semantics_.symbolOf(callee);
                    symbol != nullptr && symbol->kind == semantic::SymbolKind::Builtin) {
                    return emitRounding(call);
                }
            }
        }

        // r3.90 (B2): T(S) of a String into a number.
        if (const TypeRef target = textConversionTarget(call)) {
            return emitTextToNumber(call, target);
        }

        // Change float-conversions: Float(X), Float32(X), and T(F) of a
        // Float into an integer type.
        if (const TypeRef target = realConversionTarget(call)) {
            return emitRealConversion(call, target);
        }

        // String(C) (change char-values): the one-character String; String(N)
        // and String(F) (r3.90, B1): the number as Put writes it.
        if (call.arguments().size() == 1 && equalsIgnoreCase(callee.name(), "String") &&
            calledRoutine(callee) == nullptr) {
            const TypeRef source = typeOf(*call.arguments().front());
            const std::string value = emitExpression(*call.arguments().front());
            if (source != nullptr && source->isChar()) {
                return emitCharString(value);
            }
            if (source != nullptr && (source->isInteger() || source->isFloat())) {
                return emitNumberString(value, source, expressionLlvmType(*call.arguments().front()));
            }
            if (source != nullptr && source->isDecimal()) {
                requireRuntime(RuntimeFunction::StringFromDecimal);
                requireStringRuntime();
                const auto [low, high] = decimalHalves(value, source);
                const std::string status = newStringStatus();
                const std::string text = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << text << " = call ptr @__inox_str_from_dec(i64 " << low << ", i64 " << high
                        << ", i32 " << source->decimalPlaces() << ", ptr " << status << ")\n";
                emitStringStatusCheck(status);
                holdOwnedResult(text, semantics_.types().stringType(), "ptr");
                return text;
            }
            return value;
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

// Change decimals (r3.91): the two 64-bit halves of a Currency (its value
// and its sign) or a Crypto, as libinoxrt takes them.
std::pair<std::string, std::string> FunctionEmitter::decimalHalves(const std::string& value, TypeRef type)
{
    const std::string low = "%tmp" + std::to_string(nextTemporary_++);
    const std::string high = "%tmp" + std::to_string(nextTemporary_++);
    if (type->bits() == 128) {
        const std::string shifted = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << low << " = trunc i128 " << value << " to i64\n";
        output_ << "  " << shifted << " = ashr i128 " << value << ", 64\n";
        output_ << "  " << high << " = trunc i128 " << shifted << " to i64\n";
        return {low, high};
    }
    output_ << "  " << high << " = ashr i64 " << value << ", 63\n";
    return {value, high};
}

// A slot of 16 bytes that libinoxrt fills with a decimal, low half first.
std::string FunctionEmitter::newDecimalSlot()
{
    const std::string slot = "%decimal.slot" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca i128, align 16\n";
    return slot;
}

std::string FunctionEmitter::loadDecimal(const std::string& slot, TypeRef type)
{
    const std::string value = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << value << " = load " << (type->bits() == 128 ? "i128" : "i64") << ", ptr " << slot << '\n';
    return value;
}

// A status of libinoxrt: 0 ok, else the fault of `kinds` (1 and 2).
void FunctionEmitter::emitDecimalStatus(const std::string& status, int firstKind, int secondKind)
{
    const std::string ok = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << ok << " = icmp ne i32 " << status << ", 1\n";
    emitTrapUnless(ok, firstKind);
    if (secondKind != 0) {
        const std::string fine = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << fine << " = icmp ne i32 " << status << ", 2\n";
        emitTrapUnless(fine, secondKind);
    }
}

// A1: + and - checked inline ("decimal overflow", kind 20); * and / exact in
// libinoxrt, rounded half to even, / by zero is "division by zero".
std::string FunctionEmitter::emitDecimalArithmetic(ast::BinaryOperator op, TypeRef type, const std::string& left,
                                                   const std::string& right)
{
    const std::string llvmType = type->bits() == 128 ? "i128" : "i64";
    if (op == ast::BinaryOperator::Add || op == ast::BinaryOperator::Subtract) {
        const std::string intrinsic =
            std::string(op == ast::BinaryOperator::Add ? "llvm.sadd.with.overflow." : "llvm.ssub.with.overflow.") +
            llvmType;
        if (llvmType == "i128") {
            requireModuleLine("declare { i128, i1 } @" + intrinsic + "(i128, i128)");
        }
        const std::string pair = "%tmp" + std::to_string(nextTemporary_++);
        const std::string value = "%tmp" + std::to_string(nextTemporary_++);
        const std::string overflow = "%tmp" + std::to_string(nextTemporary_++);
        const std::string fits = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << pair << " = call { " << llvmType << ", i1 } @" << intrinsic << '(' << llvmType << ' '
                << left << ", " << llvmType << ' ' << right << ")\n";
        output_ << "  " << value << " = extractvalue { " << llvmType << ", i1 } " << pair << ", 0\n";
        output_ << "  " << overflow << " = extractvalue { " << llvmType << ", i1 } " << pair << ", 1\n";
        output_ << "  " << fits << " = xor i1 " << overflow << ", true\n";
        emitTrapUnless(fits, 20);
        return value;
    }
    if (op != ast::BinaryOperator::Multiply && op != ast::BinaryOperator::Divide) {
        throw CodegenError("operator not defined for " + type->name());
    }
    const bool multiply = op == ast::BinaryOperator::Multiply;
    requireRuntime(multiply ? RuntimeFunction::DecimalMultiply : RuntimeFunction::DecimalDivide);
    const auto [leftLow, leftHigh] = decimalHalves(left, type);
    const auto [rightLow, rightHigh] = decimalHalves(right, type);
    const std::string slot = newDecimalSlot();
    const std::string status = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << status << " = call i32 @" << (multiply ? "__inox_dec_mul" : "__inox_dec_div") << "(i64 "
            << leftLow << ", i64 " << leftHigh << ", i64 " << rightLow << ", i64 " << rightHigh << ", i32 "
            << type->decimalPlaces() << ", ptr " << slot << ")\n";
    emitDecimalStatus(status, 20, 2);
    return loadDecimal(slot, type);
}

// Change decimals (r3.91, CANON-8, A3-A4): conversions into and out of a
// Currency or a Crypto. Into: an integer exactly, a Float or the other
// decimal rounded half to even, a String exactly (ConversionError
// otherwise); beyond the type, "integer conversion out of range" (kind 8).
// Out of: an integer truncated toward zero, a real the nearest one.
std::string FunctionEmitter::emitDecimalConversion(const ast::CallExpression& call, TypeRef target)
{
    const ast::Expression& argument = *call.arguments().front();
    const TypeRef source = typeOf(argument);
    const std::string value = emitExpression(argument);
    if (source == target) {
        return value;
    }
    if (target->isDecimal()) {
        const int places = static_cast<int>(target->decimalPlaces());
        const bool crypto = target->bits() == 128;
        if (source->isString()) {
            const std::string slot = newDecimalSlot();
            emitRuntimeCall(RuntimeFunction::StringToDecimal, "void", "__inox_str_to_dec",
                            "ptr " + value + ", i32 " + std::to_string(places) + ", ptr " + slot);
            return loadDecimal(slot, target);
        }
        if (source->isDecimal()) {
            requireRuntime(RuntimeFunction::DecimalRescale);
            const auto [low, high] = decimalHalves(value, source);
            const std::string slot = newDecimalSlot();
            const std::string status = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << status << " = call i32 @__inox_dec_rescale(i64 " << low << ", i64 " << high << ", i32 "
                    << source->decimalPlaces() << ", i32 " << places << ", ptr " << slot << ")\n";
            emitDecimalStatus(status, 8, 0);
            return loadDecimal(slot, target);
        }
        if (source->isFloat()) {
            std::string real = value;
            if (source->bits() == 32) {
                real = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << real << " = fpext float " << value << " to double\n";
            }
            requireRuntime(RuntimeFunction::DecimalFromFloat);
            const std::string slot = newDecimalSlot();
            const std::string status = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << status << " = call i32 @__inox_dec_from_f64(double " << real << ", i32 " << places
                    << ", ptr " << slot << ")\n";
            emitDecimalStatus(status, 8, 0);
            return loadDecimal(slot, target);
        }
        // An integer, exactly.
        const bool unsignedSource = isUInt64Type(source);
        if (crypto) {
            const std::string wide = "%tmp" + std::to_string(nextTemporary_++);
            const std::string scaled = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << wide << " = " << (unsignedSource ? "zext" : "sext") << " i64 " << value
                    << " to i128\n";
            output_ << "  " << scaled << " = mul i128 " << wide << ", 1000000000000000000\n";
            return scaled;
        }
        if (unsignedSource) {
            const std::string fits = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << fits << " = icmp sge i64 " << value << ", 0\n";
            emitTrapUnless(fits, 8);
        }
        const std::string pair = "%tmp" + std::to_string(nextTemporary_++);
        const std::string scaled = "%tmp" + std::to_string(nextTemporary_++);
        const std::string overflow = "%tmp" + std::to_string(nextTemporary_++);
        const std::string fits = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << pair << " = call { i64, i1 } @llvm.smul.with.overflow.i64(i64 " << value
                << ", i64 1000000)\n";
        output_ << "  " << scaled << " = extractvalue { i64, i1 } " << pair << ", 0\n";
        output_ << "  " << overflow << " = extractvalue { i64, i1 } " << pair << ", 1\n";
        output_ << "  " << fits << " = xor i1 " << overflow << ", true\n";
        emitTrapUnless(fits, 8);
        return scaled;
    }
    const auto [low, high] = decimalHalves(value, source);
    if (target->isFloat()) {
        const bool float32 = target->bits() == 32;
        requireRuntime(float32 ? RuntimeFunction::DecimalToFloat32 : RuntimeFunction::DecimalToFloat);
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = call " << (float32 ? "float @__inox_dec_to_f32" : "double @__inox_dec_to_f64")
                << "(i64 " << low << ", i64 " << high << ", i32 " << source->decimalPlaces() << ")\n";
        return result;
    }
    return emitDecimalToInteger(low, high, source, target, false);
}

// The integer of a decimal, truncated toward zero or (Round) rounded half to
// even, checked against the target type (kind 8).
std::string FunctionEmitter::emitDecimalToInteger(const std::string& low, const std::string& high, TypeRef source,
                                                  TypeRef target, bool round)
{
    requireRuntime(RuntimeFunction::DecimalToInteger);
    const std::string status = "%tmp" + std::to_string(nextTemporary_++) + ".status";
    const std::string code = "%tmp" + std::to_string(nextTemporary_++);
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << status << " = alloca i32\n";
    output_ << "  " << result << " = call i64 @__inox_dec_to_int(i64 " << low << ", i64 " << high << ", i32 "
            << source->decimalPlaces() << ", i32 " << (round ? 1 : 0) << ", ptr " << status << ")\n";
    output_ << "  " << code << " = load i32, ptr " << status << '\n';
    emitDecimalStatus(code, 8, 0);
    if (isUInt64Type(target)) {
        const std::string fits = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << fits << " = icmp sge i64 " << result << ", 0\n";
        emitTrapUnless(fits, 8);
        return result;
    }
    if (target->isRange()) {
        return emitRangeCheck(result, IntegerRange{target->minValue(), target->maxValue()}, 8);
    }
    if (const std::optional<IntegerRange> range = narrowIntegerRange(target)) {
        return emitRangeCheck(result, *range, 8);
    }
    return result;
}

// A3 (Std.Math): Round(X) to the nearest integer, a half to the even one;
// Trunc(X) toward zero; of a Float, a Float32, a Currency or a Crypto.
// Beyond Integer (or NaN) is "integer conversion out of range".
std::string FunctionEmitter::emitRounding(const ast::CallExpression& call)
{
    const bool round = equalsIgnoreCase(static_cast<const ast::IdentifierExpression&>(call.callee()).name(), "Round");
    const ast::Expression& argument = *call.arguments().front();
    const TypeRef source = typeOf(argument);
    const std::string value = emitExpression(argument);
    if (source->isDecimal()) {
        const auto [low, high] = decimalHalves(value, source);
        return emitDecimalToInteger(low, high, source, semantics_.types().int64Type(), round);
    }
    std::string real = value;
    if (source->bits() == 32) {
        real = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << real << " = fpext float " << value << " to double\n";
    }
    const std::string intrinsic = round ? "llvm.roundeven.f64" : "llvm.trunc.f64";
    requireModuleLine("declare double @" + intrinsic + "(double)");
    const std::string whole = "%tmp" + std::to_string(nextTemporary_++);
    const std::string above = "%tmp" + std::to_string(nextTemporary_++);
    const std::string below = "%tmp" + std::to_string(nextTemporary_++);
    const std::string fits = "%tmp" + std::to_string(nextTemporary_++);
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << whole << " = call double @" << intrinsic << "(double " << real << ")\n";
    // -2^63 <= whole < 2^63, and not NaN (ordered comparisons).
    output_ << "  " << above << " = fcmp oge double " << whole << ", -9.2233720368547758e+18\n";
    output_ << "  " << below << " = fcmp olt double " << whole << ", 9.2233720368547758e+18\n";
    output_ << "  " << fits << " = and i1 " << above << ", " << below << '\n';
    emitTrapUnless(fits, 8);
    output_ << "  " << result << " = fptosi double " << whole << " to i64\n";
    return result;
}

// r3.90 (B1): String(N) or String(F), a new String held to the end of the
// statement.
std::string FunctionEmitter::emitNumberString(const std::string& value, TypeRef source, const std::string& llvmType)
{
    RuntimeFunction function = RuntimeFunction::StringFromInteger;
    std::string symbol = "__inox_str_from_i64";
    std::string type = "i64";
    if (source->isFloat()) {
        const bool float32 = llvmType == "float";
        function = float32 ? RuntimeFunction::StringFromFloat32 : RuntimeFunction::StringFromFloat;
        symbol = float32 ? "__inox_str_from_f32" : "__inox_str_from_f64";
        type = float32 ? "float" : "double";
    } else if (isUInt64Type(source)) {
        function = RuntimeFunction::StringFromUnsigned;
        symbol = "__inox_str_from_u64";
    }
    requireRuntime(function);
    requireStringRuntime();
    const std::string status = newStringStatus();
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << result << " = call ptr @" << symbol << "(" << type << " " << value << ", ptr " << status
            << ")\n";
    emitStringStatusCheck(status);
    holdOwnedResult(result, semantics_.types().stringType(), "ptr");
    return result;
}

// r3.90 (B2): the numeric target of `T(S)` with S a String; else null.
TypeRef FunctionEmitter::textConversionTarget(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 1 ||
        calledRoutine(static_cast<const ast::IdentifierExpression&>(call.callee())) != nullptr) {
        return nullptr;
    }
    const TypeRef source = typeOf(*call.arguments().front());
    if (source == nullptr || !source->isString()) {
        return nullptr;
    }
    const TypeRef target = typeOf(call);
    if (target == nullptr || target->isEnum() || !(target->isInteger() || target->isFloat())) {
        return nullptr;
    }
    return target;
}

// r3.90 (B2): text into a number, by libinoxrt; text that is not a number
// of the target raises ConversionError (an `invoke` inside try).
std::string FunctionEmitter::emitTextToNumber(const ast::CallExpression& call, TypeRef target)
{
    const std::string text = emitExpression(*call.arguments().front());
    if (target->isFloat()) {
        const bool float32 = target->bits() == 32;
        return emitRuntimeCall(float32 ? RuntimeFunction::StringToFloat32 : RuntimeFunction::StringToFloat,
                               float32 ? "float" : "double", float32 ? "__inox_str_to_f32" : "__inox_str_to_f64",
                               "ptr " + text);
    }
    if (isUInt64Type(target)) {
        return emitRuntimeCall(RuntimeFunction::StringToUnsigned, "i64", "__inox_str_to_u64", "ptr " + text);
    }
    std::int64_t low = 0;
    std::int64_t high = 0;
    if (target->isRange() || target->isCharRange()) {
        low = target->minValue();
        high = target->maxValue();
    } else if (const std::optional<IntegerRange> range = narrowIntegerRange(target)) {
        low = range->low;
        high = range->high;
    } else {
        low = std::numeric_limits<std::int64_t>::min();
        high = std::numeric_limits<std::int64_t>::max();
    }
    return emitRuntimeCall(RuntimeFunction::StringToInteger, "i64", "__inox_str_to_i64",
                           "ptr " + text + ", i64 " + std::to_string(low) + ", i64 " + std::to_string(high));
}

// Change float-conversions: the target of a conversion `T(X)` that involves
// a Float, as source or as target; else null.
TypeRef FunctionEmitter::realConversionTarget(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 1 ||
        calledRoutine(static_cast<const ast::IdentifierExpression&>(call.callee())) != nullptr) {
        return nullptr;
    }
    const TypeRef target =
        semantics_.types().lookup(static_cast<const ast::IdentifierExpression&>(call.callee()).name());
    const TypeRef source = typeOf(*call.arguments().front());
    if (target == nullptr || source == nullptr) {
        return nullptr;
    }
    const bool real = target->isFloat() || source->isFloat();
    const bool numeric = (target->isFloat() || target->isInteger()) && (source->isFloat() || source->isInteger());
    return real && numeric ? target : nullptr;
}

// Branches to a trap of `kind` unless `condition` (an i1) holds.
void FunctionEmitter::emitTrapUnless(const std::string& condition, int kind)
{
    const std::string label = std::to_string(nextLabel_++);
    output_ << "  br i1 " << condition << ", label %conv.ok" << label << ", label %conv.fault" << label << "\n\n";
    output_ << "conv.fault" << label << ":\n  call void @__inox_arith_fault(i32 " << kind << ")\n  unreachable\n\n";
    output_ << "conv.ok" << label << ":\n";
}

// CANON-8 (change float-conversions):
// - Float(I) and Float32(I) round an integer to the nearest real (a UInt64
//   is read unsigned);
// - Float(G) widens a Float32 exactly; Float32(F) rounds to the nearest
//   Float32, and a finite F beyond Float32's range stops the program with
//   "value out of range" (kind 11);
// - T(F) of a Float into an integer type truncates toward zero; NaN, an
//   infinity or a value outside T stops the program with "integer
//   conversion out of range" (kind 8).
std::string FunctionEmitter::emitRealConversion(const ast::CallExpression& call, TypeRef target)
{
    const ast::Expression& argument = *call.arguments().front();
    const TypeRef source = typeOf(argument);
    const std::string value = emitExpression(argument);
    const std::string sourceType = valueLlvmType(source);
    const std::string targetType = valueLlvmType(target);
    const auto temporary = [this] { return "%tmp" + std::to_string(nextTemporary_++); };

    if (target->isFloat()) {
        if (source->isInteger()) {
            const std::string result = temporary();
            output_ << "  " << result << " = " << (isUInt64Type(source) ? "uitofp" : "sitofp") << " i64 " << value
                    << " to " << targetType << '\n';
            return result;
        }
        if (sourceType == targetType) {
            return value;
        }
        const std::string result = temporary();
        if (target->bits() == 64) {
            output_ << "  " << result << " = fpext float " << value << " to double\n";
            return result;
        }
        output_ << "  " << result << " = fptrunc double " << value << " to float\n";
        // Overflow: the rounded value is infinite while the source was finite.
        const std::string sourceMagnitude = temporary();
        const std::string resultMagnitude = temporary();
        const std::string sourceFinite = temporary();
        const std::string resultInfinite = temporary();
        const std::string overflow = temporary();
        const std::string fits = temporary();
        output_ << "  " << sourceMagnitude << " = call double @llvm.fabs.f64(double " << value << ")\n";
        output_ << "  " << resultMagnitude << " = call float @llvm.fabs.f32(float " << result << ")\n";
        output_ << "  " << sourceFinite << " = fcmp one double " << sourceMagnitude << ", 0x7FF0000000000000\n";
        output_ << "  " << resultInfinite << " = fcmp oeq float " << resultMagnitude << ", 0x7FF0000000000000\n";
        output_ << "  " << overflow << " = and i1 " << sourceFinite << ", " << resultInfinite << '\n';
        output_ << "  " << fits << " = xor i1 " << overflow << ", true\n";
        // llvm.fabs.f64 is declared with every module (LlvmIrEmitter).
        requireModuleLine("declare float @llvm.fabs.f32(float)");
        emitTrapUnless(fits, 11);
        return result;
    }

    // A Float into an integer type: in range (and not NaN) before converting.
    const bool unsignedTarget = isUInt64Type(target);
    const std::string low = temporary();
    const std::string high = temporary();
    const std::string inRange = temporary();
    // -2^63 <= F < 2^63, or 0 <= F < 2^64 for UInt64: exact powers of two.
    const std::string lowBound = llvmRealConstant(unsignedTarget ? 0.0 : -9223372036854775808.0, sourceType == "float");
    const std::string highBound =
        llvmRealConstant(unsignedTarget ? 18446744073709551616.0 : 9223372036854775808.0, sourceType == "float");
    output_ << "  " << low << " = fcmp oge " << sourceType << ' ' << value << ", " << lowBound << '\n';
    output_ << "  " << high << " = fcmp olt " << sourceType << ' ' << value << ", " << highBound << '\n';
    output_ << "  " << inRange << " = and i1 " << low << ", " << high << '\n';
    emitTrapUnless(inRange, 8);
    const std::string integer = temporary();
    output_ << "  " << integer << " = " << (unsignedTarget ? "fptoui " : "fptosi ") << sourceType << ' ' << value
            << " to i64\n";
    if (unsignedTarget || target == semantics_.types().int64Type()) {
        return integer;
    }
    return emitRangeCheck(integer, IntegerRange{target->minValue(), target->maxValue()}, 8);
}

} // namespace inox::compiler::codegen::detail
