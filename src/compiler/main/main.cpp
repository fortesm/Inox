// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "../codegen/LlvmIrEmitter.h"
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../semantic/ContractLowering.h"
#include "../semantic/SemanticAnalyzer.h"
#include "../semantic/SemanticDumper.h"
#include "../support/Environment.h"
#include "../support/FileSystem.h"
#include "../support/Platform.h"
#include "../support/Process.h"
#include "../support/Version.h"

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
#include <vector>
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

// CANON-3 (r3.76): a file is a module named after the file: the name its
// Use gives (`Use Calc.Core` finds Calc.Core.inox or Calc/Core.inox), or the
// file's stem for the entry file.
std::unique_ptr<ModuleNode> parseSource(const fs::path& sourcePath, std::string name)
{
    const std::string source = readFile(sourcePath);
    inox::compiler::lexer::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    throwOnInvalidToken(tokens);
    inox::compiler::parser::Parser parser(std::move(tokens));
    return parser.parseModule(name.empty() ? sourcePath.stem().string() : std::move(name));
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
            program->addLoadedModule(module->name());
            for (auto& item : module->items()) {
                if (item->kind() != inox::compiler::ast::AstNodeKind::UseDeclaration) {
                    program->items().push_back(std::move(item));
                }
            }
        }
        // CANON-24: contracts become checks once the whole program is known
        // (an Override inherits the contracts of methods in other modules).
        inox::compiler::semantic::lowerContracts(*program);
        return program;
    }

private:
    void loadModule(const fs::path& sourcePath, std::string_view expectedName)
    {
        auto module = parseSource(sourcePath, std::string(expectedName));

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

bool clangExists()
{
    return inox::compiler::support::commandExists("clang");
}

bool clangxxExists()
{
    return inox::compiler::support::commandExists("clang++");
}

fs::path findExceptionRuntimeLibrary(const fs::path& executableDir)
{
    const fs::path configured = pathFromEnvironment("INOX_RUNTIME_LIB");
    if (!configured.empty() && fs::is_regular_file(configured)) {
        return configured;
    }

#if defined(_WIN32)
    constexpr const char* runtimeName = "inoxrt.lib";
#else
    constexpr const char* runtimeName = "libinoxrt.a";
#endif

    const std::vector<fs::path> candidates{
        executableDir / runtimeName,
        executableDir.parent_path() / "lib" / runtimeName,
        fs::current_path() / "build" / "linux-clang-debug" / runtimeName
    };
    for (const auto& candidate : candidates) {
        if (fs::is_regular_file(candidate)) {
            return candidate;
        }
    }
    return {};
}

// Build the error for a failed clang/clang++ invocation: what was being built,
// the exit status, and the toolchain's own output (bounded, so a flood of
// diagnostics stays readable). The full output stays in `logPath`.
std::string toolchainFailureMessage(const std::string& tool, const fs::path& sourcePath, int exitCode,
                                    const fs::path& logPath)
{
    constexpr std::size_t kMaxLines = 40;
    std::string message = tool + " failed while building: " + sourcePath.string();
    message += exitCode < 0 ? std::string(" (could not be started)")
                            : " (exit code " + std::to_string(exitCode) + ")";
    std::ifstream log(logPath, std::ios::binary);
    std::vector<std::string> lines;
    for (std::string line; std::getline(log, line);) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    while (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }
    if (lines.empty()) {
        message += "\nnote: " + tool + " printed no diagnostics";
        return message;
    }
    message += "\n" + tool + " output:";
    const std::size_t shown = lines.size() < kMaxLines ? lines.size() : kMaxLines;
    for (std::size_t index = 0; index < shown; ++index) {
        message += "\n  " + lines[index];
    }
    if (lines.size() > shown) {
        message += "\n  ... (" + std::to_string(lines.size() - shown) + " more lines)";
    }
    message += "\nnote: full toolchain output: " + logPath.string();
    return message;
}

struct BuildArtifacts {
    fs::path llvmIr;
    fs::path executable;
};

BuildArtifacts buildProgram(const fs::path& sourcePath, const ModuleNode& module,
                            const inox::compiler::semantic::SemanticResult& semantics,
                            const fs::path& executableDir)
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
    const inox::compiler::codegen::LlvmModule llvmModule =
        inox::compiler::codegen::LlvmIrEmitter().emitModule(module, semantics);
    const std::string& llvmIr = llvmModule.ir;
    writeFile(artifacts.llvmIr, llvmIr);

    // E1 (change console-abi-terminal): every program links libinoxrt, which
    // holds the console, the exception transport and the String runtime.
    if (!llvmModule.needsInoxRuntime) {
        throw std::runtime_error("internal error: a module that does not declare libinoxrt");
    }
    if (!clangxxExists()) {
        throw std::runtime_error(
            "clang++ was not found; Inox programs link the C++ runtime library libinoxrt");
    }
    const fs::path runtimeLibrary = findExceptionRuntimeLibrary(executableDir);
    if (runtimeLibrary.empty()) {
        throw std::runtime_error(
            "libinoxrt was not found; set INOX_RUNTIME_LIB or keep the runtime library beside the Inox compiler");
    }
    std::vector<std::string> clangArgs{"clang++", artifacts.llvmIr.string(), "-o", artifacts.executable.string(),
                                       runtimeLibrary.string()};
    if (inox::compiler::support::hostOperatingSystem() != inox::compiler::support::OperatingSystem::Windows) {
        clangArgs.push_back("-lm");
    }
    // The toolchain's own diagnostics are kept: when clang (or the linker it
    // drives) fails, the user sees what it said instead of only "clang failed".
    const fs::path toolchainLog = outputDirectory / (stem + ".toolchain.log");
    const int clangExit = inox::compiler::support::runProcessCapturingOutput(clangArgs, toolchainLog.string());
    if (clangExit != 0) {
        throw std::runtime_error(toolchainFailureMessage(clangArgs.front(), sourcePath, clangExit, toolchainLog));
    }
    std::error_code ignored;
    fs::remove(toolchainLog, ignored);
    return artifacts;
}

} // namespace

