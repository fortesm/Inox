// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "SemanticSupport.h"

#include "SemanticAnalyzer.h"

#include <array>
#include <cctype>
#include <cstddef>
#include <limits>

namespace inox::compiler::semantic::detail {

bool equalsIgnoreCase(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t index = 0; index < left.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(left[index])) !=
            std::tolower(static_cast<unsigned char>(right[index]))) {
            return false;
        }
    }

    return true;
}

bool usesModule(const ast::ModuleNode& module, std::string_view name)
{
    for (const std::string& loaded : module.loadedModules()) {
        if (equalsIgnoreCase(loaded, name)) {
            return true;
        }
    }
    return false;
}

std::string_view binaryOperatorName(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::Assign:
        return ":=";
    case ast::BinaryOperator::Add:
        return "+";
    case ast::BinaryOperator::Subtract:
        return "-";
    case ast::BinaryOperator::Multiply:
        return "*";
    case ast::BinaryOperator::Divide:
        return "/";
    case ast::BinaryOperator::IntegerDivide:
        return "div";
    case ast::BinaryOperator::Modulo:
        return "mod";
    case ast::BinaryOperator::ShiftLeft:
        return "shl";
    case ast::BinaryOperator::ShiftRight:
        return "shr";
    case ast::BinaryOperator::BitAnd:
        return "bitand";
    case ast::BinaryOperator::BitXor:
        return "bitxor";
    case ast::BinaryOperator::BitOr:
        return "bitor";
    case ast::BinaryOperator::Power:
        return "^";
    case ast::BinaryOperator::Range:
        return "..";
    case ast::BinaryOperator::In:
        return "in";
    case ast::BinaryOperator::Equal:
        return "=";
    case ast::BinaryOperator::NotEqual:
        return "#";
    case ast::BinaryOperator::Less:
        return "<";
    case ast::BinaryOperator::Greater:
        return ">";
    case ast::BinaryOperator::LessEqual:
        return "<=";
    case ast::BinaryOperator::GreaterEqual:
        return ">=";
    case ast::BinaryOperator::And:
        return "and";
    case ast::BinaryOperator::Xor:
        return "xor";
    case ast::BinaryOperator::Or:
        return "or";
    }
    return "?";
}

const std::vector<PreludeSignature>& preludeSignatures()
{
    static const std::vector<PreludeSignature> signatures = {
        {"Put", {"*"}, "Void"},
        {"PutLn", {"*"}, "Void"},
        {"Get", {}, "Void"},
        {"GetLn", {}, "Void"},
        {"ReadLn", {}, "String"},

        {"Abs", {"Int64"}, "Int64"},
        {"Abs", {"Float64"}, "Float64"},
        {"Sqrt", {"Float64"}, "Float64"},
        {"Cbrt", {"Float64"}, "Float64"},
        {"Sin", {"Float64"}, "Float64"},
        {"Cos", {"Float64"}, "Float64"},
        {"Tan", {"Float64"}, "Float64"},
        {"ArcSin", {"Float64"}, "Float64"},
        {"ArcCos", {"Float64"}, "Float64"},
        {"ArcTan", {"Float64"}, "Float64"},
        {"ArcTan2", {"Float64", "Float64"}, "Float64"},
        {"Sinh", {"Float64"}, "Float64"},
        {"Cosh", {"Float64"}, "Float64"},
        {"Tanh", {"Float64"}, "Float64"},
        {"Exp", {"Float64"}, "Float64"},
        {"Ln", {"Float64"}, "Float64"},
        {"LnXP1", {"Float64"}, "Float64"},
        {"Log2", {"Float64"}, "Float64"},
        {"Log10", {"Float64"}, "Float64"},
        {"LogN", {"Float64", "Float64"}, "Float64"},
        {"Power", {"Float64", "Float64"}, "Float64"},
        {"Floor", {"Float64"}, "Float64"},
        {"Ceil", {"Float64"}, "Float64"},
        {"FMod", {"Float64", "Float64"}, "Float64"},
        {"Hypot", {"Float64", "Float64"}, "Float64"},
        {"Hypot3", {"Float64", "Float64", "Float64"}, "Float64"},
        {"RadToDeg", {"Float64"}, "Float64"},
        {"DegToRad", {"Float64"}, "Float64"},
        {"RadToGrad", {"Float64"}, "Float64"},
        {"GradToRad", {"Float64"}, "Float64"},
        {"RadToCycle", {"Float64"}, "Float64"},
        {"CycleToRad", {"Float64"}, "Float64"}
    };
    return signatures;
}

