// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Token.h"

#include <string>
#include <string_view>
#include <vector>

namespace inox::compiler::lexer {

class Lexer {
public:
    explicit Lexer(std::string_view source);

    Token nextToken();
    std::vector<Token> tokenize();

private:
    bool isAtEnd() const;
    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);

    void skipWhitespaceAndComments();
    Token makeToken(TokenKind kind, SourceLocation start, std::size_t startOffset);
    Token invalidToken(SourceLocation start, std::size_t startOffset, std::string message);

    Token identifierOrKeyword(SourceLocation start, std::size_t startOffset);
    Token number(SourceLocation start, std::size_t startOffset);
    Token dollarHexNumber(SourceLocation start, std::size_t startOffset);
    Token stringLiteral(SourceLocation start, std::size_t startOffset);
    Token charLiteral(SourceLocation start, std::size_t startOffset);

    static bool isIdentifierStart(char ch);
    static bool isIdentifierPart(char ch);
    static bool isDigit(char ch);
    static bool isHexDigit(char ch);
    static std::string normalize(std::string_view text);
    static bool isKeyword(std::string_view normalized);

    std::string_view source_;
    std::size_t current_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
};

} // namespace inox::compiler::lexer