namespace {

// The commands of the driver (CANON-23; Marcelo Fortes, 2026-10-13): the
// subcommands are the user's contract and the `--` switches their aliases;
// `--parse-only`, `--dump-tokens` and `--dump-types` are developer switches.
enum class Command {
    Run,       // inox <file>, inox run <file>, --run
    Build,     // inox build <file>, --build
    Check,     // inox check <file>: parse and semantic analysis
    EmitLlvm,  // inox emit-llvm <file>, --emit-llvm
    ParseOnly,
    DumpTokens,
    DumpTypes,
};

constexpr const char* kUsage =
    "usage: inox <file.inox> [args]       build and run the program, giving it args\n"
    "       inox run <file.inox> [args]   build and run the program, giving it args\n"
    "       inox build <file.inox>        build the program, print the executable path\n"
    "       inox check <file.inox>        parse and check the program\n"
    "       inox emit-llvm <file.inox>    print the LLVM IR\n"
    "       inox help                     print this help\n"
    "       inox version                  print the version\n"
    "developer switches: --parse-only, --dump-tokens, --dump-types <file.inox>;\n"
    "--run, --build, --emit-llvm, --help and --version are aliases\n";

bool commandFromWord(const std::string& word, Command& command)
{
    struct Entry {
        const char* word;
        Command command;
    };
    static constexpr Entry entries[] = {
        {"run", Command::Run},           {"--run", Command::Run},
        {"build", Command::Build},       {"--build", Command::Build},
        {"check", Command::Check},       {"emit-llvm", Command::EmitLlvm},
        {"--emit-llvm", Command::EmitLlvm}, {"--parse-only", Command::ParseOnly},
        {"--dump-tokens", Command::DumpTokens}, {"--dump-types", Command::DumpTypes},
    };
    for (const Entry& entry : entries) {
        if (word == entry.word) {
            command = entry.command;
            return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char** argv)
{
    const std::string first = argc > 1 ? argv[1] : "";
    if (argc == 2 && (first == "version" || first == "--version")) {
        std::cout << "inox " << inox::compiler::support::versionText() << ' '
                  << inox::compiler::support::kVersionStage << " (canon "
                  << inox::compiler::support::kCanonRevision << ")\n";
        return 0;
    }
    if (argc == 2 && (first == "help" || first == "--help" || first == "-h")) {
        std::cout << kUsage;
        return 0;
    }

    Command command = Command::Run;
    const char* sourcePath = nullptr;
    // D4 (r3.92): `inox run file.inox A B` and `inox file.inox A B` give the
    // words after the file to the program (ParamStr).
    std::vector<std::string> programArguments;
    if (argc >= 3 && commandFromWord(first, command)) {
        sourcePath = argv[2];
        if (argc > 3) {
            if (command != Command::Run) {
                std::cerr << kUsage;
                return 1;
            }
            programArguments.assign(argv + 3, argv + argc);
        }
    } else if (argc >= 2 && !first.empty() && first[0] != '-' && !commandFromWord(first, command)) {
        command = Command::Run;
        sourcePath = argv[1];
        programArguments.assign(argv + 2, argv + argc);
    } else {
        std::cerr << kUsage;
        return 1;
    }

    const bool dumpTypes = command == Command::DumpTypes;
    const bool dumpTokensMode = command == Command::DumpTokens;
    const bool parseOnly = command == Command::ParseOnly;
    const bool emitLlvm = command == Command::EmitLlvm;
    const bool build = command == Command::Build;
    const bool run = command == Command::Run;
    const fs::path executableDir = inox::compiler::support::executableDirectory(argc > 0 ? argv[0] : nullptr);

    try {
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
        if (!parseOnly) {
            module = loadProgram(fs::path(sourcePath), executableDir);
        } else {
            inox::compiler::parser::Parser parser(tokens);
            module = parser.parseModule(fs::path(sourcePath).stem().string());
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
            std::cout << inox::compiler::codegen::LlvmIrEmitter().emit(*module, semanticResult);
        } else if (build || run) {
            const BuildArtifacts artifacts = buildProgram(fs::path(sourcePath), *module, semanticResult, executableDir);
            if (run) {
                std::vector<std::string> programCommand{artifacts.executable.string()};
                programCommand.insert(programCommand.end(), programArguments.begin(), programArguments.end());
                const int programExit = inox::compiler::support::runProcess(programCommand, false);
                if (programExit == inox::compiler::codegen::kRuntimeFaultExitStatus) {
                    // The program already printed "Inox runtime error: <category>".
                    std::cerr << "inox: program stopped by an Inox runtime error (exit code "
                              << programExit << ")\n";
                    return 1;
                }
                if (programExit != 0) {
                    // Distinguish "the program failed at run time" (a crash, an abort,
                    // Halt(Code)) from a compile error; the program's own status is
                    // kept for a script (r3.92), 1 when it has none.
                    std::cerr << "inox: program exited with code " << programExit << "\n";
                    return programExit > 0 && programExit <= 255 ? programExit : 1;
                }
                return 0;
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
    } catch (const inox::compiler::codegen::CodegenUnsupported& error) {
        // The program passed the semantic analyzer: the construct is legal
        // Inox and the limitation is in this backend. Say so explicitly
        // instead of letting it look like a program error.
        std::cerr << "codegen error: not yet implemented in the LLVM backend: "
                  << error.what() << '\n';
        std::cerr << "note: this program is valid Inox; the gap is in code "
                     "generation (see docs/BACKEND_GAPS.md)\n";
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
