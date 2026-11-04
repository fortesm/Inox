// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "AstNormalizer.h"

#include "../ast/AstClone.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace inox::compiler::normalization {

namespace {

using namespace ast;

ExpressionPtr syntheticIdentifier(std::string name)
{
    return std::make_unique<IdentifierExpression>(std::move(name));
}

// A copy of a compound assignment's target (ADR-0012): a variable, a field
// path or an element with pure indices, so reading it again is exact. The
// copy has no positions, as the parser's had.
ExpressionPtr clonePureExpression(const Expression& expression)
{
    switch (expression.kind()) {
    case AstNodeKind::LiteralExpression: {
        const auto& literal = static_cast<const LiteralExpression&>(expression);
        return std::make_unique<LiteralExpression>(literal.literalKind(), literal.value());
    }
    case AstNodeKind::IdentifierExpression:
        return syntheticIdentifier(static_cast<const IdentifierExpression&>(expression).name());
    case AstNodeKind::UnaryExpression: {
        const auto& unary = static_cast<const UnaryExpression&>(expression);
        return std::make_unique<UnaryExpression>(unary.op(), clonePureExpression(unary.operand()));
    }
    case AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const BinaryExpression&>(expression);
        return std::make_unique<BinaryExpression>(binary.op(), clonePureExpression(binary.left()),
                                                  clonePureExpression(binary.right()));
    }
    default: {
        const auto& call = static_cast<const CallExpression&>(expression);
        std::vector<ExpressionPtr> arguments;
        for (const auto& argument : call.arguments()) {
            arguments.push_back(clonePureExpression(*argument));
        }
        return std::make_unique<CallExpression>(
            syntheticIdentifier(static_cast<const IdentifierExpression&>(call.callee()).name()),
            std::move(arguments));
    }
    }
}

// A copy of a chained assignment's target (a variable or a field path),
// without positions, as the parser made it.
ExpressionPtr cloneChainTarget(const Expression& expression)
{
    if (expression.kind() == AstNodeKind::IdentifierExpression) {
        return syntheticIdentifier(static_cast<const IdentifierExpression&>(expression).name());
    }
    const auto& call = static_cast<const CallExpression&>(expression);
    std::vector<ExpressionPtr> arguments;
    arguments.push_back(cloneChainTarget(*call.arguments()[0]));
    arguments.push_back(cloneChainTarget(*call.arguments()[1]));
    return std::make_unique<CallExpression>(syntheticIdentifier("__member"), std::move(arguments));
}

ExpressionPtr contractCheck(int kind, ExpressionPtr condition, const std::string& message)
{
    std::vector<ExpressionPtr> arguments;
    arguments.push_back(std::make_unique<LiteralExpression>(LiteralKind::Integer, std::to_string(kind)));
    arguments.push_back(std::move(condition));
    arguments.push_back(std::make_unique<LiteralExpression>(LiteralKind::String, message));
    return std::make_unique<CallExpression>(syntheticIdentifier("__check"), std::move(arguments));
}

std::string clauseMessage(const std::string& category, const std::string& routine, const ContractClause& clause,
                          const std::string& extra = {})
{
    std::string message = category + ": " + routine + ": " + clause.text + extra;
    if (!clause.message.empty()) {
        message += ": " + clause.message;
    }
    return message;
}

StatementPtr statementOf(ExpressionPtr expression)
{
    return std::make_unique<ExpressionStatement>(std::move(expression));
}

ExpressionPtr assignment(const std::string& name, ExpressionPtr value)
{
    return std::make_unique<BinaryExpression>(BinaryOperator::Assign, syntheticIdentifier(name), std::move(value));
}

StatementPtr positioned(StatementPtr statement, SourcePosition position)
{
    if (!statement->position().known()) {
        statement->setPosition(position);
    }
    return statement;
}

const std::vector<ContractClause>* loopContracts(const Statement& statement)
{
    switch (statement.kind()) {
    case AstNodeKind::WhileStatement:
        return &static_cast<const WhileStatement&>(statement).contracts();
    case AstNodeKind::RepeatStatement:
        return &static_cast<const RepeatStatement&>(statement).contracts();
    case AstNodeKind::ForInStatement:
        return &static_cast<const ForInStatement&>(statement).contracts();
    default:
        return nullptr;
    }
}

