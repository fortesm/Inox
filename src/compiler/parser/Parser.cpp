// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Parser.h"

#include "../lexer/Lexer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace inox::compiler::parser {

namespace {

constexpr const char* kVarRemovedMessage =
    "the 'Var' block and 'var'/'mut var' declarations were removed (CANON-5); "
    "declare locals inline: 'Name := Value', 'Name Type := Value', or 'Name TStruct'";

constexpr const char* kAssignmentInExpressionMessage =
    "':=' is a statement, not an expression (CANON-5): an assignment cannot "
    "appear inside an expression, a condition or an initializer";

constexpr const char* kOldForStepMessage =
    "the for-loop step is written 'step S' (ADR-0010): write "
    "'for I in A..B step S' instead of 'for I in A..B (S)'";

// Sets a parser flag for the lifetime of a scope and restores it afterwards,
// also when a ParseError propagates.
class FlagScope {
public:
    explicit FlagScope(bool& flag) : flag_(flag), saved_(flag) { flag_ = true; }
    ~FlagScope() { flag_ = saved_; }
    FlagScope(const FlagScope&) = delete;
    FlagScope& operator=(const FlagScope&) = delete;

private:
    bool& flag_;
    bool saved_;
};

ast::ExpressionPtr makeSyntheticIdentifier(std::string name)
{
    return std::make_unique<ast::IdentifierExpression>(std::move(name));
}

// A chained-assignment target is a variable or a field path (`P.X`, `.X` in a
// `with` body). Index targets are excluded: re-reading `V[I]` after `I`
// changed would read a different element.
bool isChainTarget(const ast::Expression& expression)
{
    if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
        return true;
    }
    if (expression.kind() != ast::AstNodeKind::CallExpression) {
        return false;
    }
    const auto& call = static_cast<const ast::CallExpression&>(expression);
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression ||
        static_cast<const ast::IdentifierExpression&>(call.callee()).name() != "__member" ||
        call.arguments().size() != 2 ||
        call.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
        return false;
    }
    return isChainTarget(*call.arguments()[0]);
}

ast::ExpressionPtr cloneChainTarget(const ast::Expression& expression)
{
    if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
        return makeSyntheticIdentifier(
            static_cast<const ast::IdentifierExpression&>(expression).name());
    }
    const auto& call = static_cast<const ast::CallExpression&>(expression);
    std::vector<ast::ExpressionPtr> arguments;
    arguments.push_back(cloneChainTarget(*call.arguments()[0]));
    arguments.push_back(cloneChainTarget(*call.arguments()[1]));
    return std::make_unique<ast::CallExpression>(
        makeSyntheticIdentifier("__member"), std::move(arguments));
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
        tokens_.push_back(lexer::Token{TokenKind::EndOfFile, "", "", {}});
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

    auto module = std::make_unique<ast::ModuleNode>(
        parseQualifiedName("expected module name"));

    while (!isAtEnd()) {
        module->items().push_back(parseModuleItem());
    }

    return module;
}

ast::ExpressionPtr Parser::parseExpression()
{
    auto expression = parseValue();
    if (!isAtEnd()) {
        errorAtCurrent("unexpected token after expression");
    }
    return expression;
}

Parser::DepthGuard::DepthGuard(const Parser& owner, std::size_t& counter,
                               std::size_t limit, const char* what)
    : counter_(counter)
{
    if (++counter_ > limit) {
        --counter_;
        owner.errorAtCurrent("maximum " + std::string(what) + " nesting depth exceeded (implementation limit: " +
                             std::to_string(limit) + " levels)");
    }
}

Parser::DepthGuard::~DepthGuard()
{
    --counter_;
}

void Parser::checkExpressionDepth(const ast::Expression& expression) const
{
    if (expression.depth() > kMaxExpressionDepth) {
        errorAtCurrent("maximum expression nesting depth exceeded (implementation limit: " +
                       std::to_string(kMaxExpressionDepth) +
                       " levels); split it into smaller expressions");
    }
}

// CANON-4: newlines terminate statements. A simple statement must therefore be
// followed by a line break, the end of the block, or the end of the input.
ast::StatementPtr Parser::endSimpleStatement(ast::StatementPtr statement)
{
    if (!isAtEnd() && !check(TokenKind::Semicolon) &&
        peek().location.line == previous().location.line) {
        errorAtCurrent("expected line break after statement; newlines terminate statements in Inox");
    }
    return statement;
}

ast::StatementPtr Parser::parseStatement()
{
    DepthGuard guard(*this, statementNesting_, kMaxStatementNesting, "statement");
    // CANON-4: `:` has one meaning, it declares a function or subroutine and
    // opens its body. A bare `:` ... `;` block inside a routine is illegal.
    if (check(TokenKind::Colon)) {
        errorAtCurrent(
            "a bare ':' block is illegal (CANON-4): ':' only declares a "
            "function or subroutine; write the statements directly in the "
            "enclosing body");
    }

    // CANON-5: the old `Var ... ;` block and `var`/`mut var` declarations were
    // removed. `Var` and `mut` stay reserved; reject them with a migration hint.
    if (checkKeyword("var") || checkKeyword("mut")) {
        errorAtCurrent(kVarRemovedMessage);
    }

    if (matchKeyword("if")) {
        return parseIfStatement();
    }
    if (matchKeyword("unless")) {
        return parseUnlessStatement();
    }
    if (matchKeyword("with")) {
        return parseWithStatement();
    }
    if (matchKeyword("while")) {
        return parseWhileStatement();
    }
    if (matchKeyword("repeat")) {
        return parseRepeatStatement();
    }
    if (matchKeyword("until")) {
        return endSimpleStatement(parseUntilStatement());
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
        return endSimpleStatement(parseRaiseStatement());
    }
    if (matchKeyword("retry")) {
        return endSimpleStatement(parseRetryStatement());
    }
    if (matchKeyword("return")) {
        return endSimpleStatement(parseReturnStatement());
    }
    if (matchKeyword("exit")) {
        return endSimpleStatement(std::make_unique<ast::ExitStatement>());
    }
    if (matchKeyword("leave")) {
        return endSimpleStatement(std::make_unique<ast::LeaveStatement>());
    }
    if (matchKeyword("continue")) {
        return endSimpleStatement(std::make_unique<ast::ContinueStatement>());
    }

    if (atTypedLocalStatementStart()) {
        return endSimpleStatement(parseTypedLocalStatement());
    }

    return endSimpleStatement(parseExpressionStatement());
}

