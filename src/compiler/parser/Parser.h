#pragma once

#include "../ast/Ast.h"
#include "../lexer/Token.h"

#include <cstddef>
#include <initializer_list>
#include <memory>
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

    std::unique_ptr<ast::ModuleNode> parseModule();
    ast::ExpressionPtr parseExpression();
    ast::StatementPtr parseStatement();
    std::vector<ast::StatementPtr> parseStatements();
    std::unique_ptr<ast::BlockStatement> parseBlockStatement();

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
    ast::ExpressionPtr parseForIterable();

    std::vector<ast::ExpressionPtr> parseArgumentList();

    ast::StatementPtr parseVarStatement(bool isMutable);
    ast::StatementPtr parseIfStatement();
    ast::StatementPtr parseUnlessStatement();
    ast::StatementPtr parseWhileStatement();
    ast::StatementPtr parseRepeatStatement();
    ast::StatementPtr parseForInStatement();
    ast::StatementPtr parseCaseStatement();
    ast::StatementPtr parseTryStatement();
    ast::StatementPtr parseRaiseStatement();
    ast::StatementPtr parseExpressionStatement();

    ast::AstNodePtr parseModuleItem();
    ast::AstNodePtr parseUseDeclaration();
    ast::AstNodePtr parseSectionDeclaration(ast::SectionKind sectionKind);
    ast::AstNodePtr parseRawDeclaration();
    ast::AstNodePtr parseFunctionDeclaration();

    std::vector<ast::StatementPtr> parseBlockBody();
    std::vector<ast::StatementPtr> parseDelimitedBody(std::initializer_list<std::string_view> stopKeywords);
    bool atAnyKeyword(std::initializer_list<std::string_view> keywords) const;
    bool atStatementBoundary() const;
    void consumeBlockClose();

    bool isAtEnd() const;
    const lexer::Token& peek() const;
    const lexer::Token& previous() const;
    bool check(TokenKind kind) const;
    bool checkKeyword(std::string_view normalized) const;
    bool checkIdentifierLike() const;
    bool match(TokenKind kind);
    bool matchKeyword(std::string_view normalized);
    const lexer::Token& consume(TokenKind kind, std::string_view message);
    const lexer::Token& consumeIdentifierLike(std::string_view message);
    const lexer::Token& advance();

    [[noreturn]] void errorAtCurrent(std::string_view message) const;
    [[noreturn]] void errorAt(const lexer::Token& token, std::string_view message) const;

    static ast::BinaryOperator binaryOperatorFor(const lexer::Token& token);
    static ast::UnaryOperator unaryOperatorFor(const lexer::Token& token);
    static std::string tokenText(const lexer::Token& token);

    std::vector<lexer::Token> tokens_;
    std::size_t current_ = 0;
};

} // namespace inox::compiler::parser