// The bodies a statement holds, in source order.
template <typename Visit>
void forEachBody(const Statement& statement, Visit visit)
{
    switch (statement.kind()) {
    case AstNodeKind::BlockStatement:
        visit(static_cast<const BlockStatement&>(statement).statements());
        return;
    case AstNodeKind::VarBlockStatement:
        visit(static_cast<const VarBlockStatement&>(statement).declarations());
        return;
    case AstNodeKind::IfStatement: {
        const auto& node = static_cast<const IfStatement&>(statement);
        visit(node.thenBody());
        for (const ElseIfClause& clause : node.elseIfClauses()) {
            visit(clause.body);
        }
        visit(node.elseBody());
        return;
    }
    case AstNodeKind::UnlessStatement:
        visit(static_cast<const UnlessStatement&>(statement).body());
        return;
    case AstNodeKind::WhileStatement:
        visit(static_cast<const WhileStatement&>(statement).body());
        return;
    case AstNodeKind::RepeatStatement:
        visit(static_cast<const RepeatStatement&>(statement).body());
        return;
    case AstNodeKind::ForInStatement:
        visit(static_cast<const ForInStatement&>(statement).body());
        return;
    case AstNodeKind::CaseStatement: {
        const auto& node = static_cast<const CaseStatement&>(statement);
        for (const CaseArm& arm : node.arms()) {
            visit(arm.body);
        }
        visit(node.elseBody());
        return;
    }
    case AstNodeKind::TryStatement: {
        const auto& node = static_cast<const TryStatement&>(statement);
        visit(node.body());
        for (const ExceptionHandler& handler : node.handlers()) {
            visit(handler.body);
        }
        visit(node.elseBody());
        visit(node.exceptBody());
        visit(node.ensureBody());
        return;
    }
    case AstNodeKind::WithStatement:
        visit(static_cast<const WithStatement&>(statement).body());
        return;
    default:
        return;
    }
}

bool expressionHasWithTarget(const Expression& expression)
{
    switch (expression.kind()) {
    case AstNodeKind::WithTargetExpression:
        return true;
    case AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const BinaryExpression&>(expression);
        return expressionHasWithTarget(binary.left()) || expressionHasWithTarget(binary.right());
    }
    case AstNodeKind::UnaryExpression:
        return expressionHasWithTarget(static_cast<const UnaryExpression&>(expression).operand());
    case AstNodeKind::CallExpression: {
        const auto& call = static_cast<const CallExpression&>(expression);
        if (expressionHasWithTarget(call.callee())) {
            return true;
        }
        for (const auto& argument : call.arguments()) {
            if (expressionHasWithTarget(*argument)) {
                return true;
            }
        }
        return false;
    }
    case AstNodeKind::ConditionalExpression: {
        const auto& conditional = static_cast<const ConditionalExpression&>(expression);
        return expressionHasWithTarget(conditional.condition()) ||
               expressionHasWithTarget(conditional.thenValue()) || expressionHasWithTarget(conditional.elseValue());
    }
    default:
        return false;
    }
}

// Whether a body holds a form of phase 2b: a chained or compound assignment,
// a `with` (its binding is named here), or a loop with contracts. A
// WithTargetExpression is only ever inside a `with`.
bool hasWrittenForms(const std::vector<StatementPtr>& statements)
{
    for (const auto& statement : statements) {
        const AstNodeKind kind = statement->kind();
        if (kind == AstNodeKind::ChainedAssignmentStatement || kind == AstNodeKind::CompoundAssignmentStatement ||
            kind == AstNodeKind::WithStatement) {
            return true;
        }
        if (const auto* contracts = loopContracts(*statement); contracts != nullptr && !contracts->empty()) {
            return true;
        }
        bool found = false;
        forEachBody(*statement, [&](const std::vector<StatementPtr>& body) { found = found || hasWrittenForms(body); });
        if (found) {
            return true;
        }
    }
    return false;
}