// The range of an integer type, clipped to the Int64 constants the compiler
// folds. UInt64 and Natural share the upper bound Int64.Max for constants.
bool integerTypeRange(const types::Type* type, std::int64_t& low, std::int64_t& high)
{
    if (type == nullptr || !type->isInteger()) {
        return false;
    }
    low = type->minValue();
    high = type->maxValue();
    return true;
}

bool isUInt64Type(const types::Type* type)
{
    return type != nullptr && type->isInteger() && type->bits() == 64 && !type->isSigned();
}

bool constantFits(IntegerConstant value, const types::Type* target)
{
    if (target == nullptr || !target->isInteger()) {
        return false;
    }
    if (isUInt64Type(target)) {
        return value.isUnsigned || value.bits >= 0;
    }
    if (value.isUnsigned) {
        return target->maxValue() >= 0 &&
               static_cast<std::uint64_t>(value.bits) <= static_cast<std::uint64_t>(target->maxValue());
    }
    return value.bits >= target->minValue() && value.bits <= target->maxValue();
}

std::string constantText(IntegerConstant value)
{
    return value.isUnsigned ? std::to_string(static_cast<std::uint64_t>(value.bits)) : std::to_string(value.bits);
}

bool checkedAddUnsigned(std::uint64_t a, std::uint64_t b, std::uint64_t& out)
{
    if (a > kUInt64Max - b) {
        return false;
    }
    out = a + b;
    return true;
}

bool checkedSubUnsigned(std::uint64_t a, std::uint64_t b, std::uint64_t& out)
{
    if (b > a) {
        return false;
    }
    out = a - b;
    return true;
}

bool checkedMulUnsigned(std::uint64_t a, std::uint64_t b, std::uint64_t& out)
{
    if (a != 0 && b > kUInt64Max / a) {
        return false;
    }
    out = a * b;
    return true;
}

// The fixed-width integer types narrower than 64 bits. Their values are
// computed in the Int64 family and range-checked where a result has the type.
bool isNarrowIntegerType(const types::Type* type)
{
    return type != nullptr && type->isNarrowInteger();
}

// Parses an Inox integer literal (decimal, `0x...` or `$...`). Returns false if
// the magnitude does not fit in 64 bits or the text is malformed. Portable:
// no compiler-specific wide integer types are used.
bool parseIntegerLiteralMagnitude(std::string_view text, std::uint64_t& value)
{
    std::uint64_t base = 10;
    if (!text.empty() && text.front() == '$') {
        base = 16;
        text.remove_prefix(1);
    } else if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        base = 16;
        text.remove_prefix(2);
    }
    if (text.empty()) {
        return false;
    }
    value = 0;
    for (const char ch : text) {
        std::uint64_t digit = 0;
        if (ch >= '0' && ch <= '9') {
            digit = static_cast<std::uint64_t>(ch - '0');
        } else if (base == 16 && ch >= 'a' && ch <= 'f') {
            digit = static_cast<std::uint64_t>(ch - 'a') + 10U;
        } else if (base == 16 && ch >= 'A' && ch <= 'F') {
            digit = static_cast<std::uint64_t>(ch - 'A') + 10U;
        } else {
            return false;
        }
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / base) {
            return false;
        }
        value = value * base + digit;
    }
    return true;
}

bool checkedAdd(std::int64_t a, std::int64_t b, std::int64_t& out)
{
    if ((b > 0 && a > kInt64Max - b) || (b < 0 && a < kInt64Min - b)) {
        return false;
    }
    out = a + b;
    return true;
}

bool checkedSub(std::int64_t a, std::int64_t b, std::int64_t& out)
{
    if ((b < 0 && a > kInt64Max + b) || (b > 0 && a < kInt64Min + b)) {
        return false;
    }
    out = a - b;
    return true;
}

bool checkedMul(std::int64_t a, std::int64_t b, std::int64_t& out)
{
    if (a == 0 || b == 0) {
        out = 0;
        return true;
    }
    if ((a == -1 && b == kInt64Min) || (b == -1 && a == kInt64Min)) {
        return false;
    }
    if (a > 0) {
        if (b > 0 ? a > kInt64Max / b : b < kInt64Min / a) {
            return false;
        }
    } else {
        if (b > 0 ? a < kInt64Min / b : a < kInt64Max / b) {
            return false;
        }
    }
    out = a * b;
    return true;
}

[[noreturn]] void throwConstantOverflow(const char* what, const char* type)
{
    throw SemanticError(std::string("constant integer overflow in ") + what + ": result does not fit in " + type +
                        " (integer overflow is never wraparound)");
}

