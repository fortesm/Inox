#include "../codegen/LlvmIrEmitter.h"
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

void dumpTokens(const std::vector<Token>& tokens)
{
    for (const auto& token : tokens) {
        std::cout
            << token.location.line << ':'
            << token.location.column << ' '
            << inox::compiler::lexer::tokenKindName(token.kind)
            << " lexeme=\"" << token.lexeme << "\""
            << " normalized=\"" << token.normalized << "\""
            << '\n';
    }
}

} // namespace

int main(int argc, char** argv)
{
    const bool dumpTypes = argc == 3 && std::string(argv[1]) == "--dump-types";
    const bool dumpTokensMode = argc == 3 && std::string(argv[1]) == "--dump-tokens";
    const bool parseOnly = argc == 3 && std::string(argv[1]) == "--parse-only";
    const bool emitLlvm = argc == 3 && std::string(argv[1]) == "--emit-llvm";
    const bool hasMode = dumpTypes || dumpTokensMode || parseOnly || emitLlvm;
    if ((!hasMode && argc != 2) || (hasMode && argc != 3)) {
        std::cerr << "usage: inox [--dump-tokens|--parse-only|--dump-types|--emit-llvm] <source.inox>\n";
        return 1;
    }

    try {
        const char* sourcePath = hasMode ? argv[2] : argv[1];
        const std::string source = readFile(sourcePath);
        inox::compiler::lexer::Lexer lexer(source);
        const auto tokens = lexer.tokenize();

        if (dumpTokensMode) {
            dumpTokens(tokens);
            throwOnInvalidToken(tokens);
            return 0;
        }

        throwOnInvalidToken(tokens);

        inox::compiler::parser::Parser parser(tokens);
        auto module = parser.parseModule();

        if (!emitLlvm) {
            std::cout << "parse ok\n";
        }

        if (parseOnly) {
            return 0;
        }

        inox::compiler::semantic::SemanticAnalyzer semanticAnalyzer;
        const auto& semanticResult = semanticAnalyzer.analyze(*module);

        if (emitLlvm) {
            std::cout << inox::compiler::codegen::LlvmIrEmitter().emit(*module);
        } else {
            std::cout << "semantic ok\n";
        }
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
    } catch (const inox::compiler::codegen::CodegenError& error) {
        std::cerr << "codegen error: " << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
