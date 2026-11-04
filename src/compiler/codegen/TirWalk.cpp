// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


#include "TirWalk.h"

namespace inox::compiler::codegen::detail {

namespace {

bool anyContainsTry(TirStatements statements)
{
    for (const tir::Stmt* statement : statements) {
        if (statementContainsTry(*statement)) {
            return true;
        }
    }
    return false;
}

bool anyUsesExceptions(TirStatements statements)
{
    for (const tir::Stmt* statement : statements) {
        if (statementUsesExceptions(*statement)) {
            return true;
        }
    }
    return false;
}

} // namespace

bool statementsContainTry(TirStatements statements)
{
    return anyContainsTry(statements);
}

bool statementContainsTry(const tir::Stmt& statement)
{
    switch (statement.kind) {
    case tir::StmtKind::Try:
        return true;
    case tir::StmtKind::If:
        for (const tir::Branch& branch : statement.branches) {
            if (anyContainsTry(branch.body)) {
                return true;
            }
        }
        return anyContainsTry(statement.body) || anyContainsTry(statement.elseBody);
    case tir::StmtKind::Case:
        for (const tir::Branch& branch : statement.branches) {
            if (anyContainsTry(branch.body)) {
                return true;
            }
        }
        return anyContainsTry(statement.elseBody);
    case tir::StmtKind::Block:
    case tir::StmtKind::Unless:
    case tir::StmtKind::While:
    case tir::StmtKind::Repeat:
    case tir::StmtKind::ForIn:
    case tir::StmtKind::With:
        return anyContainsTry(statement.body);
    default:
        return false;
    }
}

void forEachStatement(TirStatements statements, const std::function<void(const tir::Stmt&)>& visit)
{
    for (const tir::Stmt* statement : statements) {
        visit(*statement);
        switch (statement->kind) {
        case tir::StmtKind::If:
            forEachStatement(statement->body, visit);
            for (const tir::Branch& branch : statement->branches) {
                forEachStatement(branch.body, visit);
            }
            forEachStatement(statement->elseBody, visit);
            break;
        case tir::StmtKind::Case:
            for (const tir::Branch& branch : statement->branches) {
                forEachStatement(branch.body, visit);
            }
            forEachStatement(statement->elseBody, visit);
            break;
        case tir::StmtKind::Try:
            forEachStatement(statement->body, visit);
            forEachStatement(statement->tryParts->exceptBody, visit);
            for (const tir::Handler& handler : statement->tryParts->handlers) {
                forEachStatement(handler.body, visit);
            }
            forEachStatement(statement->tryParts->elseBody, visit);
            forEachStatement(statement->tryParts->ensureBody, visit);
            break;
        case tir::StmtKind::Block:
        case tir::StmtKind::Unless:
        case tir::StmtKind::While:
        case tir::StmtKind::Repeat:
        case tir::StmtKind::ForIn:
        case tir::StmtKind::With:
            forEachStatement(statement->body, visit);
            break;
        default:
            break;
        }
    }
}

void forEachSubExpression(const tir::Place& place, const std::function<void(const tir::Expr&)>& visit)
{
    if (place.base != nullptr) {
        forEachSubExpression(*place.base, visit);
    }
    for (const tir::Expr* index : place.indices) {
        forEachSubExpression(*index, visit);
    }
    if (place.value != nullptr) {
        forEachSubExpression(*place.value, visit);
    }
}

void forEachSubExpression(const tir::Expr& expression, const std::function<void(const tir::Expr&)>& visit)
{
    visit(expression);
    if (expression.place != nullptr) {
        forEachSubExpression(*expression.place, visit);
    }
    for (const tir::Expr* operand : expression.operands) {
        forEachSubExpression(*operand, visit);
    }
    for (const tir::FieldValue& field : expression.fields) {
        if (field.value != nullptr) {
            forEachSubExpression(*field.value, visit);
        }
    }
}

void forEachExpressionOf(const tir::Stmt& statement, const std::function<void(const tir::Expr&)>& visit)
{
    switch (statement.kind) {
    case tir::StmtKind::Assign:
        forEachSubExpression(*statement.target, visit);
        forEachSubExpression(*statement.value, visit);
        break;
    case tir::StmtKind::If:
        forEachSubExpression(*statement.value, visit);
        for (const tir::Branch& branch : statement.branches) {
            forEachSubExpression(*branch.conditions[0], visit);
        }
        break;
    case tir::StmtKind::ForIn:
        forEachSubExpression(*statement.value, visit);
        if (statement.step != nullptr) {
            forEachSubExpression(*statement.step, visit);
        }
        break;
    case tir::StmtKind::Declare:
    case tir::StmtKind::Evaluate:
    case tir::StmtKind::Unless:
    case tir::StmtKind::While:
    case tir::StmtKind::Until:
    case tir::StmtKind::Case:
    case tir::StmtKind::Return:
    case tir::StmtKind::With:
        if (statement.value != nullptr) {
            forEachSubExpression(*statement.value, visit);
        }
        break;
    default:
        break;
    }
}

bool statementUsesExceptions(const tir::Stmt& statement)
{
    switch (statement.kind) {
    case tir::StmtKind::Try:
    case tir::StmtKind::Raise:
    case tir::StmtKind::Retry:
        return true;
    case tir::StmtKind::If:
        for (const tir::Branch& branch : statement.branches) {
            if (anyUsesExceptions(branch.body)) {
                return true;
            }
        }
        return anyUsesExceptions(statement.body) || anyUsesExceptions(statement.elseBody);
    case tir::StmtKind::Case:
        for (const tir::Branch& branch : statement.branches) {
            if (anyUsesExceptions(branch.body)) {
                return true;
            }
        }
        return anyUsesExceptions(statement.elseBody);
    case tir::StmtKind::Block:
    case tir::StmtKind::Unless:
    case tir::StmtKind::While:
    case tir::StmtKind::Repeat:
    case tir::StmtKind::ForIn:
    case tir::StmtKind::With:
        return anyUsesExceptions(statement.body);
    default:
        return false;
    }
}

bool programUsesExceptions(const tir::Program& program)
{
    for (const tir::Routine* routine : program.routines) {
        if (anyUsesExceptions(routine->body)) {
            return true;
        }
    }
    return false;
}

} // namespace inox::compiler::codegen::detail
