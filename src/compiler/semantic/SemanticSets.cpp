// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: Set<T> (CANON-13, OpenSpec change enum-range-set). A set
// over an Enum or an Integer Range of at most 256 values, a value type,
// empty by default. `[A, B]` is a literal where a Set is expected; `in`
// tests membership; `=`, `#`, `<=` and `>=` compare; Union, Intersection,
// Difference, SymmetricDifference, Include and Exclude return new sets
// (r3.81: Include and Exclude replaced With and Without).

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"

#include <array>
#include <string>
#include <string_view>

namespace inox::compiler::semantic {

using namespace detail;

namespace {

constexpr std::int64_t kMaxSetSize = 256;  // Q7

constexpr std::array<std::string_view, 6> kSetFunctions{
    "Union", "Intersection", "Difference", "SymmetricDifference", "Include", "Exclude"};

} // namespace

namespace detail {

bool isSetLiteral(const ast::Expression& expression)
{
    return expression.kind() == ast::AstNodeKind::CallExpression &&
           static_cast<const ast::CallExpression&>(expression).callee().kind() ==
               ast::AstNodeKind::IdentifierExpression &&
           static_cast<const ast::IdentifierExpression&>(static_cast<const ast::CallExpression&>(expression).callee())
                   .name() == "__set";
}

} // namespace detail

TypeRef SemanticAnalyzer::resolveSetType(const types::TypeSyntax& syntax, std::string_view written)
{
    const TypeRef base = resolveTypeSyntax(syntax.argument(), syntax.argument().spelling());
    if (!base->isEnum() && !base->isRange() && !base->isCharRange()) {
        throw SemanticError("a Set is over an Enum or a Range, of Integers or Chars (CANON-13), not " +
                            base->name() + ": " +
                            std::string(written));
    }
    if (base->maxValue() - base->minValue() + 1 > kMaxSetSize) {
        throw SemanticError("a Set holds at most 256 values (CANON-13), and " + base->name() + " has " +
                            std::to_string(base->maxValue() - base->minValue() + 1) + ": " + std::string(written));
    }
    return types().set(base);
}

// An element of a set of `setType`: a value of its base (a Range value is
// checked against the Range).
void SemanticAnalyzer::requireSetElement(const ast::Expression& element, TypeRef setType, const std::string& where)
{
    const TypeRef base = setType->element();
    const TypeRef type = analyzeExpressionFor(element, base);
    if (!canAssignValue(base, type, element)) {
        throw SemanticError(where + " is a " + base->name() + ", the element of " + setType->name() + ", got " +
                            types::nameOf(type));
    }
}

// Analyzes `expression`, a Set operand: a literal takes the Set type `context`.
TypeRef SemanticAnalyzer::analyzeSetOperand(const ast::Expression& expression, TypeRef context)
{
    if (isSetLiteral(expression)) {
        if (context == nullptr || !context->isSet()) {
            throw SemanticError("a set literal [...] is written where a Set is expected (CANON-13); array "
                                "literals are reserved");
        }
        return analyzeExpressionFor(expression, context);
    }
    return analyzeExpression(expression);
}

TypeRef SemanticAnalyzer::analyzeSetLiteral(const ast::CallExpression& call)
{
    const TypeRef setType = targetExpression_ == &call ? targetType_ : nullptr;
    if (setType == nullptr || !setType->isSet()) {
        throw SemanticError("a set literal [...] is written where a Set is expected (CANON-13); array literals "
                            "are reserved");
    }
    for (std::size_t index = 0; index < call.arguments().size(); ++index) {
        requireSetElement(*call.arguments()[index], setType, "element " + std::to_string(index + 1));
    }
    return setType;
}

bool SemanticAnalyzer::isSetFunctionCall(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 2) {
        return false;
    }
    const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
    for (const std::string_view function : kSetFunctions) {
        if (equalsIgnoreCase(function, name)) {
            return !userRoutineNamed(name);
        }
    }
    return false;
}