bool caseDomainFor(const types::Type* type, CaseDomain& domain)
{
    if (type == nullptr) {
        return false;
    }
    if (type->isBool()) {
        domain = {0, 1, true, true, false};
        return true;
    }
    if (type->isChar()) {
        domain = {0, 0x10FFFF, true, false, true};
        return true;
    }
    if (isUInt64Type(type)) {
        // Every value of UInt64 has a key in Int64 (caseKey); a case on it
        // still needs `else` (change uint64).
        domain = {caseKey(0), caseKey(-1), false, false, false, true};
        return true;
    }
    if (type->isInteger()) {
        // ADR-0014 (v3.39): Natural is 0..Int64.Max, so its coverage is
        // provable.
        domain = {type->minValue(), type->maxValue(), type->rangeIsExact()};
        return true;
    }
    return false;
}

std::string caseValueText(std::int64_t value, const CaseDomain& domain)
{
    if (domain.isBool) {
        return value != 0 ? "True" : "False";
    }
    if (domain.isChar) {
        char buffer[16];
        std::snprintf(buffer, sizeof buffer, "U+%04llX", static_cast<unsigned long long>(value));
        return buffer;
    }
    if (domain.isUnsigned64) {
        return std::to_string(static_cast<std::uint64_t>(caseKey(value)));
    }
    return std::to_string(value);
}

// Decodes a char literal token ('A', 'é'): exactly one UTF-8 encoded scalar.
bool decodeCharLiteral(std::string_view text, std::int64_t& value)
{
    if (text.size() >= 2 && text.front() == '\'' && text.back() == '\'') {
        text = text.substr(1, text.size() - 2);
    }
    if (text.empty()) {
        return false;
    }
    const auto byte = [&](std::size_t i) { return static_cast<unsigned char>(text[i]); };
    std::size_t length = 0;
    std::int64_t code = 0;
    if (byte(0) < 0x80) { length = 1; code = byte(0); }
    else if ((byte(0) & 0xE0) == 0xC0) { length = 2; code = byte(0) & 0x1F; }
    else if ((byte(0) & 0xF0) == 0xE0) { length = 3; code = byte(0) & 0x0F; }
    else if ((byte(0) & 0xF8) == 0xF0) { length = 4; code = byte(0) & 0x07; }
    else { return false; }
    if (text.size() != length) {
        return false;
    }
    for (std::size_t i = 1; i < length; ++i) {
        if ((byte(i) & 0xC0) != 0x80) {
            return false;
        }
        code = (code << 6) | (byte(i) & 0x3F);
    }
    value = code;
    return true;
}

const ast::BinaryExpression* namedArgument(const ast::Expression& argument)
{
    if (argument.kind() != ast::AstNodeKind::BinaryExpression) {
        return nullptr;
    }
    const auto& binary = static_cast<const ast::BinaryExpression&>(argument);
    if (binary.op() != ast::BinaryOperator::Assign ||
        binary.left().kind() != ast::AstNodeKind::IdentifierExpression) {
        return nullptr;
    }
    return &binary;
}

// ADR-0011 — Ada/SPARK-style case adapted to Inox.

// An expression built only from literals and operators. Its type is the
// universal literal type, so it may be read in the selector's type (and is
// then range-checked); anything that names a Const keeps that Const's type.
bool isLiteralOnly(const ast::Expression& expression)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression:
        return true;
    case ast::AstNodeKind::UnaryExpression:
        return isLiteralOnly(static_cast<const ast::UnaryExpression&>(expression).operand());
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        return isLiteralOnly(binary.left()) && isLiteralOnly(binary.right());
    }
    default:
        return false;
    }
}

void collectNames(const ast::Expression& expression, std::vector<std::string>& names)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::IdentifierExpression:
        names.push_back(static_cast<const ast::IdentifierExpression&>(expression).name());
        return;
    case ast::AstNodeKind::UnaryExpression:
        collectNames(static_cast<const ast::UnaryExpression&>(expression).operand(), names);
        return;
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        collectNames(binary.left(), names);
        collectNames(binary.right(), names);
        return;
    }
    case ast::AstNodeKind::ConditionalExpression: {
        const auto& conditional = static_cast<const ast::ConditionalExpression&>(expression);
        collectNames(conditional.condition(), names);
        collectNames(conditional.thenValue(), names);
        collectNames(conditional.elseValue(), names);
        return;
    }
    case ast::AstNodeKind::CallExpression: {
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        collectNames(call.callee(), names);
        for (const auto& argument : call.arguments()) {
            collectNames(*argument, names);
        }
        return;
    }
    default:
        return;
    }
}

} // namespace inox::compiler::semantic::detail
