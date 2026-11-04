// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Lexer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

namespace inox::compiler::lexer {

namespace {

// CANON-2 reserved words (r3.76: 42).
constexpr std::array<std::string_view, 43> kKeywords = {
    "use", "type", "const", "state", "struct", "enum",
    "if", "elif", "else", "unless", "while", "repeat", "until",
    "for", "in", "step", "case", "try", "except", "on", "do", "ensure", "raise", "retry",
    "with", "mut", "ref", "mod", "rem", "and", "xor", "or", "not",
    "shr", "shl", "return",
    "pre", "pos", "old", "invariant", "variant", "override", "public"
};

// CANON-2 predefined names (r3.76): identifiers that keep their meaning and
// can never be declared.
constexpr std::array<std::string_view, 11> kPredefinedNames = {
    "true", "false", "exit", "leave", "continue", "iif", "fill",
    "bitand", "bitor", "bitxor", "bitnot"
};

} // namespace

Lexer::Lexer(std::string_view source) : cursor_(source) {}

Token Lexer::nextToken()
{
    skipWhitespaceAndComments();

    const auto position = cursor_.position();
    SourceLocation start{position.offset, position.line, position.column};
    const std::size_t startOffset = position.offset;

    if (cursor_.atEnd()) {
        return makeToken(TokenKind::EndOfFile, start, startOffset);
    }

    const char ch = cursor_.advance();

    if (isIdentifierStart(ch)) {
        return identifierOrKeyword(start, startOffset);
    }

    if (isDigit(ch)) {
        return number(start, startOffset);
    }

    switch (ch) {
    case '"':
        return stringLiteral(start, startOffset);
    case '\'':
        return charLiteral(start, startOffset);
    case ':':
        if (cursor_.match('=')) {
            return makeToken(TokenKind::ColonEqual, start, startOffset);
        }
        return makeToken(TokenKind::Colon, start, startOffset);
    case '=':
        return makeToken(TokenKind::Equal, start, startOffset);
    case '#':
        return makeToken(TokenKind::Hash, start, startOffset);
    // r3.87: `!=` is inequality too, the same operator as `#`; a lone `!`
    // is not an operator: negation is `not`.
    case '!':
        if (cursor_.match('=')) {
            return makeToken(TokenKind::Hash, start, startOffset);
        }
        return invalidToken(start, startOffset,
                            "'!' is not an operator: negation is 'not' (not (A = B)), inequality is '#' or '!='");
    case '|':
        // ADR-0011: separates the alternatives of a case arm. It is not an
        // operator; bitwise or is spelled `bitor`.
        return makeToken(TokenKind::Pipe, start, startOffset);
    case '<':
        if (cursor_.match('=')) {
            return makeToken(TokenKind::LessEqual, start, startOffset);
        }
        return makeToken(TokenKind::Less, start, startOffset);
    case '>':
        if (cursor_.match('=')) {
            return makeToken(TokenKind::GreaterEqual, start, startOffset);
        }
        return makeToken(TokenKind::Greater, start, startOffset);
    // ADR-0012: compound assignment `+=` `-=` `*=` `/=` `^=`.
    case '+':
        return makeToken(cursor_.match('=') ? TokenKind::PlusEqual : TokenKind::Plus, start, startOffset);
    case '-':
        return makeToken(cursor_.match('=') ? TokenKind::MinusEqual : TokenKind::Minus, start, startOffset);
    case '*':
        return makeToken(cursor_.match('=') ? TokenKind::StarEqual : TokenKind::Star, start, startOffset);
    case '/':
        return makeToken(cursor_.match('=') ? TokenKind::SlashEqual : TokenKind::Slash, start, startOffset);
    case '^':
        return makeToken(cursor_.match('=') ? TokenKind::CaretEqual : TokenKind::Caret, start, startOffset);
    case '$':
        return dollarHexNumber(start, startOffset);
    case '.':
        if (cursor_.match('.')) {
            return makeToken(TokenKind::DotDot, start, startOffset);
        }
        return makeToken(TokenKind::Dot, start, startOffset);
    case '(':
        return makeToken(TokenKind::LeftParen, start, startOffset);
    case ')':
        return makeToken(TokenKind::RightParen, start, startOffset);
    case '[':
        return makeToken(TokenKind::LeftBracket, start, startOffset);
    case ']':
        return makeToken(TokenKind::RightBracket, start, startOffset);
    case ',':
        return makeToken(TokenKind::Comma, start, startOffset);
    case ';':
        return makeToken(TokenKind::Semicolon, start, startOffset);
    case '{':
    case '}':
        return invalidToken(start, startOffset,
                            "'{' and '}' are reserved for a future construct (CANON-2); "
                            "blocks close with ';' and comments start with '=='");
    default:
        return invalidToken(start, startOffset, "unexpected character");
    }
}

std::vector<Token> Lexer::tokenize()
{
    std::vector<Token> tokens;
    for (;;) {
        Token token = nextToken();
        const bool done = token.kind == TokenKind::EndOfFile;
        tokens.push_back(std::move(token));
        if (done) {
            break;
        }
    }
    return tokens;
}

void Lexer::skipWhitespaceAndComments()
{
    for (;;) {
        while (!cursor_.atEnd()) {
            const char ch = cursor_.peek();
            if (ch == ' ' || ch == '\r' || ch == '\t' || ch == '\n') {
                cursor_.advance();
            } else {
                break;
            }
        }

        if (cursor_.peek() == '=' && cursor_.peekNext() == '=') {
            while (!cursor_.atEnd() && cursor_.peek() != '\n') {
                cursor_.advance();
            }
            continue;
        }

        break;
    }
}

Token Lexer::makeToken(TokenKind kind, SourceLocation start, std::size_t startOffset)
{
    const auto text = cursor_.slice(startOffset, cursor_.offset());
    return Token{kind, std::string(text), normalize(text), start};
}

Token Lexer::invalidToken(SourceLocation start, std::size_t startOffset, std::string message)
{
    Token token = makeToken(TokenKind::Invalid, start, startOffset);
    token.normalized = std::move(message);
    return token;
}

Token Lexer::identifierOrKeyword(SourceLocation start, std::size_t startOffset)
{
    while (isIdentifierPart(cursor_.peek())) {
        cursor_.advance();
    }

    Token token = makeToken(TokenKind::Identifier, start, startOffset);
    if (token.lexeme == "_") {
        token.kind = TokenKind::Underscore;
    } else if (isKeyword(token.normalized)) {
        token.kind = TokenKind::Keyword;
    }
    return token;
}

// CANON-2 (v3.3): `_` may separate digits in any numeric literal, but only
// BETWEEN two digits: never leading, trailing, doubled, or next to `.`.
// Consumes the remaining digits of a run whose first digit was already read.
bool Lexer::digitRun(bool hex)
{
    const auto isRunDigit = [&](char ch) { return hex ? isHexDigit(ch) : isDigit(ch); };
    for (;;) {
        while (isRunDigit(cursor_.peek())) {
            cursor_.advance();
        }
        if (cursor_.peek() != '_') {
            return true;
        }
        cursor_.advance();  // '_'
        if (!isRunDigit(cursor_.peek())) {
            return false;  // trailing, doubled, or before '.'
        }
    }
}

Token Lexer::number(SourceLocation start, std::size_t startOffset)
{
    if (cursor_.at(startOffset) == '0' && (cursor_.peek() == 'x' || cursor_.peek() == 'X')) {
        // CANON-2 (v3.34, decided by Marcelo Fortes): `0xFF` is accepted next
        // to `$FF` as a convenience for programmers coming from C. Same digits
        // and separator rules; later phases see it as `$FF`.
        cursor_.advance();
        if (!isHexDigit(cursor_.peek())) {
            return invalidToken(start, startOffset, "expected hexadecimal digit after '0x'");
        }
        if (!digitRun(true)) {
            return invalidToken(start, startOffset,
                                "'_' may only separate two digits in a number (CANON-2): write '0xFF_FF'");
        }
        return makeToken(TokenKind::IntegerLiteral, start, startOffset);
    }

    const char* kSeparator =
        "'_' may only separate two digits in a number (CANON-2): write '1_000'";
    if (!digitRun(false)) {
        return invalidToken(start, startOffset, kSeparator);
    }

    if (cursor_.peek() == '.' && isDigit(cursor_.peekNext())) {
        cursor_.advance();
        if (!digitRun(false)) {
            return invalidToken(start, startOffset, kSeparator);
        }
        // r3.89 (B5): an exponent after the fraction, `1.5e3`, `2.0E-7`.
        if (cursor_.peek() == 'e' || cursor_.peek() == 'E') {
            cursor_.advance();
            if (cursor_.peek() == '+' || cursor_.peek() == '-') {
                cursor_.advance();
            }
            if (!isDigit(cursor_.peek())) {
                return invalidToken(start, startOffset,
                                    "an exponent needs digits after 'e' (r3.89): write '1.5e3' or '2.0e-7'");
            }
            if (!digitRun(false)) {
                return invalidToken(start, startOffset, kSeparator);
            }
        }
        return makeToken(TokenKind::FloatLiteral, start, startOffset);
    }

    // r3.89 (B5): an exponent needs a fraction, `1.0e3`; `1e3` is an error.
    if ((cursor_.peek() == 'e' || cursor_.peek() == 'E') &&
        (isDigit(cursor_.peekNext()) || ((cursor_.peekNext() == '+' || cursor_.peekNext() == '-')))) {
        const std::string digits(cursor_.slice(startOffset, cursor_.offset()));
        return invalidToken(start, startOffset,
                            "a real literal with an exponent has a fraction (r3.89): write '" + digits +
                                ".0e...', not '" + digits + "e...'");
    }

    return makeToken(TokenKind::IntegerLiteral, start, startOffset);
}

Token Lexer::dollarHexNumber(SourceLocation start, std::size_t startOffset)
{
    if (!isHexDigit(cursor_.peek())) {
        return invalidToken(start, startOffset, "expected hexadecimal digit after '$'");
    }

    if (!digitRun(true)) {
        return invalidToken(start, startOffset,
                            "'_' may only separate two digits in a number (CANON-2): write '$FF_FF'");
    }

    return makeToken(TokenKind::IntegerLiteral, start, startOffset);
}

Token Lexer::stringLiteral(SourceLocation start, std::size_t startOffset)
{
    // CANON-2: a String is delimited by double quotes; `""` is the empty
    // String, also when another `""` follows on the line (`F("", "")`).
    const std::size_t contentStart = cursor_.offset();
    bool escaped = false;

    while (!cursor_.atEnd() && cursor_.peek() != '\n') {
        if (cursor_.peek() == '"' && !escaped) {
            break;
        }

        escaped = cursor_.peek() == '\\' && !escaped;
        if (cursor_.peek() != '\\') {
            escaped = false;
        }
        cursor_.advance();
    }

    if (cursor_.atEnd() || cursor_.peek() == '\n') {
        return invalidToken(start, startOffset, "unterminated string literal");
    }

    const auto content = cursor_.slice(contentStart, cursor_.offset());
    cursor_.advance();

    return Token{
        TokenKind::StringLiteral,
        std::string(content),
        std::string(content),
        start
    };
}

Token Lexer::charLiteral(SourceLocation start, std::size_t startOffset)
{
    // A backslash escapes the next character, so '\'' is one quote (char-values).
    while (!cursor_.atEnd() && cursor_.peek() != '\'' && cursor_.peek() != '\n') {
        if (cursor_.peek() == '\\') {
            cursor_.advance();
            if (cursor_.atEnd() || cursor_.peek() == '\n') {
                break;
            }
        }
        cursor_.advance();
    }

    if (cursor_.atEnd() || cursor_.peek() == '\n') {
        return invalidToken(start, startOffset, "unterminated char literal");
    }

    cursor_.advance();
    return makeToken(TokenKind::CharLiteral, start, startOffset);
}

bool Lexer::isIdentifierStart(char ch)
{
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool Lexer::isIdentifierPart(char ch)
{
    return isIdentifierStart(ch) || isDigit(ch);
}

bool Lexer::isDigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

bool Lexer::isHexDigit(char ch)
{
    return isDigit(ch) ||
           (ch >= 'a' && ch <= 'f') ||
           (ch >= 'A' && ch <= 'F');
}

std::string Lexer::normalize(std::string_view text)
{
    std::string normalized;
    normalized.reserve(text.size());
    for (const char ch : text) {
        normalized.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

bool Lexer::isPredefinedName(std::string_view normalized)
{
    return std::find(kPredefinedNames.begin(), kPredefinedNames.end(), normalized) != kPredefinedNames.end();
}

bool Lexer::isKeyword(std::string_view normalized)
{
    return std::find(kKeywords.begin(), kKeywords.end(), normalized) != kKeywords.end();
}

std::string tokenKindName(TokenKind kind)
{
    switch (kind) {
    case TokenKind::Identifier:
        return "Identifier";
    case TokenKind::Keyword:
        return "Keyword";
    case TokenKind::IntegerLiteral:
        return "IntegerLiteral";
    case TokenKind::FloatLiteral:
        return "FloatLiteral";
    case TokenKind::StringLiteral:
        return "StringLiteral";
    case TokenKind::CharLiteral:
        return "CharLiteral";
    case TokenKind::ColonEqual:
        return "ColonEqual";
    case TokenKind::Equal:
        return "Equal";
    case TokenKind::Hash:
        return "Hash";
    case TokenKind::PlusEqual:
        return "PlusEqual";
    case TokenKind::MinusEqual:
        return "MinusEqual";
    case TokenKind::StarEqual:
        return "StarEqual";
    case TokenKind::SlashEqual:
        return "SlashEqual";
    case TokenKind::CaretEqual:
        return "CaretEqual";
    case TokenKind::Pipe:
        return "Pipe";
    case TokenKind::Less:
        return "Less";
    case TokenKind::Greater:
        return "Greater";
    case TokenKind::LessEqual:
        return "LessEqual";
    case TokenKind::GreaterEqual:
        return "GreaterEqual";
    case TokenKind::Plus:
        return "Plus";
    case TokenKind::Minus:
        return "Minus";
    case TokenKind::Star:
        return "Star";
    case TokenKind::Slash:
        return "Slash";
    case TokenKind::Caret:
        return "Caret";
    case TokenKind::DotDot:
        return "DotDot";
    case TokenKind::LeftParen:
        return "LeftParen";
    case TokenKind::RightParen:
        return "RightParen";
    case TokenKind::LeftBracket:
        return "LeftBracket";
    case TokenKind::RightBracket:
        return "RightBracket";
    case TokenKind::Comma:
        return "Comma";
    case TokenKind::Dot:
        return "Dot";
    case TokenKind::Colon:
        return "Colon";
    case TokenKind::Semicolon:
        return "Semicolon";
    case TokenKind::Underscore:
        return "Underscore";
    case TokenKind::EndOfFile:
        return "EndOfFile";
    case TokenKind::Invalid:
        return "Invalid";
    }
    return "Unknown";
}

} // namespace inox::compiler::lexer
