// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: type predicates, assignability, and the resolution of
// written types into type objects (OpenSpec change type-representation).

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"
#include "../exceptions/ExceptionTypes.h"
#include "../types/TypeSyntax.h"

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

TypeRef SemanticAnalyzer::resolveStruct(std::string_view name) const
{
    const TypeRef type = types().lookup(name);
    return type != nullptr && type->isStruct() ? type : nullptr;
}

const types::StructField* SemanticAnalyzer::resolveStructField(TypeRef structType, std::string_view fieldName)
{
    return structType != nullptr && structType->isStruct() ? structType->field(fieldName) : nullptr;
}

bool SemanticAnalyzer::isIndexTarget(const ast::Expression& expression)
{
    if (expression.kind() != ast::AstNodeKind::CallExpression) {
        return false;
    }
    const auto& call = static_cast<const ast::CallExpression&>(expression);
    return call.callee().kind() == ast::AstNodeKind::IdentifierExpression &&
           static_cast<const ast::IdentifierExpression&>(call.callee()).name() == "__index" &&
           call.arguments().size() == 2;
}

bool SemanticAnalyzer::isInternalSyntheticName(std::string_view name)
{
    return name == "__index" || name == "__member";
}

bool SemanticAnalyzer::isNumericType(TypeRef type)
{
    return type != nullptr && type->isNumeric();
}

bool SemanticAnalyzer::isIntegerType(TypeRef type)
{
    return type != nullptr && type->isInteger();
}

bool SemanticAnalyzer::isExceptionType(std::string_view name)
{
    return exceptions::isExceptionType(name);
}