TypeRef SemanticAnalyzer::analyzeSetFunction(const ast::CallExpression& call)
{
    const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
    const ast::Expression& first = *call.arguments()[0];
    const ast::Expression& second = *call.arguments()[1];
    const bool element = equalsIgnoreCase(name, "Include") || equalsIgnoreCase(name, "Exclude");
    // The set argument that is not a literal gives the other its type.
    TypeRef setType = nullptr;
    if (isSetLiteral(first) && !element) {
        const TypeRef other = analyzeExpression(second);
        setType = analyzeSetOperand(first, other);
        if (setType != other) {
            throw SemanticError(name + " takes two sets of one type, got " + types::nameOf(setType) + " and " +
                                types::nameOf(other));
        }
        return setType;
    }
    setType = analyzeSetOperand(first, targetExpression_ == &call ? targetType_ : nullptr);
    if (setType == nullptr || !setType->isSet()) {
        throw SemanticError(name + " takes a Set as its first argument (CANON-13), got " + types::nameOf(setType));
    }
    if (element) {
        requireSetElement(second, setType, "the second argument of " + name);
        return setType;
    }
    const TypeRef other = analyzeSetOperand(second, setType);
    if (other != setType) {
        throw SemanticError(name + " takes two sets of one type, got " + setType->name() + " and " +
                            types::nameOf(other));
    }
    return setType;
}

// `X in S` with S a set, or `X in A..B` with integer bounds.
TypeRef SemanticAnalyzer::analyzeMembership(const ast::BinaryExpression& expression)
{
    const ast::Expression& right = expression.right();
    if (right.kind() == ast::AstNodeKind::BinaryExpression &&
        static_cast<const ast::BinaryExpression&>(right).op() == ast::BinaryOperator::Range) {
        const TypeRef value = analyzeExpression(expression.left());
        const TypeRef range = analyzeExpression(right);
        const auto family = [](TypeRef type) { return type != nullptr && type->isRange() ? type->rangeBase() : type; };
        if (value == nullptr || range == nullptr || !(family(value) == family(range) ||
                                                      (isIntegerType(value) && isIntegerType(range) &&
                                                       !value->isEnum() && !isUInt64Type(value) &&
                                                       !isUInt64Type(range)))) {
            throw SemanticError("'in' tests a value against a range of its type, got " + types::nameOf(value) +
                                " in " + types::nameOf(range));
        }
        return types().boolType();
    }
    TypeRef setType = nullptr;
    if (isSetLiteral(right)) {
        const TypeRef value = analyzeExpression(expression.left());
        if (value == nullptr || (!value->isEnum() && !value->isRange() && !value->isCharRange())) {
            throw SemanticError("'in' with a set literal needs an Enum or Range value on its left, got " +
                                types::nameOf(value));
        }
        setType = analyzeSetOperand(right, types().set(value));
        return types().boolType();
    }
    setType = analyzeExpression(right);
    if (setType == nullptr || !setType->isSet()) {
        throw SemanticError("'in' tests membership in a Set or a range A..B (CANON-13), got " +
                            types::nameOf(setType));
    }
    const TypeRef base = setType->element();
    const TypeRef value = analyzeExpressionFor(expression.left(), base);
    // A Range set accepts any Integer value: one outside the Range is not in it.
    // A Char Range set accepts any Char the same way (change char-values).
    const bool charFits = base->isCharRange() && value != nullptr && value->isChar();
    const bool fits = value == base || charFits || (base->isRange() && value != nullptr && value->isInteger() &&
                                        !isUInt64Type(value) && (value == base->rangeBase() ||
                                                                 value == types().naturalType() ||
                                                                 value->rangeBase() == base->rangeBase()));
    if (!fits) {
        throw SemanticError("'in' tests a " + base->name() + " in " + setType->name() + ", got " +
                            types::nameOf(value));
    }
    return types().boolType();
}

} // namespace inox::compiler::semantic
