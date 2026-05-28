#pragma once

#include "../ast/Ast.h"
#include "../lexer/Token.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace inox::compiler::parser {

class ParseError final : public std::runtime_error {
public:
    ParseError(std::string message, lexer::SourceLocation location);

    const lexer::SourceLocation& location() const;

private:
    lexer::SourceLocation location_;
};

class Parser {
public:
    explicit Parser(std::vector<lexer::Token> tokens);
    explicit Parser(std::string_view source);

    ast::ExpressionPtr parseExpression();

private:
    using TokenKind = lexer::TokenKind;

    ast::ExpressionPtr parseAssignment();
    ast::ExpressionPtr parseLogical();
    ast::ExpressionPtr parseRelational();
    ast::ExpressionPtr parseMembership();
    ast::ExpressionPtr parseRange();
    ast::ExpressionPtr parseAdditive();
    ast::ExpressionPtr parseMultiplicative();
    ast::ExpressionPtr parseUnary();
    ast::ExpressionPtr parsePower();
    ast::ExpressionPtr parsePostfix();
    ast::ExpressionPtr parsePrimary();

    std::vector<ast::ExpressionPtr> parseArgumentList();

    bool isAtEnd() const;
    const lexer::Token& peek() const;
    const lexer::Token& previous() const;
    bool check(TokenKind kind) const;
    bool checkKeyword(std::string_view normalized) const;
    bool match(TokenKind kind);
    bool matchKeyword(std::string_view normalized);
    const lexer::Token& consume(TokenKind kind, std::string_view message);
    const lexer::Token& advance();

    [[noreturn]] void errorAtCurrent(std::string_view message) const;
    [[noreturn]] void errorAt(const lexer::Token& token, std::string_view message) const;

    static ast::BinaryOperator binaryOperatorFor(const lexer::Token& token);
    static ast::UnaryOperator unaryOperatorFor(const lexer::Token& token);

    std::vector<lexer::Token> tokens_;
    std::size_t current_ = 0;
};

} // namespace inox::compiler::parser
