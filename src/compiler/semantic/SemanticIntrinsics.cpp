// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: Std.Terminal (change console-abi-terminal, r3.88). The
// routines exist only in a program that uses Std.Terminal; without it their
// names stay free for the program's own routines.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"
#include "StdIntrinsics.h"

#include <string>
#include <string_view>

namespace inox::compiler::semantic {

using namespace detail;

void SemanticAnalyzer::declareModuleIntrinsics(const ast::ModuleNode& module)
{
    intrinsicModules_.clear();
    for (const ModuleIntrinsic& function : kModuleIntrinsics) {
        if (!usesModule(module, function.module)) {
            continue;
        }
        intrinsicModules_.insert(std::string(function.module));
        if (!symbols_.globalScope().containsLocal(function.name)) {
            symbols_.globalScope().declare(std::string(function.name), SymbolKind::Builtin);
        }
    }
}

const ModuleIntrinsic* SemanticAnalyzer::moduleIntrinsic(std::string_view name) const
{
    const ModuleIntrinsic* function = findModuleIntrinsic(name);
    return function != nullptr && intrinsicModules_.contains(std::string(function->module)) ? function : nullptr;
}

TypeRef SemanticAnalyzer::intrinsicResultType(const ModuleIntrinsic& function)
{
    if (function.result == "Void") {
        return types().voidType();
    }
    const TypeRef type = types().lookup(std::string(function.result));
    if (type == nullptr) {
        throw SemanticError(std::string(function.result) + " is not declared: " + std::string(function.module) +
                            " was not loaded");
    }
    return type;
}

// `ReadKey`, `ScreenSize`, `ClearScreen`, ...: a routine without arguments
// is called without parentheses.
TypeRef SemanticAnalyzer::analyzeIntrinsicIdentifier(const ast::IdentifierExpression& identifier,
                                                    const ModuleIntrinsic& function)
{
    rejectInContract("the library routine " + std::string(function.name));
    if (function.parameterCount != 0) {
        throw SemanticError("function " + identifier.name() +
                            " requires arguments; use parentheses only when passing arguments");
    }
    result_.bind(identifier, resolveOrThrow(identifier.name()));
    return intrinsicResultType(function);
}

TypeRef SemanticAnalyzer::analyzeIntrinsicCall(const ast::CallExpression& call, const ModuleIntrinsic& function)
{
    rejectInContract("the library routine " + std::string(function.name));
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    const Symbol& symbol = resolveOrThrow(callee.name());
    result_.bind(callee, symbol);
    result_.bind(call, symbol);
    const std::string name(function.name);
    if (function.parameterCount == 0) {
        throw SemanticError(name + " takes no arguments: write " + name + " without parentheses");
    }
    if (call.arguments().size() != function.parameterCount) {
        throw SemanticError(name + " expects " + std::to_string(function.parameterCount) +
                            (function.parameterCount == 1 ? " argument" : " arguments") + ", got " +
                            std::to_string(call.arguments().size()));
    }
    for (std::size_t index = 0; index < function.parameterCount; ++index) {
        const std::string_view expectedName = function.parameters[index];
        const TypeRef expected = expectedName == "Integer"  ? types().int64Type()
                                 : expectedName == "Bool"   ? types().boolType()
                                 : expectedName == "String" ? types().stringType()
                                                            : types().lookup(std::string(expectedName));
        const ast::Expression& argument = *call.arguments()[index];
        const TypeRef actual = analyzeExpressionFor(argument, expected);
        if (expected == nullptr || !canAssignValue(expected, actual, argument)) {
            throw SemanticError("argument " + std::to_string(index + 1) + " of " + name + " must be " +
                                std::string(expectedName) + ", got " + types::nameOf(actual));
        }
    }
    return intrinsicResultType(function);
}

} // namespace inox::compiler::semantic
