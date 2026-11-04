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

// Natural(X), Integer(X) or Int64(X) applied to one i64 value.
bool FunctionEmitter::isNaturalConversion(const tir::Expr& conversion) const
{
    if (conversion.operands.size() != 1 || conversionTarget(conversion) == nullptr) {
        return false;
    }
    return expressionLlvmType(*conversion.operands[0]) == "i64";
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
        if (!source->isSigned() || source == types_.naturalType()) {
            return value;
        }
        return emitRangeCheck(value, IntegerRange{0, std::numeric_limits<std::int64_t>::max()}, 8);
    }
    const std::int64_t high = target->maxValue();
    return emitRangeCheck(value, IntegerRange{std::max<std::int64_t>(target->minValue(), 0), high}, 8);
}

// The integer type of a conversion `T(X)` lowered as a range check: Natural,
// Integer (Int64) or a fixed width narrower than 64 bits; else null.
TypeRef FunctionEmitter::conversionTarget(const tir::Expr& conversion) const
{
    if (conversion.operands.size() != 1) {
        return nullptr;
    }
    const TypeRef target = conversion.type;
    const bool lowered = target != nullptr && (target == types_.naturalType() ||
                                               target == types_.int64Type() ||
                                               target->isNarrowInteger() || isUInt64Type(target) ||
                                               target->isEnum() || target->isRange() || target->isChar());
    return lowered ? target : nullptr;
}

std::string FunctionEmitter::emitRangeCheck(const std::string& value, IntegerRange range, int faultKind)
{
    const std::string checked = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << checked << " = call i64 @__inox_range_i64(i64 " << value << ", i64 "
            << range.low << ", i64 " << range.high << ", i32 " << faultKind << ")\n";
    return checked;
}

const tir::Expr& FunctionEmitter::bare(const tir::Expr& expression)
{
    const tir::Expr* value = &expression;
    while (value->kind == tir::ExprKind::RangeCheck || value->kind == tir::ExprKind::NaturalCheck) {
        value = value->operands[0];
    }
    return *value;
}

std::string FunctionEmitter::emitValue(const tir::Expr& expression)
{
    return finishValue(expression, emitValueKind(bare(expression)));
}

// The value of `expression` on its way out: a borrowed read held, a narrow
// arithmetic result checked (CANON-8), then the checks of the analysis, the
// nodes around the value: CANON-13 Range (Q1, kind 11) inside, the Natural
// check (ADR-0015) outside.
std::string FunctionEmitter::finishValue(const tir::Expr& expression, std::string value)
{
    if (expression.kind == tir::ExprKind::RangeCheck) {
        value = finishValue(*expression.operands[0], std::move(value));
        return emitRangeCheck(value, IntegerRange{expression.type->minValue(), expression.type->maxValue()}, 11);
    }
    if (expression.kind == tir::ExprKind::NaturalCheck) {
        value = finishValue(*expression.operands[0], std::move(value));
        const std::string checked = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << checked << " = call i64 @__inox_natural_i64(i64 " << value << ")\n";
        return checked;
    }
    if (statementRunsRoutine_ && isBorrowedRead(expression)) {
        value = protectBorrowed(expression.type, value);
    }
    if (isRangeCheckedArithmetic(expression)) {
        if (const auto range = narrowIntegerRange(expression.type)) {
            value = emitRangeCheck(value, *range, 1);
        }
    }
    return value;
}

