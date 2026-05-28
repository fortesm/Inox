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
    CallExpression,
    BlockStatement,
    ExpressionStatement,
    VarStatement,
    VarBlockStatement,
    IfStatement,
    UnlessStatement,
    WhileStatement,
    RepeatStatement,
    ForInStatement,
    CaseStatement,
    TryStatement,
    RaiseStatement
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

struct ElseIfClause {
    ExpressionPtr condition;
    std::vector<StatementPtr> body;
};

struct CaseArm {
    std::vector<ExpressionPtr> choices;
    std::vector<StatementPtr> body;
};

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

class BlockStatement final : public Statement {
public:
    explicit BlockStatement(std::vector<StatementPtr> statements);

    const std::vector<StatementPtr>& statements() const;
    std::vector<StatementPtr> takeStatements();

private:
    std::vector<StatementPtr> statements_;
};

class ExpressionStatement final : public Statement {
public:
    explicit ExpressionStatement(ExpressionPtr expression);

    const Expression& expression() const;
    ExpressionPtr takeExpression();

private:
    ExpressionPtr expression_;
};

class VarStatement final : public Statement {
public:
    VarStatement(bool isMutable, std::string name, ExpressionPtr initializer);

    bool isMutable() const;
    const std::string& name() const;
    const Expression* initializer() const;
    ExpressionPtr takeInitializer();

private:
    bool isMutable_ = false;
    std::string name_;
    ExpressionPtr initializer_;
};

class VarBlockStatement final : public Statement {
public:
    explicit VarBlockStatement(std::vector<StatementPtr> declarations);

    const std::vector<StatementPtr>& declarations() const;
    std::vector<StatementPtr> takeDeclarations();

private:
    std::vector<StatementPtr> declarations_;
};

class IfStatement final : public Statement {
public:
    IfStatement(ExpressionPtr condition,
                std::vector<StatementPtr> thenBody,
                std::vector<ElseIfClause> elseIfClauses,
                std::vector<StatementPtr> elseBody);

    const Expression& condition() const;
    const std::vector<StatementPtr>& thenBody() const;
    const std::vector<ElseIfClause>& elseIfClauses() const;
    const std::vector<StatementPtr>& elseBody() const;

private:
    ExpressionPtr condition_;
    std::vector<StatementPtr> thenBody_;
    std::vector<ElseIfClause> elseIfClauses_;
    std::vector<StatementPtr> elseBody_;
};

class UnlessStatement final : public Statement {
public:
    UnlessStatement(ExpressionPtr condition, std::vector<StatementPtr> body);

    const Expression& condition() const;
    const std::vector<StatementPtr>& body() const;

private:
    ExpressionPtr condition_;
    std::vector<StatementPtr> body_;
};

class WhileStatement final : public Statement {
public:
    WhileStatement(ExpressionPtr condition, std::vector<StatementPtr> body);

    const Expression& condition() const;
    const std::vector<StatementPtr>& body() const;

private:
    ExpressionPtr condition_;
    std::vector<StatementPtr> body_;
};

class RepeatStatement final : public Statement {
public:
    RepeatStatement(std::vector<StatementPtr> body, ExpressionPtr condition);

    const std::vector<StatementPtr>& body() const;
    const Expression& condition() const;

private:
    std::vector<StatementPtr> body_;
    ExpressionPtr condition_;
};

class ForInStatement final : public Statement {
public:
    ForInStatement(std::string iterator,
                   ExpressionPtr iterable,
                   ExpressionPtr step,
                   std::vector<StatementPtr> body);

    const std::string& iterator() const;
    const Expression& iterable() const;
    const Expression* step() const;
    const std::vector<StatementPtr>& body() const;

private:
    std::string iterator_;
    ExpressionPtr iterable_;
    ExpressionPtr step_;
    std::vector<StatementPtr> body_;
};

class CaseStatement final : public Statement {
public:
    CaseStatement(ExpressionPtr expression,
                  std::vector<CaseArm> arms,
                  std::vector<StatementPtr> otherwiseBody);

    const Expression& expression() const;
    const std::vector<CaseArm>& arms() const;
    const std::vector<StatementPtr>& otherwiseBody() const;

private:
    ExpressionPtr expression_;
    std::vector<CaseArm> arms_;
    std::vector<StatementPtr> otherwiseBody_;
};

class TryStatement final : public Statement {
public:
    TryStatement(std::vector<StatementPtr> body,
                 std::vector<StatementPtr> exceptBody,
                 std::vector<StatementPtr> finallyBody);

    const std::vector<StatementPtr>& body() const;
    const std::vector<StatementPtr>& exceptBody() const;
    const std::vector<StatementPtr>& finallyBody() const;

private:
    std::vector<StatementPtr> body_;
    std::vector<StatementPtr> exceptBody_;
    std::vector<StatementPtr> finallyBody_;
};

class RaiseStatement final : public Statement {
public:
    explicit RaiseStatement(ExpressionPtr expression);

    const Expression* expression() const;
    ExpressionPtr takeExpression();

private:
    ExpressionPtr expression_;
};

} // namespace inox::compiler::ast
