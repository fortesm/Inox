#pragma once

#include <memory>
#include <string>
#include <vector>

namespace inox::compiler::ast {

enum class AstNodeKind {
    Module,
    LiteralExpression,
    IdentifierExpression,
    BinaryExpression,
    UnaryExpression,
    CallExpression
};

enum class LiteralKind {
    Integer,
    Float,
    String,
    Char,
    Boolean
};

enum class BinaryOperator {
    Assign,
    Add,
    Subtract,
    Multiply,
    Divide,
    IntegerDivide,
    Modulo,
    Power,
    Range,
    In,
    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    And,
    Xor,
    Or
};

enum class UnaryOperator {
    Plus,
    Minus,
    Not
};

class AstNode {
public:
    explicit AstNode(AstNodeKind kind);
    virtual ~AstNode() = default;

    AstNodeKind kind() const;

private:
    AstNodeKind kind_;
};

class Expression : public AstNode {
public:
    explicit Expression(AstNodeKind kind);
    ~Expression() override = default;
};

class Statement : public AstNode {
public:
    explicit Statement(AstNodeKind kind);
    ~Statement() override = default;
};

using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;

class ModuleNode final : public AstNode {
public:
    explicit ModuleNode(std::string name);

    const std::string& name() const;
    std::vector<StatementPtr>& statements();
    const std::vector<StatementPtr>& statements() const;

private:
    std::string name_;
    std::vector<StatementPtr> statements_;
};

class LiteralExpression final : public Expression {
public:
    LiteralExpression(LiteralKind literalKind, std::string value);

    LiteralKind literalKind() const;
    const std::string& value() const;

private:
    LiteralKind literalKind_;
    std::string value_;
};

class IdentifierExpression final : public Expression {
public:
    explicit IdentifierExpression(std::string name);

    const std::string& name() const;

private:
    std::string name_;
};

class BinaryExpression final : public Expression {
public:
    BinaryExpression(BinaryOperator op, ExpressionPtr left, ExpressionPtr right);

    BinaryOperator op() const;
    const Expression& left() const;
    const Expression& right() const;
    ExpressionPtr takeLeft();
    ExpressionPtr takeRight();

private:
    BinaryOperator op_;
    ExpressionPtr left_;
    ExpressionPtr right_;
};

class UnaryExpression final : public Expression {
public:
    UnaryExpression(UnaryOperator op, ExpressionPtr operand);

    UnaryOperator op() const;
    const Expression& operand() const;
    ExpressionPtr takeOperand();

private:
    UnaryOperator op_;
    ExpressionPtr operand_;
};

class CallExpression final : public Expression {
public:
    CallExpression(ExpressionPtr callee, std::vector<ExpressionPtr> arguments);

    const Expression& callee() const;
    const std::vector<ExpressionPtr>& arguments() const;
    ExpressionPtr takeCallee();
    std::vector<ExpressionPtr> takeArguments();

private:
    ExpressionPtr callee_;
    std::vector<ExpressionPtr> arguments_;
};

} // namespace inox::compiler::ast
