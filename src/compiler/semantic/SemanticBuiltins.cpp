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
        "Put", "PutLn", "Get", "GetLn", "ReadLn", "EndOfInput",
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
    for (std::string_view name : types::TypeContext::coreBuiltinNames()) {
        declareTypeName(name, types().builtin(name));
    }

    declareTypeName("Integer", types().builtin("Int64"));
    declareTypeName("UInteger", types().builtin("UInt64"));
    // CANON-8: Byte is the friendly name of UInt8.
    declareTypeName("Byte", types().builtin("UInt8"));
    declareTypeName("Float", types().builtin("Float64"));

    // Exception identifiers are genuine nominal TYPES, not enum constants or
    // ordinary values. Their taxonomy is centralized in ExceptionTypes.h; the
    // stdlib documents the public surface while the compiler owns bootstrap
    // type identity until user-defined Exception declarations are specified.
    for (const auto& info : exceptions::kStandardExceptionTypes) {
        declareTypeName(info.name, types().exception(info.name));
    }
}

TypeRef SemanticAnalyzer::analyzeInputCall(
    std::string_view name,
    const std::vector<ast::ExpressionPtr>& arguments)
{
    if (arguments.empty()) {
        return types().voidType();
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
        result_.setExpressionType(identifier, symbol.type);

        if (symbol.kind == SymbolKind::LoopIterator) {
            throw SemanticError(std::string(name) + " cannot assign to read-only loop iterator: " + identifier.name());
        }
        if ((symbol.kind != SymbolKind::Variable && symbol.kind != SymbolKind::State) ||
            !symbol.isMutable) {
            throw SemanticError(std::string(name) + " argument must be an assignable variable: " + identifier.name());
        }
        // CANON-17 (v3.50): GetLn(S) reads the rest of the line into a String.
        if (symbol.type != nullptr && symbol.type->isString()) {
            if (!equalsIgnoreCase(name, "GetLn")) {
                throw SemanticError("Get reads one value, not a line; use GetLn(" + identifier.name() +
                                    ") to read a line of text into a String");
            }
            continue;
        }
        // r3.89 (C1, C2): a Char reads the next character exactly; Float and
        // Float32 read a real number strictly.
        if (symbol.type != nullptr && (symbol.type->isChar() || symbol.type->isFloat() || symbol.type->isDecimal())) {
            continue;
        }
        if (!isIntegerType(symbol.type)) {
            throw SemanticError(std::string(name) + " reads integers, Float, Float32, Currency, Crypto, Char and (GetLn) String, got " +
                                types::nameOf(symbol.type) + " for " + identifier.name());
        }
    }

    return types().voidType();
}

TypeRef SemanticAnalyzer::analyzePreludeCall(std::string_view name, const std::vector<TypeRef>& argumentTypes) const
{
    if (equalsIgnoreCase(name, "Put") || equalsIgnoreCase(name, "PutLn")) {
        if (argumentTypes.empty()) {
            throw SemanticError(std::string(name) + " expects at least one argument");
        }

        for (std::size_t index = 0; index < argumentTypes.size(); ++index) {
            const TypeRef type = argumentTypes[index];
            if (type != nullptr &&
                (type->isString() || type->isBool() || type->isInteger() || type->isEnum() || type->isChar() ||
                 type->isFloat() || type->isDecimal())) {
                continue;
            }

            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + std::string(name) +
                " cannot be printed yet: " + types::nameOf(type));
        }

        return types().voidType();
    }

    for (const PreludeSignature& signature : preludeSignatures()) {
        if (!equalsIgnoreCase(signature.name, name) ||
            signature.parameterTypes.size() != argumentTypes.size()) {
            continue;
        }

        bool matches = true;
        for (std::size_t index = 0; index < argumentTypes.size(); ++index) {
            const std::string_view expected = signature.parameterTypes[index];
            if (expected != "*" && types::nameOf(argumentTypes[index]) != expected) {
                matches = false;
                break;
            }
        }

        if (matches) {
            return types().builtin(signature.returnType) != nullptr ? types().builtin(signature.returnType)
                                                                    : types().voidType();
        }
    }

    // Length takes an array or a slice (CANON-13), or a String with Std.Strings
    // (CANON-14), handled with the call. Ord takes an enum value (CANON-13),
    // never a Char: Char conversions are explicit (CANON-14; Marcelo Fortes,
    // 2026-10-12).
    if (equalsIgnoreCase(name, "Length")) {
        throw SemanticError("Length takes an array or a slice (CANON-13), or a String with `Use Std.Strings` "
                            "(CANON-14)");
    }
    if (equalsIgnoreCase(name, "Ord")) {
        throw SemanticError("Ord takes an enum value (CANON-13); a Char is not an ordinal: its "
                            "conversions are explicit (CANON-14)");
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

// Change decimals (r3.91, A3): Round(X) and Trunc(X) of Std.Math take a
// Float, a Float32, a Currency or a Crypto and give an Integer: Round to the
// nearest, a half to the even one (as the decimal operators round), Trunc
// toward zero; a value beyond Integer stops the program.
bool SemanticAnalyzer::isRoundingCall(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
        return false;
    }
    const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
    if (!equalsIgnoreCase(name, "Round") && !equalsIgnoreCase(name, "Trunc")) {
        return false;
    }
    const Symbol* symbol = symbols_.currentScope().resolve(name);
    return symbol != nullptr && symbol->kind == SymbolKind::Builtin;
}

TypeRef SemanticAnalyzer::analyzeRounding(const ast::CallExpression& call)
{
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    const Symbol& symbol = resolveOrThrow(callee.name());
    result_.bind(callee, symbol);
    result_.bind(call, symbol);
    if (call.arguments().size() != 1) {
        throw SemanticError(callee.name() + " takes one value: " + callee.name() + "(X)");
    }
    const TypeRef type = analyzeExpression(*call.arguments()[0]);
    if (type == nullptr || !(type->isFloat() || type->isDecimal())) {
        throw SemanticError(callee.name() + "(X) takes a Float, a Float32, a Currency or a Crypto, got " +
                            types::nameOf(type));
    }
    return types().int64Type();
}

} // namespace inox::compiler::semantic
