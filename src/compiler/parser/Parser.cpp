#include "Parser.h"

#include "../lexer/Lexer.h"

#include <memory>
#include <utility>

namespace inox::compiler::parser {

namespace {

ast::ExpressionPtr makeSyntheticIdentifier(std::string name)
{
    return std::make_unique<ast::IdentifierExpression>(std::move(name));
}

} // namespace

ParseError::ParseError(std::string message, lexer::SourceLocation location)
    : std::runtime_error(std::move(message)), location_(location)
{
}

const lexer::SourceLocation& ParseError::location() const
{
    return location_;
}

Parser::Parser(std::vector<lexer::Token> tokens) : tokens_(std::move(tokens))
{
    if (tokens_.empty() || tokens_.back().kind != TokenKind::EndOfFile) {
        tokens_.push_back(lexer::Token{TokenKind::EndOfFile});
    }
}

Parser::Parser(std::string_view source)
    : tokens_(lexer::Lexer(source).tokenize())
{
}

ast::ExpressionPtr Parser::parseExpression()
{
    auto expression = parseAssignment();
    if (!isAtEnd()) {
        errorAtCurrent("unexpected token after expression");
    }
    return expression;
}

ast::ExpressionPtr Parser::parseAssignment()
{
    auto left = parseLogical();

    if (match(TokenKind::ColonEqual)) {
        const lexer::Token& op = previous();
        auto right = parseAssignment();
        return std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(left), std::move(right));
    }

    return left;
}

