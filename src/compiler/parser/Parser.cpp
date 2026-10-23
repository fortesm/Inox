// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Parser.h"
#include "../ast/AstClone.h"
#include "../types/TypeSyntax.h"

#include "../lexer/Lexer.h"

#include <cctype>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace inox::compiler::parser {

namespace {

// The words that open a pointer type (change safe-pointers), lower case.
bool isPointerTypeWord(std::string_view normalized)
{
    return normalized == "own" || normalized == "link" || normalized == "option";
}

constexpr const char* kDiscardOnlyMessage =
    "'_' is not a name: it only discards the result of a call, _ := F(X) (r3.93)";

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
    explicit FlagScope(bool& flag, bool value = true) : flag_(flag), saved_(flag) { flag_ = value; }
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

bool isIndexCall(const ast::Expression& expression)
{
    if (expression.kind() != ast::AstNodeKind::CallExpression) {
        return false;
    }
    const auto& call = static_cast<const ast::CallExpression&>(expression);
    return call.callee().kind() == ast::AstNodeKind::IdentifierExpression &&
           static_cast<const ast::IdentifierExpression&>(call.callee()).name() == "__index" &&
           call.arguments().size() == 2;
}

// An expression without side effects: literals, names, operators, field and
// index reads. Evaluating it twice gives the same value.
bool isPureExpression(const ast::Expression& expression)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression:
    case ast::AstNodeKind::IdentifierExpression:
        return true;
    case ast::AstNodeKind::UnaryExpression:
        return isPureExpression(static_cast<const ast::UnaryExpression&>(expression).operand());
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        return binary.op() != ast::BinaryOperator::Assign && isPureExpression(binary.left()) &&
               isPureExpression(binary.right());
    }
    case ast::AstNodeKind::CallExpression: {
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
            return false;
        }
        const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
        if (name != "__index" && name != "__member") {
            return false;
        }
        for (const auto& argument : call.arguments()) {
            if (!isPureExpression(*argument)) {
                return false;
            }
        }
        return true;
    }
    default:
        return false;
    }
}

ast::ExpressionPtr clonePureExpression(const ast::Expression& expression)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression: {
        const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
        return std::make_unique<ast::LiteralExpression>(literal.literalKind(), literal.value());
    }
    case ast::AstNodeKind::IdentifierExpression:
        return makeSyntheticIdentifier(static_cast<const ast::IdentifierExpression&>(expression).name());
    case ast::AstNodeKind::UnaryExpression: {
        const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
        return std::make_unique<ast::UnaryExpression>(unary.op(), clonePureExpression(unary.operand()));
    }
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        return std::make_unique<ast::BinaryExpression>(
            binary.op(), clonePureExpression(binary.left()), clonePureExpression(binary.right()));
    }
    default: {
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        std::vector<ast::ExpressionPtr> arguments;
        for (const auto& argument : call.arguments()) {
            arguments.push_back(clonePureExpression(*argument));
        }
        return std::make_unique<ast::CallExpression>(
            makeSyntheticIdentifier(static_cast<const ast::IdentifierExpression&>(call.callee()).name()),
            std::move(arguments));
    }
    }
}