std::string SemanticAnalyzer::normalizeName(std::string_view name)
{
    std::string normalized;
    normalized.reserve(name.size());
    for (const char ch : name) {
        normalized.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

bool SemanticAnalyzer::canAssign(TypeRef targetType, TypeRef valueType) const
{
    // CANON-8 (v3.39): Natural is the non-negative subtype of Integer; a
    // Natural value is always a valid Integer.
    if (targetType == types().int64Type() && valueType == types().naturalType()) {
        return true;
    }
    // CANON-13: a Range value is always a valid value of its base.
    if (valueType != nullptr && valueType->isRange() && targetType == valueType->rangeBase()) {
        return true;
    }
    return targetType == nullptr || (valueType != nullptr && targetType == valueType);
}

// CANON-8 (v3.39): an Integer value enters Natural through `Natural(X)`, unless
// it is a constant expression whose value is >= 0 (statically in range). A
// negative constant is a compile-time range error.
bool SemanticAnalyzer::canAssignValue(TypeRef targetType, TypeRef valueType, const ast::Expression& value)
{
    // Safe pointers: an owner is never copied; None fills a Link or an
    // Option; an Own is wrapped into an Option (safe-pointers).
    if (const std::optional<bool> pointer = canAssignPointer(targetType, valueType, value)) {
        return *pointer;
    }
    // F6 (file-io): a File is moved from the call that opened it, never
    // copied from another File.
    if (isFileType(targetType) || isFileType(valueType)) {
        if (!isFileType(targetType) || !isFileType(valueType)) {
            return false;
        }
        requireFreshFile(value);
        return true;
    }
    if (canAssign(targetType, valueType)) {
        return true;
    }
    // CANON-13: an enum and an Integer never stand for each other.
    if (targetType != nullptr && valueType != nullptr && targetType->isEnum() && valueType->isInteger()) {
        throw SemanticError("an integer is not a " + targetType->name() + ": convert it with " +
                            targetType->name() + "(I), which checks the ordinal (CANON-13)");
    }
    if (targetType != nullptr && valueType != nullptr && targetType->isInteger() && valueType->isEnum()) {
        throw SemanticError("a " + valueType->name() + " is not an integer: its ordinal is Ord(E) (CANON-13)");
    }
    // CANON-8: a constant Integer is read in a fixed-width target type when it
    // fits (`B Byte := 200`); otherwise it is a compile-time range error.
    if (isNarrowIntegerType(targetType) && valueType == types().int64Type()) {
        std::int64_t constant = 0;
        std::int64_t low = 0;
        std::int64_t high = 0;
        if (constantIntegerValue(value, constant) && integerTypeRange(targetType, low, high)) {
            if (constant < low || constant > high) {
                throw SemanticError("value out of range for " + targetType->name() + ": " +
                                    std::to_string(constant));
            }
            return true;
        }
        return false;
    }
    // CANON-13 Range (Q1): an Integer-family value enters a Range checked
    // against its bounds; a constant outside them is a compile error.
    if (targetType != nullptr && targetType->isRange() && valueType != nullptr && valueType->isInteger() &&
        (valueType == types().int64Type() || valueType == types().naturalType() ||
         valueType->rangeBase() == types().int64Type())) {
        std::int64_t constant = 0;
        if (constantIntegerValue(value, constant)) {
            if (constant < targetType->minValue() || constant > targetType->maxValue()) {
                throw SemanticError("value out of range for " + targetType->name() + ": " +
                                    std::to_string(constant) + " (it holds " +
                                    std::to_string(targetType->minValue()) + ".." +
                                    std::to_string(targetType->maxValue()) + ")");
            }
            return true;
        }
        result_.requireRangeCheck(value, targetType);
        return true;
    }
    // Change uint64: a constant Integer that is not negative is a UInt64.
    if (isUInt64Type(targetType) && valueType == types().int64Type()) {
        std::int64_t constant = 0;
        if (constantIntegerValue(value, constant)) {
            if (constant < 0) {
                throw SemanticError("value out of range for UInt64: " + std::to_string(constant));
            }
            return true;
        }
        return false;
    }
    if (targetType == types().naturalType() && valueType == types().int64Type()) {
        std::int64_t constant = 0;
        if (constantIntegerValue(value, constant)) {
            if (constant < 0) {
                throw SemanticError("value out of range for Natural: " + std::to_string(constant));
            }
            return true;
        }
        // ADR-0015 (Ada): storing an Integer into a Natural checks the range
        // at run time, with no conversion written.
        result_.requireNaturalCheck(value);
        return true;
    }
    return false;
}

bool SemanticAnalyzer::typesMatch(TypeRef left, TypeRef right)
{
    return left == nullptr || right == nullptr || left == right;
}

// A built-in, alias or struct name of the program (`Byte` is UInt8).
void SemanticAnalyzer::declareTypeName(std::string_view name, TypeRef type)
{
    if (types().isDeclared(name)) {
        throw SemanticError("type already declared: " + std::string(name));
    }
    declareOrThrow(name, SymbolKind::Type);
    types().declareName(name, type);
}

TypeRef SemanticAnalyzer::resolveWrittenType(std::string_view written)
{
    const auto syntax = types::readTypeSyntax(written);
    if (!syntax) {
        if (types::isArraySpelling(written)) {
            throw SemanticError("malformed array type: " + std::string(written));
        }
        throw SemanticError("unknown type: " + std::string(written));
    }
    return resolveTypeSyntax(*syntax, written);
}

TypeRef SemanticAnalyzer::lookupWrittenType(std::string_view written)
{
    if (!types::isArraySpelling(written) && !types::isGenericSpelling(written)) {
        return types().lookup(written);
    }
    return resolveWrittenType(written);
}

TypeRef SemanticAnalyzer::resolveTypeSyntax(const types::TypeSyntax& syntax, std::string_view written)
{
    switch (syntax.form) {
    case types::TypeSyntax::Form::Array:
        return resolveArrayType(syntax, written);
    case types::TypeSyntax::Form::Generic:
        if (detail::equalsIgnoreCase(syntax.name, "Set") && syntax.arguments.size() == 1) {
            return resolveSetType(syntax, written);  // CANON-13
        }
        if (!detail::equalsIgnoreCase(syntax.name, "Slice") && !detail::equalsIgnoreCase(syntax.name, "Own") &&
            !detail::equalsIgnoreCase(syntax.name, "Link") && !detail::equalsIgnoreCase(syntax.name, "Option")) {
            return resolveGenericStructType(syntax, written);  // CANON-8: TPair<String, Integer>
        }
        if (syntax.arguments.size() != 1) {
            throw SemanticError(syntax.name + " takes one type argument: " + std::string(written));
        }
        if (detail::equalsIgnoreCase(syntax.name, "Slice")) {
            const std::string elementText = syntax.argument().spelling();
            const TypeRef element = resolveTypeSyntax(syntax.argument(), elementText);
            if (!element->isArray() && !element->isNumeric() && !element->isBool() && !element->isEnum()) {
                throw SemanticError("slices of " + element->name() +
                                    " are not supported yet; use an integer, Float, Bool or array element");
            }
            return types().slice(element);
        }
        return resolvePointerType(syntax, written);
    case types::TypeSyntax::Form::Named:
        break;
    }
    if (const TypeRef type = types().lookup(syntax.name)) {
        return type;
    }
    if (const ast::TypeDeclaration* generic = genericStruct(syntax.name)) {
        std::string list;
        for (const std::string& parameter : generic->typeParameters) {
            list += (list.empty() ? "" : ", ") + parameter;
        }
        throw SemanticError(generic->name + " is generic: write its type arguments, " + generic->name + "<" + list +
                            "> with the types (CANON-8)");
    }
    if (detail::equalsIgnoreCase(syntax.name, "File")) {
        throw SemanticError("unknown type: File (it is declared in Std.Files: add `Use Std.Files`)");
    }
    throw SemanticError("unknown type: " + std::string(written));
}

// CANON-13: a fixed array type resolves its bounds (literals or Const names)
// and its element type, so `Array[1..Size] Byte` and `Array[1..10] UInt8`
// are the same type when Size is 10.
TypeRef SemanticAnalyzer::resolveArrayType(const types::TypeSyntax& syntax, std::string_view written)
{
    (void)written;
    const auto boundValue = [&](const std::string& text) -> std::int64_t {
        std::uint64_t magnitude = 0;
        const bool negative = !text.empty() && text.front() == '-';
        const std::string digits = negative ? text.substr(1) : text;
        if (!digits.empty() &&
            (std::isdigit(static_cast<unsigned char>(digits.front())) != 0 || digits.front() == '$')) {
            if (!parseIntegerLiteralMagnitude(digits, magnitude) ||
                magnitude > static_cast<std::uint64_t>(kInt64Max)) {
                throw SemanticError("array bound out of range: " + text);
            }
            const auto value = static_cast<std::int64_t>(magnitude);
            return negative ? -value : value;
        }
        const Symbol* symbol = symbols_.currentScope().resolve(text);
        if (symbol == nullptr || symbol->kind != SymbolKind::Constant) {
            throw SemanticError("an array bound must be an integer literal or a Const: " + text);
        }
        const ConstantValue* constant = result_.constantValueOf(*symbol);
        if (constant == nullptr || constant->kind != ConstantValue::Kind::Integer) {
            throw SemanticError("an array bound must be an integer constant: " + text);
        }
        return constant->integer;
    };
    const std::int64_t low = boundValue(syntax.low);
    const std::int64_t high = boundValue(syntax.high);
    if (low > high) {
        throw SemanticError("empty array bounds: " + std::to_string(low) + ".." + std::to_string(high));
    }
    const types::TypeSyntax& elementSyntax = syntax.argument();
    if (elementSyntax.form == types::TypeSyntax::Form::Array) {
        return types().array(low, high, resolveArrayType(elementSyntax, elementSyntax.spelling()));
    }
    const TypeRef element = resolveTypeSyntax(elementSyntax, elementSyntax.spelling());
    rejectFileIn(element, "an array element");
    if (element->isOwn()) {
        throw SemanticError("an array of owners is Array[...] Option<Own<T>>: every element is born None");
    }
    const bool isPointer = element->isLink() || element->isOptionOwn();
    if (!element->isNumeric() && !element->isBool() && !element->isString() && !element->isStruct() && !isPointer &&
        !element->isEnum() && !element->isSet()) {
        throw SemanticError("array elements of type " + element->name() +
                            " are not supported yet; use an integer, Float, Bool, enum, Set, String or struct element");
    }
    return types().array(low, high, element);
}

} // namespace inox::compiler::semantic
