// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: constant evaluation and folding, static values, case choices.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"
#include "../exceptions/ExceptionTypes.h"
#include "../support/ArrayType.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
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

void SemanticAnalyzer::rejectInvalidConstantRightOperand(ast::BinaryOperator op, std::int64_t value)
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
        if (value < 0 || value > 63) {
            throw SemanticError("constant shift count out of range: must be between 0 and 63");
        }
        return;
    case ast::BinaryOperator::Power:
        if (value < 0) {
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
        }
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
        throw SemanticError("integer literal does not fit in Int64 in Const " + std::string(name) +
                            ": " + std::string(valueToken));
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
        if (parseIntegerLiteralMagnitude(literal.value(), magnitude) && magnitude < kInt64MagnitudeOfMin) {
            constants_[&expression] = static_cast<std::int64_t>(magnitude);
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
        // CANON-8: a CONSTANT divisor of zero is a compile-time error even when the
        // dividend is only known at run time (`A div 0`). The same holds for a
        // constant shift count outside 0..63 and a constant negative exponent:
        // such an operation can never succeed.
        if (constantIntegerValue(binary.right(), b)) {
            rejectInvalidConstantRightOperand(binary.op(), b);
        }
        if (!constantIntegerValue(binary.left(), a) || !constantIntegerValue(binary.right(), b)) {
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
    if (constantIntegerValue(value, integer)) {
        constant.kind = ConstantValue::Kind::Integer;
        constant.integer = integer;
        result_.setConstantValue(*symbol, constant);
    } else if (staticBoolValue(value, boolean)) {
        constant.kind = ConstantValue::Kind::Boolean;
        constant.boolean = boolean;
        result_.setConstantValue(*symbol, constant);
    }
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

std::int64_t SemanticAnalyzer::caseChoiceValue(const ast::Expression& choice,
                                               const std::string& selectorType)
{
    CaseDomain domain;
    caseDomainFor(selectorType, domain);
    const std::string choiceType = canonicalTypeName(analyzeExpression(choice));
    const char* kStatic =
        "a case choice must be a static value: a literal, a Const, or a constant "
        "expression (ADR-0011)";
    const auto mismatch = [&]() {
        return SemanticError("case choice type " + choiceType +
                             " does not match selector type " + selectorType);
    };

    if (domain.isBool) {
        if (choiceType != "Bool") {
            throw mismatch();
        }
        bool value = false;
        if (!staticBoolValue(choice, value)) {
            throw SemanticError(kStatic);
        }
        return value ? 1 : 0;
    }

    if (domain.isChar) {
        if (choiceType != "Char") {
            throw mismatch();
        }
        std::int64_t code = 0;
        if (!staticCharValue(choice, code)) {
            throw SemanticError(kStatic);
        }
        return code;
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
    return value;
}

} // namespace inox::compiler::semantic
