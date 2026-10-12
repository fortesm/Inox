// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Read-only walks over the AST used by the emitter (OpenSpec change
// compiler-modularization, step 1).

#pragma once

#include "../ast/Ast.h"

#include <functional>
#include <vector>

namespace inox::compiler::codegen::detail {

bool statementContainsTry(const ast::Statement& statement);
bool statementsContainTry(const std::vector<ast::StatementPtr>& statements);
bool functionContainsTry(const ast::FunctionDeclaration& function);

// Visits every statement of a body, nested bodies included (try, handlers,
// ensure, loops, case arms, with).
void forEachStatement(const std::vector<ast::StatementPtr>& statements,
                      const std::function<void(const ast::Statement&)>& visit);
// Visits an expression and every expression inside it.
void forEachSubExpression(const ast::Expression& expression,
                          const std::function<void(const ast::Expression&)>& visit);
// The expressions a statement evaluates itself (not those of nested bodies).
void forEachExpressionOf(const ast::Statement& statement,
                         const std::function<void(const ast::Expression&)>& visit);

bool statementUsesExceptions(const ast::Statement& statement);
bool moduleUsesExceptions(const ast::ModuleNode& module);

} // namespace inox::compiler::codegen::detail
