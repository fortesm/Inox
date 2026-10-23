// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: constant evaluation and folding, static values, case choices.

#include "SemanticAnalyzer.h"
#include "../support/DecimalLiteral.h"
#include "../support/RealLiteral.h"

#include "SemanticSupport.h"
#include "../exceptions/ExceptionTypes.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

namespace inox::compiler::semantic {

using namespace detail;

bool SemanticAnalyzer::constantIntegerValue(const ast::Expression& expression, std::int64_t& value) const
{
    const auto found = constants_.find(&expression);
    if (found == constants_.end()) {
        return false;
    }
    value = found->second;
    return true;
}

bool SemanticAnalyzer::integerConstant(const ast::Expression& expression, IntegerConstant& value) const
{
    if (!constantIntegerValue(expression, value.bits)) {
        return false;
    }
    value.isUnsigned = isUInt64Type(result_.typeOf(expression));
    return true;
}

// A constant right operand of a UInt64 operation is read unsigned: a pattern
// with the top bit set is a huge count or exponent, never a negative one.
void SemanticAnalyzer::rejectInvalidConstantRightOperand(ast::BinaryOperator op, std::int64_t value, bool isUnsigned)
{
    switch (op) {
    case ast::BinaryOperator::IntegerDivide:
        if (value == 0) {
            throw SemanticError("constant division by zero in 'div'");
        }
        return;
    case ast::BinaryOperator::Modulo:
        if (value == 0) {
            throw SemanticError("constant division by zero in 'mod'");
        }
        return;
    case ast::BinaryOperator::ShiftLeft:
    case ast::BinaryOperator::ShiftRight:
        if (static_cast<std::uint64_t>(value) > 63) {
            throw SemanticError("constant shift count out of range: must be between 0 and 63");
        }
        return;
    case ast::BinaryOperator::Power:
        if (value < 0 && !isUnsigned) {
            throw SemanticError("constant exponent must not be negative");
        }
        return;
    default:
        return;
    }
}

// A module Const whose value is a single Integer or Bool literal is resolved
// here, once. Other forms stay unresolved; the backend then reports them as
// "not yet implemented" instead of guessing (docs/BACKEND_GAPS.md).
void SemanticAnalyzer::recordConstantValue(std::string_view name, std::string_view valueToken)
{
    const Symbol* symbol = symbols_.currentScope().resolve(name);
    if (symbol == nullptr || symbol->kind != SymbolKind::Constant) {
        return;
    }
    ConstantValue value;
    if (!valueToken.empty() && valueToken.front() == '\'') {
        std::int64_t code = 0;
        if (decodeCharLiteral(valueToken, code)) {
            charConstants_[normalizeName(name)] = code;
            // The backend reads a Char Const as its code point (char-values).
            value.kind = ConstantValue::Kind::Integer;
            value.integer = code;
            result_.setConstantValue(*symbol, value);
        }
        return;
    }
    // A real literal: a universal real Const (r3.86).
    if (!valueToken.empty() && std::isdigit(static_cast<unsigned char>(valueToken.front())) != 0 &&
        valueToken.find('.') != std::string_view::npos) {
        value.kind = ConstantValue::Kind::Real;
        value.real = support::parseRealLiteral(valueToken);
        if (std::isinf(value.real)) {
            throw SemanticError("value out of range for Float: " + std::string(valueToken));
        }
        value.text = std::string(valueToken);
        value.universal = true;
        result_.setConstantValue(*symbol, value);
        return;
    }
    if (equalsIgnoreCase(valueToken, "true") || equalsIgnoreCase(valueToken, "false")) {
        value.kind = ConstantValue::Kind::Boolean;
        value.boolean = equalsIgnoreCase(valueToken, "true");
        result_.setConstantValue(*symbol, value);
        return;
    }
    const bool looksInteger = !valueToken.empty() &&
        (valueToken.front() == '$' || std::isdigit(static_cast<unsigned char>(valueToken.front())) != 0) &&
        valueToken.find('.') == std::string_view::npos;
    if (!looksInteger) {
        return;
    }
    std::uint64_t magnitude = 0;
    if (!parseIntegerLiteralMagnitude(valueToken, magnitude) || magnitude >= kInt64MagnitudeOfMin) {
        // A Const takes the type of its value; a UInt64 one is a conversion
        // (change uint64).
        throw SemanticError("integer literal does not fit in Int64 in Const " + std::string(name) + ": " +
                            std::string(valueToken) +
                            (magnitude >= kInt64MagnitudeOfMin ? " (a UInt64 Const is written Const " +
                                                                    std::string(name) + " := UInt64(" +
                                                                    std::string(valueToken) + "))"
                                                              : std::string()));
    }
    value.kind = ConstantValue::Kind::Integer;
    value.integer = static_cast<std::int64_t>(magnitude);
    result_.setConstantValue(*symbol, value);
}

void SemanticAnalyzer::foldConstantExpression(const ast::Expression& expression)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::IdentifierExpression: {
        // A named Integer constant is a constant expression (CANON-19): overflow
        // or a zero divisor involving it is a compile-time error.
        const Symbol* symbol = result_.symbolOf(static_cast<const ast::IdentifierExpression&>(expression));
        if (symbol == nullptr || symbol->kind != SymbolKind::Constant) {
            return;
        }
        const ConstantValue* value = result_.constantValueOf(*symbol);
        if (value != nullptr && value->kind == ConstantValue::Kind::Integer) {
            constants_[&expression] = value->integer;
        }
        return;
    }
    case ast::AstNodeKind::LiteralExpression: {
        const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
        if (literal.literalKind() != ast::LiteralKind::Integer) {
            return;
        }
        std::uint64_t magnitude = 0;
        if (parseIntegerLiteralMagnitude(literal.value(), magnitude) &&
            (magnitude < kInt64MagnitudeOfMin || isUInt64Type(result_.typeOf(expression)))) {
            constants_[&expression] = static_cast<std::int64_t>(magnitude);
        }
        return;
    }
    case ast::AstNodeKind::CallExpression: {
        // `UInt64(18446744073709551615)` of a constant is the constant, so a
        // UInt64 Const can be written (change uint64); its range was checked
        // with the call. Other conversions are not folded yet.
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        std::int64_t operand = 0;
        if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression && call.arguments().size() == 1) {
            const TypeRef target = types().lookup(static_cast<const ast::IdentifierExpression&>(call.callee()).name());
            if (isUInt64Type(target) && result_.typeOf(expression) == target &&
                constantIntegerValue(*call.arguments()[0], operand)) {
                constants_[&expression] = operand;
            }
        }
        return;
    }
    case ast::AstNodeKind::UnaryExpression: {
        const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
        std::int64_t operand = 0;
        if (!constantIntegerValue(unary.operand(), operand)) {
            return;
        }
        switch (unary.op()) {
        case ast::UnaryOperator::Plus:
            constants_[&expression] = operand;
            return;
        case ast::UnaryOperator::Minus:
            if (operand == kInt64Min) {
                throwConstantOverflow("negation");
            }
            constants_[&expression] = -operand;
            return;
        case ast::UnaryOperator::BitNot:
            constants_[&expression] = ~operand;
            return;
        default:
            return;
        }
    }
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        std::int64_t a = 0;
        std::int64_t b = 0;
        // A UInt64 operation: a constant next to a UInt64 keeps its Int64
        // type in the result, so either side (or the result) tells.
        const bool isUnsigned = isUInt64Type(result_.typeOf(binary)) || isUInt64Type(result_.typeOf(binary.left())) ||
                                isUInt64Type(result_.typeOf(binary.right()));
        // CANON-8: a CONSTANT divisor of zero is a compile-time error even when the
        // dividend is only known at run time (`A div 0`). The same holds for a
        // constant shift count outside 0..63 and a constant negative exponent:
        // such an operation can never succeed.
        if (constantIntegerValue(binary.right(), b)) {
            rejectInvalidConstantRightOperand(binary.op(), b, isUnsigned);
        }
        if (!constantIntegerValue(binary.left(), a) || !constantIntegerValue(binary.right(), b)) {
            return;
        }
        if (isUnsigned) {
            foldUnsignedBinary(binary, static_cast<std::uint64_t>(a), static_cast<std::uint64_t>(b));
            return;
        }
        std::int64_t out = 0;
        switch (binary.op()) {
        case ast::BinaryOperator::Add:
            if (!checkedAdd(a, b, out)) throwConstantOverflow("addition");
            constants_[&expression] = out;
            return;
        case ast::BinaryOperator::Subtract:
            if (!checkedSub(a, b, out)) throwConstantOverflow("subtraction");
            constants_[&expression] = out;
            return;
        case ast::BinaryOperator::Multiply:
            if (!checkedMul(a, b, out)) throwConstantOverflow("multiplication");
            constants_[&expression] = out;
            return;
        case ast::BinaryOperator::IntegerDivide:
            if (b == 0) {
                throw SemanticError("constant division by zero in 'div'");
            }
            if (a == kInt64Min && b == -1) {
                throwConstantOverflow("division");
            }
            constants_[&expression] = a / b;
            return;
        case ast::BinaryOperator::Modulo:
            if (b == 0) {
                throw SemanticError("constant division by zero in 'mod'");
            }
            constants_[&expression] = (b == -1) ? 0 : a % b;
            return;
        case ast::BinaryOperator::ShiftLeft:
        case ast::BinaryOperator::ShiftRight:
            if (b < 0 || b > 63) {
                throw SemanticError("constant shift count out of range: must be between 0 and 63");
            }
            if (binary.op() == ast::BinaryOperator::ShiftLeft) {
                constants_[&expression] =
                    static_cast<std::int64_t>(static_cast<std::uint64_t>(a) << static_cast<unsigned>(b));
            } else {
                constants_[&expression] = a >> static_cast<unsigned>(b);
            }
            return;
        case ast::BinaryOperator::BitAnd:
            constants_[&expression] = a & b;
            return;
        case ast::BinaryOperator::BitOr:
            constants_[&expression] = a | b;
            return;
        case ast::BinaryOperator::BitXor:
            constants_[&expression] = a ^ b;
            return;
        case ast::BinaryOperator::Power: {
            if (b < 0) {
                throw SemanticError("constant exponent must not be negative");
            }
            std::int64_t result = 1;
            std::int64_t base = a;
            std::int64_t exponent = b;
            while (exponent > 0) {
                if ((exponent & 1) != 0 && !checkedMul(result, base, result)) {
                    throwConstantOverflow("exponentiation");
                }
                exponent >>= 1;
                if (exponent > 0 && !checkedMul(base, base, base)) {
                    throwConstantOverflow("exponentiation");
                }
            }
            constants_[&expression] = result;
            return;
        }
        default:
            return;
        }
    }
    default:
        return;
    }
}

