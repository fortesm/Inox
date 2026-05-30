#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../semantic/SemanticAnalyzer.h"
#include "../semantic/SemanticDumper.h"

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
    const bool dumpTypes = argc == 3 && std::string(argv[1]) == "--dump-types";
    if ((!dumpTypes && argc != 2) || (dumpTypes && argc != 3)) {
        std::cerr << "usage: inox [--dump-types] <source.inox>\n";
        return 1;
    }

    try {
        const char* sourcePath = dumpTypes ? argv[2] : argv[1];
        const std::string source = readFile(sourcePath);
        inox::compiler::lexer::Lexer lexer(source);
        const auto tokens = lexer.tokenize();
        throwOnInvalidToken(tokens);

        inox::compiler::parser::Parser parser(tokens);
        auto module = parser.parseModule();

        std::cout << "parse ok\n";

        inox::compiler::semantic::SemanticAnalyzer semanticAnalyzer;
        const auto& semanticResult = semanticAnalyzer.analyze(*module);

        std::cout << "semantic ok\n";
        if (dumpTypes) {
            inox::compiler::semantic::SemanticDumper(std::cout, semanticResult).dump(*module);
        }
    } catch (const inox::compiler::parser::ParseError& error) {
        std::cerr
            << "parse error at "
            << error.location().line << ':'
            << error.location().column << ": "
            << error.what() << '\n';
        return 1;
    } catch (const inox::compiler::semantic::SemanticError& error) {
        std::cerr << "semantic error: " << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
