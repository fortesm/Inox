// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: type predicates, assignability, canonical type names.

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

const StructType* SemanticAnalyzer::resolveStruct(std::string_view name) const
{
    const auto iterator = structs_.find(normalizeName(name));
    return iterator != structs_.end() ? &iterator->second : nullptr;
}

const StructField* SemanticAnalyzer::resolveStructField(std::string_view structName, std::string_view fieldName) const
{
    const StructType* structType = resolveStruct(structName);
    if (structType == nullptr) {
        return nullptr;
    }
    for (const StructField& field : structType->fields) {
        if (equalsIgnoreCase(field.name, fieldName)) {
            return &field;
        }
    }
    return nullptr;
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

bool SemanticAnalyzer::isNumericType(std::string_view typeName)
{
    return isIntegerType(typeName) ||
           typeName == "Float32" ||
           typeName == "Float64";
}

bool SemanticAnalyzer::isIntegerType(std::string_view typeName)
{
    return typeName == "Integer" ||
           typeName == "UInteger" ||
           typeName == "Int8" ||
           typeName == "Int16" ||
           typeName == "Int32" ||
           typeName == "Int64" ||
           typeName == "UInt8" ||
           typeName == "UInt16" ||
           typeName == "UInt32" ||
           typeName == "UInt64" ||
           typeName == "Natural";
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

bool SemanticAnalyzer::canAssign(std::string_view targetType, std::string_view valueType)
{
    // CANON-8 (v3.39): Natural is the non-negative subtype of Integer; a
    // Natural value is always a valid Integer.
    if (targetType == "Int64" && valueType == "Natural") {
        return true;
    }
    return targetType.empty() || (!valueType.empty() && targetType == valueType);
}

// CANON-8 (v3.39): an Integer value enters Natural through `Natural(X)`, unless
// it is a constant expression whose value is >= 0 (statically in range). A
// negative constant is a compile-time range error.
bool SemanticAnalyzer::canAssignValue(std::string_view targetType, std::string_view valueType,
                                      const ast::Expression& value)
{
    if (canAssign(targetType, valueType)) {
        return true;
    }
    // CANON-8: a constant Integer is read in a fixed-width target type when it
    // fits (`B Byte := 200`); otherwise it is a compile-time range error.
    if (const std::string canonical = canonicalTypeName(targetType);
        isNarrowIntegerType(canonical) && valueType == "Int64") {
        std::int64_t constant = 0;
        std::int64_t low = 0;
        std::int64_t high = 0;
        if (constantIntegerValue(value, constant) && integerTypeRange(canonical, low, high)) {
            if (constant < low || constant > high) {
                throw SemanticError("value out of range for " + canonical + ": " +
                                    std::to_string(constant));
            }
            return true;
        }
        return false;
    }
    if (targetType == "Natural" && valueType == "Int64") {
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

bool SemanticAnalyzer::typesMatch(std::string_view left, std::string_view right)
{
    return left.empty() || right.empty() || left == right;
}

ResolvedType SemanticAnalyzer::resolvedType(std::string typeName) const
{
    return ResolvedType{typeName, types_.resolve(typeName)};
}

// CANON-13: a fixed array type resolves its bounds (literals or Const names)
// and its element type, so `Array[1..Size] Byte` and `Array[1..10] UInt8`
// name the same type when Size is 10.
std::string SemanticAnalyzer::canonicalArrayTypeName(std::string_view name) const
{
    const auto parts = support::splitArrayTypeName(name);
    if (!parts) {
        throw SemanticError("malformed array type: " + std::string(name));
    }
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
    const std::int64_t low = boundValue(parts->low);
    const std::int64_t high = boundValue(parts->high);
    if (low > high) {
        throw SemanticError("empty array bounds: " + std::to_string(low) + ".." + std::to_string(high));
    }
    std::string element;
    if (support::isArrayTypeName(parts->element)) {
        element = canonicalArrayTypeName(parts->element);
    } else {
        resolveTypeOrThrow(parts->element);
        element = canonicalTypeName(parts->element);
        const bool isStruct = structs_.find(normalizeName(element)) != structs_.end();
        if (!isNumericType(element) && element != "Bool" && element != "String" && !isStruct) {
            throw SemanticError("array elements of type " + element +
                                " are not supported yet; use an integer, Float, Bool, String or struct element");
        }
    }
    return support::makeArrayTypeName(std::to_string(low), std::to_string(high), element);
}

} // namespace inox::compiler::semantic
