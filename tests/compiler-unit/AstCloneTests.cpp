// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Unit tests of AST copies (OpenSpec change type-representation): a copy has
// the same shape as its original, shares no node with it, and passes every
// type name and expression through the rewriters.

#include "UnitTest.h"

#include "compiler/ast/AstClone.h"
#include "compiler/parser/Parser.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace inox::compiler::unit {

namespace {

using namespace ast;

// Every statement kind the parser builds, in one program.
constexpr const char* kProgram = R"(Twice(N Integer) Integer :
    Return N * 2
;

Main :
    A, B Integer := 1
    X Integer := 2
    if X > 0
        X := X + 1
    elif X = 0
        X += 2
    else
        PutLn("neg")
    ;
    unless X > 9
        PutLn(X)
    ;
    while X < 3
        X := X + 1
        continue
    ;
    repeat
        X := X + 1
        until X >= 5
    ;
    for I in 1..3 step 1
        leave
    ;
    case X
        1..2 | 4 PutLn("a")
        else PutLn("b")
    ;
    try
        Raise OverflowError
    except
        On OverflowError
            Retry(2)
        ;
        On E IOError
            PutLn("io")
        ;
        Else
            PutLn("other")
        ;
    ensure
        PutLn("cleanup")
    ;
    with P
        .FX := IIf(X > 1, 1, 2)
    ;
    Exit
;
)";

// The test's own printer of a tree: every node with what it holds. It is
// written apart from the copy, so it checks it.
std::string shape(const Expression& expression);
std::string shape(const std::vector<StatementPtr>& statements);

std::string shape(const Expression& expression)
{
    switch (expression.kind()) {
    case AstNodeKind::LiteralExpression:
        return "lit:" + static_cast<const LiteralExpression&>(expression).value();
    case AstNodeKind::IdentifierExpression:
        return "id:" + static_cast<const IdentifierExpression&>(expression).name();
    case AstNodeKind::BinaryExpression: {
        const auto& node = static_cast<const BinaryExpression&>(expression);
        return "(bin" + std::to_string(static_cast<int>(node.op())) + " " + shape(node.left()) + " " +
               shape(node.right()) + ")";
    }
    case AstNodeKind::UnaryExpression: {
        const auto& node = static_cast<const UnaryExpression&>(expression);
        return "(un" + std::to_string(static_cast<int>(node.op())) + " " + shape(node.operand()) + ")";
    }
    case AstNodeKind::CallExpression: {
        const auto& node = static_cast<const CallExpression&>(expression);
        std::string text = "(call " + shape(node.callee());
        for (const auto& argument : node.arguments()) {
            text += " " + shape(*argument);
        }
        return text + ")";
    }
    case AstNodeKind::ConditionalExpression: {
        const auto& node = static_cast<const ConditionalExpression&>(expression);
        return "(cond " + shape(node.condition()) + " " + shape(node.thenValue()) + " " + shape(node.elseValue()) +
               ")";
    }
    default:
        return "?";
    }
}

