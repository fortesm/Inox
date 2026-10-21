// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: the ordinal built-ins of enums (CANON-13, OpenSpec change
// enum-range-set): Ord(E), Succ(E), Pred(E), Low(T) and High(T). An enum is
// never an Integer: Ord gives its ordinal and TEnum(I) converts back.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"

#include <cmath>
#include <limits>

#include <string>
#include <string_view>

namespace inox::compiler::semantic {

using namespace detail;

// The enum or Integer Range type a type name in an argument names
// (`Low(TSuit)`, `High(TMonth)`), or null.
TypeRef SemanticAnalyzer::enumTypeNamed(const ast::Expression& expression) const
{
    if (expression.kind() != ast::AstNodeKind::IdentifierExpression) {
        return nullptr;
    }
    const TypeRef type = types().lookup(static_cast<const ast::IdentifierExpression&>(expression).name());
    return type != nullptr && (type->isEnum() || type->isRange() || type->isCharRange()) ? type : nullptr;
}

// Null when the call is not one of these built-ins applied to an enum.
TypeRef SemanticAnalyzer::analyzeOrdinalBuiltin(const ast::CallExpression& call)
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 1) {
        return nullptr;
    }
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    const std::string& name = callee.name();
    const bool bound = equalsIgnoreCase(name, "Low") || equalsIgnoreCase(name, "High");
    const bool step = equalsIgnoreCase(name, "Succ") || equalsIgnoreCase(name, "Pred");
    const bool ord = equalsIgnoreCase(name, "Ord");
    if (!bound && !step && !ord) {
        return nullptr;
    }
    if (userRoutineNamed(name)) {
        return nullptr;
    }
    const ast::Expression& argument = *call.arguments()[0];
    if (bound) {
        // Low(TSuit) and High(TSuit): the first and the last literal; of a
        // Range, its bounds (CANON-13 Q5).
        const TypeRef type = enumTypeNamed(argument);
        if (type == nullptr) {
            return nullptr;
        }
        result_.setExpressionType(argument, type);
        constants_[&call] = equalsIgnoreCase(name, "Low") ? type->minValue() : type->maxValue();
        return type;
    }
    const TypeRef type = analyzeExpression(argument);
    if (type == nullptr || !type->isEnum()) {
        if (ord) {
            return nullptr;  // the prelude's message for Ord of anything else
        }
        if (type != nullptr && type->isChar()) {
            throw SemanticError(name + " does not apply to a Char, which is not an ordinal (CANON-14): step a code "
                                "point, Char(Integer(C) + 1)");
        }
        if (type == nullptr || !type->isRange()) {
            throw SemanticError(name + " takes an enum or Range value (CANON-13), got " + types::nameOf(type));
        }
    }
    std::int64_t value = 0;
    const bool constant = constantIntegerValue(argument, value);
    if (ord) {
        if (constant) {
            constants_[&call] = value;
        }
        return types().int64Type();
    }
    if (constant) {
        const std::int64_t next = equalsIgnoreCase(name, "Succ") ? value + 1 : value - 1;
        if (next < type->minValue() || next > type->maxValue()) {
            const std::string text =
                type->isEnum() ? type->literals()[static_cast<std::size_t>(value)] : std::to_string(value);
            throw SemanticError(name + "(" + text + ") is past the " + (next < type->minValue() ? "first" : "last") +
                                " value of " + type->name() + " (CANON-13)");
        }
        constants_[&call] = next;
    }
    return type;
}

// `TSuit(I)`: an integer converted to an enum, checked against 0..N-1.
void SemanticAnalyzer::checkEnumConversion(const ast::CallExpression& call, TypeRef enumType)
{
    if (call.arguments().size() != 1) {
        throw SemanticError(enumType->name() + "(I) converts one integer to an enum (CANON-13)");
    }
    const ast::Expression& argument = *call.arguments()[0];
    const TypeRef type = result_.typeOf(argument);
    if (type == enumType) {
        return;
    }
    if (!isIntegerType(type)) {
        throw SemanticError(enumType->name() + "(I) converts an integer to an enum (CANON-13), got " +
                            types::nameOf(type));
    }
    IntegerConstant constant;
    if (!integerConstant(argument, constant)) {
        return;  // checked when the program runs
    }
    if (constant.isUnsigned || constant.bits < enumType->minValue() || constant.bits > enumType->maxValue()) {
        throw SemanticError("value out of range for " + enumType->name() + ": " + constantText(constant) +
                            " (its ordinals are 0.." + std::to_string(enumType->maxValue()) + ")");
    }
    constants_[&call] = constant.bits;
}

