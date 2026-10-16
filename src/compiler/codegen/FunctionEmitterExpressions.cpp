// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: expression types and the lowering of expressions.

#include "FunctionEmitter.h"

#include "AstQueries.h"
#include "ExceptionAbi.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "MathBuiltins.h"
#include "Operators.h"
#include "../exceptions/ExceptionTypes.h"
#include "../support/ArrayType.h"
#include "../support/PointerType.h"
#include "../support/Platform.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
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

std::string FunctionEmitter::expressionLlvmType(const ast::Expression& expression) const
{
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression: {
        const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
        switch (literal.literalKind()) {
        case ast::LiteralKind::Integer:
            return "i64";
        case ast::LiteralKind::Float:
            return "double";
        case ast::LiteralKind::Boolean:
            return "i1";
        case ast::LiteralKind::String:
            return "ptr";
        case ast::LiteralKind::Char:
            return "i32";
        }
        break;
    }
    case ast::AstNodeKind::IdentifierExpression: {
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
        const std::string normalizedName = normalize(identifier.name());
        const auto local = locals_.find(normalizedName);
        if (local != locals_.end()) {
            return local->second.llvmType;
        }
        if (semanticTypeName(expression) == "None") {
            return "ptr";
        }
        const auto parameter = parameterTypes_.find(normalizedName);
        if (parameter != parameterTypes_.end()) {
            return parameter->second;
        }
        const auto signature = signatures_.find(normalizedName);
        if (signature != signatures_.end()) {
            return signature->second.llvmReturnType;
        }
        if (const semantic::ConstantValue* constant = resolvedConstant(identifier)) {
            if (constant->kind == semantic::ConstantValue::Kind::String) {
                return "ptr";
            }
            return constant->kind == semantic::ConstantValue::Kind::Boolean ? "i1" : "i64";
        }
        break;
    }
    case ast::AstNodeKind::UnaryExpression: {
        const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
        if (unary.op() == ast::UnaryOperator::Not) {
            return "i1";
        }
        return expressionLlvmType(unary.operand());
    }
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        if (llvmComparisonPredicate(binary.op()).size() != 0 || fcmpPredicate(binary.op()).size() != 0) {
            return "i1";
        }
        if (llvmBooleanOperation(binary.op()).size() != 0) {
            return "i1";
        }
        return expressionLlvmType(binary.left());
    }
    case ast::AstNodeKind::ConditionalExpression:
        // ADR-0013: semantic analysis guarantees compatible branch types.
        return expressionLlvmType(
            static_cast<const ast::ConditionalExpression&>(expression).thenValue());
    case ast::AstNodeKind::CallExpression: {
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
            if (equalsIgnoreCase(callee.name(), "Abs") && call.arguments().size() == 1) {
                return expressionLlvmType(*call.arguments().front());
            }
            if (callee.name() == "__pure" && call.arguments().size() == 1) {
                return expressionLlvmType(*call.arguments().front());
            }
            if (isPointerBuiltin(call)) {
                return equalsIgnoreCase(callee.name(), "Free") ? std::string("void")
                                                               : valueLlvmType(semanticTypeName(expression));
            }
            if (isFileCall(call)) {
                return fileCallLlvmType(call);
            }
            if (isMathBuiltin(callee.name())) {
                return "double";
            }
            if (isNaturalConversion(call)) {
                return "i64";
            }
            if (callee.name() == "__index" || callee.name() == "__slice") {
                return valueLlvmType(semanticTypeName(expression));
            }
            if (const StructDefinition* structType = findStruct(structs_, callee.name())) {
                return structType->llvmName;
            }
            if (equalsIgnoreCase(callee.name(), "__member")) {
                if (const auto field = memberField(call)) {
                    return field->second->llvmType;
                }
                if (call.arguments().size() == 2 &&
                    call.arguments()[1]->kind() == ast::AstNodeKind::IdentifierExpression) {
                    const auto& method = static_cast<const ast::IdentifierExpression&>(*call.arguments()[1]);
                    if (const FunctionSignature* signature = findMethodSignature(*call.arguments()[0], method.name())) {
                        return signature->llvmReturnType;
                    }
                }
            }
            if (arrayBoundCall(call)) {
                return "i64";
            }
            const auto signature = signatures_.find(normalize(callee.name()));
            if (signature != signatures_.end()) {
                return signature->second.llvmReturnType;
            }
        }
        if (isMemberAccessCall(call.callee())) {
            const auto& member = static_cast<const ast::CallExpression&>(call.callee());
            if (member.arguments().size() == 2 && member.arguments()[0]) {
                if (member.arguments()[1]->kind() == ast::AstNodeKind::IdentifierExpression) {
                    const auto& method = static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
                    if (const FunctionSignature* signature = findMethodSignature(*member.arguments()[0], method.name())) {
                        return signature->llvmReturnType;
                    }
                }
            }
        }
        break;
    }
    default:
        break;
    }
    return "i64";
}

