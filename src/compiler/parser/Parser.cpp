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

std::unique_ptr<ast::ModuleNode> Parser::parseModule()
{
    if (!matchKeyword("module")) {
        errorAtCurrent("expected 'Module'");
    }

    const lexer::Token& name = consumeIdentifierLike("expected module name");
    auto module = std::make_unique<ast::ModuleNode>(name.lexeme);

    while (!isAtEnd()) {
        module->items().push_back(parseModuleItem());
    }

    return module;
}

ast::ExpressionPtr Parser::parseExpression()
{
    auto expression = parseAssignment();
    if (!isAtEnd()) {
        errorAtCurrent("unexpected token after expression");
    }
    return expression;
}

ast::StatementPtr Parser::parseStatement()
{
    if (match(TokenKind::Colon)) {
        auto body = parseBlockBody();
        consumeBlockClose();
        return std::make_unique<ast::BlockStatement>(std::move(body));
    }

    if (matchKeyword("mut")) {
        consume(TokenKind::Keyword, "expected 'var' after 'mut'");
        if (previous().normalized != "var") {
            errorAt(previous(), "expected 'var' after 'mut'");
        }
        return parseVarStatement(true);
    }

    if (matchKeyword("var")) {
        if (match(TokenKind::Colon)) {
            auto declarations = parseBlockBody();
            consumeBlockClose();
            return std::make_unique<ast::VarBlockStatement>(std::move(declarations));
        }
        return parseVarStatement(false);
    }

    if (matchKeyword("if")) {
        return parseIfStatement();
    }
    if (matchKeyword("unless")) {
        return parseUnlessStatement();
    }
    if (matchKeyword("while")) {
        return parseWhileStatement();
    }
    if (matchKeyword("repeat")) {
        return parseRepeatStatement();
    }
    if (matchKeyword("until")) {
        return parseUntilStatement();
    }
    if (matchKeyword("for")) {
        return parseForInStatement();
    }
    if (matchKeyword("case")) {
        return parseCaseStatement();
    }
    if (matchKeyword("try")) {
        return parseTryStatement();
    }
    if (matchKeyword("raise")) {
        return parseRaiseStatement();
    }
    if (matchKeyword("return")) {
        return parseReturnStatement();
    }
    if (matchKeyword("exit")) {
        return std::make_unique<ast::ExitStatement>();
    }
    if (matchKeyword("break")) {
        return std::make_unique<ast::BreakStatement>();
    }
    if (matchKeyword("continue")) {
        return std::make_unique<ast::ContinueStatement>();
    }

    return parseExpressionStatement();
}

std::vector<ast::StatementPtr> Parser::parseStatements()
{
    std::vector<ast::StatementPtr> statements;
    while (!isAtEnd()) {
        statements.push_back(parseStatement());
    }
    return statements;
}

std::unique_ptr<ast::BlockStatement> Parser::parseBlockStatement()
{
    consume(TokenKind::Colon, "expected ':' to open block");
    auto body = parseBlockBody();
    consumeBlockClose();
    return std::make_unique<ast::BlockStatement>(std::move(body));
}

ast::ExpressionPtr Parser::parseAssignment()
{
    auto left = parseOr();

    if (match(TokenKind::ColonEqual)) {
        const lexer::Token& op = previous();
        auto right = parseAssignment();
        return std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(left), std::move(right));
    }

    return left;
}

