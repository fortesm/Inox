// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: Std.Debug (OpenSpec change debug-assert). A failed
// Assert is a deterministic trap like an overflow (CANON-19; Marcelo
// Fortes, 2026-10-13: "3 escolho a alternativa A").

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"

#include <string>

namespace inox::compiler::semantic {

using namespace detail;

std::string SemanticAnalyzer::analyzeAssertCall(const ast::CallExpression& call)
{
    if (!debugVisible_) {
        throw SemanticError("Assert is declared in Std.Debug: add `Use Std.Debug`");
    }
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    const Symbol& symbol = resolveOrThrow(callee.name());
    result_.bind(callee, symbol);
    result_.bind(call, symbol);

    const auto& arguments = call.arguments();
    if (arguments.empty() || arguments.size() > 2) {
        throw SemanticError("Assert expects a Bool condition and an optional String message, got " +
                            std::to_string(arguments.size()) + " arguments");
    }
    const std::string condition = analyzeExpression(*arguments[0]);
    if (condition != "Bool") {
        throw SemanticError("argument 1 of Assert expects Bool, got " + condition);
    }
    if (arguments.size() == 2) {
        const std::string message = analyzeExpression(*arguments[1]);
        if (message != "String") {
            throw SemanticError("argument 2 of Assert expects String, got " + message);
        }
    }
    return "Void";
}

} // namespace inox::compiler::semantic