std::string shape(const Statement& statement)
{
    switch (statement.kind()) {
    case AstNodeKind::BlockStatement:
        return "(block " + shape(static_cast<const BlockStatement&>(statement).statements()) + ")";
    case AstNodeKind::ExpressionStatement: {
        const auto& node = static_cast<const ExpressionStatement&>(statement);
        return std::string(node.isCompoundAssignment() ? "(compound " : "(expr ") + shape(node.expression()) + ")";
    }
    case AstNodeKind::VarStatement: {
        const auto& node = static_cast<const VarStatement&>(statement);
        return "(var " + node.name() + " type:" + node.typeName() +
               (node.initializer() != nullptr ? " " + shape(*node.initializer()) : std::string{}) + ")";
    }
    case AstNodeKind::VarBlockStatement:
        return "(vars " + shape(static_cast<const VarBlockStatement&>(statement).declarations()) + ")";
    case AstNodeKind::IfStatement: {
        const auto& node = static_cast<const IfStatement&>(statement);
        std::string text = "(if " + shape(node.condition()) + " " + shape(node.thenBody());
        for (const ElseIfClause& clause : node.elseIfClauses()) {
            text += " elif " + shape(*clause.condition) + " " + shape(clause.body);
        }
        return text + " else " + shape(node.elseBody()) + ")";
    }
    case AstNodeKind::UnlessStatement: {
        const auto& node = static_cast<const UnlessStatement&>(statement);
        return "(unless " + shape(node.condition()) + " " + shape(node.body()) + ")";
    }
    case AstNodeKind::WhileStatement: {
        const auto& node = static_cast<const WhileStatement&>(statement);
        return "(while " + shape(node.condition()) + " " + shape(node.body()) + ")";
    }
    case AstNodeKind::RepeatStatement:
        return "(repeat " + shape(static_cast<const RepeatStatement&>(statement).body()) + ")";
    case AstNodeKind::UntilStatement:
        return "(until " + shape(static_cast<const UntilStatement&>(statement).condition()) + ")";
    case AstNodeKind::ForInStatement: {
        const auto& node = static_cast<const ForInStatement&>(statement);
        return "(for " + node.iterator() + " " + shape(node.iterable()) +
               (node.step() != nullptr ? " step " + shape(*node.step()) : std::string{}) + " " + shape(node.body()) +
               ")";
    }
    case AstNodeKind::CaseStatement: {
        const auto& node = static_cast<const CaseStatement&>(statement);
        std::string text = "(case " + shape(node.expression());
        for (const CaseArm& arm : node.arms()) {
            text += " arm";
            for (const auto& choice : arm.choices) {
                text += " " + shape(*choice);
            }
            text += " " + shape(arm.body);
        }
        return text + (node.hasElse() ? " else " + shape(node.elseBody()) : std::string{}) + ")";
    }
    case AstNodeKind::TryStatement: {
        const auto& node = static_cast<const TryStatement&>(statement);
        std::string text = "(try " + shape(node.body()) + " except:" + std::to_string(node.hasExcept()) +
                           " plain:" + std::to_string(node.hasPlainExcept()) + " " + shape(node.exceptBody());
        for (const ExceptionHandler& handler : node.handlers()) {
            text += " on " + handler.bindingName + ":" + handler.typeName + " " + shape(handler.body);
        }
        return text + " else " + shape(node.elseBody()) + " ensure:" + std::to_string(node.hasEnsure()) + " " +
               shape(node.ensureBody()) + ")";
    }
    case AstNodeKind::RaiseStatement: {
        const Expression* raised = static_cast<const RaiseStatement&>(statement).expression();
        return "(raise" + (raised != nullptr ? " " + shape(*raised) : std::string{}) + ")";
    }
    case AstNodeKind::RetryStatement:
        return "(retry " + shape(static_cast<const RetryStatement&>(statement).count()) + ")";
    case AstNodeKind::ReturnStatement:
        return "(return " + shape(static_cast<const ReturnStatement&>(statement).expression()) + ")";
    case AstNodeKind::ExitStatement:
        return "(exit)";
    case AstNodeKind::LeaveStatement:
        return "(leave)";
    case AstNodeKind::ContinueStatement:
        return "(continue)";
    case AstNodeKind::WithStatement: {
        const auto& node = static_cast<const WithStatement&>(statement);
        return "(with " + node.bindingName() + " " + shape(node.target()) + " " + shape(node.body()) + ")";
    }
    default:
        return "?";
    }
}

std::string shape(const std::vector<StatementPtr>& statements)
{
    std::string text = "[";
    for (const auto& statement : statements) {
        text += shape(*statement) + " ";
    }
    return text + "]";
}

// The address of every statement and expression a printer would visit, to
// show that a copy shares nothing with its original.
void collectNodes(const std::vector<StatementPtr>& statements, std::unordered_set<const AstNode*>& nodes);

void collectNodes(const Expression& expression, std::unordered_set<const AstNode*>& nodes)
{
    nodes.insert(&expression);
    if (expression.kind() == AstNodeKind::BinaryExpression) {
        collectNodes(static_cast<const BinaryExpression&>(expression).left(), nodes);
        collectNodes(static_cast<const BinaryExpression&>(expression).right(), nodes);
    } else if (expression.kind() == AstNodeKind::UnaryExpression) {
        collectNodes(static_cast<const UnaryExpression&>(expression).operand(), nodes);
    } else if (expression.kind() == AstNodeKind::CallExpression) {
        const auto& call = static_cast<const CallExpression&>(expression);
        collectNodes(call.callee(), nodes);
        for (const auto& argument : call.arguments()) {
            collectNodes(*argument, nodes);
        }
    }
}

void collectNodes(const std::vector<StatementPtr>& statements, std::unordered_set<const AstNode*>& nodes)
{
    for (const auto& statement : statements) {
        nodes.insert(statement.get());
        switch (statement->kind()) {
        case AstNodeKind::ExpressionStatement:
            collectNodes(static_cast<const ExpressionStatement&>(*statement).expression(), nodes);
            break;
        case AstNodeKind::VarBlockStatement:
            collectNodes(static_cast<const VarBlockStatement&>(*statement).declarations(), nodes);
            break;
        case AstNodeKind::IfStatement: {
            const auto& node = static_cast<const IfStatement&>(*statement);
            collectNodes(node.condition(), nodes);
            collectNodes(node.thenBody(), nodes);
            collectNodes(node.elseBody(), nodes);
            break;
        }
        case AstNodeKind::TryStatement: {
            const auto& node = static_cast<const TryStatement&>(*statement);
            collectNodes(node.body(), nodes);
            for (const ExceptionHandler& handler : node.handlers()) {
                collectNodes(handler.body, nodes);
            }
            collectNodes(node.ensureBody(), nodes);
            break;
        }
        default:
            break;
        }
    }
}