std::string FunctionEmitter::emitValueKind(const tir::Expr& expression)
{
    switch (expression.kind) {
    case tir::ExprKind::Literal:
        return emitLiteral(expression);
    case tir::ExprKind::Read:
        return emitPlaceRead(*expression.place);
    case tir::ExprKind::Binary: {
        if (isStringConcatenation(expression)) {
            return emitConcatenation(expression);
        }
        if (expression.binary == tir::BinaryOp::In) {
            return emitMembership(expression);
        }
        if (const TypeRef leftSet = bare(*expression.operands[0]).type; leftSet != nullptr && leftSet->isSet()) {
            return emitSetComparison(expression);
        }
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        const std::string left = emitValue(*expression.operands[0]);
        const std::string right = emitValue(*expression.operands[1]);
        return emitBinaryValues(expression, result, left, right);
    }
    case tir::ExprKind::Unary:
        return emitUnary(expression);
    case tir::ExprKind::Conditional:
        return emitConditional(expression);
    case tir::ExprKind::Call:
        return emitCall(expression, true);
    case tir::ExprKind::Builtin:
        return emitBuiltin(expression);
    case tir::ExprKind::Convert:
        return emitConversion(expression);
    case tir::ExprKind::String:
        return emitStringCall(expression);
    case tir::ExprKind::Intrinsic:
        return emitIntrinsicCall(expression);
    case tir::ExprKind::Construct:
        return emitConstruction(expression);
    case tir::ExprKind::SetLiteral:
        return emitSetCall(expression);
    case tir::ExprKind::Slice:
        return emitSlice(expression);
    // CANON-24: `old X` and a loop variant, evaluated where they stand.
    case tir::ExprKind::Pure:
        return emitValue(*expression.operands[0]);
    default:
        break;
    }
    throw CodegenUnsupported("unsupported expression in function: " + std::string(routine_.name));
}

std::string FunctionEmitter::emitBuiltin(const tir::Expr& call)
{
    const semantic::BuiltinOp op = call.op;
    // r3.89 (C1): EndOfInput.
    if (op == semantic::BuiltinOp::EndOfInput) {
        requireRuntime(RuntimeFunction::ConsoleAtEnd);
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = call zeroext i1 @__inox_con_at_end()\n";
        return result;
    }
    if (isOrdinalBuiltin(call)) {
        return emitOrdinalBuiltin(call);
    }
    if (op == semantic::BuiltinOp::Fill && call.unit != nullptr) {
        return emitFill(call);
    }
    if (isSetCall(call)) {
        return emitSetCall(call);
    }
    if (isPointerBuiltin(op)) {
        if (op == semantic::BuiltinOp::Free) {
            throw CodegenError("void function call cannot be used as an expression");
        }
        return emitPointerBuiltin(call);
    }
    const bool bound = op == semantic::BuiltinOp::Low || op == semantic::BuiltinOp::High ||
                       op == semantic::BuiltinOp::Length;
    if (bound && call.operands.size() == 1) {
        const TypeRef type = bare(*call.operands[0]).type;
        if (isSliceType(type)) {
            const SliceParts slice = extractSlice(emitValue(*call.operands[0]));
            if (op == semantic::BuiltinOp::Low) {
                return slice.low;
            }
            if (op == semantic::BuiltinOp::High) {
                return slice.high;
            }
            const std::string span = "%tmp" + std::to_string(nextTemporary_++);
            const std::string length = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << span << " = sub i64 " << slice.high << ", " << slice.low << '\n';
            output_ << "  " << length << " = add i64 " << span << ", 1\n";
            return length;
        }
        // Low(A), High(A) and Length(A) of a fixed array are constants (CANON-13).
        if (isArrayType(type)) {
            return std::to_string(op == semantic::BuiltinOp::Low ? type->low()
                                  : op == semantic::BuiltinOp::High ? type->high() : type->length());
        }
    }
    if (op == semantic::BuiltinOp::Abs && call.operands.size() == 1 && expressionLlvmType(*call.operands[0]) == "i64") {
        const std::string value = emitValue(*call.operands[0]);
        if (isUInt64Type(bare(*call.operands[0]).type)) {
            return value;  // never negative
        }
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = call i64 @__inox_abs_i64(i64 " << value << ")\n";
        return result;
    }
    if (isFileCall(op)) {
        if (fileCallLlvmType(op) == "void") {
            throw CodegenError("void function call cannot be used as an expression");
        }
        return emitFileCall(call);
    }
    if (isMathBuiltin(op)) {
        return emitMathBuiltinCall(op, call.operands);
    }
    // A3 (Std.Math): Round and Trunc.
    if ((op == semantic::BuiltinOp::Round || op == semantic::BuiltinOp::Trunc) && call.operands.size() == 1) {
        return emitRounding(call);
    }
    throw CodegenUnsupported("unsupported expression in function: " + std::string(routine_.name));
}

