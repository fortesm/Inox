// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Deep copies of AST nodes. The contract pass checks one condition in two
// places and an Override inherits the replaced method's clauses (CANON-24);
// a generic routine is instantiated by copying its body with its type
// parameters replaced (OpenSpec change type-representation, stage 2).

#pragma once

#include "Ast.h"

#include <functional>
#include <string>
#include <vector>

namespace inox::compiler::ast {

// May return a replacement for any expression node; it is asked before the
// node is copied, and nullptr keeps the copy.
using ExpressionRewriter = std::function<ExpressionPtr(const Expression&)>;

// Changes, in the copy, the written type of a typed local (F4). A type
// named inside an expression (`New(T)`, `T(Field := ...)`) is an
// identifier: the expression rewriter sees it.
using TypeRewriter = std::function<void(TypeExpr&)>;

// Maps the exception type an `on E: T` handler names (a name until F7) to
// the name written in the copy.
using ExceptionNameRewriter = std::function<std::string(const std::string&)>;

struct CloneRewriters {
    ExpressionRewriter expression;
    TypeRewriter type;
    ExceptionNameRewriter exceptionName;
};

ExpressionPtr cloneExpression(const Expression& expression, const ExpressionRewriter& rewrite = {});
StatementPtr cloneStatement(const Statement& statement, const CloneRewriters& rewrite = {});
std::vector<StatementPtr> cloneStatements(const std::vector<StatementPtr>& statements,
                                          const CloneRewriters& rewrite = {});

} // namespace inox::compiler::ast