std::unique_ptr<ModuleNode> parseProgram()
{
    parser::Parser parser{std::string_view(kProgram)};
    return parser.parseModule("CloneTest");
}

const FunctionDeclaration& routine(const ModuleNode& module, std::string_view name)
{
    for (const auto& item : module.items()) {
        if (item->kind() == AstNodeKind::FunctionDeclaration &&
            static_cast<const FunctionDeclaration&>(*item).name() == name) {
            return static_cast<const FunctionDeclaration&>(*item);
        }
    }
    throw std::logic_error("no routine " + std::string(name));
}

void testCopyKeepsShape()
{
    const auto module = parseProgram();
    for (const char* name : {"Main", "Twice"}) {
        const auto& original = routine(*module, name).body();
        const std::vector<StatementPtr> copy = cloneStatements(original);
        checkEqual(shape(copy), shape(original), std::string("the copy of ") + name + " has its shape");
    }
    const std::string text = shape(routine(*module, "Main").body());
    for (const char* kind : {"(vars ", "(if ", " elif ", "(compound ", "(unless ", "(while ", "(continue)",
                             "(repeat ", "(until ", "(for ", " step ", "(leave)", "(case ", " arm", "(try ",
                             "(raise ", "(retry ", " on E:IOError ", "ensure:1", "(with ", "(exit)"}) {
        check(text.find(kind) != std::string::npos, std::string("the fixture has ") + kind);
    }
}

void testCopySharesNothing()
{
    const auto module = parseProgram();
    const auto& original = routine(*module, "Main").body();
    const std::vector<StatementPtr> copy = cloneStatements(original);
    std::unordered_set<const AstNode*> originalNodes;
    std::unordered_set<const AstNode*> copyNodes;
    collectNodes(original, originalNodes);
    collectNodes(copy, copyNodes);
    check(originalNodes.size() == copyNodes.size() && originalNodes.size() > 20, "both trees are walked");
    bool shared = false;
    for (const AstNode* node : copyNodes) {
        shared = shared || originalNodes.contains(node);
    }
    check(!shared, "a copy shares no node with its original");
}

void testRewriters()
{
    const auto module = parseProgram();
    const auto& original = routine(*module, "Main").body();
    CloneRewriters rewrite;
    rewrite.typeName = [](const std::string& name) {
        return name == "Integer" ? std::string("Int64") : name == "IOError" ? std::string("RangeError") : name;
    };
    rewrite.expression = [](const Expression& node) -> ExpressionPtr {
        if (node.kind() == AstNodeKind::IdentifierExpression &&
            static_cast<const IdentifierExpression&>(node).name() == "X") {
            return std::make_unique<IdentifierExpression>("Y");
        }
        return nullptr;
    };
    const std::string text = shape(cloneStatements(original, rewrite));
    check(text.find("type:Integer") == std::string::npos && text.find("(var A type:Int64") != std::string::npos,
          "every declared type name passes through the type rewriter");
    check(text.find("id:X") == std::string::npos && text.find("(bin") != std::string::npos &&
              text.find("id:Y") != std::string::npos,
          "every expression, in every nested body, passes through the expression rewriter");
    check(text.find("on E:RangeError") != std::string::npos, "an exception handler's type is a type name too");
    check(text.find("on :OverflowError") != std::string::npos, "a type name the rewriter keeps stays");
    checkEqual(shape(original).find("id:X") != std::string::npos ? "kept" : "changed", "kept",
               "the original is not changed");
}

void testNodesTheParserDoesNotBuild()
{
    std::vector<StatementPtr> inner;
    inner.push_back(std::make_unique<VarStatement>(true, "Z", std::make_unique<LiteralExpression>(LiteralKind::Integer, "1"),
                                                   "T"));
    std::vector<StatementPtr> body;
    body.push_back(std::make_unique<BlockStatement>(std::move(inner)));
    CloneRewriters rewrite;
    rewrite.typeName = [](const std::string& name) { return name == "T" ? std::string("String") : name; };
    checkEqual(shape(cloneStatements(body, rewrite)), "[(block [(var Z type:String lit:1) ]) ]", "a block statement");
}

} // namespace

int runAstCloneTests()
{
    const int before = failureCount();
    testCopyKeepsShape();
    testCopySharesNothing();
    testRewriters();
    testNodesTheParserDoesNotBuild();
    return failureCount() - before;
}

} // namespace inox::compiler::unit
