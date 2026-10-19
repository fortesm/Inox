// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: `Fill(Value)` (CANON-5, CANON-13, r3.81, OpenSpec change
// array-fill). Every array is initialized where it is declared; `Fill(Value)`
// is the array its target expects, every element a copy of Value. Its type
// and bounds come from the target, never from the call. In a
// multi-dimensional array, Value is an element (`Fill(0)`) or a whole row
// (`Fill(Row)`): the outermost element type Value fits is the one copied.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"

#include <string>

namespace inox::compiler::semantic {

using namespace detail;

// The diagnostic of an array declared without a value (r3.81), with the
// value its element type would be filled with.
std::string SemanticAnalyzer::arrayWithoutInitializer(const std::string& name, const std::string& written,
                                                      TypeRef arrayType)
{
    TypeRef element = arrayType->element();
    while (element != nullptr && element->isArray()) {
        element = element->element();
    }
    std::string example = "Value";
    if (element != nullptr) {
        if (element->isEnum()) {
            example = element->literals().front();
        } else if (element->isBool()) {
            example = "False";
        } else if (element->isFloat()) {
            example = "0.0";
        } else if (element->isRange()) {
            example = std::to_string(element->minValue());
        } else if (element->isInteger()) {
            example = "0";
        } else if (element->isString()) {
            example = "\"\"";
        } else if (element->isSet()) {
            example = "[]";
        } else if (element->isOptionOwn()) {
            example = "None";
        }
    }
    return "an array needs an initializer (CANON-5, r3.81): " + name + " " + written + " := Fill(" +
           example + ")";
}

bool SemanticAnalyzer::isFillCall(const ast::CallExpression& call) const
{
    return call.callee().kind() == ast::AstNodeKind::IdentifierExpression &&
           equalsIgnoreCase(static_cast<const ast::IdentifierExpression&>(call.callee()).name(), "Fill");
}

TypeRef SemanticAnalyzer::analyzeFill(const ast::CallExpression& call)
{
    const TypeRef target = targetExpression_ == &call ? targetType_ : nullptr;
    if (target == nullptr || !target->isArray()) {
        throw SemanticError("Fill(Value) is written where an array is expected, and takes its type from there "
                            "(A Array[1..3] Integer := Fill(0)), got " +
                            (target == nullptr ? std::string("no array target") : target->name()));
    }
    if (call.arguments().size() != 1) {
        throw SemanticError("Fill takes one value, the value of every element: Fill(0)");
    }
    const ast::Expression& value = *call.arguments()[0];
    // The innermost element type is the context of a literal (`Fill(0)` in
    // an Array of Byte, `Fill([])` in an Array of Set<TSuit>).
    TypeRef innermost = target->element();
    while (innermost != nullptr && innermost->isArray()) {
        innermost = innermost->element();
    }
    const TypeRef valueType = analyzeExpressionFor(value, innermost);
    // An owner is never copied: an array of owners starts empty only.
    if (isMoveOnlyType(innermost) && !(valueType != nullptr && valueType->isNone())) {
        throw SemanticError("an owner is never copied: an array of " + innermost->name() +
                            " is filled with None, Fill(None)");
    }
    TypeRef unit = nullptr;
    for (TypeRef element = target->element(); element != nullptr; element = element->element()) {
        if (element->isArray() ? element == valueType : canAssignValue(element, valueType, value)) {
            unit = element;
            break;
        }
        if (!element->isArray()) {
            break;
        }
    }
    if (unit == nullptr) {
        throw SemanticError("Fill fills " + target->name() + " with values of " + innermost->name() + ", got " +
                            types::nameOf(valueType));
    }
    result_.setFillUnit(call, unit);
    return target;
}

} // namespace inox::compiler::semantic