void Parser::appendStatement(std::vector<ast::StatementPtr>& statements)
{
    statements.push_back(parseStatement());
    for (auto& pending : pendingStatements_) {
        statements.push_back(std::move(pending));
    }
    pendingStatements_.clear();
}

std::vector<ast::StatementPtr> Parser::parseStatements()
{
    std::vector<ast::StatementPtr> statements;
    while (!isAtEnd()) {
        appendStatement(statements);
    }
    return statements;
}

std::vector<ast::StatementPtr> Parser::parseHeaderDelimitedBlock()
{
    requireHeaderLineBreak();
    auto body = parseBlockBody();
    consumeBlockClose();
    return body;
}

// A value expression: everything except assignment. `:=` is a statement in
// Inox (CANON-5, v3.27), so it may not appear inside an expression.
ast::ExpressionPtr Parser::parseValue()
{
    DepthGuard guard(*this, expressionNesting_, kMaxExpressionNesting, "expression");
    auto expression = parseOr();
    if (check(TokenKind::ColonEqual)) {
        errorAtCurrent(kAssignmentInExpressionMessage);
    }
    return expression;
}

// A call argument: a value, or `Field := Value` for named struct construction
// (CANON-9). The named form takes exactly one `:=`.
ast::ExpressionPtr Parser::parseArgument()
{
    if (check(TokenKind::Identifier) && current_ + 1 < tokens_.size() &&
        tokens_[current_ + 1].kind == TokenKind::ColonEqual) {
        const lexer::Token& field = advance();
        const lexer::Token& op = advance();
        auto value = parseValue();
        return makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(op),
            std::make_unique<ast::IdentifierExpression>(field.lexeme),
            std::move(value));
    }
    return parseValue();
}

void Parser::markParenthesized(const ast::Expression* node)
{
    if (node != nullptr) {
        parenthesized_.insert(node);
    }
}

bool Parser::isParenthesized(const ast::Expression* node) const
{
    return node != nullptr && parenthesized_.contains(node);
}

void Parser::requireNoLogicalMix(
    const ast::Expression* operand, std::string_view outerOp,
    const lexer::Token& opToken) const
{
    // Canonical Layer A: and/or/xor are all relational-combining logical
    // operators. Their relative precedence IS defined (and > xor > or), but
    // people memorize it wrong, so mixing two DIFFERENT logical operators
    // without parentheses is rejected. An explicitly parenthesized operand is
    // always allowed.
    if (operand == nullptr || isParenthesized(operand)) {
        return;
    }
    if (operand->kind() != ast::AstNodeKind::BinaryExpression) {
        return;
    }
    const auto& binary = static_cast<const ast::BinaryExpression&>(*operand);
    const ast::BinaryOperator inner = binary.op();
    const bool innerIsLogical =
        inner == ast::BinaryOperator::And ||
        inner == ast::BinaryOperator::Or ||
        inner == ast::BinaryOperator::Xor;
    if (!innerIsLogical) {
        return;
    }
    std::string innerName;
    switch (inner) {
        case ast::BinaryOperator::And: innerName = "and"; break;
        case ast::BinaryOperator::Or: innerName = "or"; break;
        case ast::BinaryOperator::Xor: innerName = "xor"; break;
        default: return;
    }
    if (innerName == std::string(outerOp)) {
        return;  // same operator chained (a and b and c) is fine
    }
    throw ParseError(
        "mixing '" + innerName + "' and '" + std::string(outerOp) +
            "' requires explicit parentheses; write '" + innerName +
            "' grouped in parentheses to make the order unambiguous",
        opToken.location);
}

void Parser::requireNoBitwiseMix(
    const ast::Expression* operand, std::string_view outerOp,
    const lexer::Token& opToken) const
{
    // Canonical Layer A: different bitwise families (bitand/bitxor/bitor) and
    // shifts mixed together must be parenthesized. Same-operator chains are fine.
    if (operand == nullptr || isParenthesized(operand)) {
        return;
    }
    if (operand->kind() != ast::AstNodeKind::BinaryExpression) {
        return;
    }
    const auto& binary = static_cast<const ast::BinaryExpression&>(*operand);
    const ast::BinaryOperator inner = binary.op();
    std::string innerName;
    switch (inner) {
        case ast::BinaryOperator::BitAnd: innerName = "bitand"; break;
        case ast::BinaryOperator::BitXor: innerName = "bitxor"; break;
        case ast::BinaryOperator::BitOr: innerName = "bitor"; break;
        case ast::BinaryOperator::ShiftLeft: innerName = "shl"; break;
        case ast::BinaryOperator::ShiftRight: innerName = "shr"; break;
        default: return;
    }
    if (innerName == std::string(outerOp)) {
        return;  // same operator chained is fine
    }
    throw ParseError(
        "mixing bitwise operators '" + innerName + "' and '" +
            std::string(outerOp) +
            "' requires explicit parentheses to make the order unambiguous",
        opToken.location);
}