// UInt64 constants fold in the unsigned domain (change uint64): a result
// below 0 or above 2^64-1 is a compile-time overflow, never a wraparound.
void SemanticAnalyzer::foldUnsignedBinary(const ast::BinaryExpression& binary, std::uint64_t a, std::uint64_t b)
{
    std::uint64_t out = 0;
    const auto store = [&](std::uint64_t value) { constants_[&binary] = static_cast<std::int64_t>(value); };
    switch (binary.op()) {
    case ast::BinaryOperator::Add:
        if (!checkedAddUnsigned(a, b, out)) throwConstantOverflow("addition", "UInt64");
        return store(out);
    case ast::BinaryOperator::Subtract:
        if (!checkedSubUnsigned(a, b, out)) throwConstantOverflow("subtraction", "UInt64");
        return store(out);
    case ast::BinaryOperator::Multiply:
        if (!checkedMulUnsigned(a, b, out)) throwConstantOverflow("multiplication", "UInt64");
        return store(out);
    case ast::BinaryOperator::IntegerDivide:
        return store(a / b);  // a zero divisor was rejected above
    case ast::BinaryOperator::Modulo:
        return store(a % b);
    case ast::BinaryOperator::ShiftLeft:
        if (b > 0 && (a >> (64 - b)) != 0) throwConstantOverflow("shift", "UInt64");
        return store(a << b);
    case ast::BinaryOperator::ShiftRight:
        return store(a >> b);
    case ast::BinaryOperator::BitAnd:
        return store(a & b);
    case ast::BinaryOperator::BitOr:
        return store(a | b);
    case ast::BinaryOperator::BitXor:
        return store(a ^ b);
    case ast::BinaryOperator::Power: {
        std::uint64_t result = 1;
        for (std::uint64_t exponent = b; exponent > 0; exponent >>= 1) {
            if ((exponent & 1) != 0 && !checkedMulUnsigned(result, a, result)) {
                throwConstantOverflow("exponentiation", "UInt64");
            }
            if (exponent > 1 && !checkedMulUnsigned(a, a, a)) {
                throwConstantOverflow("exponentiation", "UInt64");
            }
        }
        return store(result);
    }
    default:
        return;
    }
}