class AstNormalizer {
public:
    AstNormalizer()
    {
        rewriters_.expression = [this](const Expression& node) -> ExpressionPtr {
            if (node.kind() != AstNodeKind::WithTargetExpression) {
                return nullptr;
            }
            const std::size_t level = static_cast<const WithTargetExpression&>(node).level();
            if (level >= withStack_.size()) {
                throw std::logic_error("phase 2b: a '.Member' outside its 'with'");
            }
            return syntheticIdentifier(withStack_[level]);
        };
        rewriters_.statement = [this](const Statement& statement, std::vector<StatementPtr>& out) {
            return normalize(statement, out);
        };
    }

    void body(const std::string& routine, std::vector<StatementPtr>& statements)
    {
        if (!hasWrittenForms(statements)) {
            return;
        }
        routine_ = routine;
        number(statements);
        statements = cloneStatements(statements, rewriters_);
    }

private:
    // The parser's numbering, kept so every name stays the same: a `with`
    // takes its number when its header is read (pre-order), a loop with
    // contracts when its body is done (post-order); both count per module.
    void number(const std::vector<StatementPtr>& statements)
    {
        for (const auto& statement : statements) {
            if (statement->kind() == AstNodeKind::WithStatement) {
                withNames_[statement.get()] = "__with_" + std::to_string(withCounter_++);
            }
            forEachBody(*statement, [&](const std::vector<StatementPtr>& body) { number(body); });
            if (const auto* contracts = loopContracts(*statement); contracts != nullptr && !contracts->empty()) {
                loopIds_[statement.get()] = std::to_string(++contractCounter_);
            }
        }
    }

    ExpressionPtr expression(const Expression& node) { return cloneExpression(node, rewriters_.expression); }

    bool normalize(const Statement& statement, std::vector<StatementPtr>& out)
    {
        switch (statement.kind()) {
        case AstNodeKind::ChainedAssignmentStatement:
            normalizeChain(static_cast<const ChainedAssignmentStatement&>(statement), out);
            return true;
        case AstNodeKind::CompoundAssignmentStatement:
            normalizeCompound(static_cast<const CompoundAssignmentStatement&>(statement), out);
            return true;
        case AstNodeKind::WithStatement:
            normalizeWith(static_cast<const WithStatement&>(statement), out);
            return true;
        default:
            if (const auto* contracts = loopContracts(statement); contracts != nullptr && !contracts->empty()) {
                normalizeLoop(statement, *contracts, out);
                return true;
            }
            return false;
        }
    }

    // CANON-5: `A := B := C := X` stores X in C, then C in B, then B in A;
    // X is evaluated once.
    void normalizeChain(const ChainedAssignmentStatement& chain, std::vector<StatementPtr>& out)
    {
        std::vector<ExpressionPtr> parts;
        for (const auto& target : chain.targets()) {
            parts.push_back(expression(*target));
        }
        const std::size_t last = parts.size();
        std::vector<ExpressionPtr> sources(last);
        sources[last - 1] = expression(chain.value());
        for (std::size_t i = 0; i + 1 < last; ++i) {
            sources[i] = cloneChainTarget(*parts[i + 1]);
        }
        for (std::size_t i = last; i-- > 0;) {
            out.push_back(positioned(
                std::make_unique<ExpressionStatement>(std::make_unique<BinaryExpression>(
                    BinaryOperator::Assign, std::move(parts[i]), std::move(sources[i]))),
                chain.position()));
        }
    }

    // ADR-0012: `L op= R` is `L := L op R`, L evaluated once (it is pure),
    // marked compound.
    void normalizeCompound(const CompoundAssignmentStatement& compound, std::vector<StatementPtr>& out)
    {
        ExpressionPtr target = expression(compound.target());
        ExpressionPtr value = expression(compound.value());
        ExpressionPtr current = clonePureExpression(*target);
        auto combined = std::make_unique<BinaryExpression>(compound.op(), std::move(current), std::move(value));
        out.push_back(positioned(std::make_unique<ExpressionStatement>(
                                     std::make_unique<BinaryExpression>(BinaryOperator::Assign, std::move(target),
                                                                        std::move(combined)),
                                     true),
                                 compound.position()));
    }

