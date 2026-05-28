#include "../lexer/Lexer.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string readFile(const char* path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("could not open input file");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void printToken(const inox::compiler::lexer::Token& token)
{
    std::cout
        << token.location.line << ':'
        << token.location.column << ' '
        << inox::compiler::lexer::tokenKindName(token.kind)
        << " lexeme=" << std::quoted(token.lexeme);

    if (token.kind == inox::compiler::lexer::TokenKind::Identifier ||
        token.kind == inox::compiler::lexer::TokenKind::Keyword) {
        std::cout << " normalized=" << std::quoted(token.normalized);
    }

    if (token.kind == inox::compiler::lexer::TokenKind::Invalid) {
        std::cout << " error=" << std::quoted(token.normalized);
    }

    std::cout << '\n';
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "usage: inox <source.inox>\n";
        return 1;
    }

    try {
        const std::string source = readFile(argv[1]);
        inox::compiler::lexer::Lexer lexer(source);

        for (const auto& token : lexer.tokenize()) {
            printToken(token);
            if (token.kind == inox::compiler::lexer::TokenKind::Invalid) {
                return 1;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << ": " << argv[1] << '\n';
        return 1;
    }

    return 0;
}
