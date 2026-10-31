// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "AstClone.h"

#include <memory>
#include <stdexcept>
#include <utility>

namespace inox::compiler::ast {

namespace {

// Both switches below name every AstNodeKind and have no default, so a new
// kind of node is a compiler warning here until the copy knows it.

std::string copyTypeName(const std::string& name, const CloneRewriters& rewrite)
{
    return rewrite.typeName && !name.empty() ? rewrite.typeName(name) : name;
}

ExpressionPtr copyOptional(const Expression* expression, const CloneRewriters& rewrite)
{
    return expression != nullptr ? cloneExpression(*expression, rewrite.expression) : nullptr;
}

} // namespace

namespace {

ExpressionPtr cloneExpressionNode(const Expression& expression, const ExpressionRewriter& rewrite);
StatementPtr cloneStatementNode(const Statement& statement, const CloneRewriters& rewrite);

} // namespace

// A copy keeps the position of its original (change source-positions), so a
// diagnostic in a contract wrapper or a generic instance points at the
// source; a replacement keeps its own position, if it has one.
ExpressionPtr cloneExpression(const Expression& expression, const ExpressionRewriter& rewrite)
{
    ExpressionPtr copy = cloneExpressionNode(expression, rewrite);
    if (copy != nullptr && !copy->position().known()) {
        copy->setPosition(expression.position());
    }
    return copy;
}

StatementPtr cloneStatement(const Statement& statement, const CloneRewriters& rewrite)
{
    StatementPtr copy = cloneStatementNode(statement, rewrite);
    if (copy != nullptr && !copy->position().known()) {
        copy->setPosition(statement.position());
    }
    return copy;
}

namespace {

ExpressionPtr cloneExpressionNode(const Expression& expression, const ExpressionRewriter& rewrite)
{
    if (rewrite) {
        if (ExpressionPtr replacement = rewrite(expression)) {
            return replacement;
        }
    }
    switch (expression.kind()) {
    case AstNodeKind::LiteralExpression: {
        const auto& literal = static_cast<const LiteralExpression&>(expression);
        return std::make_unique<LiteralExpression>(literal.literalKind(), literal.value());
    }
    case AstNodeKind::IdentifierExpression:
        return std::make_unique<IdentifierExpression>(static_cast<const IdentifierExpression&>(expression).name());
    case AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const BinaryExpression&>(expression);
        return std::make_unique<BinaryExpression>(binary.op(), cloneExpression(binary.left(), rewrite),
                                                  cloneExpression(binary.right(), rewrite));
    }
    case AstNodeKind::UnaryExpression: {
        const auto& unary = static_cast<const UnaryExpression&>(expression);
        return std::make_unique<UnaryExpression>(unary.op(), cloneExpression(unary.operand(), rewrite));
    }
    case AstNodeKind::CallExpression: {
        const auto& call = static_cast<const CallExpression&>(expression);
        std::vector<ExpressionPtr> arguments;
        arguments.reserve(call.arguments().size());
        for (const auto& argument : call.arguments()) {
            arguments.push_back(cloneExpression(*argument, rewrite));
        }
        return std::make_unique<CallExpression>(cloneExpression(call.callee(), rewrite), std::move(arguments));
    }
    case AstNodeKind::ConditionalExpression: {
        const auto& conditional = static_cast<const ConditionalExpression&>(expression);
        return std::make_unique<ConditionalExpression>(cloneExpression(conditional.condition(), rewrite),
                                                       cloneExpression(conditional.thenValue(), rewrite),
                                                       cloneExpression(conditional.elseValue(), rewrite));
    }
    case AstNodeKind::Module:
    case AstNodeKind::UseDeclaration:
    case AstNodeKind::SectionDeclaration:
    case AstNodeKind::RawDeclaration:
    case AstNodeKind::FunctionDeclaration:
    case AstNodeKind::BlockStatement:
    case AstNodeKind::ExpressionStatement:
    case AstNodeKind::VarStatement:
    case AstNodeKind::VarBlockStatement:
    case AstNodeKind::IfStatement:
    case AstNodeKind::UnlessStatement:
    case AstNodeKind::WhileStatement:
    case AstNodeKind::RepeatStatement:
    case AstNodeKind::UntilStatement:
    case AstNodeKind::ForInStatement:
    case AstNodeKind::CaseStatement:
    case AstNodeKind::TryStatement:
    case AstNodeKind::RaiseStatement:
    case AstNodeKind::RetryStatement:
    case AstNodeKind::ReturnStatement:
    case AstNodeKind::ExitStatement:
    case AstNodeKind::LeaveStatement:
    case AstNodeKind::ContinueStatement:
    case AstNodeKind::WithStatement:
        break;
    }
    throw std::logic_error("cloneExpression: not an expression");
}

} // namespace

std::vector<StatementPtr> cloneStatements(const std::vector<StatementPtr>& statements,
                                          const CloneRewriters& rewrite)
{
    std::vector<StatementPtr> copies;
    copies.reserve(statements.size());
    for (const auto& statement : statements) {
        copies.push_back(cloneStatement(*statement, rewrite));
    }
    return copies;
}

