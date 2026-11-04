// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


// Walks of the typed IR that code generation asks before it emits a routine:
// which statements and expressions a routine holds, whether it has a `try`.

#pragma once

#include "../tir/Tir.h"

#include <functional>

namespace inox::compiler::codegen::detail {

using TirStatements = tir::Span<const tir::Stmt*>;

bool statementContainsTry(const tir::Stmt& statement);
bool statementsContainTry(TirStatements statements);
// Every statement of `statements`, nested bodies included, in source order.
void forEachStatement(TirStatements statements, const std::function<void(const tir::Stmt&)>& visit);
// `expression` and every expression it holds, those of its places included.
void forEachSubExpression(const tir::Expr& expression, const std::function<void(const tir::Expr&)>& visit);
void forEachSubExpression(const tir::Place& place, const std::function<void(const tir::Expr&)>& visit);
// The expressions a statement evaluates itself (not those of nested bodies).
void forEachExpressionOf(const tir::Stmt& statement, const std::function<void(const tir::Expr&)>& visit);
bool statementUsesExceptions(const tir::Stmt& statement);
bool programUsesExceptions(const tir::Program& program);

} // namespace inox::compiler::codegen::detail