// A Const whose initializer is a constant expression (`Const K := 2 + 3`,
// `Const Ready := not False`) gets its value here, after the initializer was
// analyzed and folded. Single-token values were already recorded by the
// section scanner; this covers the rest (ADR-0011 static choices; the
// backend reads the same value).
void SemanticAnalyzer::recordConstantExpression(const std::string& name,
                                                const ast::Expression& value)
{
    const Symbol* symbol = symbols_.currentScope().resolve(name);
    if (symbol == nullptr || symbol->kind != SymbolKind::Constant ||
        result_.constantValueOf(*symbol) != nullptr ||
        charConstants_.contains(normalizeName(name))) {
        return;
    }
    ConstantValue constant;
    std::int64_t integer = 0;
    bool boolean = false;
    // Change decimals (r3.91): `Const Fee := Currency(2.5)`, `Crypto(-1)`,
    // or another decimal Const.
    if (const TypeRef type = result_.typeOf(value); type != nullptr && type->isDecimal()) {
        std::string scaled;
        if (staticDecimalValue(value, scaled)) {
            constant.kind = ConstantValue::Kind::Decimal;
            constant.text = scaled;
            result_.setConstantValue(*symbol, constant);
        }
        return;
    }
    if (constantIntegerValue(value, integer)) {
        constant.kind = ConstantValue::Kind::Integer;
        constant.integer = integer;
        result_.setConstantValue(*symbol, constant);
    } else if (double real = 0.0; staticRealValue(value, real)) {
        constant.kind = ConstantValue::Kind::Real;
        constant.real = real;
        // `Const Low := -2.5`: a negated real literal is universal too.
        if (value.kind() == ast::AstNodeKind::UnaryExpression &&
            static_cast<const ast::UnaryExpression&>(value).op() == ast::UnaryOperator::Minus &&
            static_cast<const ast::UnaryExpression&>(value).operand().kind() == ast::AstNodeKind::LiteralExpression) {
            const auto& literal =
                static_cast<const ast::LiteralExpression&>(static_cast<const ast::UnaryExpression&>(value).operand());
            if (literal.literalKind() == ast::LiteralKind::Float) {
                constant.text = "-" + std::string(literal.value());
                constant.universal = true;
            }
        }
        result_.setConstantValue(*symbol, constant);
    } else if (staticBoolValue(value, boolean)) {
        constant.kind = ConstantValue::Kind::Boolean;
        constant.boolean = boolean;
        result_.setConstantValue(*symbol, constant);
    } else if (value.kind() == ast::AstNodeKind::LiteralExpression &&
               static_cast<const ast::LiteralExpression&>(value).literalKind() == ast::LiteralKind::String) {
        // `Const Arquivo := "dados.txt"`: the backend emits the literal.
        constant.kind = ConstantValue::Kind::String;
        constant.text = static_cast<const ast::LiteralExpression&>(value).value();
        result_.setConstantValue(*symbol, constant);
    }
}

