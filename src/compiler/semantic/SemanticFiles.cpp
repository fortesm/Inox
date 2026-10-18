// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: text files (OpenSpec changes file-io and module-pairs).
// Std.Files holds the File type and the basic operations (open, read, write,
// close); Std.IOUtils, which uses Std.Files, the conveniences (Exists,
// DeleteFile, RenameFile) (canon r3.64 module pairs). They are compiler
// intrinsics that exist only in a program that uses the module, so their
// names (Create, Close, Write, Exists, ...) stay free elsewhere.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"

#include <array>
#include <string>
#include <string_view>

namespace inox::compiler::semantic {

using namespace detail;

namespace {

struct FileFunction {
    std::string_view name;
    // Fixed parameters; a variadic function takes values after them.
    std::array<std::string_view, 2> parameters;
    std::size_t parameterCount;
    std::string_view result;
    bool variadic = false;
    std::size_t minimumValues = 0;
    bool convenience = false;  // declared in Std.IOUtils, not Std.Files
};

// F2 opening, F4 text I/O, F7 file system (OpenSpec change file-io).
constexpr std::array<FileFunction, 11> kFileFunctions{{
    {"OpenRead", {"String", ""}, 1, "File"},
    {"Create", {"String", ""}, 1, "File"},
    {"OpenAppend", {"String", ""}, 1, "File"},
    {"ReadLn", {"File", ""}, 1, "String"},
    {"Eof", {"File", ""}, 1, "Bool"},
    {"Close", {"File", ""}, 1, "Void"},
    {"Write", {"File", ""}, 1, "Void", true, 1},
    {"WriteLn", {"File", ""}, 1, "Void", true, 0},
    {"Exists", {"String", ""}, 1, "Bool", false, 0, true},
    {"DeleteFile", {"String", ""}, 1, "Void", false, 0, true},
    {"RenameFile", {"String", "String"}, 2, "Void", false, 0, true},
}};

const FileFunction* findFileFunction(std::string_view name)
{
    for (const FileFunction& function : kFileFunctions) {
        if (equalsIgnoreCase(function.name, name)) {
            return &function;
        }
    }
    return nullptr;
}

} // namespace

void SemanticAnalyzer::declareStandardModuleNames(const ast::ModuleNode& module)
{
    ioUtilsVisible_ = usesModule(module, "Std.IOUtils");
    // Std.IOUtils uses Std.Files, so it also gives the type and the basics.
    filesVisible_ = ioUtilsVisible_ || usesModule(module, "Std.Files");
    if (filesVisible_ && !types().isDeclared("File")) {
        declareTypeName("File", types().fileType());
    }
    debugVisible_ = usesModule(module, "Std.Debug");
    if (debugVisible_ && !symbols_.globalScope().containsLocal("Assert")) {
        symbols_.globalScope().declare("Assert", SymbolKind::Builtin);
    }
    declareStringFunctionNames(module);
    for (const FileFunction& function : kFileFunctions) {
        // ReadLn is also the console prelude function (no argument).
        if (fileFunctionVisible(function.name) && !symbols_.globalScope().containsLocal(function.name)) {
            symbols_.globalScope().declare(std::string(function.name), SymbolKind::Builtin);
        }
    }
}

bool SemanticAnalyzer::fileFunctionVisible(std::string_view name) const
{
    const FileFunction* function = findFileFunction(name);
    return function != nullptr && (function->convenience ? ioUtilsVisible_ : filesVisible_);
}

bool SemanticAnalyzer::isFileType(TypeRef type)
{
    return type != nullptr && type->isFile();
}

bool SemanticAnalyzer::isFileFunction(std::string_view name, std::size_t argumentCount)
{
    const FileFunction* function = findFileFunction(name);
    if (function == nullptr) {
        return false;
    }
    return !equalsIgnoreCase(name, "ReadLn") || argumentCount == 1;
}

TypeRef SemanticAnalyzer::analyzeFileCall(std::string_view name, const ast::CallExpression& call)
{
    rejectInContract("the file function " + std::string(name));
    const FileFunction& function = *findFileFunction(name);
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    const std::string functionName(function.name);
    if (function.convenience && !ioUtilsVisible_) {
        throw SemanticError(functionName + " is declared in Std.IOUtils: add `Use Std.IOUtils`");
    }
    if (!filesVisible_) {
        throw SemanticError(functionName + " is declared in Std.Files: add `Use Std.Files`");
    }
    const Symbol& symbol = resolveOrThrow(callee.name());
    result_.bind(callee, symbol);
    result_.bind(call, symbol);

    const auto& arguments = call.arguments();
    const std::size_t fixed = function.parameterCount;
    if (function.variadic) {
        if (arguments.size() < fixed + function.minimumValues) {
            throw SemanticError(functionName + " expects a File and at least one value");
        }
    } else if (arguments.size() != fixed) {
        throw SemanticError(functionName + " expects " + std::to_string(fixed) +
                            (fixed == 1 ? " argument" : " arguments") + ", got " +
                            std::to_string(arguments.size()));
    }
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const TypeRef type = analyzeExpression(*arguments[index]);
        if (index < fixed) {
            const std::string_view expected = function.parameters[index];
            if (expected == "File" ? !isFileType(type) : types::nameOf(type) != expected) {
                throw SemanticError("argument " + std::to_string(index + 1) + " of " + functionName +
                                    " expects " + std::string(expected) + ", got " + types::nameOf(type));
            }
            continue;
        }
        // Write and WriteLn take the values PutLn prints.
        if (type == nullptr ||
            (!type->isString() && !type->isBool() && type != types().float64Type() && !type->isInteger())) {
            throw SemanticError("argument " + std::to_string(index + 1) + " of " + functionName +
                                " cannot be written yet: " + types::nameOf(type));
        }
    }
    if (function.result == "File") {
        return types().fileType();
    }
    return types().builtin(function.result) != nullptr ? types().builtin(function.result) : types().voidType();
}

void SemanticAnalyzer::requireFreshFile(const ast::Expression& value) const
{
    if (value.kind() == ast::AstNodeKind::CallExpression) {
        const auto& call = static_cast<const ast::CallExpression&>(value);
        if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
            const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
            if (equalsIgnoreCase(name, "OpenRead") || equalsIgnoreCase(name, "Create") ||
                equalsIgnoreCase(name, "OpenAppend")) {
                return;
            }
        }
    }
    throw SemanticError("a File cannot be copied: store a file just opened (OpenRead, Create, "
                        "OpenAppend) or pass the File as ref mut");
}

void SemanticAnalyzer::rejectFileIn(TypeRef type, const std::string& what)
{
    if (isFileType(type)) {
        throw SemanticError("a File cannot be " + what +
                            " (a File lives in a local variable or a ref mut parameter and is never copied)");
    }
}

} // namespace inox::compiler::semantic
