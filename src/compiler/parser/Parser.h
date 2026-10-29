// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "../ast/Ast.h"
#include "../lexer/Token.h"

#include <cstddef>
#include <initializer_list>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
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

    // CANON-3 (r3.76): the module's name is its file's name, given by the driver.
    std::unique_ptr<ast::ModuleNode> parseModule(std::string name);
    ast::ExpressionPtr parseExpression();
    std::vector<ast::StatementPtr> parseStatements();
    std::vector<ast::StatementPtr> parseHeaderDelimitedBlock();

private:
    // One source statement may produce several (a chained assignment is
    // desugared, ADR-0009), so statement lists are built with appendStatement,
    // which drains pendingStatements_. Both stay private for that reason.
    ast::StatementPtr parseStatement();
    void appendStatement(std::vector<ast::StatementPtr>& statements);
    using TokenKind = lexer::TokenKind;

    ast::ExpressionPtr parseValue();
    ast::ExpressionPtr parseArgument();
    ast::ExpressionPtr parseOr();
    ast::ExpressionPtr parseXor();
    ast::ExpressionPtr parseAnd();
    ast::ExpressionPtr parseRelational();
    ast::ExpressionPtr parseMembership();
    ast::ExpressionPtr parseRange();
    ast::ExpressionPtr parseBitOr();
    ast::ExpressionPtr parseBitXor();
    ast::ExpressionPtr parseBitAnd();
    ast::ExpressionPtr parseShift();
    ast::ExpressionPtr parseAdditive();
    ast::ExpressionPtr parseMultiplicative();
    ast::ExpressionPtr parseUnary();
    ast::ExpressionPtr parsePower();
    ast::ExpressionPtr parsePostfix();
    ast::ExpressionPtr parsePrimary();
    void rejectOldForStep() const;
    ast::ExpressionPtr parseCaseChoice();
    bool atLineBreakInHeader() const;
    bool atCompoundAssignment() const;

    std::vector<ast::ExpressionPtr> parseArgumentList();

    ast::StatementPtr parseTypedLocalStatement();
    std::string parseTypeName(const char* message);
    // CANON-8 (r3.77): `<T, U>` after the name of a routine or a struct.
    std::vector<std::string> parseTypeParameters(const lexer::Token& owner);
    void rejectWrittenTypeArguments() const;
    ast::StatementPtr parseIfStatement();
    ast::StatementPtr parseUnlessStatement();
    ast::StatementPtr parseWhileStatement();
    ast::StatementPtr parseRepeatStatement();
    ast::StatementPtr parseUntilStatement();
    ast::StatementPtr parseForInStatement();
    ast::StatementPtr parseCaseStatement();
    std::vector<ast::StatementPtr> parseCaseArmBody(std::size_t armLine, std::size_t armColumn);
    ast::StatementPtr parseTryStatement();
    ast::ExceptionHandler parseExceptionHandler();
    ast::StatementPtr parseRaiseStatement();
    ast::StatementPtr parseRetryStatement();
    ast::StatementPtr parseWithStatement();
    ast::StatementPtr parseReturnStatement();
    ast::StatementPtr parseExpressionStatement();
    ast::StatementPtr parseDiscardStatement();

    ast::AstNodePtr parseModuleItem();
    ast::AstNodePtr parseUseDeclaration(bool exported);
    ast::AstNodePtr parseSectionDeclaration(ast::SectionKind sectionKind);
    ast::TypeDeclaration parseTypeDeclaration();
    ast::StructFieldDeclaration parseStructField();
    std::string parseStructFieldDefault(const std::string& fieldName);
    ast::VariableDeclaration parseSectionVariable(ast::SectionKind sectionKind);
    void expectEndOfDeclarationLine(std::size_t line, const char* what);
    ast::AstNodePtr parseFunctionDeclaration();
    std::string parseQualifiedName(std::string_view message);

    std::vector<ast::StatementPtr> parseBlockBody();
    std::vector<ast::StatementPtr> parseDelimitedBody(std::initializer_list<std::string_view> stopKeywords);
    bool atAnyKeyword(std::initializer_list<std::string_view> keywords) const;
    bool atStatementBoundary() const;
    // Whether another module item starts at the current token (Use, Type,
    // Const, State, Var or a routine header): the end of a Type section, or a
    // State section missing its closing ';'.
    bool atSectionBoundary() const;
    bool atTypedLocalStatementStart() const;
    void requireHeaderLineBreak();
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

    // Implementation limits (CANON-19). They turn pathological input into a
    // normal diagnostic instead of a native stack overflow.
    // The values are measured, not guessed: they are the largest nesting the
    // deepest recursive pass survives on a 1 MiB main-thread stack (the Windows
    // default) in the worst build we have, Debug + AddressSanitizer, divided by
    // a safety factor of about two (constitution CANON-19).
    static constexpr std::size_t kMaxStatementNesting = 64;
    static constexpr std::size_t kMaxExpressionNesting = 256;
    static constexpr std::size_t kMaxExpressionDepth = 128;

    class DepthGuard {
    public:
        DepthGuard(const Parser& owner, std::size_t& counter, std::size_t limit, const char* what);
        ~DepthGuard();
        DepthGuard(const DepthGuard&) = delete;
        DepthGuard& operator=(const DepthGuard&) = delete;

    private:
        std::size_t& counter_;
    };

    template <typename T, typename... Args>
    std::unique_ptr<T> makeExpr(Args&&... args)
    {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        checkExpressionDepth(*node);
        return node;
    }

    void checkExpressionDepth(const ast::Expression& expression) const;
    ast::StatementPtr endSimpleStatement(ast::StatementPtr statement);

    std::size_t statementNesting_ = 0;
    std::size_t expressionNesting_ = 0;

    [[noreturn]] void errorAtCurrent(std::string_view message) const;
    [[noreturn]] void errorAt(const lexer::Token& token, std::string_view message) const;

    static ast::BinaryOperator binaryOperatorFor(const lexer::Token& token);
    static ast::UnaryOperator unaryOperatorFor(const lexer::Token& token);
    static std::string tokenText(const lexer::Token& token);

    // Operator-precedence ambiguity guard (canonical Layer A). The precedence
    // table is unambiguous, but the canonical rule REQUIRES explicit parentheses
    // when mixing operators from families whose relative order people memorize
    // wrong (and/or/xor mixed together; different bitwise families mixed). An
    // operand is exempt from the check when it was written inside parentheses,
    // which is recorded here.
    void markParenthesized(const ast::Expression* node);
    bool isParenthesized(const ast::Expression* node) const;
    void requireNoLogicalMix(
        const ast::Expression* operand, std::string_view outerOp,
        const lexer::Token& opToken) const;
    void requireNoBitwiseMix(
        const ast::Expression* operand, std::string_view outerOp,
        const lexer::Token& opToken) const;

    std::vector<lexer::Token> tokens_;
    std::size_t current_ = 0;
    std::unordered_set<const ast::Expression*> parenthesized_;
    // Statements produced by desugaring one source statement into several
    // (chained assignment). appendStatement() drains them in order.
    std::vector<ast::StatementPtr> pendingStatements_;
    bool headerExpression_ = false;
    std::vector<std::string> withTargetStack_;
    std::size_t withCounter_ = 0;

    // Design by contract (CANON-24). `pre`, `pos`, `invariant`, `variant`,
    // `old` and `Result` are contextual words: a line that starts with one of
    // them (not followed by `:=`) is a clause where clauses are allowed.
    bool atContractWord(std::string_view word) const;
    ast::ContractClause parseContractClause(ast::ContractClause::Kind kind);
    std::string sourceText(std::size_t first, std::size_t last) const;
    std::vector<ast::ContractClause> parseLoopContracts(bool allowVariant);
    void lowerLoopContracts(std::vector<ast::ContractClause> contracts,
                            std::vector<ast::StatementPtr>& body,
                            bool checkAfterLoop);
    bool inPosClause_ = false;
    bool inContractClause_ = false;
    std::string currentRoutine_;  // for the messages of loop contracts
    std::size_t contractCounter_ = 0;
    // Statements a loop contract puts before the loop (the variant's start
    // value); appendStatement() emits them before the statement just parsed.
    std::vector<ast::StatementPtr> precedingStatements_;
};

} // namespace inox::compiler::parser