    void normalizeWith(const WithStatement& with, std::vector<StatementPtr>& out)
    {
        const std::string& name = withNames_.at(&with);
        ExpressionPtr target = expression(with.target());
        withStack_.push_back(name);
        std::vector<StatementPtr> body = cloneStatements(with.body(), rewriters_);
        withStack_.pop_back();
        out.push_back(positioned(std::make_unique<WithStatement>(std::move(target), std::move(body), name),
                                 with.position()));
    }

    // The statements a loop's contracts add: before it, at the top of its
    // body, after it.
    struct LoopChecks {
        std::vector<StatementPtr> preceding;
        std::vector<StatementPtr> top;
        std::vector<StatementPtr> pending;
    };

    // CANON-24: a loop's invariant is checked at the start of every iteration
    // and, for while and repeat, once more after the loop; its variant must
    // be >= 0 at the start of every iteration and smaller than at the start
    // of the previous one. The variant's previous value lives before the
    // loop. The checks are built out of line, so the frame that recurses
    // into the body stays small (design.md, stack budget).
    void normalizeLoop(const Statement& loop, const std::vector<ContractClause>& written, std::vector<StatementPtr>& out)
    {
        const std::unique_ptr<LoopChecks> checks = buildLoopChecks(loop, written);
        StatementPtr normalized = rebuildLoop(loop, checks->top);
        for (StatementPtr& statement : checks->preceding) {
            out.push_back(positioned(std::move(statement), loop.position()));
        }
        out.push_back(positioned(std::move(normalized), loop.position()));
        for (StatementPtr& statement : checks->pending) {
            out.push_back(positioned(std::move(statement), loop.position()));
        }
    }

    std::unique_ptr<LoopChecks> buildLoopChecks(const Statement& loop, const std::vector<ContractClause>& written)
    {
        auto checks = std::make_unique<LoopChecks>();
        const bool checkAfterLoop = loop.kind() != AstNodeKind::ForInStatement;
        const std::string& id = loopIds_.at(&loop);
        std::vector<ContractClause> contracts = cloneContractClauses(written, rewriters_.expression);
        for (ContractClause& clause : contracts) {
            if (clause.kind == ContractClause::Kind::LoopInvariant) {
                if (checkAfterLoop) {
                    checks->pending.push_back(statementOf(
                        contractCheck(4, cloneExpression(*clause.condition),
                                      clauseMessage("loop invariant violated", routine_, clause, " (at the end)"))));
                }
                checks->top.push_back(statementOf(contractCheck(
                    4, std::move(clause.condition), clauseMessage("loop invariant violated", routine_, clause))));
                continue;
            }
            addVariantChecks(*checks, clause, id);
        }
        return checks;
    }

    void addVariantChecks(LoopChecks& checks, ContractClause& clause, const std::string& id)
    {
        const std::string current = "__variant_" + id;
        const std::string previousName = "__variant_previous_" + id;
        checks.preceding.push_back(statementOf(assignment(
            previousName, std::make_unique<UnaryExpression>(
                              UnaryOperator::Minus, std::make_unique<LiteralExpression>(LiteralKind::Integer, "1")))));
        std::vector<ExpressionPtr> conversion;
        conversion.push_back(std::move(clause.condition));
        std::vector<ExpressionPtr> pure;
        pure.push_back(std::make_unique<CallExpression>(syntheticIdentifier("Integer"), std::move(conversion)));
        checks.top.push_back(statementOf(
            assignment(current, std::make_unique<CallExpression>(syntheticIdentifier("__pure"), std::move(pure)))));
        checks.top.push_back(statementOf(contractCheck(
            5,
            std::make_unique<BinaryExpression>(BinaryOperator::GreaterEqual, syntheticIdentifier(current),
                                               std::make_unique<LiteralExpression>(LiteralKind::Integer, "0")),
            clauseMessage("loop variant violated", routine_, clause, " is negative"))));
        checks.top.push_back(statementOf(contractCheck(
            5,
            std::make_unique<BinaryExpression>(
                BinaryOperator::Or,
                std::make_unique<BinaryExpression>(BinaryOperator::Less, syntheticIdentifier(previousName),
                                                   std::make_unique<LiteralExpression>(LiteralKind::Integer, "0")),
                std::make_unique<BinaryExpression>(BinaryOperator::Less, syntheticIdentifier(current),
                                                   syntheticIdentifier(previousName))),
            clauseMessage("loop variant violated", routine_, clause, " did not decrease"))));
        checks.top.push_back(statementOf(assignment(previousName, syntheticIdentifier(current))));
    }