// The exact scaled value of a constant Currency or Crypto: a conversion of
// a numeric literal (possibly negated) or a decimal Const.
bool SemanticAnalyzer::staticDecimalValue(const ast::Expression& expression, std::string& scaled) const
{
    const TypeRef type = result_.typeOf(expression);
    if (type == nullptr || !type->isDecimal()) {
        return false;
    }
    const ast::Expression* inner = &expression;
    if (expression.kind() == ast::AstNodeKind::CallExpression) {
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (call.arguments().size() != 1 || result_.typeOf(*call.arguments()[0]) != type) {
            return false;
        }
        inner = call.arguments()[0].get();
    }
    if (const Symbol* symbol = resolveConstant(*inner)) {
        const ConstantValue* constant = result_.constantValueOf(*symbol);
        if (constant == nullptr || constant->kind != ConstantValue::Kind::Decimal) {
            return false;
        }
        scaled = constant->text;
        return true;
    }
    bool negative = false;
    if (inner->kind() == ast::AstNodeKind::UnaryExpression) {
        const auto& unary = static_cast<const ast::UnaryExpression&>(*inner);
        if (unary.op() != ast::UnaryOperator::Minus && unary.op() != ast::UnaryOperator::Plus) {
            return false;
        }
        negative = unary.op() == ast::UnaryOperator::Minus;
        inner = &unary.operand();
    }
    if (inner->kind() != ast::AstNodeKind::LiteralExpression) {
        return false;
    }
    const support::DecimalLiteral literal =
        support::parseDecimalLiteral(static_cast<const ast::LiteralExpression&>(*inner).value(),
                                     type->decimalPlaces());
    if (literal.status != support::DecimalLiteralStatus::Ok) {
        return false;
    }
    scaled = negative && literal.scaled != "0" ? "-" + literal.scaled : literal.scaled;
    return true;
}