std::string FunctionEmitter::emitConversion(const tir::Expr& conversion)
{
    if (conversion.operands.size() == 1) {
        // Change decimals (r3.91): Currency(X), Crypto(X), and T(C) of a
        // decimal into a number.
        const TypeRef target = conversion.type;
        const TypeRef source = bare(*conversion.operands[0]).type;
        if (target != nullptr && source != nullptr &&
            (target->isDecimal() || (source->isDecimal() && (target->isInteger() || target->isFloat())))) {
            return emitDecimalConversion(conversion, target);
        }
    }
    // r3.90 (B2): T(S) of a String into a number.
    if (const TypeRef target = textConversionTarget(conversion)) {
        return emitTextToNumber(conversion, target);
    }
    // Change float-conversions: Float(X), Float32(X), and T(F) of a
    // Float into an integer type.
    if (const TypeRef target = realConversionTarget(conversion)) {
        return emitRealConversion(conversion, target);
    }
    // String(C) (change char-values): the one-character String; String(N)
    // and String(F) (r3.90, B1): the number as Put writes it.
    if (const TypeRef converted = conversion.type;
        conversion.operands.size() == 1 && converted != nullptr && converted->isString()) {
        const tir::Expr& argument = *conversion.operands[0];
        const TypeRef source = bare(argument).type;
        const std::string value = emitValue(argument);
        if (source != nullptr && source->isChar()) {
            return emitCharString(value);
        }
        if (source != nullptr && (source->isInteger() || source->isFloat())) {
            return emitNumberString(value, source, expressionLlvmType(argument));
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
            holdOwnedResult(text, types_.stringType(), "ptr");
            return text;
        }
        return value;
    }
    // CANON-8: Natural(X) checks X >= 0 at run time (fault kind 7);
    // Integer(X) and Int64(X) of an integer value are the identity,
    // because Natural shares Integer's i64 representation.
    if (isNaturalConversion(conversion)) {
        const std::string value = emitValue(*conversion.operands[0]);
        const TypeRef target = conversionTarget(conversion);
        const TypeRef source = bare(*conversion.operands[0]).type;
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
        if (target != types_.naturalType()) {
            return value;
        }
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = call i64 @__inox_natural_i64(i64 " << value << ")\n";
        return result;
    }
    throw CodegenUnsupported("unsupported expression in function: " + std::string(routine_.name));
}

// A literal, or a Const read (its value as the analysis evaluated it).
std::string FunctionEmitter::emitLiteral(const tir::Expr& literal)
{
    const TypeRef type = literal.type;
    if (const semantic::ConstantValue* constant = literal.constant) {
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
            const bool float32 = type != nullptr && type->bits() == 32;
            // A universal Const is read once from its digits (r3.86).
            const double value =
                constant->universal ? support::parseRealLiteral(constant->text, float32) : constant->real;
            return llvmRealConstant(value, float32);
        }
        return std::to_string(constant->integer);
    }
    // None: an empty Link or an absent owner (safe-pointers).
    if (literal.literal == tir::LiteralKind::None) {
        return "zeroinitializer";
    }
    // Change decimals (r3.91): a literal read as a Currency or a Crypto
    // is its exact value scaled by 10^places.
    if (type != nullptr && type->isDecimal()) {
        return support::parseDecimalLiteral(std::string(literal.text), type->decimalPlaces()).scaled;
    }
    switch (literal.literal) {
    case tir::LiteralKind::Integer:
        return llvmIntegerLiteral(std::string(literal.text));
    case tir::LiteralKind::Real: {
        const bool float32 = type != nullptr && type->isFloat() && type->bits() == 32;
        return llvmRealConstant(support::parseRealLiteral(std::string(literal.text), float32), float32);
    }
    case tir::LiteralKind::Bool:
        return literal.boolean ? "1" : "0";
    case tir::LiteralKind::String:
        return emitStringValue(literal.text);
    case tir::LiteralKind::Char:
        return std::to_string(support::decodeCharLiteral(std::string(literal.text)).code);
    default:
        break;
    }
    throw CodegenUnsupported("unsupported expression in function: " + std::string(routine_.name));
}