// A compound-assignment target (ADR-0012): a chain target, or an array
// element `A[I]` whose base and indices have no side effects, so the
// desugared `A[I] := A[I] op R` reads and writes the same element.
bool isCompoundTarget(const ast::Expression& expression)
{
    if (isChainTarget(expression)) {
        return true;
    }
    return isIndexCall(expression) && isPureExpression(expression);
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

// CANON-3 (r3.76): a file is a module named after the file; the driver
// passes the name. There is no `Module` header.
std::unique_ptr<ast::ModuleNode> Parser::parseModule(std::string name)
{
    auto module = std::make_unique<ast::ModuleNode>(std::move(name));

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

    if (atContractWord("pre") || atContractWord("pos")) {
        errorAtCurrent("'pre' and 'pos' go on the first lines of a routine, before its statements (CANON-24)");
    }
    if (atContractWord("invariant") || atContractWord("variant")) {
        errorAtCurrent("'invariant' and 'variant' go on the first lines of a loop, or 'invariant' in a "
                       "Struct (CANON-24)");
    }

    // CANON-2: `Old := 1`: a reserved word never names a variable.
    if (check(TokenKind::Keyword) && current_ + 1 < tokens_.size() &&
        tokens_[current_ + 1].kind == TokenKind::ColonEqual) {
        errorAtCurrent(peek().lexeme + " is a reserved word of Inox and cannot be declared (CANON-2)");
    }
    // CANON-2 (r3.74): `True := 5`, `Exit Integer := 1`: a predefined name
    // is never declared or assigned.
    if (check(TokenKind::Identifier) && lexer::Lexer::isPredefinedName(peek().normalized) &&
        current_ + 1 < tokens_.size() &&
        tokens_[current_ + 1].location.line == peek().location.line &&
        (tokens_[current_ + 1].kind == TokenKind::ColonEqual ||
         tokens_[current_ + 1].kind == TokenKind::Identifier)) {
        errorAtCurrent(peek().lexeme + " is a predefined name of Inox and cannot be declared (CANON-2)");
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

    if (check(TokenKind::Underscore)) {
        return endSimpleStatement(parseDiscardStatement());
    }

    if (atTypedLocalStatementStart()) {
        return endSimpleStatement(parseTypedLocalStatement());
    }

    return endSimpleStatement(parseExpressionStatement());
}

void Parser::appendStatement(std::vector<ast::StatementPtr>& statements)
{
    ast::StatementPtr statement = parseStatement();
    for (auto& preceding : precedingStatements_) {
        statements.push_back(std::move(preceding));
    }
    precedingStatements_.clear();
    statements.push_back(std::move(statement));
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
    rejectWrittenTypeArguments();
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

// CANON-8 (r3.77): type arguments are inferred, never written in an
// expression. `Max<Integer>(3, 7)` would read as the comparisons `Max <
// Integer > (3, 7)`; when a comma shows it cannot be (inside `<...>` or in the
// parentheses after it), the reader gets the rule instead of a parse error.
// `Max<Integer>(3)` reads as comparisons; the analyzer names the rule there.
void Parser::rejectWrittenTypeArguments() const
{
    if (current_ + 2 >= tokens_.size() || tokens_[current_].kind != TokenKind::Identifier ||
        tokens_[current_ + 1].kind != TokenKind::Less) {
        return;
    }
    const std::size_t line = tokens_[current_].location.line;
    std::size_t index = current_ + 1;
    int depth = 0;
    bool comma = false;
    for (; index < tokens_.size() && tokens_[index].location.line == line; ++index) {
        const TokenKind kind = tokens_[index].kind;
        if (kind == TokenKind::Less || kind == TokenKind::LeftBracket) {
            ++depth;
        } else if (kind == TokenKind::Greater || kind == TokenKind::RightBracket) {
            if (--depth == 0) {
                break;
            }
        } else if (kind == TokenKind::Comma) {
            comma = comma || depth == 1;
        } else if (kind != TokenKind::Identifier && kind != TokenKind::IntegerLiteral && kind != TokenKind::DotDot) {
            return;  // not the shape of type arguments
        }
    }
    if (index >= tokens_.size() || depth != 0 || tokens_[index].kind != TokenKind::Greater) {
        return;
    }
    ++index;
    if (index < tokens_.size() && tokens_[index].kind == TokenKind::LeftParen && tokens_[index].location.line == line) {
        int parens = 0;
        for (; index < tokens_.size() && tokens_[index].location.line == line; ++index) {
            const TokenKind kind = tokens_[index].kind;
            parens += kind == TokenKind::LeftParen ? 1 : kind == TokenKind::RightParen ? -1 : 0;
            if (parens == 0) {
                break;
            }
            comma = comma || (kind == TokenKind::Comma && parens == 1);
        }
    }
    if (comma) {
        const std::string& name = tokens_[current_].lexeme;
        throw ParseError("type arguments are not written in an expression (CANON-8): they come from the "
                         "arguments, " + name + "(...), or from the type of the target",
                         tokens_[current_ + 1].location);
    }
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
    // CANON-24: `old X` in a `pos` clause is X as it was when the routine
    // started; it binds to one operand (`old Count + 1` is `(old Count) + 1`).
    if (inPosClause_ && checkKeyword("old") &&
        current_ + 1 < tokens_.size() && tokens_[current_ + 1].location.line == peek().location.line &&
        (tokens_[current_ + 1].kind == TokenKind::Identifier || tokens_[current_ + 1].kind == TokenKind::LeftParen)) {
        advance();
        if (checkKeyword("old")) {
            errorAtCurrent("'old old X' is not Inox (CANON-24): old is taken once, on entry");
        }
        auto operand = parsePostfix();
        return makeExpr<ast::UnaryExpression>(ast::UnaryOperator::Old, std::move(operand));
    }
    if (inContractClause_ && checkKeyword("old") &&
        current_ + 1 < tokens_.size() && tokens_[current_ + 1].location.line == peek().location.line &&
        tokens_[current_ + 1].kind == TokenKind::Identifier) {
        errorAtCurrent("'old' is valid only in a 'pos' clause: a precondition, an invariant or a variant "
                       "is checked now (CANON-24)");
    }
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
            // CANON-13: `M[I, J]` indexes a multi-dimensional array, which is
            // an array of arrays, so it is `M[I][J]`. ADR-0017: `A[L..H]`,
            // `A[..H]`, `A[L..]` and `A[..]` are slices; an omitted bound is
            // the base's own Low or High (`__low`/`__high`).
            bool first = true;
            do {
                ast::ExpressionPtr low;
                if (!check(TokenKind::DotDot)) {
                    low = parseBitOr();
                }
                if (match(TokenKind::DotDot)) {
                    if (!first || check(TokenKind::Comma)) {
                        errorAtCurrent("a slice takes one range: write A[L..H] (ADR-0017)");
                    }
                    ast::ExpressionPtr high = check(TokenKind::RightBracket)
                        ? makeSyntheticIdentifier("__high")
                        : parseBitOr();
                    std::vector<ast::ExpressionPtr> arguments;
                    arguments.push_back(std::move(expression));
                    arguments.push_back(low ? std::move(low) : makeSyntheticIdentifier("__low"));
                    arguments.push_back(std::move(high));
                    expression = makeExpr<ast::CallExpression>(
                        makeSyntheticIdentifier("__slice"), std::move(arguments));
                    break;
                }
                if (!low) {
                    errorAtCurrent("expected index expression");
                }
                std::vector<ast::ExpressionPtr> arguments;
                arguments.push_back(std::move(expression));
                arguments.push_back(std::move(low));
                expression = makeExpr<ast::CallExpression>(
                    makeSyntheticIdentifier("__index"), std::move(arguments));
                first = false;
            } while (match(TokenKind::Comma));
            consume(TokenKind::RightBracket, "expected ']' after index expression");
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
    if (check(TokenKind::Underscore)) {
        errorAtCurrent(kDiscardOnlyMessage);
    }
    // CANON-12 (r3.76): the conditional expression is `IIf(C, A, B)`; only
    // the chosen branch is evaluated. `if C then A else B` is gone.
    if (checkKeyword("if")) {
        errorAtCurrent("the conditional expression is IIf(Condition, A, B) (r3.76); "
                       "'if' starts a statement");
    }
    if (checkKeyword("iif") && current_ + 1 < tokens_.size() &&
        tokens_[current_ + 1].kind == TokenKind::LeftParen) {
        advance();
        advance();
        if (check(TokenKind::RightParen)) {
            errorAtCurrent("IIf needs a condition and two values: IIf(Condition, A, B) (CANON-12)");
        }
        auto condition = parseValue();
        consume(TokenKind::Comma, "IIf needs a condition and two values: IIf(Condition, A, B) (CANON-12)");
        auto thenValue = parseValue();
        consume(TokenKind::Comma, "IIf needs a condition and two values: IIf(Condition, A, B) (CANON-12)");
        auto elseValue = parseValue();
        consume(TokenKind::RightParen, "IIf takes exactly three arguments: IIf(Condition, A, B) (CANON-12)");
        return makeExpr<ast::ConditionalExpression>(
            std::move(condition), std::move(thenValue), std::move(elseValue));
    }
    if (checkKeyword("iif")) {
        errorAtCurrent("IIf is the conditional expression: IIf(Condition, A, B) (CANON-12)");
    }

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

    // r3.81: the set function `With(S, X)` became `Include(S, X)`; `with`
    // only starts a statement.
    if (checkKeyword("with") && current_ + 1 < tokens_.size() &&
        tokens_[current_ + 1].kind == TokenKind::LeftParen) {
        errorAtCurrent("the set function With(S, X) is now Include(S, X), and Without(S, X) is Exclude(S, X) "
                       "(r3.81); 'with' starts a statement");
    }

    if (match(TokenKind::LeftParen)) {
        auto expression = parseValue();
        consume(TokenKind::RightParen, "expected ')' after expression");
        markParenthesized(expression.get());
        return expression;
    }

    // CANON-13: `[Club, Heart]` is a set literal, `[]` the empty set; the
    // analyzer gives it the Set type its place expects.
    if (match(TokenKind::LeftBracket)) {
        std::vector<ast::ExpressionPtr> elements;
        if (!check(TokenKind::RightBracket)) {
            do {
                elements.push_back(parseValue());
            } while (match(TokenKind::Comma));
        }
        consume(TokenKind::RightBracket, "expected ']' after the elements of a set literal");
        return makeExpr<ast::CallExpression>(makeSyntheticIdentifier("__set"), std::move(elements));
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

// A type name: an identifier, or a fixed array `Array[L..H, ...] Element`
// (CANON-13). A bound is an integer literal, optionally negative, or a Const
// name. The result is the front end's array spelling (types/TypeSyntax.h).
std::string Parser::parseTypeName(const char* message)
{
    const lexer::Token& name = consume(TokenKind::Identifier, message);
    // ADR-0017: `Slice<T>`, a read-only view (generic arguments use <...>).
    if (name.normalized == "slice" && check(TokenKind::Less) &&
        peek().location.line == name.location.line) {
        advance();
        std::string element = parseTypeName("expected the element type of the slice");
        consume(TokenKind::Greater, "expected '>' after the slice element type");
        return "Slice<" + element + ">";
    }
    // Safe pointers (change safe-pointers): `Own<T>`, `Link<T>` and
    // `Option<Own<T>>`, spelled with their canonical capitals.
    if (isPointerTypeWord(name.normalized) && check(TokenKind::Less) &&
        peek().location.line == name.location.line) {
        advance();
        std::string target = parseTypeName("expected the type the pointer reaches");
        consume(TokenKind::Greater, "expected '>' after the type of the pointer");
        const std::string word = name.normalized == "own" ? "Own" : name.normalized == "link" ? "Link" : "Option";
        return word + "<" + target + ">";
    }
    // CANON-8 (r3.77): a generic struct of the program, `TPair<String, Integer>`.
    if (name.normalized != "array" && check(TokenKind::Less) && peek().location.line == name.location.line) {
        advance();
        std::string text = name.lexeme + "<";
        do {
            if (text.back() != '<') {
                text += ", ";
            }
            text += parseTypeName("expected a type argument");
        } while (match(TokenKind::Comma));
        consume(TokenKind::Greater, "expected '>' after the type arguments of " + name.lexeme);
        return text + ">";
    }
    if (!(name.normalized == "array") || !check(TokenKind::LeftBracket) ||
        peek().location.line != name.location.line) {
        return name.lexeme;
    }
    advance();
    const auto bound = [&]() {
        std::string text;
        if (match(TokenKind::Minus)) {
            text = "-";
        }
        if (match(TokenKind::IntegerLiteral)) {
            return text + tokenText(previous());
        }
        if (text.empty() && match(TokenKind::Identifier)) {
            return previous().lexeme;
        }
        errorAtCurrent("an array bound is an integer literal or a Const name (CANON-13)");
        return std::string{};
    };
    std::vector<std::pair<std::string, std::string>> dimensions;
    do {
        std::string low = bound();
        consume(TokenKind::DotDot, "expected '..' in array bounds");
        std::string high = bound();
        dimensions.emplace_back(std::move(low), std::move(high));
    } while (match(TokenKind::Comma));
    consume(TokenKind::RightBracket, "expected ']' after array bounds");
    if (peek().location.line != previous().location.line) {
        errorAtCurrent("expected the element type after the array bounds");
    }
    std::string element = parseTypeName("expected the element type after the array bounds");
    for (auto it = dimensions.rbegin(); it != dimensions.rend(); ++it) {
        element = types::spellArrayType(it->first, it->second, element);
    }
    return element;
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

    const std::string typeName = parseTypeName("expected type name");
    struct {
        std::string lexeme;
    } type{typeName};

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
    if (check(TokenKind::Identifier) && peek().normalized == "then") {
        errorAtCurrent("the if statement has no 'then' (CANON-4); for a value, write "
                       "IIf(Condition, A, B) (CANON-12)");
    }
    requireHeaderLineBreak();
    auto thenBody = parseDelimitedBody({"elif", "else"});

    std::vector<ast::ElseIfClause> elseIfClauses;
    while (matchKeyword("elif")) {
        auto elseIfCondition = parseValue();
        if (check(TokenKind::Identifier) && peek().normalized == "then") {
            errorAtCurrent("the if statement has no 'then' (CANON-4)");
        }
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
    requireHeaderLineBreak();
    auto contracts = parseLoopContracts(true);
    auto body = parseBlockBody();
    consumeBlockClose();
    lowerLoopContracts(std::move(contracts), body, true);
    return std::make_unique<ast::WhileStatement>(
        std::move(condition), std::move(body));
}

ast::StatementPtr Parser::parseRepeatStatement()
{
    auto contracts = parseLoopContracts(true);
    auto body = parseBlockBody();
    consumeBlockClose();
    lowerLoopContracts(std::move(contracts), body, true);
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

    requireHeaderLineBreak();
    auto contracts = parseLoopContracts(false);
    auto body = parseBlockBody();
    consumeBlockClose();
    lowerLoopContracts(std::move(contracts), body, false);
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
//       else           body
//   ;
// `|` separates the alternatives of an arm; a choice is a static value or a
// static range `A..B`. `else` is at most one and is the last arm.
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
    std::vector<ast::StatementPtr> elseBody;
    bool hasElse = false;

    while (!isAtEnd() && !check(TokenKind::Semicolon)) {
        if (check(TokenKind::Identifier) && peek().normalized == "otherwise" &&
            current_ + 1 < tokens_.size() && tokens_[current_ + 1].kind != TokenKind::Pipe &&
            tokens_[current_ + 1].kind != TokenKind::DotDot) {
            errorAtCurrent("the last arm of a case is 'else', not 'otherwise' (r3.74)");
        }
        if (checkKeyword("else")) {
            if (hasElse) {
                errorAtCurrent("a case has at most one 'else' (ADR-0011)");
            }
            advance();
            const std::size_t armLine = previous().location.line;
            const std::size_t armColumn = previous().location.column;
            elseBody = parseCaseArmBody(armLine, armColumn);
            hasElse = true;
            continue;
        }
        if (hasElse) {
            errorAtCurrent("'else' must be the last arm of a case (ADR-0011)");
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
        std::move(elseBody),
        hasElse);
}

std::vector<ast::StatementPtr> Parser::parseCaseArmBody(std::size_t armLine, std::size_t armColumn)
{
    std::vector<ast::StatementPtr> statements;

    if (!isAtEnd() && !check(TokenKind::Semicolon) && peek().location.line == armLine) {
        // A single-line arm ends at its line: a next arm such as `-5..-1`
        // must not be read as `Return 0 - 5 .. -1`.
        FlagScope scope(headerExpression_);
        appendStatement(statements);
        return statements;
    }

    while (!isAtEnd() && !check(TokenKind::Semicolon) && !checkKeyword("else")) {
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

// r3.93 (E2): `_ := F(X)` drops the result of a call on purpose. `_` is a
// discard target, not a name: it is written only here, never read, never
// declared, never chained.
ast::StatementPtr Parser::parseDiscardStatement()
{
    advance();  // '_'
    if (!check(TokenKind::ColonEqual)) {
        errorAtCurrent(kDiscardOnlyMessage);
    }
    advance();  // ':='
    auto value = parseValue();
    if (check(TokenKind::ColonEqual) || atCompoundAssignment()) {
        errorAtCurrent("a discard is not chained: write _ := F(X) alone (r3.93)");
    }
    return std::make_unique<ast::ExpressionStatement>(std::move(value), false, true);
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
        if (!isCompoundTarget(*parts.front())) {
            throw ParseError(
                "a compound assignment target must be a variable, a field or an array "
                "element whose indices have no side effects (ADR-0012)",
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
        auto current = clonePureExpression(*parts.front());
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

    // CANON-3 (r3.76): a file is a module named after the file.
    if (check(TokenKind::Identifier) && peek().normalized == "module" && current_ + 1 < tokens_.size() &&
        tokens_[current_ + 1].location.line == peek().location.line &&
        tokens_[current_ + 1].kind == TokenKind::Identifier) {
        errorAtCurrent("Inox has no 'Module' header: a file is a module named after the file (r3.76); "
                       "remove this line");
    }

    if (checkIdentifierLike()) {
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

    // Anything else was once kept as a raw declaration and ignored, so a
    // line such as `+ Integer : ... ;` compiled silently (found with r3.93).
    if (check(TokenKind::Underscore)) {
        errorAtCurrent(kDiscardOnlyMessage);
    }
    errorAtCurrent("expected a declaration: Use, Type, Const, State or a routine, Name :");
}

ast::AstNodePtr Parser::parseUseDeclaration()
{
    std::vector<std::string> path;
    path.push_back(parseQualifiedName("expected module name after 'Use'"));
    return std::make_unique<ast::UseDeclaration>(std::move(path));
}

// Type, Const and State sections (CANON-5) are parsed into typed
// declarations: Struct fields, Range bounds and Enum literals for Type; one
// name, type and initializer per line for Const and State. Every later phase
// reads these nodes; nothing re-scans tokens.
ast::AstNodePtr Parser::parseSectionDeclaration(ast::SectionKind sectionKind)
{
    std::vector<ast::TypeDeclaration> types;
    std::vector<ast::VariableDeclaration> variables;

    if (sectionKind == ast::SectionKind::Type) {
        // `Type` is a section/declarator: no ':' and no closing ';' (CANON-5).
        if (check(TokenKind::Colon)) {
            errorAtCurrent("'Type' has no ':' (CANON-5): write 'Type' and its declarations on the next lines");
        }
        while (!isAtEnd() && !atSectionBoundary()) {
            types.push_back(parseTypeDeclaration());
        }
        return std::make_unique<ast::SectionDeclaration>(sectionKind, std::move(types),
                                                         std::move(variables));
    }

    // `State` opens its variables at the line break and closes them with
    // `;`, like a control structure; it has no ':' (CANON-4, Marcelo Fortes,
    // 2026-10-12: "O State não deve usar :"; 2026-10-13: "State fecha com ;").
    if (sectionKind == ast::SectionKind::State) {
        if (check(TokenKind::Colon)) {
            errorAtCurrent("'State' has no ':' (CANON-4): ':' only declares a routine; write 'State', "
                           "its variables on the next lines, and ';'");
        }
        while (!check(TokenKind::Semicolon)) {
            if (isAtEnd() || atSectionBoundary()) {
                errorAtCurrent("'State' closes with ';' (CANON-5): end the State variables with ';'");
            }
            variables.push_back(parseSectionVariable(sectionKind));
        }
        consumeBlockClose();
        return std::make_unique<ast::SectionDeclaration>(sectionKind, std::move(types),
                                                         std::move(variables));
    }

    // A Const is one line, `Const Name := Expr`, without ':' (CANON-5).
    if (check(TokenKind::Colon)) {
        errorAtCurrent("a Const is one line, 'Const Name := Value' (CANON-5): ':' only declares a routine");
    }
    variables.push_back(parseSectionVariable(sectionKind));
    return std::make_unique<ast::SectionDeclaration>(sectionKind, std::move(types),
                                                     std::move(variables));
}

// A section declaration ends at the end of its line (one per line).
void Parser::expectEndOfDeclarationLine(std::size_t line, const char* what)
{
    if (!isAtEnd() && !check(TokenKind::Semicolon) && peek().location.line == line) {
        errorAtCurrent(std::string("expected line break after ") + what);
    }
}

// `TName Struct <fields> ;`, `TName Range Low..High`, `TName (A, B, C)` or
// `TName Enum <one literal per line> ;` (CANON-5, CANON-9, CANON-13).
ast::TypeDeclaration Parser::parseTypeDeclaration()
{
    ast::TypeDeclaration declaration;
    const lexer::Token& name = consume(TokenKind::Identifier, "expected a type name in the Type section");
    declaration.name = name.lexeme;
    declaration.line = name.location.line;
    if (check(TokenKind::Less) && peek().location.line == name.location.line) {
        declaration.typeParameters = parseTypeParameters(name);
        if (!checkKeyword("struct")) {
            throw ParseError("only a Struct takes type parameters (CANON-8): " + declaration.name,
                             isAtEnd() ? name.location : peek().location);
        }
    }

    if (matchKeyword("struct")) {
        declaration.kind = ast::TypeDeclarationKind::Struct;
        // Method reuse (CANON-9, Marcelo Fortes 2026-10-13): `TName Struct
        // (TA, TB)` reuses TA and TB. Each part is a field named after its
        // type, written first, so `X.TA` reaches it.
        if (check(TokenKind::LeftParen) && peek().location.line == name.location.line) {
            advance();
            if (check(TokenKind::RightParen)) {
                errorAtCurrent("empty parentheses are not Inox: write 'TName Struct' or name the reused "
                               "structs, 'TName Struct (TPart)'");
            }
            do {
                const lexer::Token& part =
                    consume(TokenKind::Identifier, "expected the name of a reused struct");
                ast::StructFieldDeclaration field;
                field.name = part.lexeme;
                field.typeName = part.lexeme;
                field.line = part.location.line;
                field.isPart = true;
                declaration.fields.push_back(std::move(field));
            } while (match(TokenKind::Comma));
            consume(TokenKind::RightParen, "expected ')' after the reused structs");
        }
        expectEndOfDeclarationLine(name.location.line, "'Struct'");
        while (!isAtEnd() && !check(TokenKind::Semicolon)) {
            if (atContractWord("invariant")) {
                advance();
                declaration.invariants.push_back(parseContractClause(ast::ContractClause::Kind::Invariant));
                continue;
            }
            declaration.fields.push_back(parseStructField());
        }
        consumeBlockClose();
        if (!declaration.typeParameters.empty()) {
            for (const ast::StructFieldDeclaration& field : declaration.fields) {
                if (field.isPart) {
                    throw ParseError("a generic struct cannot reuse parts yet: " + declaration.name, name.location);
                }
            }
            if (!declaration.invariants.empty()) {
                throw ParseError("a generic struct cannot have invariant lines yet: " + declaration.name,
                                 name.location);
            }
        }
        return declaration;
    }

    if (check(TokenKind::Identifier) && peek().normalized == "range" &&
        peek().location.line == name.location.line) {
        advance();
        declaration.kind = ast::TypeDeclarationKind::Range;
        declaration.bounds = parseValue();
        const auto* bounds = declaration.bounds.get();
        if (bounds->kind() != ast::AstNodeKind::BinaryExpression ||
            static_cast<const ast::BinaryExpression*>(bounds)->op() != ast::BinaryOperator::Range) {
            throw ParseError("a Range is written 'TName Range Low..High' (CANON-13): " + declaration.name,
                             name.location);
        }
        expectEndOfDeclarationLine(previous().location.line, "a Range declaration");
        return declaration;
    }

    if (match(TokenKind::LeftParen)) {
        declaration.kind = ast::TypeDeclarationKind::Enum;
        do {
            const lexer::Token& literal =
                consume(TokenKind::Identifier, "expected an enum literal name");
            declaration.literals.push_back(literal.lexeme);
        } while (match(TokenKind::Comma));
        consume(TokenKind::RightParen, "expected ')' after the enum literals");
        expectEndOfDeclarationLine(previous().location.line, "an Enum declaration");
        return declaration;
    }

    if (matchKeyword("enum")) {
        declaration.kind = ast::TypeDeclarationKind::Enum;
        expectEndOfDeclarationLine(name.location.line, "'Enum'");
        while (!isAtEnd() && !check(TokenKind::Semicolon)) {
            const lexer::Token& literal =
                consume(TokenKind::Identifier, "expected an enum literal name");
            declaration.literals.push_back(literal.lexeme);
            expectEndOfDeclarationLine(literal.location.line, "an enum literal (one per line)");
        }
        if (declaration.literals.empty()) {
            errorAtCurrent("an Enum declares at least one literal");
        }
        consumeBlockClose();
        return declaration;
    }

    throw ParseError("expected 'Struct', 'Range', 'Enum' or '(' after the type name " + declaration.name +
                         " (CANON-5)",
                     isAtEnd() ? name.location : peek().location);
}

// `FName Type [:= Literal]` on one line (CANON-9).
ast::StructFieldDeclaration Parser::parseStructField()
{
    ast::StructFieldDeclaration field;
    const lexer::Token& name = consume(TokenKind::Identifier, "expected a field name");
    field.name = name.lexeme;
    field.line = name.location.line;
    if (isAtEnd() || peek().location.line != name.location.line || !check(TokenKind::Identifier)) {
        throw ParseError("expected the type of struct field " + field.name + " on its line",
                         isAtEnd() || peek().location.line != name.location.line ? name.location
                                                                                  : peek().location);
    }
    field.typeName = parseTypeName("expected the type of the struct field");
    if (match(TokenKind::ColonEqual)) {
        field.hasDefault = true;
        // r3.82: an array field defaults to `Fill(Literal)`.
        if (check(TokenKind::Identifier) && peek().normalized == "fill" && current_ + 1 < tokens_.size() &&
            tokens_[current_ + 1].kind == TokenKind::LeftParen) {
            advance();
            advance();
            field.fillDefault = true;
            if (match(TokenKind::LeftBracket)) {
                consume(TokenKind::RightBracket, "the default of an array of sets is the empty set, Fill([])");
                field.defaultValue = "[]";
            } else {
                field.defaultValue = parseStructFieldDefault(field.name);
            }
            consume(TokenKind::RightParen, "the default of an array field is Fill(Literal)");
        } else {
            field.defaultValue = parseStructFieldDefault(field.name);
        }
        if (!isAtEnd() && !check(TokenKind::Semicolon) && peek().location.line == name.location.line) {
            throw ParseError("the default of struct field " + field.name + " must be one literal (CANON-9)",
                             peek().location);
        }
    }
    expectEndOfDeclarationLine(name.location.line, "a struct field");
    return field;
}

// A field default is one literal, with `-` before a negative number
// (CANON-9: "Fields may have literal defaults").
std::string Parser::parseStructFieldDefault(const std::string& fieldName)
{
    std::string sign;
    if (match(TokenKind::Minus)) {
        sign = "-";
    }
    if (match(TokenKind::IntegerLiteral) || match(TokenKind::FloatLiteral)) {
        return sign + tokenText(previous());
    }
    if (sign.empty() && (matchKeyword("true") || matchKeyword("false"))) {
        return previous().lexeme;
    }
    // A String default keeps its quotes, so it is never mistaken for a
    // number or a Bool (`FAge Integer := "12"` is a type error).
    if (sign.empty() && match(TokenKind::StringLiteral)) {
        return '"' + previous().lexeme + '"';
    }
    // A Char default keeps its quotes ('x', char-values).
    if (sign.empty() && match(TokenKind::CharLiteral)) {
        return previous().lexeme;
    }
    // An enum literal (`FSuit TSuit := Heart`); the analyzer checks that it
    // belongs to the field's enum (CANON-13).
    if (sign.empty() && match(TokenKind::Identifier)) {
        return previous().lexeme;
    }
    throw ParseError("the default of struct field " + fieldName + " must be one literal (CANON-9)",
                     isAtEnd() ? previous().location : peek().location);
}

// One Const or State declaration (CANON-5): `Name := Expr`,
// `Name Type := Expr` or `Name Type` (State only: type defaults). The old
// `Name : Type` form is rejected: ':' only declares a routine (CANON-4).
ast::VariableDeclaration Parser::parseSectionVariable(ast::SectionKind sectionKind)
{
    const bool isConst = sectionKind == ast::SectionKind::Const;
    ast::VariableDeclaration declaration;
    if (check(TokenKind::ColonEqual)) {
        errorAtCurrent("expected a name before ':='");
    }
    const lexer::Token& name = consume(TokenKind::Identifier,
                                       isConst ? "expected a Const name" : "expected a State variable name");
    declaration.name = name.lexeme;
    declaration.line = name.location.line;
    if (check(TokenKind::Comma)) {
        errorAtCurrent(isConst ? "a Const declares one name"
                               : "grouped declarations in State are not supported yet; declare one name per line");
    }

    if (check(TokenKind::Colon)) {
        errorAtCurrent("':' only declares a routine (CANON-4): write '" +
                       (isConst ? "Const " + declaration.name + " := Value"
                                : declaration.name + " Type := Value") +
                       "', without ':'");
    }
    if (!match(TokenKind::ColonEqual)) {
        declaration.typeName = parseTypeName("expected a type or ':=' after the name");
        if (isConst && check(TokenKind::ColonEqual)) {
            // OPEN-5 (closed v3.38, Marcelo Fortes): a Const has no written
            // type; its type is the value's. A conversion gives another type.
            throw ParseError("a Const is written 'Const " + declaration.name +
                                 " := Value' (CANON-5): to choose its type, convert the value, "
                                 "e.g. 'Const " + declaration.name + " := " + declaration.typeName +
                                 "(Value)'",
                             peek().location);
        }
        if (!match(TokenKind::ColonEqual)) {
            if (isConst) {
                errorAtCurrent("a Const needs a value: 'Const Name := Value' (CANON-5)");
            }
            expectEndOfDeclarationLine(name.location.line, "a section declaration");
            return declaration;
        }
    }

    declaration.value = parseValue();
    expectEndOfDeclarationLine(previous().location.line, "a section declaration");
    return declaration;
}

ast::AstNodePtr Parser::parseFunctionDeclaration()
{
    const lexer::Token& name = consumeIdentifierLike("expected function name");
    std::string functionName = name.lexeme;
    std::vector<std::string> typeParameters;
    if (check(TokenKind::Less) && peek().location.line == name.location.line) {
        typeParameters = parseTypeParameters(name);
    }
    if (match(TokenKind::Dot)) {
        if (!typeParameters.empty()) {
            std::string list;
            for (const std::string& parameter : typeParameters) {
                list += (list.empty() ? "" : ", ") + parameter;
            }
            errorAt(name, "methods of generic structs are not in Inox yet: write a generic routine, Name<" + list +
                              ">(X " + functionName + "<" + list + ">)");
        }
        const lexer::Token& methodName = consume(
            TokenKind::Identifier, "expected method name after '.'");
        functionName += ".";
        functionName += methodName.lexeme;
        if (check(TokenKind::Less) && peek().location.line == name.location.line) {
            errorAtCurrent("a method cannot have type parameters yet (CANON-8): write a generic routine, "
                           "Name<T>(...)");
        }
    }

    std::vector<std::string> signatureTokens;

    while (!isAtEnd() && !check(TokenKind::Colon)) {
        // CANON-13: an array type in a signature is one type token,
        // `Array[1..3] Integer`, so later phases read it like any type name;
        // so is a generic type, `Slice<T>`, `Own<T>`, `TPair<K, V>` (CANON-8).
        if (check(TokenKind::Identifier) && current_ + 1 < tokens_.size() &&
            tokens_[current_ + 1].location.line == peek().location.line &&
            ((peek().normalized == "array" && tokens_[current_ + 1].kind == TokenKind::LeftBracket) ||
             tokens_[current_ + 1].kind == TokenKind::Less)) {
            signatureTokens.push_back(parseTypeName("expected type name"));
            continue;
        }
        if (check(TokenKind::Underscore)) {
            errorAtCurrent(kDiscardOnlyMessage);
        }
        signatureTokens.push_back(tokenText(advance()));
    }

    if (signatureTokens.size() >= 2 && signatureTokens[0] == "(" && signatureTokens[1] == ")") {
        errorAt(name, "empty parentheses are not allowed in declarations; omit parentheses when there are no parameters");
    }

    // `Override` ends the signature of a method that replaces a reused one
    // (CANON-9): `TName.Method(Self) Type Override :`.
    bool isOverride = false;
    const auto isOverrideWord = [](std::string word) {
        for (char& ch : word) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        return word == "override";
    };
    if (!signatureTokens.empty() && isOverrideWord(signatureTokens.back())) {
        if (functionName.find('.') == std::string::npos) {
            errorAt(name, "'Override' marks a method that replaces a reused one (CANON-9); " +
                              functionName + " is not a method");
        }
        signatureTokens.pop_back();
        isOverride = true;
    }

    consume(TokenKind::Colon, "expected ':' after function signature");
    // CANON-24: `pre` and `pos` lines open the body; `pre else` and
    // `pos then` are for an Override (Eiffel's require else / ensure then).
    currentRoutine_ = functionName;
    std::vector<ast::ContractClause> contracts;
    while (atContractWord("pre") || atContractWord("pos")) {
        const bool isPre = peek().normalized == "pre";
        advance();
        ast::ContractClause::Kind kind = isPre ? ast::ContractClause::Kind::Pre : ast::ContractClause::Kind::Pos;
        if (isPre && matchKeyword("else")) {
            kind = ast::ContractClause::Kind::PreElse;
        } else if (!isPre && check(TokenKind::Identifier) && peek().normalized == "then") {
            advance();
            kind = ast::ContractClause::Kind::PosThen;
        }
        contracts.push_back(parseContractClause(kind));
    }
    auto body = parseBlockBody();
    consumeBlockClose();

    return std::make_unique<ast::FunctionDeclaration>(
        std::move(functionName),
        std::move(signatureTokens),
        std::move(body),
        isOverride,
        std::move(contracts),
        std::move(typeParameters));
}

// `<T, U>`: distinct names, on the line of the name they follow.
std::vector<std::string> Parser::parseTypeParameters(const lexer::Token& owner)
{
    consume(TokenKind::Less, "expected '<'");
    std::vector<std::string> parameters;
    std::vector<std::string> normalized;
    do {
        const lexer::Token& parameter = consume(TokenKind::Identifier, "expected a type parameter name");
        for (const std::string& seen : normalized) {
            if (seen == parameter.normalized) {
                errorAt(parameter, "duplicate type parameter " + parameter.lexeme + " of " + owner.lexeme);
            }
        }
        parameters.push_back(parameter.lexeme);
        normalized.push_back(parameter.normalized);
    } while (match(TokenKind::Comma));
    consume(TokenKind::Greater, "expected '>' after the type parameters of " + owner.lexeme);
    return parameters;
}

// ---------- Design by contract (CANON-24) ----------

// A contract line starts with the contextual word followed by more on the
// same line, never by `:=` (so `Pos := 3` stays an assignment).
bool Parser::atContractWord(std::string_view word) const
{
    if (isAtEnd() || (peek().kind != TokenKind::Identifier && peek().kind != TokenKind::Keyword) ||
        peek().normalized != word ||
        current_ + 1 >= tokens_.size()) {
        return false;
    }
    const lexer::Token& next = tokens_[current_ + 1];
    return next.location.line == peek().location.line && next.kind != TokenKind::ColonEqual &&
           next.kind != TokenKind::Dot && next.kind != TokenKind::Semicolon;
}

// The condition as written, for the run-time message.
std::string Parser::sourceText(std::size_t first, std::size_t last) const
{
    std::string text;
    for (std::size_t index = first; index < last && index < tokens_.size(); ++index) {
        const lexer::Token& token = tokens_[index];
        std::string piece = token.kind == TokenKind::StringLiteral ? '"' + token.lexeme + '"' : token.lexeme;
        const bool glueLeft = token.kind == TokenKind::Dot || token.kind == TokenKind::Comma ||
                              token.kind == TokenKind::RightParen || token.kind == TokenKind::RightBracket ||
                              (token.kind == TokenKind::LeftParen && index > first &&
                               tokens_[index - 1].kind == TokenKind::Identifier) ||
                              token.kind == TokenKind::LeftBracket;
        const bool previousGlues = index > first && (tokens_[index - 1].kind == TokenKind::Dot ||
                                                     tokens_[index - 1].kind == TokenKind::LeftParen ||
                                                     tokens_[index - 1].kind == TokenKind::LeftBracket);
        if (!text.empty() && !glueLeft && !previousGlues) {
            text += ' ';
        }
        text += piece;
    }
    return text;
}

// `Condition` or `Condition, "message"`, alone on its line.
ast::ContractClause Parser::parseContractClause(ast::ContractClause::Kind kind)
{
    ast::ContractClause clause;
    clause.kind = kind;
    clause.line = previous().location.line;
    if (isAtEnd() || peek().location.line != clause.line) {
        errorAtCurrent("expected a condition on the same line (CANON-24)");
    }
    const std::size_t first = current_;
    const bool isPos = kind == ast::ContractClause::Kind::Pos || kind == ast::ContractClause::Kind::PosThen;
    {
        FlagScope posScope(inPosClause_, isPos);
        FlagScope clauseScope(inContractClause_);
        clause.condition = parseValue();
    }
    clause.text = sourceText(first, current_);
    if (match(TokenKind::Comma)) {
        const lexer::Token& message = consume(TokenKind::StringLiteral,
                                              "a contract's message is a String literal (CANON-24)");
        clause.message = message.lexeme;
    }
    expectEndOfDeclarationLine(clause.line, "a contract clause");
    return clause;
}

std::vector<ast::ContractClause> Parser::parseLoopContracts(bool allowVariant)
{
    std::vector<ast::ContractClause> contracts;
    bool sawVariant = false;
    while (atContractWord("invariant") || atContractWord("variant")) {
        const bool isVariant = peek().normalized == "variant";
        if (isVariant && !allowVariant) {
            errorAtCurrent("a 'for' loop always ends: it has no 'variant' (CANON-24)");
        }
        if (isVariant && sawVariant) {
            errorAtCurrent("a loop has one 'variant' (CANON-24)");
        }
        sawVariant = sawVariant || isVariant;
        advance();
        contracts.push_back(parseContractClause(isVariant ? ast::ContractClause::Kind::LoopVariant
                                                          : ast::ContractClause::Kind::LoopInvariant));
    }
    return contracts;
}

namespace {

ast::ExpressionPtr contractCheck(int kind, ast::ExpressionPtr condition, const std::string& message)
{
    std::vector<ast::ExpressionPtr> arguments;
    arguments.push_back(std::make_unique<ast::LiteralExpression>(ast::LiteralKind::Integer, std::to_string(kind)));
    arguments.push_back(std::move(condition));
    arguments.push_back(std::make_unique<ast::LiteralExpression>(ast::LiteralKind::String, message));
    return std::make_unique<ast::CallExpression>(makeSyntheticIdentifier("__check"), std::move(arguments));
}

std::string clauseMessage(const std::string& category, const std::string& routine, const ast::ContractClause& clause,
                          const std::string& extra = {})
{
    std::string message = category + ": " + routine + ": " + clause.text + extra;
    if (!clause.message.empty()) {
        message += ": " + clause.message;
    }
    return message;
}

ast::StatementPtr statementOf(ast::ExpressionPtr expression)
{
    return std::make_unique<ast::ExpressionStatement>(std::move(expression));
}

ast::ExpressionPtr assignment(const std::string& name, ast::ExpressionPtr value)
{
    return std::make_unique<ast::BinaryExpression>(ast::BinaryOperator::Assign, makeSyntheticIdentifier(name),
                                                   std::move(value));
}

} // namespace

// A loop's `invariant` is checked at the start of every iteration and, for
// while and repeat, once more when the loop ends; its `variant` (an integer)
// must be >= 0 at the start of every iteration and smaller than at the
// start of the previous one (Eiffel; CANON-24). The checks are statements
// put into the body; the variant's previous value lives before the loop.
void Parser::lowerLoopContracts(std::vector<ast::ContractClause> contracts,
                                std::vector<ast::StatementPtr>& body,
                                bool checkAfterLoop)
{
    if (contracts.empty()) {
        return;
    }
    const std::string id = std::to_string(++contractCounter_);
    std::vector<ast::StatementPtr> top;
    for (ast::ContractClause& clause : contracts) {
        if (clause.kind == ast::ContractClause::Kind::LoopInvariant) {
            if (checkAfterLoop) {
                // The same condition once more, right after the loop.
                pendingStatements_.push_back(statementOf(contractCheck(
                    4, ast::cloneExpression(*clause.condition), clauseMessage("loop invariant violated", currentRoutine_, clause, " (at the end)"))));
            }
            top.push_back(statementOf(
                contractCheck(4, std::move(clause.condition), clauseMessage("loop invariant violated", currentRoutine_, clause))));
            continue;
        }
        const std::string current = "__variant_" + id;
        const std::string previousName = "__variant_previous_" + id;
        precedingStatements_.push_back(statementOf(assignment(
            previousName, std::make_unique<ast::UnaryExpression>(
                              ast::UnaryOperator::Minus,
                              std::make_unique<ast::LiteralExpression>(ast::LiteralKind::Integer, "1")))));
        std::vector<ast::ExpressionPtr> conversion;
        conversion.push_back(std::move(clause.condition));
        std::vector<ast::ExpressionPtr> pure;
        pure.push_back(std::make_unique<ast::CallExpression>(makeSyntheticIdentifier("Integer"), std::move(conversion)));
        top.push_back(statementOf(assignment(
            current, std::make_unique<ast::CallExpression>(makeSyntheticIdentifier("__pure"), std::move(pure)))));
        top.push_back(statementOf(contractCheck(
            5,
            std::make_unique<ast::BinaryExpression>(ast::BinaryOperator::GreaterEqual, makeSyntheticIdentifier(current),
                                                    std::make_unique<ast::LiteralExpression>(ast::LiteralKind::Integer, "0")),
            clauseMessage("loop variant violated", currentRoutine_, clause, " is negative"))));
        top.push_back(statementOf(contractCheck(
            5,
            std::make_unique<ast::BinaryExpression>(
                ast::BinaryOperator::Or,
                std::make_unique<ast::BinaryExpression>(ast::BinaryOperator::Less, makeSyntheticIdentifier(previousName),
                                                        std::make_unique<ast::LiteralExpression>(ast::LiteralKind::Integer, "0")),
                std::make_unique<ast::BinaryExpression>(ast::BinaryOperator::Less, makeSyntheticIdentifier(current),
                                                        makeSyntheticIdentifier(previousName))),
            clauseMessage("loop variant violated", currentRoutine_, clause, " did not decrease"))));
        top.push_back(statementOf(assignment(previousName, makeSyntheticIdentifier(current))));
    }
    for (auto& statement : body) {
        top.push_back(std::move(statement));
    }
    body = std::move(top);
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
           checkKeyword("until");
}

bool Parser::atSectionBoundary() const
{
    if (isAtEnd()) {
        return true;
    }

    if (checkKeyword("use") || checkKeyword("type") || checkKeyword("const") ||
        checkKeyword("state")) {
        return true;
    }

    if (!checkIdentifierLike()) {
        return false;
    }

    // A declaration header (`Name ... :`) lives on a single source line, so the
    // look-ahead never leaves the current line. This keeps large Type sections
    // linear instead of quadratic.
    const std::size_t headerLine = tokens_[current_].location.line;
    // `Name : Type` with more on its line is the removed declaration form,
    // reported by the section parser, not a routine header (`Main :`).
    if (current_ + 2 < tokens_.size() && tokens_[current_ + 1].kind == TokenKind::Colon &&
        tokens_[current_ + 2].location.line == headerLine &&
        tokens_[current_ + 2].kind != TokenKind::EndOfFile) {
        return false;
    }
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
    // CANON-13: `A Array[1..10] Integer` declares a fixed array.
    if ((type.normalized == "array") && tokens_[index].kind == TokenKind::LeftBracket &&
        tokens_[index].location.line == type.location.line) {
        return true;
    }
    // `S Slice<Int64>`, `P Own<TNode>`, `P TPair<String, Integer>` (CANON-8):
    // two names in a row never start an expression, so `<` after the second
    // opens type arguments.
    if (tokens_[index].kind == TokenKind::Less && tokens_[index].location.line == type.location.line) {
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
    // A predefined name (r3.74: True, False, Exit, bitand, bitor, bitxor,
    // bitnot) is an identifier token that keeps its meaning: it can never be
    // declared (CANON-2), so where a keyword is expected it is that word.
    return !isAtEnd() &&
           (peek().kind == TokenKind::Keyword ||
            (peek().kind == TokenKind::Identifier && lexer::Lexer::isPredefinedName(normalized))) &&
           peek().normalized == normalized;
}

bool Parser::checkIdentifierLike() const
{
    return !isAtEnd() &&
           peek().kind == TokenKind::Identifier;
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
    case TokenKind::Identifier:
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
    if (token.normalized == "bitnot") {
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
        std::string_view text = token.lexeme;
        // `0xFF` and `$FF` are the same literal (CANON-2, v3.34).
        if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
            digits = "$";
            text.remove_prefix(2);
        }
        for (const char ch : text) {
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