ast::ExpressionPtr Parser::parseOr()
{
    auto expression = parseXor();

    while (checkKeyword("or")) {
        const lexer::Token& op = advance();
        auto right = parseXor();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseXor()
{
    auto expression = parseAnd();

    while (checkKeyword("xor")) {
        const lexer::Token& op = advance();
        auto right = parseAnd();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseAnd()
{
    auto expression = parseRelational();

    while (checkKeyword("and")) {
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
    auto expression = parseBitOr();

    if (match(TokenKind::DotDot)) {
        const lexer::Token& op = previous();
        auto right = parseBitOr();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseBitOr()
{
    auto expression = parseBitXor();

    while (checkKeyword("bitor")) {
        const lexer::Token& op = advance();
        auto right = parseBitXor();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseBitXor()
{
    auto expression = parseBitAnd();

    while (checkKeyword("bitxor")) {
        const lexer::Token& op = advance();
        auto right = parseBitAnd();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseBitAnd()
{
    auto expression = parseShift();

    while (checkKeyword("bitand")) {
        const lexer::Token& op = advance();
        auto right = parseShift();
        expression = std::make_unique<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseShift()
{
    auto expression = parseAdditive();

    while (checkKeyword("shl") || checkKeyword("shr")) {
        const lexer::Token& op = advance();
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
    if (check(TokenKind::Plus) || check(TokenKind::Minus) ||
        checkKeyword("not") || checkKeyword("bitnot")) {
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

    if (checkKeyword("true") || checkKeyword("false")) {
        const lexer::Token& token = advance();
        return std::make_unique<ast::LiteralExpression>(
            ast::LiteralKind::Boolean, token.normalized);
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

ast::ExpressionPtr Parser::parseForIterable()
{
    auto lower = parseAdditive();
    if (!match(TokenKind::DotDot)) {
        return lower;
    }

    const lexer::Token& op = previous();
    auto upper = parsePrimary();
    return std::make_unique<ast::BinaryExpression>(
        binaryOperatorFor(op), std::move(lower), std::move(upper));
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

ast::StatementPtr Parser::parseVarStatement(bool isMutable)
{
    const lexer::Token& name = consume(TokenKind::Identifier, "expected variable name");
    ast::ExpressionPtr initializer;

    if (match(TokenKind::ColonEqual)) {
        initializer = parseAssignment();
    }

    return std::make_unique<ast::VarStatement>(
        isMutable, name.lexeme, std::move(initializer));
}

ast::StatementPtr Parser::parseIfStatement()
{
    auto condition = parseAssignment();
    requireHeaderLineBreak();
    auto thenBody = parseDelimitedBody({"elif", "else"});

    std::vector<ast::ElseIfClause> elseIfClauses;
    while (matchKeyword("elif")) {
        auto elseIfCondition = parseAssignment();
        requireHeaderLineBreak();
        elseIfClauses.push_back(ast::ElseIfClause{
            std::move(elseIfCondition),
            parseDelimitedBody({"elif", "else"})
        });
    }

    std::vector<ast::StatementPtr> elseBody;
    if (matchKeyword("else")) {
        requireHeaderLineBreak();
        elseBody = parseDelimitedBody({});
    }
    consumeBlockClose();

    return std::make_unique<ast::IfStatement>(
        std::move(condition),
        std::move(thenBody),
        std::move(elseIfClauses),
        std::move(elseBody));
}

ast::StatementPtr Parser::parseUnlessStatement()
{
    auto condition = parseAssignment();
    auto body = parseBlockStatement();
    return std::make_unique<ast::UnlessStatement>(
        std::move(condition), body->takeStatements());
}

ast::StatementPtr Parser::parseWhileStatement()
{
    auto condition = parseAssignment();
    auto body = parseBlockStatement();
    return std::make_unique<ast::WhileStatement>(
        std::move(condition), body->takeStatements());
}

ast::StatementPtr Parser::parseRepeatStatement()
{
    auto body = parseBlockBody();
    consumeBlockClose();
    return std::make_unique<ast::RepeatStatement>(std::move(body));
}

ast::StatementPtr Parser::parseUntilStatement()
{
    if (atStatementBoundary()) {
        errorAtCurrent("expected condition after 'until'");
    }
    return std::make_unique<ast::UntilStatement>(parseAssignment());
}

ast::StatementPtr Parser::parseForInStatement()
{
    const lexer::Token& iterator = consume(TokenKind::Identifier, "expected loop iterator");
    if (!matchKeyword("in")) {
        errorAtCurrent("expected 'in' after loop iterator");
    }

    auto iterable = parseForIterable();
    ast::ExpressionPtr step;
    if (match(TokenKind::LeftParen)) {
        step = parseAssignment();
        consume(TokenKind::RightParen, "expected ')' after loop step");
    }

    auto body = parseBlockStatement();
    return std::make_unique<ast::ForInStatement>(
        iterator.lexeme,
        std::move(iterable),
        std::move(step),
        body->takeStatements());
}

ast::StatementPtr Parser::parseCaseStatement()
{
    auto expression = parseAssignment();
    consume(TokenKind::Colon, "expected ':' after case expression");

    std::vector<ast::CaseArm> arms;
    std::vector<ast::StatementPtr> otherwiseBody;

    while (!isAtEnd() && !check(TokenKind::Semicolon) && !checkKeyword("end")) {
        if (matchKeyword("otherwise")) {
            auto block = parseBlockStatement();
            otherwiseBody = block->takeStatements();
            break;
        }

        std::vector<ast::ExpressionPtr> choices;
        choices.push_back(parseAssignment());
        while (match(TokenKind::Comma)) {
            choices.push_back(parseAssignment());
        }

        auto body = parseBlockStatement();
        arms.push_back(ast::CaseArm{std::move(choices), body->takeStatements()});
    }

    consumeBlockClose();
    return std::make_unique<ast::CaseStatement>(
        std::move(expression),
        std::move(arms),
        std::move(otherwiseBody));
}

ast::StatementPtr Parser::parseTryStatement()
{
    auto body = parseBlockStatement();

    std::vector<ast::StatementPtr> exceptBody;
    if (matchKeyword("except")) {
        auto exceptBlock = parseBlockStatement();
        exceptBody = exceptBlock->takeStatements();
    }

    std::vector<ast::StatementPtr> finallyBody;
    if (matchKeyword("finally")) {
        auto finallyBlock = parseBlockStatement();
        finallyBody = finallyBlock->takeStatements();
    }

    return std::make_unique<ast::TryStatement>(
        body->takeStatements(),
        std::move(exceptBody),
        std::move(finallyBody));
}

ast::StatementPtr Parser::parseRaiseStatement()
{
    ast::ExpressionPtr expression;
    if (!atStatementBoundary()) {
        expression = parseAssignment();
    }
    return std::make_unique<ast::RaiseStatement>(std::move(expression));
}

ast::StatementPtr Parser::parseReturnStatement()
{
    if (atStatementBoundary()) {
        errorAtCurrent("expected expression after 'return'");
    }
    return std::make_unique<ast::ReturnStatement>(parseAssignment());
}

ast::StatementPtr Parser::parseExpressionStatement()
{
    return std::make_unique<ast::ExpressionStatement>(parseAssignment());
}

ast::AstNodePtr Parser::parseModuleItem()
{
    if (matchKeyword("use")) {
        return parseUseDeclaration();
    }

    if (matchKeyword("type")) {
        return parseSectionDeclaration(ast::SectionKind::Type);
    }

    if (matchKeyword("const")) {
        return parseSectionDeclaration(ast::SectionKind::Const);
    }

    if (matchKeyword("state")) {
        return parseSectionDeclaration(ast::SectionKind::State);
    }

    if (matchKeyword("var")) {
        return parseSectionDeclaration(ast::SectionKind::Var);
    }

    if (checkIdentifierLike() || checkKeyword("main")) {
        for (std::size_t index = current_; index < tokens_.size(); ++index) {
            if (tokens_[index].kind == TokenKind::Colon) {
                return parseFunctionDeclaration();
            }
            if (tokens_[index].kind == TokenKind::Semicolon ||
                tokens_[index].kind == TokenKind::EndOfFile) {
                break;
            }
        }
    }

    return parseRawDeclaration();
}

ast::AstNodePtr Parser::parseUseDeclaration()
{
    std::vector<std::string> path;
    while (!isAtEnd() && !check(TokenKind::Semicolon) && !checkKeyword("end")) {
        path.push_back(tokenText(advance()));
    }
    consumeBlockClose();
    return std::make_unique<ast::UseDeclaration>(std::move(path));
}

ast::AstNodePtr Parser::parseSectionDeclaration(ast::SectionKind sectionKind)
{
    consume(TokenKind::Colon, "expected ':' after section header");

    std::vector<std::string> tokens;
    while (!isAtEnd() && !check(TokenKind::Semicolon) && !checkKeyword("end")) {
        tokens.push_back(tokenText(advance()));
    }

    consumeBlockClose();
    return std::make_unique<ast::SectionDeclaration>(
        sectionKind, std::move(tokens));
}

ast::AstNodePtr Parser::parseRawDeclaration()
{
    const lexer::Token& head = advance();
    std::vector<std::string> tokens;

    while (!isAtEnd() && !check(TokenKind::Semicolon) && !checkKeyword("end")) {
        tokens.push_back(tokenText(advance()));
    }

    if (!isAtEnd()) {
        consumeBlockClose();
    }

    return std::make_unique<ast::RawDeclaration>(
        tokenText(head), std::move(tokens));
}

ast::AstNodePtr Parser::parseFunctionDeclaration()
{
    const lexer::Token& name = consumeIdentifierLike("expected function name");
    std::vector<std::string> signatureTokens;

    while (!isAtEnd() && !check(TokenKind::Colon)) {
        signatureTokens.push_back(tokenText(advance()));
    }

    consume(TokenKind::Colon, "expected ':' after function signature");
    auto body = parseBlockBody();
    consumeBlockClose();

    return std::make_unique<ast::FunctionDeclaration>(
        name.lexeme,
        std::move(signatureTokens),
        std::move(body));
}

std::vector<ast::StatementPtr> Parser::parseBlockBody()
{
    std::vector<ast::StatementPtr> statements;
    while (!isAtEnd() && !check(TokenKind::Semicolon) && !checkKeyword("end")) {
        statements.push_back(parseStatement());
    }
    return statements;
}

std::vector<ast::StatementPtr> Parser::parseDelimitedBody(std::initializer_list<std::string_view> stopKeywords)
{
    std::vector<ast::StatementPtr> statements;
    while (!isAtEnd() && !check(TokenKind::Semicolon) && !checkKeyword("end") &&
           !atAnyKeyword(stopKeywords)) {
        statements.push_back(parseStatement());
    }
    return statements;
}

bool Parser::atAnyKeyword(std::initializer_list<std::string_view> keywords) const
{
    for (std::string_view keyword : keywords) {
        if (checkKeyword(keyword)) {
            return true;
        }
    }
    return false;
}

bool Parser::atStatementBoundary() const
{
    return isAtEnd() ||
           check(TokenKind::Semicolon) ||
           checkKeyword("end") ||
           checkKeyword("elif") ||
           checkKeyword("else") ||
           checkKeyword("except") ||
           checkKeyword("finally") ||
           checkKeyword("until") ||
           checkKeyword("otherwise");
}

void Parser::requireHeaderLineBreak()
{
    if (isAtEnd() || peek().location.line <= previous().location.line) {
        errorAtCurrent("expected line break after conditional header");
    }
}

void Parser::consumeBlockClose()
{
    if (match(TokenKind::Semicolon) || matchKeyword("end")) {
        return;
    }
    errorAtCurrent("expected block close ';' or 'End'");
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

bool Parser::checkIdentifierLike() const
{
    return !isAtEnd() &&
           (peek().kind == TokenKind::Identifier || checkKeyword("main"));
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

const lexer::Token& Parser::consumeIdentifierLike(std::string_view message)
{
    if (checkIdentifierLike()) {
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
        if (token.normalized == "shl") {
            return ast::BinaryOperator::ShiftLeft;
        }
        if (token.normalized == "shr") {
            return ast::BinaryOperator::ShiftRight;
        }
        if (token.normalized == "bitand") {
            return ast::BinaryOperator::BitAnd;
        }
        if (token.normalized == "bitxor") {
            return ast::BinaryOperator::BitXor;
        }
        if (token.normalized == "bitor") {
            return ast::BinaryOperator::BitOr;
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
    if (token.kind == TokenKind::Keyword && token.normalized == "bitnot") {
        return ast::UnaryOperator::BitNot;
    }

    throw ParseError("unsupported unary operator", token.location);
}

std::string Parser::tokenText(const lexer::Token& token)
{
    if (!token.lexeme.empty()) {
        return token.lexeme;
    }
    return token.normalized;
}

} // namespace inox::compiler::parser