const Symbol* SemanticAnalyzer::resolveConstant(const ast::Expression& expression) const
{
    if (expression.kind() != ast::AstNodeKind::IdentifierExpression) {
        return nullptr;
    }
    const Symbol* symbol = symbols_.currentScope().resolve(
        static_cast<const ast::IdentifierExpression&>(expression).name());
    return symbol != nullptr && symbol->kind == SymbolKind::Constant ? symbol : nullptr;
}

bool SemanticAnalyzer::staticRealValue(const ast::Expression& expression, double& value) const
{
    const TypeRef type = result_.typeOf(expression);
    if (type == nullptr || !type->isFloat()) {
        return false;
    }
    const auto rounded = [&](double real) {
        return type->bits() == 32 ? static_cast<double>(static_cast<float>(real)) : real;
    };
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression: {
        const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
        if (literal.literalKind() != ast::LiteralKind::Float) {
            return false;
        }
        // A Float32 literal is rounded once, from its decimal text (r3.86).
        value = support::parseRealLiteral(literal.value(), type->bits() == 32);
        return true;
    }
    case ast::AstNodeKind::IdentifierExpression: {
        const Symbol* symbol = resolveConstant(expression);
        const ConstantValue* constant = symbol != nullptr ? result_.constantValueOf(*symbol) : nullptr;
        if (constant == nullptr || constant->kind != ConstantValue::Kind::Real) {
            return false;
        }
        value = constant->universal ? support::parseRealLiteral(constant->text, type->bits() == 32)
                                    : rounded(constant->real);
        return true;
    }
    case ast::AstNodeKind::UnaryExpression: {
        const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
        double operand = 0.0;
        if ((unary.op() != ast::UnaryOperator::Minus && unary.op() != ast::UnaryOperator::Plus) ||
            !staticRealValue(unary.operand(), operand)) {
            return false;
        }
        value = unary.op() == ast::UnaryOperator::Minus ? -operand : operand;
        return true;
    }
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        double left = 0.0;
        double right = 0.0;
        if (!staticRealValue(binary.left(), left) || !staticRealValue(binary.right(), right)) {
            return false;
        }
        switch (binary.op()) {
        case ast::BinaryOperator::Add:
            value = rounded(left + right);
            return true;
        case ast::BinaryOperator::Subtract:
            value = rounded(left - right);
            return true;
        case ast::BinaryOperator::Multiply:
            value = rounded(left * right);
            return true;
        case ast::BinaryOperator::Divide:
            value = rounded(left / right);
            return true;
        default:
            return false;
        }
    }
    case ast::AstNodeKind::CallExpression: {
        // Float(X) and Float32(X) of a constant real or integer.
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 1 ||
            types().lookup(static_cast<const ast::IdentifierExpression&>(call.callee()).name()) != type) {
            return false;
        }
        double operand = 0.0;
        std::int64_t integer = 0;
        if (staticRealValue(*call.arguments()[0], operand)) {
            value = rounded(operand);
            return true;
        }
        const TypeRef from = result_.typeOf(*call.arguments()[0]);
        if (from != nullptr && from->isInteger() && !isUInt64Type(from) &&
            constantIntegerValue(*call.arguments()[0], integer)) {
            value = rounded(static_cast<double>(integer));
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

bool SemanticAnalyzer::staticBoolValue(const ast::Expression& expression, bool& value) const
{
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression: {
        const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
        if (literal.literalKind() != ast::LiteralKind::Boolean) {
            return false;
        }
        value = equalsIgnoreCase(literal.value(), "true");
        return true;
    }
    case ast::AstNodeKind::IdentifierExpression: {
        const Symbol* symbol = resolveConstant(expression);
        if (symbol == nullptr) {
            return false;
        }
        const ConstantValue* constant = result_.constantValueOf(*symbol);
        if (constant == nullptr || constant->kind != ConstantValue::Kind::Boolean) {
            return false;
        }
        value = constant->boolean;
        return true;
    }
    case ast::AstNodeKind::UnaryExpression: {
        const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
        bool operand = false;
        if (unary.op() != ast::UnaryOperator::Not || !staticBoolValue(unary.operand(), operand)) {
            return false;
        }
        value = !operand;
        return true;
    }
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        bool left = false;
        bool right = false;
        switch (binary.op()) {
        case ast::BinaryOperator::And:
        case ast::BinaryOperator::Or:
        case ast::BinaryOperator::Xor:
            if (!staticBoolValue(binary.left(), left) || !staticBoolValue(binary.right(), right)) {
                return false;
            }
            value = binary.op() == ast::BinaryOperator::And ? (left && right)
                  : binary.op() == ast::BinaryOperator::Or  ? (left || right)
                                                            : (left != right);
            return true;
        case ast::BinaryOperator::Equal:
        case ast::BinaryOperator::NotEqual:
        case ast::BinaryOperator::Less:
        case ast::BinaryOperator::Greater:
        case ast::BinaryOperator::LessEqual:
        case ast::BinaryOperator::GreaterEqual: {
            std::int64_t a = 0;
            std::int64_t b = 0;
            if (constantIntegerValue(binary.left(), a) && constantIntegerValue(binary.right(), b)) {
                if (isUInt64Type(result_.typeOf(binary.left())) || isUInt64Type(result_.typeOf(binary.right()))) {
                    a = caseKey(a);  // the unsigned order, as Int64
                    b = caseKey(b);
                }
                switch (binary.op()) {
                case ast::BinaryOperator::Equal: value = a == b; break;
                case ast::BinaryOperator::NotEqual: value = a != b; break;
                case ast::BinaryOperator::Less: value = a < b; break;
                case ast::BinaryOperator::Greater: value = a > b; break;
                case ast::BinaryOperator::LessEqual: value = a <= b; break;
                default: value = a >= b; break;
                }
                return true;
            }
            if ((binary.op() == ast::BinaryOperator::Equal ||
                 binary.op() == ast::BinaryOperator::NotEqual) &&
                staticBoolValue(binary.left(), left) && staticBoolValue(binary.right(), right)) {
                value = (left == right) == (binary.op() == ast::BinaryOperator::Equal);
                return true;
            }
            return false;
        }
        default:
            return false;
        }
    }
    default:
        return false;
    }
}