// A variable of the routine, a parameter, a binding or a State variable.
bool FunctionEmitter::isVariablePlace(const tir::Place& place)
{
    switch (place.kind) {
    case tir::PlaceKind::Local:
    case tir::PlaceKind::Parameter:
    case tir::PlaceKind::Receiver:
    case tir::PlaceKind::LoopIterator:
    case tir::PlaceKind::WithBinding:
    case tir::PlaceKind::HandlerBinding:
    case tir::PlaceKind::State:
        return true;
    default:
        return false;
    }
}

std::string FunctionEmitter::emitVariableRead(const tir::Place& place)
{
    const std::string normalizedName = normalize(place.symbol->name);
    const auto local = locals_.find(normalizedName);
    if (local != locals_.end()) {
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        if (local->second.llvmType.empty()) {
            throw CodegenError("LLVM emission found local value with unknown LLVM type");
        }
        emitStorageGuard(normalizedName);
        output_ << "  " << result << " = load " << local->second.llvmType << ", ptr " << local->second.slot << '\n';
        return result;
    }
    const auto parameter = parameters_.find(normalizedName);
    if (parameter != parameters_.end()) {
        return parameter->second;
    }
    throw CodegenUnsupported("unsupported expression in function: " + std::string(routine_.name));
}

std::string FunctionEmitter::emitUnary(const tir::Expr& unary)
{
    const tir::Expr& operandValue = *unary.operands[0];
    const tir::Expr& operand = bare(operandValue);
    if (unary.unary == tir::UnaryOp::Not) {
        const std::string value = emitValue(operandValue);
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = xor i1 " << value << ", true\n";
        return result;
    }
    if (unary.unary == tir::UnaryOp::Plus) {
        return emitValue(operandValue);
    }
    if (unary.unary == tir::UnaryOp::BitNot) {
        // Two's-complement NOT in the operand's width: for an unsigned
        // narrow type that is `x xor (2^n - 1)`, which stays in range.
        const std::string value = emitValue(operandValue);
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        std::int64_t mask = -1;
        if (const auto range = narrowIntegerRange(operand.type); range && range->low == 0) {
            mask = range->high;
        }
        output_ << "  " << result << " = xor i64 " << value << ", " << mask << '\n';
        return result;
    }
    const std::string operandType = expressionLlvmType(operand);
    // A written literal (not a Const) is negated where it stands.
    if (operand.kind == tir::ExprKind::Literal && operand.constant == nullptr) {
        const TypeRef literalType = operand.type;
        if (literalType != nullptr && literalType->isDecimal()) {
            const std::string scaled =
                support::parseDecimalLiteral(std::string(operand.text), literalType->decimalPlaces()).scaled;
            return scaled == "0" ? scaled : "-" + scaled;
        }
        if (operand.literal == tir::LiteralKind::Integer) {
            return "-" + llvmIntegerLiteral(std::string(operand.text));
        }
        if (operand.literal == tir::LiteralKind::Real) {
            // The literal read in its type, then negated (a Float32
            // `-0.1` is the negation of the Float32 nearest 0.1).
            const bool float32 = literalType != nullptr && literalType->isFloat() && literalType->bits() == 32;
            return llvmRealConstant(-support::parseRealLiteral(std::string(operand.text), float32), float32);
        }
    }
    const std::string value = emitValue(operandValue);
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    if (const TypeRef type = operand.type; type != nullptr && type->isDecimal()) {
        return emitDecimalArithmetic(tir::BinaryOp::Subtract, type, "0", value);
    }
    if (isFloatLlvmType(operandType)) {
        output_ << "  " << result << " = fsub " << operandType << " -0.0, " << value << '\n';
    } else {
        output_ << "  " << result << " = call i64 @__inox_neg_i64(i64 " << value << ")\n";
    }
    return result;
}

