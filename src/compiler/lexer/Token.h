#pragma once

#include <cstddef>
#include <string>

namespace inox::compiler::lexer {

enum class TokenKind {
    Identifier,
    Keyword,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    CharLiteral,

    ColonEqual,
    Equal,
    Hash,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    Plus,
    Minus,
    Star,
    Slash,
    Caret,
    DotDot,

    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    Comma,
    Dot,
    Colon,
    Semicolon,

    EndOfFile,
    Invalid
};

struct SourceLocation {
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct Token {
    TokenKind kind = TokenKind::Invalid;
    std::string lexeme;
    std::string normalized;
    SourceLocation location;
};

std::string tokenKindName(TokenKind kind);

} // namespace inox::compiler::lexer
