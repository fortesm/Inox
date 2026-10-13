// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: built-in types and routines (Put/PutLn, Get/GetLn, prelude).

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

void SemanticAnalyzer::declareBuiltins()
{
    const std::vector<std::string_view> valueBuiltins = {
        "Put", "PutLn", "Get", "GetLn", "ReadLn",
        "Abs", "Sqrt", "Cbrt", "Sin", "Cos", "Tan", "ArcSin", "ArcCos", "ArcTan", "ArcTan2",
        "Sinh", "Cosh", "Tanh", "Exp", "Ln", "LnXP1", "Log2", "Log10", "LogN", "Power",
        "Floor", "Ceil", "FMod", "Hypot", "Hypot3",
        "RadToDeg", "DegToRad", "RadToGrad", "GradToRad", "RadToCycle", "CycleToRad",
        "Length", "Ord",
        "True", "False",
        "__index", "__member",
        "Sys", "IO", "Math", "Std"
    };

    for (std::string_view name : valueBuiltins) {
        symbols_.globalScope().declare(std::string(name), SymbolKind::Builtin);
    }
}

void SemanticAnalyzer::declareBuiltinTypes()
{
    constexpr std::array<std::string_view, 16> builtinTypes = {
        "Bool",
        "Int8", "Int16", "Int32", "Int64",
        "UInt8", "UInt16", "UInt32", "UInt64",
        "Natural",
        "Float32", "Float64",
        "Currency", "Crypto", "Char", "String"
    };

    for (std::string_view name : builtinTypes) {
        declareTypeOrThrow(name, true);
    }

    declareTypeOrThrow("Integer", true, "Int64");
    declareTypeOrThrow("UInteger", true, "UInt64");
    // CANON-8: Byte is the friendly name of UInt8.
    declareTypeOrThrow("Byte", true, "UInt8");
    declareTypeOrThrow("Float", true, "Float64");

    // Exception identifiers are genuine nominal TYPES, not enum constants or
    // ordinary values. Their taxonomy is centralized in ExceptionTypes.h; the
    // stdlib documents the public surface while the compiler owns bootstrap
    // type identity until user-defined Exception declarations are specified.
    for (const auto& info : exceptions::kStandardExceptionTypes) {
        declareTypeOrThrow(info.name, true);
    }
}

std::string SemanticAnalyzer::analyzeInputCall(
    std::string_view name,
    const std::vector<ast::ExpressionPtr>& arguments)
{
    if (arguments.empty()) {
        return "Void";
    }

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const ast::Expression& argument = *arguments[index];
        if (argument.kind() != ast::AstNodeKind::IdentifierExpression) {
            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + std::string(name) +
                " must be an assignable variable");
        }

        const auto& identifier = static_cast<const ast::IdentifierExpression&>(argument);
        const Symbol& symbol = resolveOrThrow(identifier.name());
        result_.bind(identifier, symbol);
        result_.setExpressionType(identifier, resolvedType(symbol.typeName));

        if (symbol.kind == SymbolKind::LoopIterator) {
            throw SemanticError(std::string(name) + " cannot assign to read-only loop iterator: " + identifier.name());
        }
        if ((symbol.kind != SymbolKind::Variable && symbol.kind != SymbolKind::State) ||
            !symbol.isMutable) {
            throw SemanticError(std::string(name) + " argument must be an assignable variable: " + identifier.name());
        }
        // CANON-17 (v3.50): GetLn(S) reads the rest of the line into a String.
        if (canonicalTypeName(symbol.typeName) == "String") {
            if (!equalsIgnoreCase(name, "GetLn")) {
                throw SemanticError("Get reads Integer values; use GetLn(" + identifier.name() +
                                    ") to read a line of text into a String");
            }
            continue;
        }
        if (!isIntegerType(symbol.typeName)) {
            throw SemanticError(
                std::string(name) + " currently supports only Integer/Int64 and String input, got " +
                symbol.typeName + " for " + identifier.name());
        }
    }

    return "Void";
}

std::string SemanticAnalyzer::analyzePreludeCall(
    std::string_view name,
    const std::vector<std::string>& argumentTypes) const
{
    if (equalsIgnoreCase(name, "Put") || equalsIgnoreCase(name, "PutLn")) {
        if (argumentTypes.empty()) {
            throw SemanticError(std::string(name) + " expects at least one argument");
        }

        for (std::size_t index = 0; index < argumentTypes.size(); ++index) {
            const std::string& typeName = argumentTypes[index];
            if (typeName == "String" || typeName == "Bool" || isIntegerType(typeName) || typeName == "Float64") {
                continue;
            }

            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + std::string(name) +
                " cannot be printed yet: " + typeName);
        }

        return "Void";
    }

    for (const PreludeSignature& signature : preludeSignatures()) {
        if (!equalsIgnoreCase(signature.name, name) ||
            signature.parameterTypes.size() != argumentTypes.size()) {
            continue;
        }

        bool matches = true;
        for (std::size_t index = 0; index < argumentTypes.size(); ++index) {
            const std::string_view expected = signature.parameterTypes[index];
            if (expected != "*" && argumentTypes[index] != expected) {
                matches = false;
                break;
            }
        }

        if (matches) {
            return std::string(signature.returnType);
        }
    }

    if (equalsIgnoreCase(name, "Length")) {
        throw SemanticError("Length expects String");
    }
    if (equalsIgnoreCase(name, "Ord")) {
        throw SemanticError("Ord expects Char");
    }

    throw SemanticError("no matching prelude signature for call: " + std::string(name));
}

bool SemanticAnalyzer::isPreludeCall(std::string_view name)
{
    for (const PreludeSignature& signature : preludeSignatures()) {
        if (equalsIgnoreCase(signature.name, name)) {
            return true;
        }
    }
    return false;
}

} // namespace inox::compiler::semantic