bool SemanticAnalyzer::staticCharValue(const ast::Expression& expression, std::int64_t& value) const
{
    if (expression.kind() == ast::AstNodeKind::LiteralExpression) {
        const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
        return literal.literalKind() == ast::LiteralKind::Char &&
               decodeCharLiteral(literal.value(), value);
    }
    if (const Symbol* symbol = resolveConstant(expression)) {
        const auto found = charConstants_.find(normalizeName(symbol->name));
        if (found != charConstants_.end()) {
            value = found->second;
            return true;
        }
    }
    return false;
}

std::int64_t SemanticAnalyzer::caseChoiceValue(const ast::Expression& choice, TypeRef selectorType)
{
    CaseDomain domain;
    caseDomainFor(selectorType, domain);
    const TypeRef choiceType = analyzeExpressionFor(choice, selectorType);
    const char* kStatic =
        "a case choice must be a static value: a literal, a Const, or a constant "
        "expression (ADR-0011)";
    const auto mismatch = [&]() {
        return SemanticError("case choice type " + types::nameOf(choiceType) +
                             " does not match selector type " + types::nameOf(selectorType));
    };

    if (domain.isBool) {
        if (choiceType == nullptr || !choiceType->isBool()) {
            throw mismatch();
        }
        bool value = false;
        if (!staticBoolValue(choice, value)) {
            throw SemanticError(kStatic);
        }
        return value ? 1 : 0;
    }

    if (domain.isChar) {
        if (choiceType == nullptr || !choiceType->isChar()) {
            throw mismatch();
        }
        std::int64_t code = 0;
        if (!staticCharValue(choice, code)) {
            throw SemanticError(kStatic);
        }
        return code;
    }

    if (domain.enumType != nullptr) {
        if (choiceType != selectorType) {
            throw mismatch();
        }
        std::int64_t ordinal = 0;
        if (!constantIntegerValue(choice, ordinal)) {
            throw SemanticError(kStatic);
        }
        return ordinal;
    }
    if (!isIntegerType(choiceType)) {
        throw mismatch();
    }
    // A typed value (a Const, or an expression that names one) keeps its
    // type: it must be the selector's type. A literal-only expression is
    // universal and only has to fit the selector's range (checked by the
    // caller).
    if (!isLiteralOnly(choice) && choiceType != selectorType) {
        throw mismatch();
    }
    std::int64_t value = 0;
    if (!constantIntegerValue(choice, value)) {
        throw SemanticError(kStatic);
    }
    return domain.isUnsigned64 ? caseKey(value) : value;
}

} // namespace inox::compiler::semantic
