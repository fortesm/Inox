// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Ast.h"

#include <utility>

namespace inox::compiler::ast {

AstNode::AstNode(AstNodeKind kind) : kind_(kind) {}

AstNodeKind AstNode::kind() const
{
    return kind_;
}

Expression::Expression(AstNodeKind kind) : AstNode(kind) {}

Statement::Statement(AstNodeKind kind) : AstNode(kind) {}

ModuleNode::ModuleNode(std::string name)
    : AstNode(AstNodeKind::Module), name_(std::move(name))
{
}

const std::string& ModuleNode::name() const
{
    return name_;
}

std::vector<AstNodePtr>& ModuleNode::items()
{
    return items_;
}

const std::vector<AstNodePtr>& ModuleNode::items() const
{
    return items_;
}

std::vector<StatementPtr>& ModuleNode::statements()
{
    return statements_;
}

const std::vector<StatementPtr>& ModuleNode::statements() const
{
    return statements_;
}

UseDeclaration::UseDeclaration(std::vector<std::string> path)
    : AstNode(AstNodeKind::UseDeclaration), path_(std::move(path))
{
}

const std::vector<std::string>& UseDeclaration::path() const
{
    return path_;
}

SectionDeclaration::SectionDeclaration(SectionKind sectionKind, std::vector<std::string> tokens)
    : AstNode(AstNodeKind::SectionDeclaration),
      sectionKind_(sectionKind),
      tokens_(std::move(tokens))
{
}

SectionKind SectionDeclaration::sectionKind() const
{
    return sectionKind_;
}

const std::vector<std::string>& SectionDeclaration::tokens() const
{
    return tokens_;
}

RawDeclaration::RawDeclaration(std::string head, std::vector<std::string> tokens)
    : AstNode(AstNodeKind::RawDeclaration),
      head_(std::move(head)),
      tokens_(std::move(tokens))
{
}

const std::string& RawDeclaration::head() const
{
    return head_;
}

const std::vector<std::string>& RawDeclaration::tokens() const
{
    return tokens_;
}

FunctionDeclaration::FunctionDeclaration(std::string name,
                                         std::vector<std::string> signatureTokens,
                                         std::vector<StatementPtr> body)
    : AstNode(AstNodeKind::FunctionDeclaration),
      name_(std::move(name)),
      signatureTokens_(std::move(signatureTokens)),
      body_(std::move(body))
{
}

const std::string& FunctionDeclaration::name() const
{
    return name_;
}

const std::vector<std::string>& FunctionDeclaration::signatureTokens() const
{
    return signatureTokens_;
}

const std::vector<StatementPtr>& FunctionDeclaration::body() const
{
    return body_;
}

LiteralExpression::LiteralExpression(LiteralKind literalKind, std::string value)
    : Expression(AstNodeKind::LiteralExpression),
      literalKind_(literalKind),
      value_(std::move(value))
{
}

LiteralKind LiteralExpression::literalKind() const
{
    return literalKind_;
}

const std::string& LiteralExpression::value() const
{
    return value_;
}

IdentifierExpression::IdentifierExpression(std::string name)
    : Expression(AstNodeKind::IdentifierExpression), name_(std::move(name))
{
}

const std::string& IdentifierExpression::name() const
{
    return name_;
}

BinaryExpression::BinaryExpression(BinaryOperator op, ExpressionPtr left, ExpressionPtr right)
    : Expression(AstNodeKind::BinaryExpression),
      op_(op),
      left_(std::move(left)),
      right_(std::move(right))
{
}

BinaryOperator BinaryExpression::op() const
{
    return op_;
}

const Expression& BinaryExpression::left() const
{
    return *left_;
}

const Expression& BinaryExpression::right() const
{
    return *right_;
}

ExpressionPtr BinaryExpression::takeLeft()
{
    return std::move(left_);
}

ExpressionPtr BinaryExpression::takeRight()
{
    return std::move(right_);
}

UnaryExpression::UnaryExpression(UnaryOperator op, ExpressionPtr operand)
    : Expression(AstNodeKind::UnaryExpression),
      op_(op),
      operand_(std::move(operand))
{
}

UnaryOperator UnaryExpression::op() const
{
    return op_;
}

const Expression& UnaryExpression::operand() const
{
    return *operand_;
}

ExpressionPtr UnaryExpression::takeOperand()
{
    return std::move(operand_);
}

CallExpression::CallExpression(ExpressionPtr callee, std::vector<ExpressionPtr> arguments)
    : Expression(AstNodeKind::CallExpression),
      callee_(std::move(callee)),
      arguments_(std::move(arguments))
{
}

const Expression& CallExpression::callee() const
{
    return *callee_;
}

const std::vector<ExpressionPtr>& CallExpression::arguments() const
{
    return arguments_;
}

ExpressionPtr CallExpression::takeCallee()
{
    return std::move(callee_);
}

std::vector<ExpressionPtr> CallExpression::takeArguments()
{
    return std::move(arguments_);
}

BlockStatement::BlockStatement(std::vector<StatementPtr> statements)
    : Statement(AstNodeKind::BlockStatement), statements_(std::move(statements))
{
}

const std::vector<StatementPtr>& BlockStatement::statements() const
{
    return statements_;
}

std::vector<StatementPtr> BlockStatement::takeStatements()
{
    return std::move(statements_);
}

ExpressionStatement::ExpressionStatement(ExpressionPtr expression)
    : Statement(AstNodeKind::ExpressionStatement), expression_(std::move(expression))
{
}

const Expression& ExpressionStatement::expression() const
{
    return *expression_;
}

ExpressionPtr ExpressionStatement::takeExpression()
{
    return std::move(expression_);
}

VarStatement::VarStatement(bool isMutable,
                           std::string name,
                           ExpressionPtr initializer,
                           std::string typeName)
    : Statement(AstNodeKind::VarStatement),
      isMutable_(isMutable),
      name_(std::move(name)),
      typeName_(std::move(typeName)),
      initializer_(std::move(initializer))
{
}

bool VarStatement::isMutable() const
{
    return isMutable_;
}

const std::string& VarStatement::name() const
{
    return name_;
}

const std::string& VarStatement::typeName() const
{
    return typeName_;
}

const Expression* VarStatement::initializer() const
{
    return initializer_.get();
}

ExpressionPtr VarStatement::takeInitializer()
{
    return std::move(initializer_);
}

VarBlockStatement::VarBlockStatement(std::vector<StatementPtr> declarations)
    : Statement(AstNodeKind::VarBlockStatement), declarations_(std::move(declarations))
{
}

const std::vector<StatementPtr>& VarBlockStatement::declarations() const
{
    return declarations_;
}

std::vector<StatementPtr> VarBlockStatement::takeDeclarations()
{
    return std::move(declarations_);
}

IfStatement::IfStatement(ExpressionPtr condition,
                         std::vector<StatementPtr> thenBody,
                         std::vector<ElseIfClause> elseIfClauses,
                         std::vector<StatementPtr> elseBody)
    : Statement(AstNodeKind::IfStatement),
      condition_(std::move(condition)),
      thenBody_(std::move(thenBody)),
      elseIfClauses_(std::move(elseIfClauses)),
      elseBody_(std::move(elseBody))
{
}

const Expression& IfStatement::condition() const
{
    return *condition_;
}

const std::vector<StatementPtr>& IfStatement::thenBody() const
{
    return thenBody_;
}

const std::vector<ElseIfClause>& IfStatement::elseIfClauses() const
{
    return elseIfClauses_;
}

const std::vector<StatementPtr>& IfStatement::elseBody() const
{
    return elseBody_;
}

UnlessStatement::UnlessStatement(ExpressionPtr condition, std::vector<StatementPtr> body)
    : Statement(AstNodeKind::UnlessStatement),
      condition_(std::move(condition)),
      body_(std::move(body))
{
}

const Expression& UnlessStatement::condition() const
{
    return *condition_;
}

const std::vector<StatementPtr>& UnlessStatement::body() const
{
    return body_;
}

WhileStatement::WhileStatement(ExpressionPtr condition, std::vector<StatementPtr> body)
    : Statement(AstNodeKind::WhileStatement),
      condition_(std::move(condition)),
      body_(std::move(body))
{
}

const Expression& WhileStatement::condition() const
{
    return *condition_;
}

const std::vector<StatementPtr>& WhileStatement::body() const
{
    return body_;
}

RepeatStatement::RepeatStatement(std::vector<StatementPtr> body)
    : Statement(AstNodeKind::RepeatStatement),
      body_(std::move(body))
{
}

const std::vector<StatementPtr>& RepeatStatement::body() const
{
    return body_;
}

UntilStatement::UntilStatement(ExpressionPtr condition)
    : Statement(AstNodeKind::UntilStatement),
      condition_(std::move(condition))
{
}

const Expression& UntilStatement::condition() const
{
    return *condition_;
}

ForInStatement::ForInStatement(std::string iterator,
                               ExpressionPtr iterable,
                               ExpressionPtr step,
                               std::vector<StatementPtr> body)
    : Statement(AstNodeKind::ForInStatement),
      iterator_(std::move(iterator)),
      iterable_(std::move(iterable)),
      step_(std::move(step)),
      body_(std::move(body))
{
}

const std::string& ForInStatement::iterator() const
{
    return iterator_;
}

const Expression& ForInStatement::iterable() const
{
    return *iterable_;
}

const Expression* ForInStatement::step() const
{
    return step_.get();
}

const std::vector<StatementPtr>& ForInStatement::body() const
{
    return body_;
}

CaseStatement::CaseStatement(ExpressionPtr expression,
                             std::vector<CaseArm> arms,
                             std::vector<StatementPtr> otherwiseBody)
    : Statement(AstNodeKind::CaseStatement),
      expression_(std::move(expression)),
      arms_(std::move(arms)),
      otherwiseBody_(std::move(otherwiseBody))
{
}

const Expression& CaseStatement::expression() const
{
    return *expression_;
}

const std::vector<CaseArm>& CaseStatement::arms() const
{
    return arms_;
}

const std::vector<StatementPtr>& CaseStatement::otherwiseBody() const
{
    return otherwiseBody_;
}

TryStatement::TryStatement(std::vector<StatementPtr> body,
                           std::vector<StatementPtr> exceptBody,
                           std::vector<StatementPtr> finallyBody)
    : Statement(AstNodeKind::TryStatement),
      body_(std::move(body)),
      exceptBody_(std::move(exceptBody)),
      finallyBody_(std::move(finallyBody))
{
}

const std::vector<StatementPtr>& TryStatement::body() const
{
    return body_;
}

const std::vector<StatementPtr>& TryStatement::exceptBody() const
{
    return exceptBody_;
}

const std::vector<StatementPtr>& TryStatement::finallyBody() const
{
    return finallyBody_;
}

RaiseStatement::RaiseStatement(ExpressionPtr expression)
    : Statement(AstNodeKind::RaiseStatement), expression_(std::move(expression))
{
}

const Expression* RaiseStatement::expression() const
{
    return expression_.get();
}

ExpressionPtr RaiseStatement::takeExpression()
{
    return std::move(expression_);
}

ReturnStatement::ReturnStatement(ExpressionPtr expression)
    : Statement(AstNodeKind::ReturnStatement), expression_(std::move(expression))
{
}

const Expression& ReturnStatement::expression() const
{
    return *expression_;
}

ExpressionPtr ReturnStatement::takeExpression()
{
    return std::move(expression_);
}

ExitStatement::ExitStatement() : Statement(AstNodeKind::ExitStatement) {}

BreakStatement::BreakStatement() : Statement(AstNodeKind::BreakStatement) {}

ContinueStatement::ContinueStatement() : Statement(AstNodeKind::ContinueStatement) {}

} // namespace inox::compiler::ast
