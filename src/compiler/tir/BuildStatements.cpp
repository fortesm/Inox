// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


// Phase 5b: statements, as the AST after phases 2b and 4 has them.

#include "Builder.h"

namespace inox::compiler::tir::detail {

Span<const Stmt*> Builder::statements(const std::vector<ast::StatementPtr>& sources)
{
    Span<const Stmt*> result = arena_.array<const Stmt*>(sources.size());
    for (std::size_t index = 0; index < sources.size(); ++index) {
        result[index] = statement(*sources[index]);
    }
    return result;
}

const Stmt* Builder::statement(const ast::Statement& source)
{
    Stmt* result = nullptr;
    switch (source.kind()) {
    case ast::AstNodeKind::BlockStatement:
        result = make(StmtKind::Block, source);
        result->body = statements(static_cast<const ast::BlockStatement&>(source).statements());
        break;
    case ast::AstNodeKind::VarBlockStatement:
        result = make(StmtKind::Block, source);
        result->body = statements(static_cast<const ast::VarBlockStatement&>(source).declarations());
        break;
    case ast::AstNodeKind::ExpressionStatement:
        return expressionStatement(static_cast<const ast::ExpressionStatement&>(source));
    case ast::AstNodeKind::VarStatement:
        return declaration(static_cast<const ast::VarStatement&>(source));
    case ast::AstNodeKind::IfStatement:
        return ifStatement(static_cast<const ast::IfStatement&>(source));
    case ast::AstNodeKind::UnlessStatement: {
        const auto& unless = static_cast<const ast::UnlessStatement&>(source);
        result = make(StmtKind::Unless, source);
        result->value = expression(unless.condition());
        result->body = statements(unless.body());
        break;
    }
    case ast::AstNodeKind::WhileStatement: {
        const auto& loop = static_cast<const ast::WhileStatement&>(source);
        result = make(StmtKind::While, source);
        result->value = expression(loop.condition());
        result->body = statements(loop.body());
        break;
    }
    case ast::AstNodeKind::RepeatStatement:
        result = make(StmtKind::Repeat, source);
        result->body = statements(static_cast<const ast::RepeatStatement&>(source).body());
        break;
    case ast::AstNodeKind::UntilStatement:
        result = make(StmtKind::Until, source);
        result->value = expression(static_cast<const ast::UntilStatement&>(source).condition());
        break;
    case ast::AstNodeKind::ForInStatement:
        return forStatement(static_cast<const ast::ForInStatement&>(source));
    case ast::AstNodeKind::CaseStatement:
        return caseStatement(static_cast<const ast::CaseStatement&>(source));
    case ast::AstNodeKind::TryStatement:
        return tryStatement(static_cast<const ast::TryStatement&>(source));
    case ast::AstNodeKind::RaiseStatement:
        return raiseStatement(static_cast<const ast::RaiseStatement&>(source));
    case ast::AstNodeKind::RetryStatement:
        result = make(StmtKind::Retry, source);
        result->value = expression(static_cast<const ast::RetryStatement&>(source).count());
        break;
    case ast::AstNodeKind::ReturnStatement:
        result = make(StmtKind::Return, source);
        result->value = expression(static_cast<const ast::ReturnStatement&>(source).expression());
        break;
    case ast::AstNodeKind::ExitStatement:
        result = make(StmtKind::Exit, source);
        break;
    case ast::AstNodeKind::LeaveStatement:
        result = make(StmtKind::Leave, source);
        break;
    case ast::AstNodeKind::ContinueStatement:
        result = make(StmtKind::Continue, source);
        break;
    case ast::AstNodeKind::WithStatement:
        return withStatement(static_cast<const ast::WithStatement&>(source));
    default:
        fail("a statement of this kind (phase 2b removes the written forms)", source);
    }
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Builder::expressionStatement(const ast::ExpressionStatement& source)
{
    const ast::Expression& expression = source.expression();
    if (expression.kind() == ast::AstNodeKind::BinaryExpression &&
        static_cast<const ast::BinaryExpression&>(expression).op() == ast::BinaryOperator::Assign) {
        const auto& assignment = static_cast<const ast::BinaryExpression&>(expression);
        if (const semantic::Symbol* declared = semantics_.declaredSymbol(&assignment)) {
            Stmt* result = make(StmtKind::Declare, source);
            result->symbol = declared;
            result->type = declared->type;
            result->inferred = true;
            result->value = this->expression(assignment.right());
            result->invariants = invariants(source, &assignment);
            return result;
        }
        Stmt* result = make(StmtKind::Assign, source);
        result->target = place(assignment.left());
        if (source.isCompoundAssignment()) {
            // `L := L op R` of phase 2b (CANON-5).
            result->compound = true;
            result->value = compoundOperation(assignment, result->target);
        } else {
            result->value = this->expression(assignment.right());
        }
        result->invariants = invariants(source, &assignment);
        return result;
    }
    Stmt* result = make(StmtKind::Evaluate, source);
    result->value = this->expression(expression);
    result->discard = source.isDiscard();
    result->invariants = invariants(source, nullptr);
    return result;
}

// `L op R` of `L op= R`: L is read through the statement's own target,
// never as a second place; the checks the analysis asked for apply to the
// result on its way into L.
const Expr* Builder::compoundOperation(const ast::BinaryExpression& assignment, const Place* target)
{
    const auto& combined = static_cast<const ast::BinaryExpression&>(assignment.right());
    Expr* operation = make(ExprKind::Binary, typeOf(combined), combined);
    operation->binary = binaryOp(combined.op());
    operation->operands = arena_.array<const Expr*>(2);
    Expr* read = make(ExprKind::Read, target->type, combined.left());
    read->place = target;
    operation->operands[0] = read;
    operation->operands[1] = expression(combined.right());
    return checked(combined, operation);
}

const Stmt* Builder::declaration(const ast::VarStatement& source)
{
    const semantic::Symbol* symbol = semantics_.declaredSymbol(&source);
    if (symbol == nullptr) {
        fail("a declaration without its symbol", source);
    }
    Stmt* result = make(StmtKind::Declare, source);
    result->symbol = symbol;
    result->type = semantics_.declaredType(source);
    result->inferred = !source.type() && source.initializer() != nullptr;
    if (source.initializer() != nullptr) {
        result->value = expression(*source.initializer());
    }
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Builder::ifStatement(const ast::IfStatement& source)
{
    Stmt* result = make(StmtKind::If, source);
    result->value = expression(source.condition());
    result->body = statements(source.thenBody());
    result->branches = arena_.array<Branch>(source.elseIfClauses().size());
    for (std::size_t index = 0; index < source.elseIfClauses().size(); ++index) {
        const ast::ElseIfClause& clause = source.elseIfClauses()[index];
        result->branches[index].conditions = arena_.array<const Expr*>(1);
        result->branches[index].conditions[0] = expression(*clause.condition);
        result->branches[index].body = statements(clause.body);
    }
    result->elseBody = statements(source.elseBody());
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Builder::caseStatement(const ast::CaseStatement& source)
{
    Stmt* result = make(StmtKind::Case, source);
    result->value = expression(source.expression());
    result->branches = arena_.array<Branch>(source.arms().size());
    for (std::size_t index = 0; index < source.arms().size(); ++index) {
        const ast::CaseArm& arm = source.arms()[index];
        result->branches[index].conditions = expressions(arm.choices, 0, nullptr);
        result->branches[index].body = statements(arm.body);
    }
    result->hasElse = source.hasElse();
    result->elseBody = statements(source.elseBody());
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Builder::tryStatement(const ast::TryStatement& source)
{
    Stmt* result = make(StmtKind::Try, source);
    result->body = statements(source.body());
    TryParts* parts = arena_.make<TryParts>();
    parts->hasExcept = source.hasExcept();
    parts->plainExcept = source.hasPlainExcept();
    parts->exceptBody = statements(source.exceptBody());
    parts->handlers = arena_.array<Handler>(source.handlers().size());
    for (std::size_t index = 0; index < source.handlers().size(); ++index) {
        const ast::ExceptionHandler& handler = source.handlers()[index];
        parts->handlers[index].type = semantics_.exceptionType(&handler);
        if (parts->handlers[index].type == nullptr) {
            fail("a handler without its exception type", source);
        }
        parts->handlers[index].binding = semantics_.declaredSymbol(&handler);
        parts->handlers[index].body = statements(handler.body);
    }
    parts->elseBody = statements(source.elseBody());
    parts->hasEnsure = source.hasEnsure();
    parts->ensureBody = statements(source.ensureBody());
    result->tryParts = parts;
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Builder::withStatement(const ast::WithStatement& source)
{
    const semantic::Symbol* binding = semantics_.declaredSymbol(&source);
    if (binding == nullptr) {
        fail("a `with` without its binding", source);
    }
    Stmt* result = make(StmtKind::With, source);
    result->symbol = binding;
    const Place* target = place(source.target());
    if (target->kind == PlaceKind::Value) {
        result->value = target->value;
    } else {
        Expr* read = make(ExprKind::Read, target->type, source.target());
        read->place = target;
        result->value = read;
    }
    withs_[binding] = result;
    result->body = statements(source.body());
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Builder::forStatement(const ast::ForInStatement& source)
{
    const semantic::Symbol* iterator = semantics_.declaredSymbol(&source);
    if (iterator == nullptr) {
        fail("a loop without its iterator", source);
    }
    Stmt* result = make(StmtKind::ForIn, source);
    result->symbol = iterator;
    result->type = iterator->type;
    result->value = expression(source.iterable());
    if (source.step() != nullptr) {
        result->step = expression(*source.step());
    }
    result->body = statements(source.body());
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Builder::raiseStatement(const ast::RaiseStatement& source)
{
    Stmt* result = make(StmtKind::Raise, source);
    // The exception type the analysis resolved (F7).
    if (source.expression() != nullptr) {
        result->type = semantics_.exceptionType(&source);
        if (result->type == nullptr || !result->type->isException()) {
            fail("a Raise without its exception type", source);
        }
    }
    return result;
}

} // namespace inox::compiler::tir::detail
