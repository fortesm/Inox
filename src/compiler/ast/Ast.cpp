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

std::vector<StatementPtr>& ModuleNode::statements()
{
    return statements_;
}

const std::vector<StatementPtr>& ModuleNode::statements() const
{
    return statements_;
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

} // namespace inox::compiler::ast
