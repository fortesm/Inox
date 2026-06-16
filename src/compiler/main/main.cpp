// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "../codegen/LlvmIrEmitter.h"
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../semantic/SemanticAnalyzer.h"
#include "../semantic/SemanticDumper.h"
#include "../support/Environment.h"
#include "../support/FileSystem.h"
#include "../support/Platform.h"
#include "../support/Process.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

namespace fs = std::filesystem;

std::string readFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("could not open input file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void writeFile(const fs::path& path, std::string_view contents)
{
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("could not write output file: " + path.string());
    }
    output << contents;
}

using Token = inox::compiler::lexer::Token;
using TokenKind = inox::compiler::lexer::TokenKind;
using ModuleNode = inox::compiler::ast::ModuleNode;
using UseDeclaration = inox::compiler::ast::UseDeclaration;

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

std::string normalize(std::string_view text)
{
    std::string result;
    result.reserve(text.size());
    for (const char ch : text) {
        result.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return result;
}

bool equalsIgnoreCase(std::string_view left, std::string_view right)
{
    return normalize(left) == normalize(right);
}

std::unique_ptr<ModuleNode> parseSource(const fs::path& sourcePath)
{
    const std::string source = readFile(sourcePath);
    inox::compiler::lexer::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    throwOnInvalidToken(tokens);
    inox::compiler::parser::Parser parser(std::move(tokens));
    return parser.parseModule();
}

std::string useModuleName(const UseDeclaration& declaration)
{
    if (declaration.path().size() != 1 || declaration.path().front().empty()) {
        throw std::runtime_error("invalid Use declaration");
    }
    return declaration.path().front();
}

bool isPreludeModule(std::string_view name)
{
    return equalsIgnoreCase(name, "Sys.IO") ||
           equalsIgnoreCase(name, "Sys.Math") ||
           equalsIgnoreCase(name, "Sys.Std");
}

class ModuleLoader {
public:
    ModuleLoader(fs::path sourceDirectory, fs::path standardLibraryDirectory)
        : sourceDirectory_(std::move(sourceDirectory)),
          standardLibraryDirectory_(std::move(standardLibraryDirectory))
    {
    }

    std::unique_ptr<ModuleNode> loadProgram(const fs::path& entryPath)
    {
        loadModule(entryPath, {});
        if (modules_.empty()) {
            throw std::runtime_error("program has no modules");
        }

        auto program = std::make_unique<ModuleNode>(entryModuleName_);
        for (auto& module : modules_) {
            for (auto& item : module->items()) {
                if (item->kind() != inox::compiler::ast::AstNodeKind::UseDeclaration) {
                    program->items().push_back(std::move(item));
                }
            }
        }
        return program;
    }

private:
    void loadModule(const fs::path& sourcePath, std::string_view expectedName)
    {
        auto module = parseSource(sourcePath);
        if (!expectedName.empty() && !equalsIgnoreCase(module->name(), expectedName)) {
            throw std::runtime_error(
                "module name mismatch: expected " + std::string(expectedName) +
                ", got " + module->name());
        }

        const std::string key = normalize(module->name());
        if (loading_.contains(key)) {
            throw std::runtime_error("cyclic Use dependency involving module: " + module->name());
        }
        if (loaded_.contains(key)) {
            return;
        }
        if (entryModuleName_.empty()) {
            entryModuleName_ = module->name();
        }

        loading_.insert(key);
        for (const auto& item : module->items()) {
            if (item->kind() != inox::compiler::ast::AstNodeKind::UseDeclaration) {
                continue;
            }
            const std::string dependencyName =
                useModuleName(static_cast<const UseDeclaration&>(*item));
            if (!isPreludeModule(dependencyName)) {
                loadModule(resolveDependency(dependencyName), dependencyName);
            }
        }
        loading_.erase(key);
        loaded_.insert(key);
        modules_.push_back(std::move(module));
    }

    fs::path resolveDependency(std::string_view moduleName) const
    {
        std::string nestedName(moduleName);
        std::replace(nestedName.begin(), nestedName.end(), '.', '/');
        for (const fs::path& directory : {sourceDirectory_, standardLibraryDirectory_}) {
            if (directory.empty()) {
                continue;
            }
            fs::path flat = directory / (std::string(moduleName) + ".inox");
            if (fs::is_regular_file(flat)) {
                return flat;
            }
            fs::path nested = directory / (nestedName + ".inox");
            if (fs::is_regular_file(nested)) {
                return nested;
            }
        }

        throw std::runtime_error("could not resolve module: " + std::string(moduleName));
    }

    fs::path sourceDirectory_;
    fs::path standardLibraryDirectory_;
    std::string entryModuleName_;
    std::unordered_set<std::string> loading_;
    std::unordered_set<std::string> loaded_;
    std::vector<std::unique_ptr<ModuleNode>> modules_;
};

fs::path pathFromEnvironment(const char* variableName)
{
    const auto value = inox::compiler::support::getEnvironmentVariable(variableName);
    return value.has_value() ? fs::path(*value) : fs::path{};
}

void requireDirectoryIfSet(const char* variableName, const fs::path& directory)
{
    if (directory.empty()) {
        return;
    }
    if (!fs::is_directory(directory)) {
        throw std::runtime_error(
            std::string(variableName) + " does not point to an existing directory: " +
            directory.string());
    }
}

fs::path findStandardLibraryDirectory(const fs::path& sourcePath, const fs::path& executableDir)
{
    const fs::path environmentCandidate = pathFromEnvironment("INOX_STDLIB");
    requireDirectoryIfSet("INOX_STDLIB", environmentCandidate);
    if (!environmentCandidate.empty()) {
        return environmentCandidate;
    }

    if (!executableDir.empty()) {
        const fs::path releaseLayoutCandidate = executableDir.parent_path() / "stdlib";
        if (fs::is_directory(releaseLayoutCandidate)) {
            return releaseLayoutCandidate;
        }

        const fs::path siblingCandidate = executableDir / "stdlib";
        if (fs::is_directory(siblingCandidate)) {
            return siblingCandidate;
        }
    }

    const fs::path workingDirectoryCandidate = fs::current_path() / "stdlib";
    if (fs::is_directory(workingDirectoryCandidate)) {
        return workingDirectoryCandidate;
    }

    for (fs::path directory = sourcePath.parent_path();
         !directory.empty();
         directory = directory.parent_path()) {
        const fs::path candidate = directory / "stdlib";
        if (fs::is_directory(candidate)) {
            return candidate;
        }
        if (directory == directory.parent_path()) {
            break;
        }
    }

    return {};
}

std::unique_ptr<ModuleNode> loadProgram(const fs::path& sourcePath, const fs::path& executableDir)
{
    const fs::path absolutePath = fs::absolute(sourcePath);
    return ModuleLoader(
        absolutePath.parent_path(),
        findStandardLibraryDirectory(absolutePath, executableDir)).loadProgram(absolutePath);
}

std::string shellQuote(const fs::path& path)
{
    std::string text = path.string();
    std::string quoted = "\"";
    for (const char ch : text) {
        quoted += ch == '"' ? "\\\"" : std::string(1, ch);
    }
    quoted += '"';
    return quoted;
}

bool clangExists()
{
    return inox::compiler::support::commandExists("clang");
}

struct BuildArtifacts {
    fs::path llvmIr;
    fs::path executable;
};

BuildArtifacts buildProgram(const fs::path& sourcePath, const ModuleNode& module)
{
    if (!clangExists()) {
        throw std::runtime_error(
            "clang was not found; install LLVM/Clang or put clang in PATH");
    }

    fs::path outputDirectory = pathFromEnvironment("INOX_OUTPUT_DIR");
    if (outputDirectory.empty()) {
        outputDirectory = fs::current_path() / "build" / "inox-artifacts";
    }
    fs::create_directories(outputDirectory);
    const std::string stem = sourcePath.stem().string();
    const BuildArtifacts artifacts{
        outputDirectory / (stem + ".ll"),
        outputDirectory / (stem + std::string(inox::compiler::support::executableSuffix()))};
    writeFile(artifacts.llvmIr, inox::compiler::codegen::LlvmIrEmitter().emit(module));

    std::string command =
        "clang " + shellQuote(artifacts.llvmIr) + " -o " + shellQuote(artifacts.executable);
    if (inox::compiler::support::hostOperatingSystem() != inox::compiler::support::OperatingSystem::Windows) {
        command += " -lm";
    }
    command += " > " + std::string(inox::compiler::support::nullDevicePath()) + " 2>&1";
    if (inox::compiler::support::runShellCommand(command) != 0) {
        throw std::runtime_error("clang failed while building: " + sourcePath.string());
    }
    return artifacts;
}

} // namespace