    // The loop with its checks at the top of its body and its own body after
    // them (recursing into that body).
    StatementPtr rebuildLoop(const Statement& loop, std::vector<StatementPtr>& top)
    {
        switch (loop.kind()) {
        case AstNodeKind::WhileStatement: {
            const auto& node = static_cast<const WhileStatement&>(loop);
            ExpressionPtr condition = expression(node.condition());
            appendBody(top, node.body());
            return std::make_unique<WhileStatement>(std::move(condition), std::move(top));
        }
        case AstNodeKind::RepeatStatement:
            appendBody(top, static_cast<const RepeatStatement&>(loop).body());
            return std::make_unique<RepeatStatement>(std::move(top));
        default: {
            const auto& node = static_cast<const ForInStatement&>(loop);
            ExpressionPtr iterable = expression(node.iterable());
            ExpressionPtr step = node.step() != nullptr ? expression(*node.step()) : nullptr;
            appendBody(top, node.body());
            return std::make_unique<ForInStatement>(node.iterator(), std::move(iterable), std::move(step),
                                                    std::move(top));
        }
        }
    }

    void appendBody(std::vector<StatementPtr>& top, const std::vector<StatementPtr>& body)
    {
        for (StatementPtr& statement : cloneStatements(body, rewriters_)) {
            top.push_back(std::move(statement));
        }
    }

    CloneRewriters rewriters_;
    std::string routine_;
    std::vector<std::string> withStack_;
    std::unordered_map<const Statement*, std::string> withNames_;
    std::unordered_map<const Statement*, std::string> loopIds_;
    std::size_t withCounter_ = 0;
    std::size_t contractCounter_ = 0;
};

void requireNoForms(const std::vector<StatementPtr>& statements)
{
    for (const auto& statement : statements) {
        const AstNodeKind kind = statement->kind();
        const auto* contracts = loopContracts(*statement);
        if (kind == AstNodeKind::ChainedAssignmentStatement || kind == AstNodeKind::CompoundAssignmentStatement ||
            (contracts != nullptr && !contracts->empty()) ||
            (kind == AstNodeKind::WithStatement &&
             static_cast<const WithStatement&>(*statement).bindingName().empty())) {
            throw std::logic_error("a form of phase 2b reached a later phase");
        }
        if (kind == AstNodeKind::ExpressionStatement &&
            expressionHasWithTarget(static_cast<const ExpressionStatement&>(*statement).expression())) {
            throw std::logic_error("a '.Member' of phase 2b reached a later phase");
        }
        forEachBody(*statement, [](const std::vector<StatementPtr>& body) { requireNoForms(body); });
    }
}

} // namespace

void normalizeModule(ModuleNode& module)
{
    AstNormalizer normalizer;
    for (auto& item : module.items()) {
        if (item->kind() == AstNodeKind::FunctionDeclaration) {
            auto& function = static_cast<FunctionDeclaration&>(*item);
            normalizer.body(function.name(), function.body());
        }
    }
    normalizer.body(std::string{}, module.statements());
}

void requireNormalized(const ModuleNode& module)
{
    for (const auto& item : module.items()) {
        if (item->kind() == AstNodeKind::FunctionDeclaration) {
            requireNoForms(static_cast<const FunctionDeclaration&>(*item).body());
        }
    }
    requireNoForms(module.statements());
}

} // namespace inox::compiler::normalization
