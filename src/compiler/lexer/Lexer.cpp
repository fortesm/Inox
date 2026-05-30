#include "Lexer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

namespace inox::compiler::lexer {

namespace {

constexpr std::array<std::string_view, 44> kKeywords = {
    "module", "use", "type", "const", "var", "state", "struct", "enum",
    "main", "if", "elif", "else", "unless", "while", "repeat", "until",
    "for", "in", "case", "otherwise", "try", "except", "finally", "raise",
    "end", "mut", "div", "mod", "and", "xor", "or", "not",
    "bitand", "bitor", "bitxor", "bitnot", "shr", "shl", "break", "continue",
    "return", "exit", "true", "false"
};

} // namespace

Lexer::Lexer(std::string_view source) : source_(source) {}

Token Lexer::nextToken()
{
    skipWhitespaceAndComments();

    SourceLocation start{current_, line_, column_};
    const std::size_t startOffset = current_;

    if (isAtEnd()) {
        return makeToken(TokenKind::EndOfFile, start, startOffset);
    }

    const char ch = advance();

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
        if (match('=')) {
            return makeToken(TokenKind::ColonEqual, start, startOffset);
        }
        return makeToken(TokenKind::Colon, start, startOffset);
    case '=':
        return makeToken(TokenKind::Equal, start, startOffset);
    case '#':
        return makeToken(TokenKind::Hash, start, startOffset);
    case '<':
        if (match('=')) {
            return makeToken(TokenKind::LessEqual, start, startOffset);
        }
        return makeToken(TokenKind::Less, start, startOffset);
    case '>':
        if (match('=')) {
            return makeToken(TokenKind::GreaterEqual, start, startOffset);
        }
        return makeToken(TokenKind::Greater, start, startOffset);
    case '+':
        return makeToken(TokenKind::Plus, start, startOffset);
    case '-':
        return makeToken(TokenKind::Minus, start, startOffset);
    case '*':
        return makeToken(TokenKind::Star, start, startOffset);
    case '/':
        return makeToken(TokenKind::Slash, start, startOffset);
    case '^':
        return makeToken(TokenKind::Caret, start, startOffset);
    case '$':
        return dollarHexNumber(start, startOffset);
    case '.':
        if (match('.')) {
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

bool Lexer::isAtEnd() const
{
    return current_ >= source_.size();
}

char Lexer::peek() const
{
    return isAtEnd() ? '\0' : source_[current_];
}

char Lexer::peekNext() const
{
    return current_ + 1 >= source_.size() ? '\0' : source_[current_ + 1];
}

char Lexer::advance()
{
    const char ch = source_[current_++];
    if (ch == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return ch;
}

bool Lexer::match(char expected)
{
    if (isAtEnd() || source_[current_] != expected) {
        return false;
    }
    advance();
    return true;
}

void Lexer::skipWhitespaceAndComments()
{
    for (;;) {
        while (!isAtEnd()) {
            const char ch = peek();
            if (ch == ' ' || ch == '\r' || ch == '\t' || ch == '\n') {
                advance();
            } else {
                break;
            }
        }

        if (peek() == '=' && peekNext() == '=') {
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
            continue;
        }

        break;
    }
}

Token Lexer::makeToken(TokenKind kind, SourceLocation start, std::size_t startOffset)
{
    const auto text = source_.substr(startOffset, current_ - startOffset);
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
    while (isIdentifierPart(peek())) {
        advance();
    }

    Token token = makeToken(TokenKind::Identifier, start, startOffset);
    if (isKeyword(token.normalized)) {
        token.kind = TokenKind::Keyword;
    }
    return token;
}

Token Lexer::number(SourceLocation start, std::size_t startOffset)
{
    if (source_[startOffset] == '0' && (peek() == 'x' || peek() == 'X')) {
        advance();
        if (!isHexDigit(peek())) {
            return invalidToken(start, startOffset, "expected hexadecimal digit after 0x");
        }
        while (isHexDigit(peek())) {
            advance();
        }
        return makeToken(TokenKind::IntegerLiteral, start, startOffset);
    }

    while (isDigit(peek())) {
        advance();
    }

    if (peek() == '.' && isDigit(peekNext())) {
        advance();
        while (isDigit(peek())) {
            advance();
        }
        return makeToken(TokenKind::FloatLiteral, start, startOffset);
    }

    return makeToken(TokenKind::IntegerLiteral, start, startOffset);
}

Token Lexer::dollarHexNumber(SourceLocation start, std::size_t startOffset)
{
    if (!isHexDigit(peek())) {
        return invalidToken(start, startOffset, "expected hexadecimal digit after '$'");
    }

    while (isHexDigit(peek())) {
        advance();
    }

    return makeToken(TokenKind::IntegerLiteral, start, startOffset);
}

Token Lexer::stringLiteral(SourceLocation start, std::size_t startOffset)
{
    bool doubledDelimiter = false;
    if (peek() == '"') {
        for (std::size_t index = current_ + 1; index + 1 < source_.size(); ++index) {
            if (source_[index] == '\n') {
                break;
            }
            if (source_[index] == '"' && source_[index + 1] == '"') {
                doubledDelimiter = true;
                break;
            }
        }
    }

    if (doubledDelimiter) {
        advance();
    }

    const std::size_t contentStart = current_;
    bool escaped = false;

    while (!isAtEnd() && peek() != '\n') {
        if (peek() == '"' && !escaped) {
            break;
        }

        escaped = peek() == '\\' && !escaped;
        if (peek() != '\\') {
            escaped = false;
        }
        advance();
    }

    if (isAtEnd() || peek() == '\n') {
        return invalidToken(start, startOffset, "unterminated string literal");
    }

    const auto content = source_.substr(contentStart, current_ - contentStart);
    advance();
    if (doubledDelimiter && peek() == '"') {
        advance();
    }

    return Token{
        TokenKind::StringLiteral,
        std::string(content),
        std::string(content),
        start
    };
}

Token Lexer::charLiteral(SourceLocation start, std::size_t startOffset)
{
    while (!isAtEnd() && peek() != '\'' && peek() != '\n') {
        advance();
    }

    if (isAtEnd() || peek() == '\n') {
        return invalidToken(start, startOffset, "unterminated char literal");
    }

    advance();
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
    case TokenKind::EndOfFile:
        return "EndOfFile";
    case TokenKind::Invalid:
        return "Invalid";
    }
    return "Unknown";
}

} // namespace inox::compiler::lexer