// Natural(X), Integer(X) or Int64(X) applied to one i64 value.
bool FunctionEmitter::isNaturalConversion(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 1) {
        return false;
    }
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    if (!equalsIgnoreCase(callee.name(), "Natural") && !equalsIgnoreCase(callee.name(), "Integer") &&
        !equalsIgnoreCase(callee.name(), "Int64") && !narrowIntegerRange(callee.name())) {
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

// The semantic type name of an expression, or empty.
std::string FunctionEmitter::semanticTypeName(const ast::Expression& expression) const
{
    const semantic::ResolvedType* type = semantics_.typeOf(expression);
    return type != nullptr ? type->name : std::string{};
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
    if (isRangeCheckedArithmetic(expression)) {
        if (const auto range = narrowIntegerRange(semanticTypeName(expression))) {
            value = emitRangeCheck(value, *range, 1);
        }
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
        const auto signature = signatures_.find(normalizedName);
        if (signature != signatures_.end()) {
            if (!signature->second.parameters.empty()) {
                throw CodegenError("function requires arguments; use parentheses only when passing arguments: " + identifier.name());
            }
            if (signature->second.llvmReturnType == "void") {
                throw CodegenError("void function call cannot be used as an expression");
            }
            // Through emitUserCall: an `invoke` inside try (the call may
            // raise) and the ownership of a String result.
            return emitUserCall(signature->second, {}, true);
        }
        // None: an empty Link or an absent owner (safe-pointers).
        if (semanticTypeName(expression) == "None") {
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
        const std::string leftType = expressionLlvmType(binary.left());
        const std::string rightType = expressionLlvmType(binary.right());
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        const std::string left = emitExpression(binary.left());
        std::string right = emitExpression(binary.right());

        // CANON-8: the shift count of a fixed-width type is 0..width-1.
        if (binary.op() == ast::BinaryOperator::ShiftLeft ||
            binary.op() == ast::BinaryOperator::ShiftRight) {
            if (const auto range = narrowIntegerRange(semanticTypeName(expression))) {
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
        if (isStringType(semanticTypeName(binary.left())) &&
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
        if (isStringType(semanticTypeName(binary.left()))) {
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
            std::string boolPredicate = predicate;
            if (isBool && boolPredicate.front() == 's') {
                boolPredicate.front() = 'u';
            }
            output_ << "  " << result << " = icmp " << (isBool ? boolPredicate : predicate) << ' '
                    << (isBool ? "i1" : "i64") << ' ' << left << ", " << right << '\n';
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
            if (const auto range = narrowIntegerRange(semanticTypeName(unary.operand()));
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
        if (callee.name() == "__index") {
            const ElementAddress element = emitElementAddress(call);
            return loadElement(element);
        }
        if (callee.name() == "__slice") {
            return emitSlice(call);
        }
        if (isPointerBuiltin(call)) {
            if (equalsIgnoreCase(callee.name(), "Free")) {
                throw CodegenError("void function call cannot be used as an expression");
            }
            return emitPointerBuiltin(call);
        }
        if (const StructDefinition* structType = findStruct(structs_, callee.name())) {
            return emitConstruction(call, *structType);
        }
        if (call.arguments().size() == 1 &&
            support::isSliceTypeName(semanticTypeName(*call.arguments()[0])) &&
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

        // CANON-8: Natural(X) checks X >= 0 at run time (fault kind 7);
        // Integer(X) and Int64(X) of an integer value are the identity,
        // because Natural shares Integer's i64 representation.
        if (isNaturalConversion(call)) {
            const std::string value = emitExpression(*call.arguments().front());
            if (const auto range = narrowIntegerRange(callee.name())) {
                return emitRangeCheck(value, *range, 8);
            }
            if (!equalsIgnoreCase(callee.name(), "Natural")) {
                return value;
            }
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << result << " = call i64 @__inox_natural_i64(i64 " << value << ")\n";
            return result;
        }

        const auto signature = signatures_.find(normalize(callee.name()));
        if (signature == signatures_.end()) {
            break;
        }
        if (signature->second.llvmReturnType == "void") {
            throw CodegenError("void function call cannot be used as an expression");
        }
        if (call.arguments().size() != signature->second.parameters.size()) {
            throw CodegenUnsupported("unsupported call argument count in function: " + function_.name());
        }

        const std::vector<std::string> arguments = emitArguments(signature->second, call.arguments(), 0);
        const std::string result = emitUserCall(signature->second, arguments, true);
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

} // namespace inox::compiler::codegen::detail