namespace {

StatementPtr cloneStatementNode(const Statement& statement, const CloneRewriters& rewrite)
{
    const auto expression = [&](const Expression& node) { return cloneExpression(node, rewrite.expression); };
    const auto body = [&](const std::vector<StatementPtr>& statements) { return cloneStatements(statements, rewrite); };

    switch (statement.kind()) {
    case AstNodeKind::BlockStatement:
        return std::make_unique<BlockStatement>(body(static_cast<const BlockStatement&>(statement).statements()));
    case AstNodeKind::ExpressionStatement: {
        const auto& node = static_cast<const ExpressionStatement&>(statement);
        return std::make_unique<ExpressionStatement>(expression(node.expression()), node.isCompoundAssignment(),
                                                     node.isDiscard());
    }
    case AstNodeKind::VarStatement: {
        const auto& node = static_cast<const VarStatement&>(statement);
        return std::make_unique<VarStatement>(node.isMutable(), node.name(), copyOptional(node.initializer(), rewrite),
                                              copyTypeName(node.typeName(), rewrite));
    }
    case AstNodeKind::VarBlockStatement:
        return std::make_unique<VarBlockStatement>(
            body(static_cast<const VarBlockStatement&>(statement).declarations()));
    case AstNodeKind::IfStatement: {
        const auto& node = static_cast<const IfStatement&>(statement);
        std::vector<ElseIfClause> clauses;
        clauses.reserve(node.elseIfClauses().size());
        for (const ElseIfClause& clause : node.elseIfClauses()) {
            clauses.push_back(ElseIfClause{expression(*clause.condition), body(clause.body)});
        }
        return std::make_unique<IfStatement>(expression(node.condition()), body(node.thenBody()),
                                             std::move(clauses), body(node.elseBody()));
    }
    case AstNodeKind::UnlessStatement: {
        const auto& node = static_cast<const UnlessStatement&>(statement);
        return std::make_unique<UnlessStatement>(expression(node.condition()), body(node.body()));
    }
    case AstNodeKind::WhileStatement: {
        const auto& node = static_cast<const WhileStatement&>(statement);
        return std::make_unique<WhileStatement>(expression(node.condition()), body(node.body()));
    }
    case AstNodeKind::RepeatStatement:
        return std::make_unique<RepeatStatement>(body(static_cast<const RepeatStatement&>(statement).body()));
    case AstNodeKind::UntilStatement:
        return std::make_unique<UntilStatement>(expression(static_cast<const UntilStatement&>(statement).condition()));
    case AstNodeKind::ForInStatement: {
        const auto& node = static_cast<const ForInStatement&>(statement);
        return std::make_unique<ForInStatement>(node.iterator(), expression(node.iterable()),
                                                copyOptional(node.step(), rewrite), body(node.body()));
    }
    case AstNodeKind::CaseStatement: {
        const auto& node = static_cast<const CaseStatement&>(statement);
        std::vector<CaseArm> arms;
        arms.reserve(node.arms().size());
        for (const CaseArm& arm : node.arms()) {
            CaseArm copy;
            copy.choices.reserve(arm.choices.size());
            for (const auto& choice : arm.choices) {
                copy.choices.push_back(expression(*choice));
            }
            copy.body = body(arm.body);
            arms.push_back(std::move(copy));
        }
        return std::make_unique<CaseStatement>(expression(node.expression()), std::move(arms),
                                               body(node.elseBody()), node.hasElse());
    }
    case AstNodeKind::TryStatement: {
        const auto& node = static_cast<const TryStatement&>(statement);
        std::vector<ExceptionHandler> handlers;
        handlers.reserve(node.handlers().size());
        for (const ExceptionHandler& handler : node.handlers()) {
            handlers.push_back(
                ExceptionHandler{handler.bindingName, copyTypeName(handler.typeName, rewrite), body(handler.body)});
        }
        return std::make_unique<TryStatement>(body(node.body()), node.hasExcept(), node.hasPlainExcept(),
                                              body(node.exceptBody()), std::move(handlers), body(node.elseBody()),
                                              node.hasEnsure(), body(node.ensureBody()));
    }
    case AstNodeKind::RaiseStatement:
        return std::make_unique<RaiseStatement>(
            copyOptional(static_cast<const RaiseStatement&>(statement).expression(), rewrite));
    case AstNodeKind::RetryStatement:
        return std::make_unique<RetryStatement>(expression(static_cast<const RetryStatement&>(statement).count()));
    case AstNodeKind::ReturnStatement:
        return std::make_unique<ReturnStatement>(
            expression(static_cast<const ReturnStatement&>(statement).expression()));
    case AstNodeKind::ExitStatement:
        return std::make_unique<ExitStatement>();
    case AstNodeKind::LeaveStatement:
        return std::make_unique<LeaveStatement>();
    case AstNodeKind::ContinueStatement:
        return std::make_unique<ContinueStatement>();
    case AstNodeKind::WithStatement: {
        const auto& node = static_cast<const WithStatement&>(statement);
        return std::make_unique<WithStatement>(expression(node.target()), body(node.body()), node.bindingName());
    }
    case AstNodeKind::Module:
    case AstNodeKind::UseDeclaration:
    case AstNodeKind::SectionDeclaration:
    case AstNodeKind::RawDeclaration:
    case AstNodeKind::FunctionDeclaration:
    case AstNodeKind::LiteralExpression:
    case AstNodeKind::IdentifierExpression:
    case AstNodeKind::BinaryExpression:
    case AstNodeKind::UnaryExpression:
    case AstNodeKind::CallExpression:
    case AstNodeKind::ConditionalExpression:
        break;
    }
    throw std::logic_error("cloneStatement: not a statement");
}

} // namespace

} // namespace inox::compiler::ast
