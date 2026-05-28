#include "../lexer/Lexer.h"
#include "../parser/Parser.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

using Token = inox::compiler::lexer::Token;
using TokenKind = inox::compiler::lexer::TokenKind;

void throwOnInvalidToken(const std::vector<Token>& tokens)
{
    for (const auto& token : tokens) {
        if (token.kind == TokenKind::Invalid) {
            throw inox::compiler::parser::ParseError(token.normalized, token.location);
        }
    }
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
        const auto tokens = lexer.tokenize();
        throwOnInvalidToken(tokens);

        inox::compiler::parser::Parser parser(tokens);
        parser.parseModule();

        std::cout << "parse ok\n";
    } catch (const inox::compiler::parser::ParseError& error) {
        std::cerr
            << "parse error at "
            << error.location().line << ':'
            << error.location().column << ": "
            << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << ": " << argv[1] << '\n';
        return 1;
    }

    return 0;
}
