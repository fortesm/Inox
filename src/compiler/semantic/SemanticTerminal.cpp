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
#include "StdTerminal.h"

#include <string>
#include <string_view>

namespace inox::compiler::semantic {

using namespace detail;

void SemanticAnalyzer::declareTerminalNames(const ast::ModuleNode& module)
{
    terminalVisible_ = usesModule(module, "Std.Terminal");
    if (!terminalVisible_) {
        return;
    }
    for (const TerminalSignature& function : kTerminalFunctions) {
        if (!symbols_.globalScope().containsLocal(function.name)) {
            symbols_.globalScope().declare(std::string(function.name), SymbolKind::Builtin);
        }
    }
}

const TerminalSignature* SemanticAnalyzer::terminalFunction(std::string_view name) const
{
    return terminalVisible_ ? findTerminalFunction(name) : nullptr;
}

TypeRef SemanticAnalyzer::terminalResultType(const TerminalSignature& function)
{
    if (function.result == "Void") {
        return types().voidType();
    }
    const TypeRef type = types().lookup(std::string(function.result));
    if (type == nullptr) {
        throw SemanticError(std::string(function.result) + " is not declared: Std.Terminal was not loaded");
    }
    return type;
}

// `ReadKey`, `ScreenSize`, `ClearScreen`, ...: a routine without arguments
// is called without parentheses.
TypeRef SemanticAnalyzer::analyzeTerminalIdentifier(const ast::IdentifierExpression& identifier,
                                                    const TerminalSignature& function)
{
    rejectInContract("the terminal routine " + std::string(function.name));
    if (function.parameterCount != 0) {
        throw SemanticError("function " + identifier.name() +
                            " requires arguments; use parentheses only when passing arguments");
    }
    result_.bind(identifier, resolveOrThrow(identifier.name()));
    return terminalResultType(function);
}

TypeRef SemanticAnalyzer::analyzeTerminalCall(const ast::CallExpression& call, const TerminalSignature& function)
{
    rejectInContract("the terminal routine " + std::string(function.name));
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
        const TypeRef expected = expectedName == "Integer" ? types().int64Type()
                                 : expectedName == "Bool"  ? types().boolType()
                                                           : types().lookup(std::string(expectedName));
        const ast::Expression& argument = *call.arguments()[index];
        const TypeRef actual = analyzeExpressionFor(argument, expected);
        if (expected == nullptr || !canAssignValue(expected, actual, argument)) {
            throw SemanticError("argument " + std::to_string(index + 1) + " of " + name + " must be " +
                                std::string(expectedName) + ", got " + types::nameOf(actual));
        }
    }
    return terminalResultType(function);
}

} // namespace inox::compiler::semantic