ast::ExpressionPtr Parser::parseLogical()
{
    auto expression = parseRelational();

    while (checkKeyword("and") || checkKeyword("xor") || checkKeyword("or")) {
        const lexer::Token& op = advance();
        auto right = parseRelational();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseRelational()
{
    auto expression = parseMembership();

    while (check(TokenKind::Equal) || check(TokenKind::Hash) ||
           check(TokenKind::Less) || check(TokenKind::Greater) ||
           check(TokenKind::LessEqual) || check(TokenKind::GreaterEqual)) {
        const lexer::Token& op = advance();
        auto right = parseMembership();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseMembership()
{
    auto expression = parseRange();

    if (matchKeyword("in")) {
        const lexer::Token& op = previous();
        auto right = parseRange();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseRange()
{
    auto expression = parseAdditive();

    if (match(TokenKind::DotDot)) {
        const lexer::Token& op = previous();
        auto right = parseAdditive();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseAdditive()
{
    auto expression = parseMultiplicative();

    while (check(TokenKind::Plus) || check(TokenKind::Minus)) {
        const lexer::Token& op = advance();
        auto right = parseMultiplicative();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseMultiplicative()
{
    auto expression = parseUnary();

    while (check(TokenKind::Star) || check(TokenKind::Slash) ||
           checkKeyword("div") || checkKeyword("mod")) {
        const lexer::Token& op = advance();
        auto right = parseUnary();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseUnary()
{
    if (check(TokenKind::Plus) || check(TokenKind::Minus) || checkKeyword("not")) {
        const lexer::Token& op = advance();
        auto operand = parseUnary();
        return std::make_unique<ast::UnaryExpression>(
            unaryOperatorFor(op), std::move(operand));
    }

    return parsePower();
}

ast::ExpressionPtr Parser::parsePower()
{
    auto expression = parsePostfix();

    if (match(TokenKind::Caret)) {
        const lexer::Token& op = previous();
        auto right = parsePower();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parsePostfix()
{
    auto expression = parsePrimary();

    for (;;) {
        if (match(TokenKind::LeftParen)) {
            auto arguments = parseArgumentList();
            consume(TokenKind::RightParen, "expected ')' after argument list");
            expression = std::make_unique<ast::CallExpression>(
                std::move(expression), std::move(arguments));
            continue;
        }

        if (match(TokenKind::LeftBracket)) {
            auto index = parseAssignment();
            consume(TokenKind::RightBracket, "expected ']' after index expression");

            std::vector<ast::ExpressionPtr> arguments;
            arguments.push_back(std::move(expression));
            arguments.push_back(std::move(index));
            expression = std::make_unique<ast::CallExpression>(
                makeSyntheticIdentifier("__index"), std::move(arguments));
            continue;
        }

        if (match(TokenKind::Dot)) {
            const lexer::Token& name = consume(
                TokenKind::Identifier, "expected member name after '.'");

            std::vector<ast::ExpressionPtr> arguments;
            arguments.push_back(std::move(expression));
            arguments.push_back(std::make_unique<ast::IdentifierExpression>(name.lexeme));
            expression = std::make_unique<ast::CallExpression>(
                makeSyntheticIdentifier("__member"), std::move(arguments));
            continue;
        }

        break;
    }

    return expression;
}

ast::ExpressionPtr Parser::parsePrimary()
{
    if (match(TokenKind::IntegerLiteral)) {
        return std::make_unique<ast::LiteralExpression>(
            ast::LiteralKind::Integer, previous().lexeme);
    }

    if (match(TokenKind::FloatLiteral)) {
        return std::make_unique<ast::LiteralExpression>(
            ast::LiteralKind::Float, previous().lexeme);
    }

    if (match(TokenKind::StringLiteral)) {
        return std::make_unique<ast::LiteralExpression>(
            ast::LiteralKind::String, previous().lexeme);
    }

    if (match(TokenKind::CharLiteral)) {
        return std::make_unique<ast::LiteralExpression>(
            ast::LiteralKind::Char, previous().lexeme);
    }

    if (check(TokenKind::Identifier)) {
        const lexer::Token& token = advance();
        return std::make_unique<ast::IdentifierExpression>(token.lexeme);
    }

    if (match(TokenKind::LeftParen)) {
        auto expression = parseAssignment();
        consume(TokenKind::RightParen, "expected ')' after expression");
        return expression;
    }

    errorAtCurrent("expected expression");
}

std::vector<ast::ExpressionPtr> Parser::parseArgumentList()
{
    std::vector<ast::ExpressionPtr> arguments;

    if (check(TokenKind::RightParen)) {
        return arguments;
    }

    do {
        arguments.push_back(parseAssignment());
    } while (match(TokenKind::Comma));

    return arguments;
}

bool Parser::isAtEnd() const
{
    return current_ >= tokens_.size() || peek().kind == TokenKind::EndOfFile;
}

const lexer::Token& Parser::peek() const
{
    return tokens_[current_];
}

const lexer::Token& Parser::previous() const
{
    return tokens_[current_ - 1];
}

bool Parser::check(TokenKind kind) const
{
    return !isAtEnd() && peek().kind == kind;
}

bool Parser::checkKeyword(std::string_view normalized) const
{
    return !isAtEnd() &&
           peek().kind == TokenKind::Keyword &&
           peek().normalized == normalized;
}

bool Parser::match(TokenKind kind)
{
    if (!check(kind)) {
        return false;
    }
    advance();
    return true;
}

bool Parser::matchKeyword(std::string_view normalized)
{
    if (!checkKeyword(normalized)) {
        return false;
    }
    advance();
    return true;
}

const lexer::Token& Parser::consume(TokenKind kind, std::string_view message)
{
    if (check(kind)) {
        return advance();
    }
    errorAtCurrent(message);
}

const lexer::Token& Parser::advance()
{
    if (!isAtEnd()) {
        ++current_;
    }
    return previous();
}

void Parser::errorAtCurrent(std::string_view message) const
{
    if (current_ >= tokens_.size()) {
        throw ParseError(std::string(message), lexer::SourceLocation{});
    }
    errorAt(peek(), message);
}

void Parser::errorAt(const lexer::Token& token, std::string_view message) const
{
    throw ParseError(std::string(message), token.location);
}

ast::BinaryOperator Parser::binaryOperatorFor(const lexer::Token& token)
{
    switch (token.kind) {
    case TokenKind::ColonEqual:
        return ast::BinaryOperator::Assign;
    case TokenKind::Plus:
        return ast::BinaryOperator::Add;
    case TokenKind::Minus:
        return ast::BinaryOperator::Subtract;
    case TokenKind::Star:
        return ast::BinaryOperator::Multiply;
    case TokenKind::Slash:
        return ast::BinaryOperator::Divide;
    case TokenKind::Caret:
        return ast::BinaryOperator::Power;
    case TokenKind::DotDot:
        return ast::BinaryOperator::Range;
    case TokenKind::Equal:
        return ast::BinaryOperator::Equal;
    case TokenKind::Hash:
        return ast::BinaryOperator::NotEqual;
    case TokenKind::Less:
        return ast::BinaryOperator::Less;
    case TokenKind::Greater:
        return ast::BinaryOperator::Greater;
    case TokenKind::LessEqual:
        return ast::BinaryOperator::LessEqual;
    case TokenKind::GreaterEqual:
        return ast::BinaryOperator::GreaterEqual;
    case TokenKind::Keyword:
        if (token.normalized == "div") {
            return ast::BinaryOperator::IntegerDivide;
        }
        if (token.normalized == "mod") {
            return ast::BinaryOperator::Modulo;
        }
        if (token.normalized == "in") {
            return ast::BinaryOperator::In;
        }
        if (token.normalized == "and") {
            return ast::BinaryOperator::And;
        }
        if (token.normalized == "xor") {
            return ast::BinaryOperator::Xor;
        }
        if (token.normalized == "or") {
            return ast::BinaryOperator::Or;
        }
        break;
    default:
        break;
    }

    throw ParseError("unsupported binary operator", token.location);
}

ast::UnaryOperator Parser::unaryOperatorFor(const lexer::Token& token)
{
    if (token.kind == TokenKind::Plus) {
        return ast::UnaryOperator::Plus;
    }
    if (token.kind == TokenKind::Minus) {
        return ast::UnaryOperator::Minus;
    }
    if (token.kind == TokenKind::Keyword && token.normalized == "not") {
        return ast::UnaryOperator::Not;
    }

    throw ParseError("unsupported unary operator", token.location);
}

} // namespace inox::compiler::parser