// ADR-0013: `if C then A else B`. Only the chosen branch is evaluated, so
// this is control flow, never an LLVM `select`. A branch may end in a block
// other than the one it started in (an `invoke` continuation, a nested
// conditional's merge), so each branch jumps to its own tail block and the
// phi names the tails, whose predecessors are known.
std::string FunctionEmitter::emitConditional(const tir::Expr& expression)
{
    const std::string label = std::to_string(nextLabel_++);
    const std::string type = expressionLlvmType(expression);
    const std::string condition = emitValue(*expression.operands[0]);
    output_ << "  br i1 " << condition << ", label %condthen" << label
            << ", label %condelse" << label << "\n\n";

    output_ << "condthen" << label << ":\n";
    const std::string thenValue = emitValue(*expression.operands[1]);
    output_ << "  br label %condthentail" << label << "\n\n";
    output_ << "condthentail" << label << ":\n";
    output_ << "  br label %condend" << label << "\n\n";

    output_ << "condelse" << label << ":\n";
    const std::string elseValue = emitValue(*expression.operands[2]);
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

// The LLVM type of an expression's value: the LLVM type of the type the
// analysis gave it (a Char is its code point, an i64); Free and the file
// calls without a result are void; None is a null pointer.
std::string FunctionEmitter::expressionLlvmType(const tir::Expr& expression) const
{
    const tir::Expr& value = bare(expression);
    if (value.kind == tir::ExprKind::Builtin && value.op == semantic::BuiltinOp::Free) {
        return "void";
    }
    if (value.kind == tir::ExprKind::Builtin && isFileCall(value.op)) {
        return fileCallLlvmType(value.op);
    }
    const TypeRef type = value.type;
    if (type != nullptr && type->isNone()) {
        return "ptr";
    }
    return valueLlvmType(type);
}

bool FunctionEmitter::isStringConcatenation(const tir::Expr& expression) const
{
    return expression.kind == tir::ExprKind::Binary && expression.binary == tir::BinaryOp::Add &&
           isStringType(expression.type);
}

bool FunctionEmitter::isRangeCheckedArithmetic(const tir::Expr& expression)
{
    if (expression.kind == tir::ExprKind::Unary) {
        return expression.unary == tir::UnaryOp::Minus;
    }
    if (expression.kind != tir::ExprKind::Binary) {
        return false;
    }
    switch (expression.binary) {
    case tir::BinaryOp::Add:
    case tir::BinaryOp::Subtract:
    case tir::BinaryOp::Multiply:
    case tir::BinaryOp::Power:
    case tir::BinaryOp::Divide:
    case tir::BinaryOp::Modulo:
    case tir::BinaryOp::Remainder:
    case tir::BinaryOp::ShiftLeft:
    case tir::BinaryOp::ShiftRight:
    case tir::BinaryOp::BitAnd:
    case tir::BinaryOp::BitOr:
    case tir::BinaryOp::BitXor:
        return true;
    default:
        return false;
    }
}

// `result` names the value; the operands are already evaluated.
std::string FunctionEmitter::emitBinaryValues(const tir::Expr& binary, const std::string& result,
                                              const std::string& left, std::string right)
{
    const tir::Expr& leftValue = bare(*binary.operands[0]);
    const tir::Expr& rightValue = bare(*binary.operands[1]);
    const std::string leftType = expressionLlvmType(leftValue);
    const std::string rightType = expressionLlvmType(rightValue);

    // CANON-8: the shift count of a fixed-width type is 0..width-1.
    if (binary.binary == tir::BinaryOp::ShiftLeft ||
        binary.binary == tir::BinaryOp::ShiftRight) {
        if (const auto range = narrowIntegerRange(binary.type)) {
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
    if (isStringType(leftValue.type) &&
        (binary.binary == tir::BinaryOp::Equal || binary.binary == tir::BinaryOp::NotEqual)) {
        requireRuntime(RuntimeFunction::StringEqual);
        const std::string equal = binary.binary == tir::BinaryOp::Equal
                                      ? result
                                      : "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << equal << " = call zeroext i1 @__inox_str_equal(ptr " << left << ", ptr " << right
                << ")\n";
        if (binary.binary == tir::BinaryOp::NotEqual) {
            output_ << "  " << result << " = xor i1 " << equal << ", true\n";
        }
        return result;
    }
    // CANON-14 (v3.56): `<` `>` `<=` `>=` order Strings byte by byte.
    if (isStringType(leftValue.type)) {
        if (const std::string predicate = llvmComparisonPredicate(binary.binary); !predicate.empty()) {
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
    if (const TypeRef type = leftValue.type; type != nullptr && type->isDecimal()) {
        if (const std::string predicate = llvmComparisonPredicate(binary.binary); !predicate.empty()) {
            output_ << "  " << result << " = icmp " << predicate << ' ' << leftType << ' ' << left << ", "
                    << right << '\n';
            return result;
        }
        return emitDecimalArithmetic(binary.binary, type, left, right);
    }

    if (isFloatLlvmType(leftType) || isFloatLlvmType(rightType)) {
        if (const std::string predicate = fcmpPredicate(binary.binary); !predicate.empty()) {
            output_ << "  " << result << " = fcmp " << predicate << ' ' << leftType
                    << ' ' << left << ", " << right << '\n';
            return result;
        }
        if (binary.binary == tir::BinaryOp::Power) {
            output_ << "  " << result << " = call double @llvm.pow.f64(double "
                    << left << ", double " << right << ")\n";
            return result;
        }
        output_ << "  " << result << " = " << llvmFloatOperation(binary.binary) << ' '
                << leftType << ' ' << left << ", " << right << '\n';
        return result;
    }

    if (const std::string predicate = llvmComparisonPredicate(binary.binary);
        !predicate.empty()) {
        // Every integer type is carried as i64 (signed compare). Bool
        // is i1 and ordered false < true (v3.57): compared unsigned,
        // since a signed i1 reads true as -1.
        const bool isBool = leftType == "i1";
        // A UInt64 is ordered by its unsigned value (change uint64).
        const bool isUnsigned =
            isBool || isUInt64Type(leftValue.type) || isUInt64Type(rightValue.type);
        std::string boolPredicate = predicate;
        if (isUnsigned && boolPredicate.front() == 's') {
            boolPredicate.front() = 'u';
        }
        output_ << "  " << result << " = icmp " << (isUnsigned ? boolPredicate : predicate) << ' '
                << (isBool ? "i1" : "i64") << ' ' << left << ", " << right << '\n';
    } else if (isUInt64Type(binary.type) && !llvmUnsignedHelper(binary.binary).empty()) {
        requireUnsignedHelpers();
        output_ << "  " << result << " = call i64 @" << llvmUnsignedHelper(binary.binary) << "(i64 " << left
                << ", i64 " << right << ")\n";
    } else if (const std::string operation = llvmBooleanOperation(binary.binary);
               !operation.empty()) {
        output_ << "  " << result << " = " << operation << " i1 "
                << left << ", " << right << '\n';
    } else if (binary.binary == tir::BinaryOp::Power) {
        output_ << "  " << result << " = call i64 @__inox_ipow_i64(i64 "
                << left << ", i64 " << right << ")\n";
    } else if (const std::string helper = checkedIntegerHelper(binary.binary); !helper.empty()) {
        output_ << "  " << result << " = call i64 @" << helper << "(i64 "
                << left << ", i64 " << right << ")\n";
    } else {
        output_ << "  " << result << " = " << llvmOperation(binary.binary) << " i64 "
                << left << ", " << right << '\n';
    }
    return result;
}

std::string FunctionEmitter::emitMathBuiltinCall(semantic::BuiltinOp op, tir::Span<const tir::Expr*> arguments)
{
    std::vector<std::string> values;
    values.reserve(arguments.size());
    for (const tir::Expr* argument : arguments) {
        values.push_back(emitValue(*argument));
    }
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);

    const auto unaryScale = [&](double factor) {
        output_ << "  " << result << " = fmul double " << values.at(0) << ", " << std::scientific << factor << std::defaultfloat << "\n";
        return result;
    };

    if (op == semantic::BuiltinOp::RadToDeg) return unaryScale(57.295779513082320876798154814105);
    if (op == semantic::BuiltinOp::DegToRad) return unaryScale(0.017453292519943295769236907684886);
    if (op == semantic::BuiltinOp::RadToGrad) return unaryScale(63.661977236758134307553505349005);
    if (op == semantic::BuiltinOp::GradToRad) return unaryScale(0.015707963267948966192313216916398);
    if (op == semantic::BuiltinOp::RadToCycle) return unaryScale(0.15915494309189533576888376337251);
    if (op == semantic::BuiltinOp::CycleToRad) return unaryScale(6.283185307179586476925286766559);

    if (op == semantic::BuiltinOp::LogN) {
        const std::string logX = "%tmp" + std::to_string(nextTemporary_++);
        const std::string logBase = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << logX << " = call double @llvm.log.f64(double " << values.at(1) << ")\n";
        output_ << "  " << logBase << " = call double @llvm.log.f64(double " << values.at(0) << ")\n";
        output_ << "  " << result << " = fdiv double " << logX << ", " << logBase << "\n";
        return result;
    }
    if (op == semantic::BuiltinOp::Hypot3) {
        const std::string xy = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << xy << " = call double @hypot(double " << values.at(0) << ", double " << values.at(1) << ")\n";
        output_ << "  " << result << " = call double @hypot(double " << xy << ", double " << values.at(2) << ")\n";
        return result;
    }

    if (const std::string_view intrinsic = mathIntrinsicName(op); !intrinsic.empty()) {
        output_ << "  " << result << " = call double @" << intrinsic << "(";
    } else if (const std::string_view libm = mathLibmName(op); !libm.empty()) {
        output_ << "  " << result << " = call double @" << libm << "(";
    } else {
        throw CodegenUnsupported("unsupported math builtin: " + std::string(semantic::builtinName(op)));
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
    holdOwnedResult(result, types_.stringType(), "ptr");
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
std::string FunctionEmitter::emitDecimalArithmetic(tir::BinaryOp op, TypeRef type, const std::string& left,
                                                   const std::string& right)
{
    const std::string llvmType = type->bits() == 128 ? "i128" : "i64";
    if (op == tir::BinaryOp::Add || op == tir::BinaryOp::Subtract) {
        const std::string intrinsic =
            std::string(op == tir::BinaryOp::Add ? "llvm.sadd.with.overflow." : "llvm.ssub.with.overflow.") +
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
    if (op != tir::BinaryOp::Multiply && op != tir::BinaryOp::Divide) {
        throw CodegenError("operator not defined for " + type->name());
    }
    const bool multiply = op == tir::BinaryOp::Multiply;
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
std::string FunctionEmitter::emitDecimalConversion(const tir::Expr& conversion, TypeRef target)
{
    const tir::Expr& argument = *conversion.operands[0];
    const TypeRef source = bare(argument).type;
    const std::string value = emitValue(argument);
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
std::string FunctionEmitter::emitRounding(const tir::Expr& call)
{
    const bool round = call.op == semantic::BuiltinOp::Round;
    const tir::Expr& argument = *call.operands[0];
    const TypeRef source = bare(argument).type;
    const std::string value = emitValue(argument);
    if (source->isDecimal()) {
        const auto [low, high] = decimalHalves(value, source);
        return emitDecimalToInteger(low, high, source, types_.int64Type(), round);
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
    holdOwnedResult(result, types_.stringType(), "ptr");
    return result;
}

// r3.90 (B2): the numeric target of `T(S)` with S a String; else null.
TypeRef FunctionEmitter::textConversionTarget(const tir::Expr& conversion) const
{
    if (conversion.operands.size() != 1) {
        return nullptr;
    }
    const TypeRef source = bare(*conversion.operands[0]).type;
    if (source == nullptr || !source->isString()) {
        return nullptr;
    }
    const TypeRef target = conversion.type;
    if (target == nullptr || target->isEnum() || !(target->isInteger() || target->isFloat())) {
        return nullptr;
    }
    return target;
}

// r3.90 (B2): text into a number, by libinoxrt; text that is not a number
// of the target raises ConversionError (an `invoke` inside try).
std::string FunctionEmitter::emitTextToNumber(const tir::Expr& conversion, TypeRef target)
{
    const std::string text = emitValue(*conversion.operands[0]);
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
TypeRef FunctionEmitter::realConversionTarget(const tir::Expr& conversion) const
{
    if (conversion.operands.size() != 1) {
        return nullptr;
    }
    const TypeRef target = conversion.type;
    const TypeRef source = bare(*conversion.operands[0]).type;
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
std::string FunctionEmitter::emitRealConversion(const tir::Expr& conversion, TypeRef target)
{
    const tir::Expr& argument = *conversion.operands[0];
    const TypeRef source = bare(argument).type;
    const std::string value = emitValue(argument);
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
    if (unsignedTarget || target == types_.int64Type()) {
        return integer;
    }
    return emitRangeCheck(integer, IntegerRange{target->minValue(), target->maxValue()}, 8);
}

} // namespace inox::compiler::codegen::detail