// Change char-values (CANON-14): `Char(I)` converts a code point, checked
// to be a Unicode scalar (0..$10FFFF without the surrogates); `TLetter(X)`
// also checks its Range; `String(C)` is the one-character String. A
// constant outside is an error here, any other value is checked when the
// program runs (kind 8).
void SemanticAnalyzer::checkCharConversion(const ast::CallExpression& call, TypeRef target)
{
    const std::string name = target->name();
    if (call.arguments().size() != 1) {
        throw SemanticError(name + "(X) converts one value");
    }
    const ast::Expression& argument = *call.arguments()[0];
    const TypeRef type = result_.typeOf(argument);
    if (target->isString()) {
        if (type == nullptr || !(type->isChar() || type->isString())) {
            throw SemanticError("String(C) converts a Char (CANON-14), got " + types::nameOf(type) +
                                "; numbers become text in a later change");
        }
        return;
    }
    if (type != nullptr && type->isChar()) {
        return;
    }
    if (!isIntegerType(type) || (type != nullptr && type->isEnum())) {
        throw SemanticError(name + "(I) converts an integer code point to a Char (CANON-14), got " +
                            types::nameOf(type));
    }
    IntegerConstant constant;
    if (!integerConstant(argument, constant)) {
        return;
    }
    const bool surrogate = constant.bits >= 0xD800 && constant.bits <= 0xDFFF;
    if (constant.isUnsigned || constant.bits < target->minValue() || constant.bits > target->maxValue() ||
        surrogate) {
        throw SemanticError("value out of range for " + name + ": " + constantText(constant) +
                            (surrogate ? " is a surrogate, never a Char (CANON-14)" : ""));
    }
}

// Change float-conversions (CANON-8): `T(X)` into a number takes a number
// (an integer, a Float, or for an integer type a Char's code point). A
// constant real converted to an integer type truncates toward zero and must
// fit; a constant beyond Float32 is an error, as the run time would trap.
void SemanticAnalyzer::checkNumericConversion(const ast::CallExpression& call, TypeRef target)
{
    const ast::Expression& argument = *call.arguments()[0];
    const TypeRef source = result_.typeOf(argument);
    // The name as written (`Float`, `Byte`), not the canonical one.
    const std::string name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
    if (source == nullptr || source->isInteger() || source->isFloat() || source->isChar() || source->isEnum()) {
        // A Char into a Float is refused with the Char rules; an enum's
        // ordinal is Ord(E).
        if (source != nullptr && source->isEnum() && target->isFloat()) {
            throw SemanticError(name + "(E) of an enum value is written " + name + "(Ord(E)) (CANON-13)");
        }
    } else {
        throw SemanticError(name + "(X) converts a number, got " + types::nameOf(source) +
                            (source->isString() ? "; text becomes a number in a later change"
                             : source->isBool() ? "; a Bool is not a number: IIf(B, 1, 0)"
                                                : ""));
    }
    double real = 0.0;
    if (source == nullptr || !source->isFloat() || !staticRealValue(argument, real)) {
        return;
    }
    if (target->isFloat()) {
        if (target->bits() == 32 && std::isfinite(real) &&
            std::fabs(real) > static_cast<double>(std::numeric_limits<float>::max())) {
            throw SemanticError("value out of range for Float32: the constant is beyond its largest value");
        }
        return;
    }
    const double truncated = std::trunc(real);
    const bool unsignedTarget = isUInt64Type(target);
    const bool fits = std::isfinite(real) && truncated >= (unsignedTarget ? 0.0 : -9223372036854775808.0) &&
                      truncated < (unsignedTarget ? 18446744073709551616.0 : 9223372036854775808.0) &&
                      (unsignedTarget || (truncated >= static_cast<double>(target->minValue()) &&
                                          truncated <= static_cast<double>(target->maxValue())));
    if (!fits) {
        throw SemanticError("value out of range for " + name + ": the constant truncates to a value " +
                            "outside it");
    }
}

} // namespace inox::compiler::semantic