int main(int argc, char** argv)
{
    const bool dumpTypes = argc == 3 && std::string(argv[1]) == "--dump-types";
    const bool dumpTokensMode = argc == 3 && std::string(argv[1]) == "--dump-tokens";
    const bool parseOnly = argc == 3 && std::string(argv[1]) == "--parse-only";
    const bool emitLlvm = argc == 3 && std::string(argv[1]) == "--emit-llvm";
    const bool build = argc == 3 && std::string(argv[1]) == "--build";
    const bool run = argc == 3 && std::string(argv[1]) == "--run";
    const fs::path executableDir = inox::compiler::support::executableDirectory(argc > 0 ? argv[0] : nullptr);
    const bool hasMode = dumpTypes || dumpTokensMode || parseOnly || emitLlvm || build || run;
    if ((!hasMode && argc != 2) || (hasMode && argc != 3)) {
        std::cerr << "usage: inox [--dump-tokens|--parse-only|--dump-types|--emit-llvm|--build|--run] <source.inox>\n";
        return 1;
    }

    try {
        const char* sourcePath = hasMode ? argv[2] : argv[1];
        const std::string source = readFile(fs::path(sourcePath));
        inox::compiler::lexer::Lexer lexer(source);
        const auto tokens = lexer.tokenize();

        if (dumpTokensMode) {
            dumpTokens(tokens);
            throwOnInvalidToken(tokens);
            return 0;
        }

        throwOnInvalidToken(tokens);

        std::unique_ptr<ModuleNode> module;
        if (dumpTypes || emitLlvm || build || run || !hasMode) {
            module = loadProgram(fs::path(sourcePath), executableDir);
        } else {
            inox::compiler::parser::Parser parser(tokens);
            module = parser.parseModule();
        }

        if (!emitLlvm && !build && !run) {
            std::cout << "parse ok\n";
        }

        if (parseOnly) {
            return 0;
        }

        inox::compiler::semantic::SemanticAnalyzer semanticAnalyzer;
        const auto& semanticResult = semanticAnalyzer.analyze(*module);

        if (emitLlvm) {
            std::cout << inox::compiler::codegen::LlvmIrEmitter().emit(*module);
        } else if (build || run) {
            const BuildArtifacts artifacts = buildProgram(fs::path(sourcePath), *module);
            if (run) {
                return inox::compiler::support::runShellCommand(shellQuote(artifacts.executable)) == 0 ? 0 : 1;
            }
            std::cout << artifacts.executable.string() << '\n';
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