ast::ExpressionPtr Parser::parseOr()
{
    auto expression = parseXor();

    while (checkKeyword("or")) {
        const lexer::Token& op = advance();
        requireNoLogicalMix(expression.get(), "or", op);
        auto right = parseXor();
        requireNoLogicalMix(right.get(), "or", op);
        expression = makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseXor()
{
    auto expression = parseAnd();

    while (checkKeyword("xor")) {
        const lexer::Token& op = advance();
        requireNoLogicalMix(expression.get(), "xor", op);
        auto right = parseAnd();
        requireNoLogicalMix(right.get(), "xor", op);
        expression = makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseAnd()
{
    auto expression = parseRelational();

    while (checkKeyword("and")) {
        const lexer::Token& op = advance();
        requireNoLogicalMix(expression.get(), "and", op);
        auto right = parseRelational();
        requireNoLogicalMix(right.get(), "and", op);
        expression = makeExpr<ast::BinaryExpression>(
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
        expression = makeExpr<ast::BinaryExpression>(
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
        expression = makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
        // CANON-20 (ADR-0008): 'in' is non-associative.
        if (checkKeyword("in")) {
            throw ParseError(
                "'in' is non-associative (CANON-20): 'X in A in B' has no "
                "meaning; parenthesize the intended grouping",
                peek().location);
        }
    }

    return expression;
}

ast::ExpressionPtr Parser::parseRange()
{
    auto expression = parseBitOr();

    if (match(TokenKind::DotDot)) {
        const lexer::Token& op = previous();
        auto right = parseBitOr();
        expression = makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
        // CANON-20 (ADR-0008): '..' is non-associative; a range has exactly
        // two bounds.
        if (check(TokenKind::DotDot)) {
            throw ParseError(
                "'..' is non-associative (CANON-20): a range has exactly two "
                "bounds, so 'A..B..C' has no meaning",
                peek().location);
        }
    }

    return expression;
}

ast::ExpressionPtr Parser::parseBitOr()
{
    auto expression = parseBitXor();

    while (checkKeyword("bitor")) {
        const lexer::Token& op = advance();
        requireNoBitwiseMix(expression.get(), "bitor", op);
        auto right = parseBitXor();
        requireNoBitwiseMix(right.get(), "bitor", op);
        expression = makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseBitXor()
{
    auto expression = parseBitAnd();

    while (checkKeyword("bitxor")) {
        const lexer::Token& op = advance();
        requireNoBitwiseMix(expression.get(), "bitxor", op);
        auto right = parseBitAnd();
        requireNoBitwiseMix(right.get(), "bitxor", op);
        expression = makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseBitAnd()
{
    auto expression = parseShift();

    while (checkKeyword("bitand")) {
        const lexer::Token& op = advance();
        requireNoBitwiseMix(expression.get(), "bitand", op);
        auto right = parseShift();
        requireNoBitwiseMix(right.get(), "bitand", op);
        expression = makeExpr<ast::BinaryExpression>(
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
        expression = makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseAdditive()
{
    auto expression = parseMultiplicative();

    while ((check(TokenKind::Plus) || check(TokenKind::Minus)) && !atLineBreakInHeader()) {
        const lexer::Token& op = advance();
        auto right = parseMultiplicative();
        expression = makeExpr<ast::BinaryExpression>(
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
        expression = makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parseUnary()
{
    DepthGuard guard(*this, expressionNesting_, kMaxExpressionNesting, "expression");
    if (check(TokenKind::Plus) || check(TokenKind::Minus) ||
        checkKeyword("not") || checkKeyword("bitnot")) {
        const lexer::Token& op = advance();
        auto operand = parseUnary();
        return makeExpr<ast::UnaryExpression>(
            unaryOperatorFor(op), std::move(operand));
    }

    return parsePower();
}

ast::ExpressionPtr Parser::parsePower()
{
    DepthGuard guard(*this, expressionNesting_, kMaxExpressionNesting, "expression");
    auto expression = parsePostfix();

    if (match(TokenKind::Caret)) {
        const lexer::Token& op = previous();
        auto right = parsePower();
        expression = makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(expression), std::move(right));
    }

    return expression;
}

ast::ExpressionPtr Parser::parsePostfix()
{
    auto expression = parsePrimary();

    for (;;) {
        // CANON-4: newlines terminate statements. A `(` or `[` that begins a
        // new line never continues the expression of the previous line as a
        // call or an index (the same rule as for a leading `.` below).
        if ((check(TokenKind::LeftParen) || check(TokenKind::LeftBracket)) &&
            current_ > 0 && peek().location.line > previous().location.line) {
            break;
        }
        if (match(TokenKind::LeftParen)) {
            if (check(TokenKind::RightParen)) {
                errorAtCurrent("empty parentheses are not allowed in calls; omit parentheses when there are no arguments");
            }
            auto arguments = parseArgumentList();
            consume(TokenKind::RightParen, "expected ')' after argument list");
            expression = makeExpr<ast::CallExpression>(
                std::move(expression), std::move(arguments));
            continue;
        }

        if (match(TokenKind::LeftBracket)) {
            auto index = parseValue();
            consume(TokenKind::RightBracket, "expected ']' after index expression");

            std::vector<ast::ExpressionPtr> arguments;
            arguments.push_back(std::move(expression));
            arguments.push_back(std::move(index));
            expression = makeExpr<ast::CallExpression>(
                makeSyntheticIdentifier("__index"), std::move(arguments));
            continue;
        }

        // A '.' that begins a NEW line is NOT postfix member access on the
        // current expression — it is a dot-prefixed statement inside a 'with'
        // body (e.g. `.FY := 20`). Newlines terminate statements in Inox, so a
        // leading-dot on the next line must not be glued to the previous
        // expression (which would wrongly parse `10` then `.FY` as `10.FY`).
        if (check(TokenKind::Dot) &&
            !withTargetStack_.empty() &&
            peek().location.line > previous().location.line) {
            break;
        }

        // A '.' that begins a NEW line is never postfix member access on the
        // current expression. Newlines terminate statements in Inox, so a
        // leading dot on the next line starts a new (dot-prefixed) statement
        // inside a 'with' body — e.g. `.FY := 20`. Without this guard, the
        // target `P` of `with P` would glue to the next line's `.FX`, and a
        // right-hand `10` would glue to the following `.FY` as `10.FY`.
        if (check(TokenKind::Dot) &&
            current_ > 0 &&
            peek().location.line > previous().location.line) {
            break;
        }

        if (match(TokenKind::Dot)) {
            const lexer::Token& name = consume(
                TokenKind::Identifier, "expected member name after '.'");

            std::vector<ast::ExpressionPtr> arguments;
            arguments.push_back(std::move(expression));
            arguments.push_back(std::make_unique<ast::IdentifierExpression>(name.lexeme));
            expression = makeExpr<ast::CallExpression>(
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
            ast::LiteralKind::Integer, tokenText(previous()));
    }

    if (match(TokenKind::FloatLiteral)) {
        return std::make_unique<ast::LiteralExpression>(
            ast::LiteralKind::Float, tokenText(previous()));
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
        auto expression = parseValue();
        consume(TokenKind::RightParen, "expected ')' after expression");
        markParenthesized(expression.get());
        return expression;
    }

    // Leading '.' inside a 'with' body: expand to __member(__with_N, Member)
    if (check(TokenKind::Dot) && !withTargetStack_.empty()) {
        advance(); // consume '.'
        const lexer::Token& memberName = consume(
            TokenKind::Identifier, "expected member name after '.' in 'with' body");
        std::vector<ast::ExpressionPtr> arguments;
        arguments.push_back(
            std::make_unique<ast::IdentifierExpression>(withTargetStack_.back()));
        arguments.push_back(
            std::make_unique<ast::IdentifierExpression>(memberName.lexeme));
        return makeExpr<ast::CallExpression>(
            makeSyntheticIdentifier("__member"), std::move(arguments));
    }

    errorAtCurrent("expected expression");
}

// ADR-0010: the old step form `for I in A..B (S)` / `A..B(S)` is gone. It is
// recognized here, before the header is parsed, so the programmer gets a
// migration message instead of "not a function". The form is a parenthesized
// group that ends the header line, follows `..` at depth 0, and follows a
// literal or `)`, which cannot be called. After an identifier (`1..F(2)`,
// `1..F (2)`) it is a call and is left to the analyzer.
void Parser::rejectOldForStep() const
{
    const std::size_t line = tokens_[current_].location.line;
    std::size_t end = current_;
    while (end < tokens_.size() && tokens_[end].kind != TokenKind::EndOfFile &&
           tokens_[end].location.line == line) {
        ++end;
    }
    if (end == current_ || tokens_[end - 1].kind != TokenKind::RightParen) {
        return;
    }
    // Find the `(` matching the final `)`.
    std::size_t depth = 0;
    std::size_t open = end - 1;
    for (std::size_t i = end; i-- > current_;) {
        if (tokens_[i].kind == TokenKind::RightParen) {
            ++depth;
        } else if (tokens_[i].kind == TokenKind::LeftParen && --depth == 0) {
            open = i;
            break;
        }
    }
    if (open == current_ || tokens_[open].kind != TokenKind::LeftParen) {
        return;
    }
    bool rangeBefore = false;
    std::size_t level = 0;
    for (std::size_t i = current_; i < open; ++i) {
        if (tokens_[i].kind == TokenKind::LeftParen) {
            ++level;
        } else if (tokens_[i].kind == TokenKind::RightParen && level > 0) {
            --level;
        } else if (tokens_[i].kind == TokenKind::DotDot && level == 0) {
            rangeBefore = true;
        }
    }
    if (!rangeBefore) {
        return;
    }
    // The group is a step only when it follows a complete operand: a literal,
    // an identifier or a `)`. After an operator or `..` it is a parenthesized
    // bound, e.g. `A..(B - 1)`.
    const lexer::Token& before = tokens_[open - 1];
    const bool literal = before.kind == TokenKind::IntegerLiteral ||
                         before.kind == TokenKind::FloatLiteral;
    const bool operandEnd = literal || before.kind == TokenKind::Identifier ||
                            before.kind == TokenKind::RightParen;
    if (!operandEnd) {
        return;
    }
    // After an identifier the group is a call, with or without a space before
    // it (`Twice(N)` and `Twice (N)` mean the same); the analyzer explains the
    // step syntax when the identifier names a value.
    if (before.kind != TokenKind::Identifier) {
        throw ParseError(kOldForStepMessage, tokens_[open].location);
    }
}

std::vector<ast::ExpressionPtr> Parser::parseArgumentList()
{
    std::vector<ast::ExpressionPtr> arguments;

    if (check(TokenKind::RightParen)) {
        return arguments;
    }

    do {
        arguments.push_back(parseArgument());
    } while (match(TokenKind::Comma));

    return arguments;
}

ast::StatementPtr Parser::parseTypedLocalStatement()
{
    std::vector<std::string> names;
    const lexer::Token& name = consume(TokenKind::Identifier, "expected variable name");
    names.push_back(name.lexeme);
    while (match(TokenKind::Comma)) {
        const lexer::Token& nextName = consume(TokenKind::Identifier, "expected variable name after ','");
        names.push_back(nextName.lexeme);
    }

    const lexer::Token& type = consume(TokenKind::Identifier, "expected type name");

    std::vector<ast::StatementPtr> declarations;
    if (match(TokenKind::ColonEqual)) {
        // CANON-5 grouped declaration (v3.26): `A, B T := X` evaluates X exactly
        // once. The first name receives X; every later name is initialized from
        // the first, so side effects in X happen once.
        ast::ExpressionPtr initializer = parseValue();
        declarations.push_back(std::make_unique<ast::VarStatement>(
            false, names.front(), std::move(initializer), type.lexeme));
        for (std::size_t i = 1; i < names.size(); ++i) {
            declarations.push_back(std::make_unique<ast::VarStatement>(
                false, names[i], makeSyntheticIdentifier(names.front()),
                type.lexeme));
        }
    } else {
        for (const std::string& declarationName : names) {
            declarations.push_back(std::make_unique<ast::VarStatement>(
                false, declarationName, nullptr, type.lexeme));
        }
    }

    if (declarations.size() == 1) {
        return std::move(declarations.front());
    }

    return std::make_unique<ast::VarBlockStatement>(std::move(declarations));
}

ast::StatementPtr Parser::parseIfStatement()
{
    auto condition = parseValue();
    requireHeaderLineBreak();
    auto thenBody = parseDelimitedBody({"elif", "else"});

    std::vector<ast::ElseIfClause> elseIfClauses;
    while (matchKeyword("elif")) {
        auto elseIfCondition = parseValue();
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
    auto condition = parseValue();
    auto body = parseHeaderDelimitedBlock();
    return std::make_unique<ast::UnlessStatement>(
        std::move(condition), std::move(body));
}

ast::StatementPtr Parser::parseWithStatement()
{
    auto target = parseValue();
    requireHeaderLineBreak();

    const std::string bindingName = "__with_" + std::to_string(withCounter_++);
    withTargetStack_.push_back(bindingName);

    auto body = parseBlockBody();
    consumeBlockClose();

    withTargetStack_.pop_back();

    return std::make_unique<ast::WithStatement>(
        std::move(target), std::move(body), bindingName);
}

ast::StatementPtr Parser::parseWhileStatement()
{
    auto condition = parseValue();
    auto body = parseHeaderDelimitedBlock();
    return std::make_unique<ast::WhileStatement>(
        std::move(condition), std::move(body));
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
    return std::make_unique<ast::UntilStatement>(parseValue());
}

ast::StatementPtr Parser::parseForInStatement()
{
    const lexer::Token& iterator = consume(TokenKind::Identifier, "expected loop iterator");
    if (!matchKeyword("in")) {
        errorAtCurrent("expected 'in' after loop iterator");
    }

    // ADR-0010: `for I in A..B step S`. The bounds are full range-level
    // expressions (CANON-20 level 10); `step` is a reserved word of the header.
    rejectOldForStep();
    const lexer::SourceLocation iterableLocation = peek().location;
    auto iterable = parseRange();
    const bool isRange =
        iterable->kind() == ast::AstNodeKind::BinaryExpression &&
        static_cast<const ast::BinaryExpression&>(*iterable).op() == ast::BinaryOperator::Range;
    if (!isRange) {
        throw ParseError("a for loop iterates over a range 'A..B' (ADR-0010)", iterableLocation);
    }
    ast::ExpressionPtr step;
    if (matchKeyword("step")) {
        step = parseValue();
    }

    auto body = parseHeaderDelimitedBlock();
    return std::make_unique<ast::ForInStatement>(
        iterator.lexeme,
        std::move(iterable),
        std::move(step),
        std::move(body));
}

// ADR-0011 (Ada/SPARK-style case, adapted to Inox):
//   case Selector
//       1 | 2          body
//       3..9           body
//       otherwise      body
//   ;
// `|` separates the alternatives of an arm; a choice is a static value or a
// static range `A..B`. `otherwise` is at most one and is the last arm.
ast::ExpressionPtr Parser::parseCaseChoice()
{
    // A choice ends at its line: the arm body on the next line never extends it.
    FlagScope scope(headerExpression_);
    auto choice = parseBitOr();
    if (match(TokenKind::DotDot)) {
        const lexer::Token& op = previous();
        auto upper = parseBitOr();
        choice = makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(op), std::move(choice), std::move(upper));
        if (check(TokenKind::DotDot)) {
            throw ParseError(
                "'..' is non-associative (CANON-20): a range has exactly two "
                "bounds, so 'A..B..C' has no meaning",
                peek().location);
        }
    }
    return choice;
}

// While a header expression is parsed (the case selector), a token on a later
// line does not continue it: `case X` followed by an arm `-1 ...` or `(A) ...`
// must not read as `X - 1` or `X(A)`.
bool Parser::atCompoundAssignment() const
{
    return check(TokenKind::PlusEqual) || check(TokenKind::MinusEqual) ||
           check(TokenKind::StarEqual) || check(TokenKind::SlashEqual) ||
           check(TokenKind::CaretEqual);
}

bool Parser::atLineBreakInHeader() const
{
    return headerExpression_ && current_ > 0 &&
           peek().location.line > previous().location.line;
}

ast::StatementPtr Parser::parseCaseStatement()
{
    ast::ExpressionPtr expression;
    {
        FlagScope scope(headerExpression_);
        expression = parseValue();
    }
    requireHeaderLineBreak();

    std::vector<ast::CaseArm> arms;
    std::vector<ast::StatementPtr> otherwiseBody;
    bool hasOtherwise = false;

    while (!isAtEnd() && !check(TokenKind::Semicolon)) {
        if (checkKeyword("otherwise")) {
            if (hasOtherwise) {
                errorAtCurrent("a case has at most one 'otherwise' (ADR-0011)");
            }
            advance();
            const std::size_t armLine = previous().location.line;
            const std::size_t armColumn = previous().location.column;
            otherwiseBody = parseCaseArmBody(armLine, armColumn);
            hasOtherwise = true;
            continue;
        }
        if (hasOtherwise) {
            errorAtCurrent("'otherwise' must be the last arm of a case (ADR-0011)");
        }

        std::vector<ast::ExpressionPtr> choices;
        const std::size_t armLine = peek().location.line;
        const std::size_t armColumn = peek().location.column;
        choices.push_back(parseCaseChoice());
        while (match(TokenKind::Pipe)) {
            choices.push_back(parseCaseChoice());
        }
        if (check(TokenKind::Comma)) {
            errorAtCurrent(
                "case alternatives are separated by '|' (ADR-0011): write "
                "'1 | 2', not '1, 2'");
        }

        auto body = parseCaseArmBody(armLine, armColumn);
        arms.push_back(ast::CaseArm{std::move(choices), std::move(body)});
    }

    consumeBlockClose();
    return std::make_unique<ast::CaseStatement>(
        std::move(expression),
        std::move(arms),
        std::move(otherwiseBody),
        hasOtherwise);
}

std::vector<ast::StatementPtr> Parser::parseCaseArmBody(std::size_t armLine, std::size_t armColumn)
{
    std::vector<ast::StatementPtr> statements;

    if (!isAtEnd() && !check(TokenKind::Semicolon) && peek().location.line == armLine) {
        appendStatement(statements);
        return statements;
    }

    while (!isAtEnd() && !check(TokenKind::Semicolon) && !checkKeyword("otherwise")) {
        if (!statements.empty() && peek().location.column <= armColumn) {
            break;
        }
        appendStatement(statements);
    }

    return statements;
}

ast::StatementPtr Parser::parseTryStatement()
{
    requireHeaderLineBreak();
    auto body = parseDelimitedBody({"except", "ensure"});

    bool hasExcept = false;
    bool plainExcept = false;
    std::vector<ast::StatementPtr> exceptBody;
    std::vector<ast::ExceptionHandler> handlers;
    std::vector<ast::StatementPtr> elseBody;

    if (matchKeyword("except")) {
        hasExcept = true;
        requireHeaderLineBreak();

        if (checkKeyword("on")) {
            while (matchKeyword("on")) {
                handlers.push_back(parseExceptionHandler());
            }

            if (matchKeyword("else")) {
                requireHeaderLineBreak();
                elseBody = parseBlockBody();
                consumeBlockClose();
            }
        } else {
            plainExcept = true;
            exceptBody = parseDelimitedBody({"ensure"});
        }
    }

    bool hasEnsure = false;
    std::vector<ast::StatementPtr> ensureBody;
    if (matchKeyword("ensure")) {
        hasEnsure = true;
        requireHeaderLineBreak();
        ensureBody = parseDelimitedBody({});
    }

    if (!hasExcept && !hasEnsure) {
        errorAtCurrent("try requires 'except' or 'ensure'");
    }

    consumeBlockClose();

    return std::make_unique<ast::TryStatement>(
        std::move(body),
        hasExcept,
        plainExcept,
        std::move(exceptBody),
        std::move(handlers),
        std::move(elseBody),
        hasEnsure,
        std::move(ensureBody));
}

ast::ExceptionHandler Parser::parseExceptionHandler()
{
    std::string bindingName;
    std::string typeName;

    const lexer::Token& first = consumeIdentifierLike(
        "expected exception type or handler name after 'On'");

    if (check(TokenKind::Colon)) {
        errorAtCurrent("exception handlers no longer use ':'; write 'On Name ExceptionType'");
    }

    if (checkIdentifierLike() && peek().location.line == first.location.line) {
        bindingName = first.lexeme;
        typeName = consumeIdentifierLike("expected exception type after handler binding").lexeme;
    } else {
        typeName = first.lexeme;
    }

    if (checkKeyword("do")) {
        errorAtCurrent("exception handlers do not use 'Do'; start the handler body on the next line");
    }

    requireHeaderLineBreak();
    auto body = parseBlockBody();
    consumeBlockClose();

    return ast::ExceptionHandler{
        std::move(bindingName),
        std::move(typeName),
        std::move(body)};
}

ast::StatementPtr Parser::parseRaiseStatement()
{
    ast::ExpressionPtr expression;
    if (!atStatementBoundary()) {
        expression = parseValue();
    }
    return std::make_unique<ast::RaiseStatement>(std::move(expression));
}

ast::StatementPtr Parser::parseRetryStatement()
{
    consume(TokenKind::LeftParen, "expected '(' after 'Retry'");
    if (check(TokenKind::RightParen)) {
        errorAtCurrent("Retry requires a retry-count expression");
    }
    auto count = parseValue();
    consume(TokenKind::RightParen, "expected ')' after Retry count");
    return std::make_unique<ast::RetryStatement>(std::move(count));
}

ast::StatementPtr Parser::parseReturnStatement()
{
    if (atStatementBoundary()) {
        errorAtCurrent("expected expression after 'return'");
    }
    return std::make_unique<ast::ReturnStatement>(parseValue());
}

// An expression statement is a call or an assignment. Assignment may be
// chained (CANON-5, v3.27): `A := B := C := X` evaluates X once and stores it
// in C, then B, then A. The chain is desugared here into
// `C := X`, `B := C`, `A := B`; the extra statements go to pendingStatements_.
ast::StatementPtr Parser::parseExpressionStatement()
{
    DepthGuard guard(*this, expressionNesting_, kMaxExpressionNesting, "expression");
    std::vector<ast::ExpressionPtr> parts;
    std::vector<lexer::Token> operators;
    parts.push_back(parseOr());

    // ADR-0012: `L op= R` is `L := L op R` with L evaluated once. L is a
    // variable or a field path, which has no side effects, so storing the
    // desugared form is exact. Index targets wait for arrays (their lvalue
    // must be evaluated once by the lowering, not duplicated here).
    if (atCompoundAssignment()) {
        const lexer::Token& op = advance();
        if (!isChainTarget(*parts.front())) {
            throw ParseError(
                "a compound assignment target must be a variable or a field (ADR-0012)",
                op.location);
        }
        auto value = parseValue();
        if (atCompoundAssignment() || check(TokenKind::ColonEqual)) {
            errorAtCurrent("a compound assignment cannot be chained (ADR-0012)");
        }
        ast::BinaryOperator binary = ast::BinaryOperator::Add;
        switch (op.kind) {
        case TokenKind::MinusEqual: binary = ast::BinaryOperator::Subtract; break;
        case TokenKind::StarEqual: binary = ast::BinaryOperator::Multiply; break;
        case TokenKind::SlashEqual: binary = ast::BinaryOperator::Divide; break;
        case TokenKind::CaretEqual: binary = ast::BinaryOperator::Power; break;
        default: break;
        }
        auto current = cloneChainTarget(*parts.front());
        auto combined = makeExpr<ast::BinaryExpression>(binary, std::move(current), std::move(value));
        return std::make_unique<ast::ExpressionStatement>(
            makeExpr<ast::BinaryExpression>(
                ast::BinaryOperator::Assign, std::move(parts.front()), std::move(combined)),
            true);
    }

    while (match(TokenKind::ColonEqual)) {
        operators.push_back(previous());
        parts.push_back(parseOr());
    }
    if (atCompoundAssignment()) {
        errorAtCurrent("a compound assignment cannot be chained (ADR-0012)");
    }

    if (parts.size() == 1) {
        return std::make_unique<ast::ExpressionStatement>(std::move(parts.front()));
    }
    if (parts.size() == 2) {
        return std::make_unique<ast::ExpressionStatement>(makeExpr<ast::BinaryExpression>(
            binaryOperatorFor(operators.front()), std::move(parts[0]), std::move(parts[1])));
    }

    for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
        if (!isChainTarget(*parts[i])) {
            throw ParseError(
                "a chained assignment target must be a variable or a field "
                "(CANON-5)",
                operators[i].location);
        }
    }

    // Copies of the targets are taken first: target i + 1 is the source of
    // target i.
    const std::size_t last = parts.size() - 1;
    std::vector<ast::ExpressionPtr> sources(last);
    sources[last - 1] = std::move(parts[last]);
    for (std::size_t i = 0; i + 1 < last; ++i) {
        sources[i] = cloneChainTarget(*parts[i + 1]);
    }

    std::vector<ast::StatementPtr> chain;
    for (std::size_t i = last; i-- > 0;) {
        chain.push_back(std::make_unique<ast::ExpressionStatement>(
            makeExpr<ast::BinaryExpression>(
                binaryOperatorFor(operators[i]), std::move(parts[i]), std::move(sources[i]))));
    }
    ast::StatementPtr first = std::move(chain.front());
    for (std::size_t i = 1; i < chain.size(); ++i) {
        pendingStatements_.push_back(std::move(chain[i]));
    }
    return first;
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

    if (checkKeyword("var")) {
        errorAtCurrent(kVarRemovedMessage);
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
    path.push_back(parseQualifiedName("expected module name after 'Use'"));
    return std::make_unique<ast::UseDeclaration>(std::move(path));
}

ast::AstNodePtr Parser::parseSectionDeclaration(ast::SectionKind sectionKind)
{
    std::vector<std::string> tokens;
    std::vector<std::size_t> lines;

    std::vector<ast::SectionInitializer> initializers;

    // Section contents stay token lists for the declaration shapes, but the
    // initializer of a Const or State declaration is parsed as a real
    // expression (ADR-0009: `:=` never appears in it; CANON-9: named
    // construction is checked by semantic analysis). Its tokens are still
    // recorded so the token-based consumers see the same list as before.
    const bool parseInitializers = sectionKind != ast::SectionKind::Type;
    std::size_t declarationStart = 0;
    std::size_t declarationLine = 0;
    const auto take = [&]() {
        const lexer::Token& token = advance();
        if (token.location.line != declarationLine) {
            declarationLine = token.location.line;
            declarationStart = tokens.size();
        }
        tokens.push_back(tokenText(token));
        lines.push_back(token.location.line);
        if (!parseInitializers || token.kind != TokenKind::ColonEqual) {
            return;
        }
        // Name := Expr | Name Type := Expr | legacy Name : Type := Expr
        const std::size_t head = tokens.size() - 1 - declarationStart;
        if (head == 0) {
            throw ParseError("expected a name before ':='", token.location);
        }
        ast::SectionInitializer initializer;
        initializer.name = tokens[declarationStart];
        if (head == 2) {
            initializer.typeName = tokens[declarationStart + 1];
        } else if (head == 3 && tokens[declarationStart + 1] == ":") {
            initializer.typeName = tokens[declarationStart + 2];
        } else if (head != 1) {
            throw ParseError(kAssignmentInExpressionMessage, token.location);
        }
        const std::size_t valueStart = current_;
        initializer.value = parseValue();
        if (!isAtEnd() && !check(TokenKind::Semicolon) &&
            peek().location.line == previous().location.line) {
            errorAtCurrent("expected line break after a section declaration");
        }
        // The whole initializer is recorded on the declaration's line, even
        // when it spans lines, so the token scanners skip it as one unit.
        for (std::size_t i = valueStart; i < current_; ++i) {
            tokens.push_back(tokenText(tokens_[i]));
            lines.push_back(token.location.line);
        }
        initializers.push_back(std::move(initializer));
    };

    // Type is always a section/declarator without ':'.
    // Const supports the canonical single-line form `Const Name := Expr` (CANON-5),
    // which has no ':'. When a ':' is present, Const falls through to the block form.
    const bool lineForm =
        (sectionKind == ast::SectionKind::Type && !check(TokenKind::Colon)) ||
        (sectionKind == ast::SectionKind::Const && !check(TokenKind::Colon));

    if (lineForm) {
        if (sectionKind == ast::SectionKind::Const) {
            // Single-line `Const Name := Expr`: consume tokens up to the end of
            // the current source line (the declaration ends at the newline).
            const std::size_t line = isAtEnd() ? 0 : peek().location.line;
            while (!isAtEnd() && peek().location.line == line &&
                   !check(TokenKind::Semicolon)) {
                take();
            }
        } else {
            while (!isAtEnd() && !atTypeSectionBoundary()) {
                take();
            }
        }
        return std::make_unique<ast::SectionDeclaration>(
            sectionKind, std::move(tokens), std::move(lines), std::move(initializers));
    }

    consume(TokenKind::Colon, "expected ':' after section header");

    while (!isAtEnd() && !check(TokenKind::Semicolon)) {
        take();
    }

    consumeBlockClose();
    return std::make_unique<ast::SectionDeclaration>(
        sectionKind, std::move(tokens), std::move(lines), std::move(initializers));
}

ast::AstNodePtr Parser::parseRawDeclaration()
{
    const lexer::Token& head = advance();
    std::vector<std::string> tokens;

    while (!isAtEnd() && !check(TokenKind::Semicolon)) {
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
    std::string functionName = name.lexeme;
    if (match(TokenKind::Dot)) {
        const lexer::Token& methodName = consume(
            TokenKind::Identifier, "expected method name after '.'");
        functionName += ".";
        functionName += methodName.lexeme;
    }

    std::vector<std::string> signatureTokens;

    while (!isAtEnd() && !check(TokenKind::Colon)) {
        signatureTokens.push_back(tokenText(advance()));
    }

    if (signatureTokens.size() >= 2 && signatureTokens[0] == "(" && signatureTokens[1] == ")") {
        errorAt(name, "empty parentheses are not allowed in declarations; omit parentheses when there are no parameters");
    }

    consume(TokenKind::Colon, "expected ':' after function signature");
    auto body = parseBlockBody();
    consumeBlockClose();

    return std::make_unique<ast::FunctionDeclaration>(
        std::move(functionName),
        std::move(signatureTokens),
        std::move(body));
}

std::string Parser::parseQualifiedName(std::string_view message)
{
    std::string name = consumeIdentifierLike(message).lexeme;
    while (match(TokenKind::Dot)) {
        name += ".";
        name += consume(TokenKind::Identifier, "expected name after '.'").lexeme;
    }
    return name;
}

std::vector<ast::StatementPtr> Parser::parseBlockBody()
{
    std::vector<ast::StatementPtr> statements;
    while (!isAtEnd() && !check(TokenKind::Semicolon)) {
        appendStatement(statements);
    }
    return statements;
}

std::vector<ast::StatementPtr> Parser::parseDelimitedBody(std::initializer_list<std::string_view> stopKeywords)
{
    std::vector<ast::StatementPtr> statements;
    while (!isAtEnd() && !check(TokenKind::Semicolon) &&
           !atAnyKeyword(stopKeywords)) {
        appendStatement(statements);
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
           checkKeyword("elif") ||
           checkKeyword("else") ||
           checkKeyword("except") ||
           checkKeyword("on") ||
           checkKeyword("ensure") ||
           checkKeyword("until") ||
           checkKeyword("otherwise");
}

bool Parser::atTypeSectionBoundary() const
{
    if (isAtEnd()) {
        return true;
    }

    if (checkKeyword("use") || checkKeyword("type") || checkKeyword("const") ||
        checkKeyword("state") || checkKeyword("var")) {
        return true;
    }

    if (!checkIdentifierLike()) {
        return false;
    }

    // A declaration header (`Name ... :`) lives on a single source line, so the
    // look-ahead never leaves the current line. This keeps large Type sections
    // linear instead of quadratic.
    const std::size_t headerLine = tokens_[current_].location.line;
    for (std::size_t index = current_ + 1; index < tokens_.size(); ++index) {
        if (tokens_[index].kind == TokenKind::Semicolon ||
            tokens_[index].kind == TokenKind::EndOfFile ||
            tokens_[index].location.line != headerLine) {
            return false;
        }
        if (tokens_[index].kind == TokenKind::Colon) {
            return true;
        }
    }

    return false;
}
bool Parser::atTypedLocalStatementStart() const
{
    if (current_ >= tokens_.size()) {
        return false;
    }

    const lexer::Token& name = tokens_[current_];
    if (name.kind != TokenKind::Identifier) {
        return false;
    }

    std::size_t index = current_;
    const std::size_t line = name.location.line;
    for (;;) {
        if (index >= tokens_.size() ||
            tokens_[index].kind != TokenKind::Identifier ||
            tokens_[index].location.line != line) {
            return false;
        }

        ++index;
        if (index < tokens_.size() &&
            tokens_[index].kind == TokenKind::Comma &&
            tokens_[index].location.line == line) {
            ++index;
            continue;
        }
        break;
    }

    if (index >= tokens_.size() ||
        tokens_[index].kind != TokenKind::Identifier ||
        tokens_[index].location.line != line) {
        return false;
    }

    const lexer::Token& type = tokens_[index];
    ++index;
    if (index >= tokens_.size()) {
        return true;
    }

    const lexer::Token& afterType = tokens_[index];
    return afterType.kind == TokenKind::ColonEqual ||
           afterType.kind == TokenKind::Semicolon ||
           afterType.kind == TokenKind::EndOfFile ||
           afterType.location.line > type.location.line;
}


void Parser::requireHeaderLineBreak()
{
    if (isAtEnd() || peek().location.line <= previous().location.line) {
        errorAtCurrent("expected line break after conditional header");
    }
}

void Parser::consumeBlockClose()
{
    if (match(TokenKind::Semicolon)) {
        // `;` closes a block; whatever follows it must start on a new line.
        if (!isAtEnd() && !check(TokenKind::Semicolon) &&
            peek().location.line == previous().location.line) {
            errorAtCurrent("expected line break after block close ';'");
        }
        return;
    }
    errorAtCurrent("expected block close ';'");
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
    // A numeric literal's value drops its digit separators (CANON-2): later
    // phases see `1000000` for `1_000_000`.
    if (token.kind == TokenKind::IntegerLiteral || token.kind == TokenKind::FloatLiteral) {
        std::string digits;
        for (const char ch : token.lexeme) {
            if (ch != '_') {
                digits += ch;
            }
        }
        return digits;
    }
    if (!token.lexeme.empty()) {
        return token.lexeme;
    }
    return token.normalized;
}

} // namespace inox::compiler::parser
