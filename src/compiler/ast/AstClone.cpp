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

std::string copyExceptionName(const std::string& name, const CloneRewriters& rewrite)
{
    return rewrite.exceptionName && !name.empty() ? rewrite.exceptionName(name) : name;
}

ExpressionPtr copyOptional(const Expression* expression, const CloneRewriters& rewrite);

// A loop's `invariant` and `variant` lines (the written form, before
// phase 2b) onto its copy; out of line and after the copy, so the
// recursive cloneStatementNode keeps its frame.
template <typename Loop>
void copyContractsOf(const Statement& original, Statement& copy, const CloneRewriters& rewrite)
{
    const auto& from = static_cast<const Loop&>(original);
    if (!from.contracts().empty()) {
        static_cast<Loop&>(copy).contracts() = cloneContractClauses(from.contracts(), rewrite.expression);
    }
}

void copyLoopContracts(const Statement& original, Statement& copy, const CloneRewriters& rewrite);

// What a copy takes from its original besides its children: its position,
// and a loop's contracts.
void finishCopy(const Statement& original, Statement& copy, const CloneRewriters& rewrite)
{
    if (!copy.position().known()) {
        copy.setPosition(original.position());
    }
    copyLoopContracts(original, copy, rewrite);
}

void copyLoopContracts(const Statement& original, Statement& copy, const CloneRewriters& rewrite)
{
    switch (original.kind()) {
    case AstNodeKind::WhileStatement:
        copyContractsOf<WhileStatement>(original, copy, rewrite);
        return;
    case AstNodeKind::RepeatStatement:
        copyContractsOf<RepeatStatement>(original, copy, rewrite);
        return;
    case AstNodeKind::ForInStatement:
        copyContractsOf<ForInStatement>(original, copy, rewrite);
        return;
    default:
        return;
    }
}

// A typed local's copy, built here so that cloneStatementNode, which
// recurses, holds no TypeExpr in its frame (tools/stack_frames.py).
StatementPtr copyVarStatement(const VarStatement& node, const CloneRewriters& rewrite)
{
    std::optional<TypeExpr> type = node.type();
    if (type && rewrite.type) {
        rewrite.type(*type);
    }
    return std::make_unique<VarStatement>(node.isMutable(), node.name(), copyOptional(node.initializer(), rewrite),
                                          std::move(type));
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
    finishCopy(statement, *copy, rewrite);
    return copy;
}

namespace {

ExpressionPtr copyWithTarget(const WithTargetExpression& node)
{
    return std::make_unique<WithTargetExpression>(node.level());
}

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
    case AstNodeKind::ChainedAssignmentStatement:
    case AstNodeKind::CompoundAssignmentStatement:
        break;
    case AstNodeKind::WithTargetExpression:
        return copyWithTarget(static_cast<const WithTargetExpression&>(expression));
    }
    throw std::logic_error("cloneExpression: not an expression");
}

} // namespace

namespace {

// Whether the statement rewriter replaced `statement` (it appended what
// replaces it); out of line, so cloneStatements keeps its frame.
bool rewritten(const Statement& statement, const CloneRewriters& rewrite, std::vector<StatementPtr>& copies)
{
    return rewrite.statement && rewrite.statement(statement, copies);
}

} // namespace

std::vector<StatementPtr> cloneStatements(const std::vector<StatementPtr>& statements,
                                          const CloneRewriters& rewrite)
{
    std::vector<StatementPtr> copies;
    copies.reserve(statements.size());
    for (const auto& statement : statements) {
        if (!rewritten(*statement, rewrite, copies)) {
            copies.push_back(cloneStatement(*statement, rewrite));
        }
    }
    return copies;
}

ContractClause cloneContractClause(const ContractClause& clause, const ExpressionRewriter& rewrite)
{
    ContractClause copy;
    copy.kind = clause.kind;
    copy.condition = clause.condition ? cloneExpression(*clause.condition, rewrite) : nullptr;
    copy.text = clause.text;
    copy.message = clause.message;
    copy.line = clause.line;
    return copy;
}

std::vector<ContractClause> cloneContractClauses(const std::vector<ContractClause>& clauses,
                                                 const ExpressionRewriter& rewrite)
{
    std::vector<ContractClause> copies;
    copies.reserve(clauses.size());
    for (const ContractClause& clause : clauses) {
        copies.push_back(cloneContractClause(clause, rewrite));
    }
    return copies;
}

namespace {

// Out of line, as copyVarStatement: cloneStatementNode recurses, and each
// case's temporaries would widen every level of it (tools/stack_frames.py).
StatementPtr copyWrittenAssignment(const Statement& statement, const CloneRewriters& rewrite)
{
    if (statement.kind() == AstNodeKind::CompoundAssignmentStatement) {
        const auto& node = static_cast<const CompoundAssignmentStatement&>(statement);
        return std::make_unique<CompoundAssignmentStatement>(cloneExpression(node.target(), rewrite.expression),
                                                             node.op(),
                                                             cloneExpression(node.value(), rewrite.expression));
    }
    const auto& node = static_cast<const ChainedAssignmentStatement&>(statement);
    std::vector<ExpressionPtr> targets;
    for (const auto& target : node.targets()) {
        targets.push_back(cloneExpression(*target, rewrite.expression));
    }
    return std::make_unique<ChainedAssignmentStatement>(std::move(targets),
                                                        cloneExpression(node.value(), rewrite.expression));
}

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
        return copyVarStatement(static_cast<const VarStatement&>(statement), rewrite);
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
    case AstNodeKind::RepeatStatement: {
        const auto& node = static_cast<const RepeatStatement&>(statement);
        return std::make_unique<RepeatStatement>(body(node.body()));
    }
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
                ExceptionHandler{handler.bindingName, copyExceptionName(handler.typeName, rewrite), body(handler.body)});
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
    case AstNodeKind::ChainedAssignmentStatement:
    case AstNodeKind::CompoundAssignmentStatement:
        return copyWrittenAssignment(statement, rewrite);
    case AstNodeKind::WithTargetExpression:
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
