// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "AstQueries.h"

namespace inox::compiler::codegen::detail {

bool statementsContainTry(const std::vector<ast::StatementPtr>& statements)
{
    for (const auto& statement : statements) {
        if (statementContainsTry(*statement)) return true;
    }
    return false;
}

bool statementContainsTry(const ast::Statement& statement)
{
    switch (statement.kind()) {
    case ast::AstNodeKind::TryStatement:
        return true;
    case ast::AstNodeKind::BlockStatement:
        return statementsContainTry(static_cast<const ast::BlockStatement&>(statement).statements());
    case ast::AstNodeKind::IfStatement: {
        const auto& node = static_cast<const ast::IfStatement&>(statement);
        if (statementsContainTry(node.thenBody()) || statementsContainTry(node.elseBody())) return true;
        for (const auto& clause : node.elseIfClauses()) if (statementsContainTry(clause.body)) return true;
        return false;
    }
    case ast::AstNodeKind::UnlessStatement:
        return statementsContainTry(static_cast<const ast::UnlessStatement&>(statement).body());
    case ast::AstNodeKind::WhileStatement:
        return statementsContainTry(static_cast<const ast::WhileStatement&>(statement).body());
    case ast::AstNodeKind::RepeatStatement:
        return statementsContainTry(static_cast<const ast::RepeatStatement&>(statement).body());
    case ast::AstNodeKind::ForInStatement:
        return statementsContainTry(static_cast<const ast::ForInStatement&>(statement).body());
    case ast::AstNodeKind::CaseStatement: {
        const auto& node = static_cast<const ast::CaseStatement&>(statement);
        for (const auto& arm : node.arms()) if (statementsContainTry(arm.body)) return true;
        return statementsContainTry(node.elseBody());
    }
    case ast::AstNodeKind::WithStatement:
        return statementsContainTry(static_cast<const ast::WithStatement&>(statement).body());
    default:
        return false;
    }
}

bool functionContainsTry(const ast::FunctionDeclaration& function)
{
    return statementsContainTry(function.body());
}

// Visits every statement of a body, nested bodies included (try, handlers,
// ensure, loops, case arms, with).
void forEachStatement(const std::vector<ast::StatementPtr>& statements,
                      const std::function<void(const ast::Statement&)>& visit)
{
    for (const auto& statement : statements) {
        visit(*statement);
        switch (statement->kind()) {
        case ast::AstNodeKind::BlockStatement:
            forEachStatement(static_cast<const ast::BlockStatement&>(*statement).statements(), visit);
            break;
        case ast::AstNodeKind::VarBlockStatement:
            forEachStatement(static_cast<const ast::VarBlockStatement&>(*statement).declarations(), visit);
            break;
        case ast::AstNodeKind::IfStatement: {
            const auto& node = static_cast<const ast::IfStatement&>(*statement);
            forEachStatement(node.thenBody(), visit);
            for (const auto& clause : node.elseIfClauses()) forEachStatement(clause.body, visit);
            forEachStatement(node.elseBody(), visit);
            break;
        }
        case ast::AstNodeKind::UnlessStatement:
            forEachStatement(static_cast<const ast::UnlessStatement&>(*statement).body(), visit);
            break;
        case ast::AstNodeKind::WhileStatement:
            forEachStatement(static_cast<const ast::WhileStatement&>(*statement).body(), visit);
            break;
        case ast::AstNodeKind::RepeatStatement:
            forEachStatement(static_cast<const ast::RepeatStatement&>(*statement).body(), visit);
            break;
        case ast::AstNodeKind::ForInStatement:
            forEachStatement(static_cast<const ast::ForInStatement&>(*statement).body(), visit);
            break;
        case ast::AstNodeKind::CaseStatement: {
            const auto& node = static_cast<const ast::CaseStatement&>(*statement);
            for (const auto& arm : node.arms()) forEachStatement(arm.body, visit);
            forEachStatement(node.elseBody(), visit);
            break;
        }
        case ast::AstNodeKind::TryStatement: {
            const auto& node = static_cast<const ast::TryStatement&>(*statement);
            forEachStatement(node.body(), visit);
            forEachStatement(node.exceptBody(), visit);
            for (const auto& handler : node.handlers()) forEachStatement(handler.body, visit);
            forEachStatement(node.elseBody(), visit);
            forEachStatement(node.ensureBody(), visit);
            break;
        }
        case ast::AstNodeKind::WithStatement:
            forEachStatement(static_cast<const ast::WithStatement&>(*statement).body(), visit);
            break;
        default:
            break;
        }
    }
}

void forEachSubExpression(const ast::Expression& expression,
                          const std::function<void(const ast::Expression&)>& visit)
{
    visit(expression);
    switch (expression.kind()) {
    case ast::AstNodeKind::BinaryExpression: {
        const auto& node = static_cast<const ast::BinaryExpression&>(expression);
        forEachSubExpression(node.left(), visit);
        forEachSubExpression(node.right(), visit);
        break;
    }
    case ast::AstNodeKind::UnaryExpression:
        forEachSubExpression(static_cast<const ast::UnaryExpression&>(expression).operand(), visit);
        break;
    case ast::AstNodeKind::CallExpression: {
        const auto& node = static_cast<const ast::CallExpression&>(expression);
        forEachSubExpression(node.callee(), visit);
        for (const auto& argument : node.arguments()) forEachSubExpression(*argument, visit);
        break;
    }
    case ast::AstNodeKind::ConditionalExpression: {
        const auto& node = static_cast<const ast::ConditionalExpression&>(expression);
        forEachSubExpression(node.condition(), visit);
        forEachSubExpression(node.thenValue(), visit);
        forEachSubExpression(node.elseValue(), visit);
        break;
    }
    default:
        break;
    }
}

// The expressions a statement evaluates itself (not those of nested bodies).
void forEachExpressionOf(const ast::Statement& statement,
                         const std::function<void(const ast::Expression&)>& visit)
{
    switch (statement.kind()) {
    case ast::AstNodeKind::VarStatement:
        if (const ast::Expression* initializer = static_cast<const ast::VarStatement&>(statement).initializer()) {
            forEachSubExpression(*initializer, visit);
        }
        break;
    case ast::AstNodeKind::ExpressionStatement:
        forEachSubExpression(static_cast<const ast::ExpressionStatement&>(statement).expression(), visit);
        break;
    case ast::AstNodeKind::IfStatement: {
        const auto& node = static_cast<const ast::IfStatement&>(statement);
        forEachSubExpression(node.condition(), visit);
        for (const auto& clause : node.elseIfClauses()) forEachSubExpression(*clause.condition, visit);
        break;
    }
    case ast::AstNodeKind::UnlessStatement:
        forEachSubExpression(static_cast<const ast::UnlessStatement&>(statement).condition(), visit);
        break;
    case ast::AstNodeKind::WhileStatement:
        forEachSubExpression(static_cast<const ast::WhileStatement&>(statement).condition(), visit);
        break;
    case ast::AstNodeKind::UntilStatement:
        forEachSubExpression(static_cast<const ast::UntilStatement&>(statement).condition(), visit);
        break;
    case ast::AstNodeKind::ForInStatement: {
        const auto& node = static_cast<const ast::ForInStatement&>(statement);
        forEachSubExpression(node.iterable(), visit);
        if (node.step() != nullptr) forEachSubExpression(*node.step(), visit);
        break;
    }
    case ast::AstNodeKind::CaseStatement:
        forEachSubExpression(static_cast<const ast::CaseStatement&>(statement).expression(), visit);
        break;
    case ast::AstNodeKind::ReturnStatement:
        forEachSubExpression(static_cast<const ast::ReturnStatement&>(statement).expression(), visit);
        break;
    case ast::AstNodeKind::WithStatement:
        forEachSubExpression(static_cast<const ast::WithStatement&>(statement).target(), visit);
        break;
    default:
        break;
    }
}

bool statementUsesExceptions(const ast::Statement& statement)
{
    if (statement.kind() == ast::AstNodeKind::TryStatement ||
        statement.kind() == ast::AstNodeKind::RaiseStatement ||
        statement.kind() == ast::AstNodeKind::RetryStatement) return true;
    switch (statement.kind()) {
    case ast::AstNodeKind::BlockStatement:
        for (const auto& s : static_cast<const ast::BlockStatement&>(statement).statements()) if (statementUsesExceptions(*s)) return true;
        break;
    case ast::AstNodeKind::IfStatement: {
        const auto& n = static_cast<const ast::IfStatement&>(statement);
        for (const auto& s : n.thenBody()) if (statementUsesExceptions(*s)) return true;
        for (const auto& c : n.elseIfClauses()) for (const auto& s : c.body) if (statementUsesExceptions(*s)) return true;
        for (const auto& s : n.elseBody()) if (statementUsesExceptions(*s)) return true;
        break;
    }
    case ast::AstNodeKind::UnlessStatement:
        for (const auto& s : static_cast<const ast::UnlessStatement&>(statement).body()) if (statementUsesExceptions(*s)) return true;
        break;
    case ast::AstNodeKind::WhileStatement:
        for (const auto& s : static_cast<const ast::WhileStatement&>(statement).body()) if (statementUsesExceptions(*s)) return true;
        break;
    case ast::AstNodeKind::RepeatStatement:
        for (const auto& s : static_cast<const ast::RepeatStatement&>(statement).body()) if (statementUsesExceptions(*s)) return true;
        break;
    case ast::AstNodeKind::ForInStatement:
        for (const auto& s : static_cast<const ast::ForInStatement&>(statement).body()) if (statementUsesExceptions(*s)) return true;
        break;
    case ast::AstNodeKind::CaseStatement: {
        const auto& n = static_cast<const ast::CaseStatement&>(statement);
        for (const auto& a : n.arms()) for (const auto& s : a.body) if (statementUsesExceptions(*s)) return true;
        for (const auto& s : n.elseBody()) if (statementUsesExceptions(*s)) return true;
        break;
    }
    case ast::AstNodeKind::WithStatement:
        for (const auto& s : static_cast<const ast::WithStatement&>(statement).body()) if (statementUsesExceptions(*s)) return true;
        break;
    default: break;
    }
    return false;
}

bool moduleUsesExceptions(const ast::ModuleNode& module)
{
    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::FunctionDeclaration) continue;
        const auto& fn = static_cast<const ast::FunctionDeclaration&>(*item);
        for (const auto& st : fn.body()) if (statementUsesExceptions(*st)) return true;
    }
    return false;
}

} // namespace inox::compiler::codegen::detail
