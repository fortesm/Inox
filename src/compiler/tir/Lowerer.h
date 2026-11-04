// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


#pragma once

// The lowering to the typed IR (phase 5b, F6), shared by its files:
// LowerExpressions.cpp (expressions and calls), LowerPlaces.cpp (places),
// LowerStatements.cpp (statements) and Lower.cpp (routines, the program,
// the helpers). Internal to tir/: only Lower.h is its interface.

#include "Lower.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace inox::compiler::tir::detail {

using semantic::CallKind;
using semantic::CallMeaning;

class Lowerer {
public:
    Lowerer(Program& program, const semantic::SemanticResult& semantics)
        : program_(program), arena_(program.arena), semantics_(semantics)
    {
    }

    void lowerModule(const ast::ModuleNode& module);

private:
    // Expressions.
    const Expr* expression(const ast::Expression& expression);
    const Expr* identifier(const ast::IdentifierExpression& identifier);
    const Expr* call(const ast::CallExpression& call, const CallMeaning& meaning);
    const Expr* construction(const ast::CallExpression& call);
    Expr* slice(const ast::CallExpression& call, const CallMeaning& meaning, TypeRef type);
    Expr* check(const ast::CallExpression& call);
    Expr* builtin(const ast::CallExpression& call, const CallMeaning& meaning, TypeRef type);
    semantic::StringFunction stringFunction(const ast::CallExpression& call) const;
    const Expr* constantLiteral(std::int64_t value, TypeRef type, ast::SourcePosition position);
    const Expr* checked(const ast::Expression& source, const Expr* value);
    Span<const Expr*> expressions(const std::vector<ast::ExpressionPtr>& expressions, std::size_t first,
                                  const ast::Expression* receiver);
    const Place* place(const ast::Expression& expression);
    const Place* variable(const ast::IdentifierExpression& identifier, const semantic::Symbol& symbol);
    const Place* through(const Place* base);
    const Invariants* invariants(const ast::AstNode& site, const ast::AstNode* other);

    // Statements.
    const Stmt* statement(const ast::Statement& statement);
    Span<const Stmt*> statements(const std::vector<ast::StatementPtr>& statements);
    const Stmt* expressionStatement(const ast::ExpressionStatement& statement);
    const Stmt* declaration(const ast::VarStatement& declaration);
    const Stmt* ifStatement(const ast::IfStatement& statement);
    const Stmt* caseStatement(const ast::CaseStatement& statement);
    const Stmt* tryStatement(const ast::TryStatement& statement);
    const Stmt* withStatement(const ast::WithStatement& statement);
    const Stmt* forStatement(const ast::ForInStatement& statement);
    const Stmt* raiseStatement(const ast::RaiseStatement& statement);

    // Routines and module declarations.
    Routine* routineShell(const ast::FunctionDeclaration& function, bool entry);
    void routineBody(Routine& routine);
    const Routine* routineOf(const semantic::FunctionSignature* signature, const ast::AstNode& site);
    const Expr* constantLiteral(const semantic::ConstantValue& value, TypeRef type, ast::SourcePosition position);

    Expr* make(ExprKind kind, TypeRef type, const ast::AstNode& source);
    Stmt* make(StmtKind kind, const ast::AstNode& source);
    TypeRef typeOf(const ast::Expression& expression) const;

    [[noreturn]] void fail(const char* what, const ast::AstNode& node) const;

    Program& program_;
    Arena& arena_;
    const semantic::SemanticResult& semantics_;
    std::unordered_map<const ast::FunctionDeclaration*, Routine*> routines_;
    std::unordered_map<const semantic::Symbol*, const Stmt*> withs_;
};

} // namespace inox::compiler::tir::detail
