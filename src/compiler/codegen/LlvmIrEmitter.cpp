// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "LlvmIrEmitter.h"

#include "AstQueries.h"
#include "ExceptionAbi.h"
#include "FunctionEmitter.h"
#include "LlvmText.h"
#include "ModuleContext.h"
#include "RuntimeAbi.h"
#include "RuntimeHelpersIr.h"
#include "TypeLayout.h"
#include "../support/ArrayType.h"

#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace inox::compiler::codegen {

using namespace detail;

namespace {

void emitFunction(std::ostringstream& output,
                  const ast::FunctionDeclaration& function,
                  const FunctionSignature& signature,
                  const FunctionSignatures& signatures,
                  const StructDefinitions& structs,
                  ModuleContext& moduleContext,
                  const semantic::SemanticResult& semantics,
                  const StateGlobals& stateGlobals,
                  bool& usesStringCleanup)
{
    std::ostringstream body;
    FunctionEmitter emitter(body, function, signature, signatures, structs, moduleContext, semantics, stateGlobals);
    emitter.emit();
    usesStringCleanup = usesStringCleanup || emitter.usesStringCleanup();

    output << "define " << signature.llvmReturnType << " @" << signature.llvmName << '(';
    for (std::size_t index = 0; index < signature.parameters.size(); ++index) {
        if (index != 0) {
            output << ", ";
        }
        output << signature.parameters[index].llvmType << " %" << signature.parameters[index].llvmName;
    }
    output << ')';
    if (functionContainsTry(function) || emitter.usesStringCleanup()) {
        output << " personality ptr @" << moduleContext.exceptionAbi.personality();
    }
    output << " {\n"
           << "entry:\n";

    // Static allocas belong in the entry block: an alloca inside a loop would grow
    // the stack on every iteration. The empty initialization of owned String
    // slots follows them, before any code that can unwind.
    std::istringstream bodyLines(body.str());
    std::string hoisted;
    std::string entryInits;
    std::string remaining;
    for (std::string line; std::getline(bodyLines, line);) {
        const bool isAlloca = line.rfind("  %", 0) == 0 && line.find(" = alloca ") != std::string::npos;
        const bool isEntryInit = line.find(" ; inox.entry-init") != std::string::npos;
        (isAlloca ? hoisted : isEntryInit ? entryInits : remaining) += line + "\n";
    }
    output << hoisted << entryInits << remaining;
    output << "}\n\n";
}

} // namespace

CodegenError::CodegenError(std::string message)
    : std::runtime_error(std::move(message))
{
}

CodegenUnsupported::CodegenUnsupported(std::string message)
    : CodegenError(std::move(message))
{
}

std::string LlvmIrEmitter::emit(const ast::ModuleNode& module, const semantic::SemanticResult& semantics) const
{
    return emitModule(module, semantics).ir;
}

LlvmModule LlvmIrEmitter::emitModule(const ast::ModuleNode& module, const semantic::SemanticResult& semantics) const
{
    const ast::FunctionDeclaration* mainFunction = nullptr;
    FunctionSignatures signatures;
    StructDefinitions structs;
    std::ostringstream functionOutput;
    std::ostringstream output;
    ModuleContext moduleContext(hostExceptionAbi());
    const bool usesExceptions = moduleUsesExceptions(module);
    bool usesStringCleanup = false;

    for (const auto& item : module.items()) {
        if (item->kind() == ast::AstNodeKind::SectionDeclaration) {
            collectStructDefinitions(static_cast<const ast::SectionDeclaration&>(*item), structs, semantics);
        }
    }

    StateGlobals stateGlobals;
    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::SectionDeclaration) {
            continue;
        }
        const auto& section = static_cast<const ast::SectionDeclaration&>(*item);
        if (section.sectionKind() != ast::SectionKind::State) {
            continue;
        }
        for (const ast::VariableDeclaration& declaration : section.variables()) {
            std::string inoxType = declaration.typeName;
            if (support::isArrayTypeName(inoxType)) {
                inoxType = semantics.canonicalTypeName(inoxType);
            }
            if (inoxType.empty() && declaration.value != nullptr) {
                if (const semantic::ResolvedType* resolved = semantics.typeOf(*declaration.value)) {
                    inoxType = resolved->name;
                }
            }
            if (const StructDefinition* structType = findStruct(structs, inoxType)) {
                inoxType = structType->inoxName;
            }
            const std::string llvmType = llvmTypeForInoxType(inoxType, structs);
            if (llvmType.empty()) {
                throw CodegenUnsupported("State variables of type " + inoxType + " are not lowered yet");
            }
            stateGlobals.push_back(StateGlobal{declaration.name, inoxType, llvmType, declaration.value.get()});
        }
    }

    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::FunctionDeclaration) {
            continue;
        }

        const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
        if (equalsIgnoreCase(function.name(), "Main")) {
            mainFunction = &function;
        } else {
            const std::string normalizedName = normalize(function.name());
            FunctionSignature signature = parseFunctionSignature(function, structs, semantics);
            // CANON-9: a method of a built-in type is also found under the
            // type's canonical name (`Integer.Twice` on an Int64 value).
            if (const std::size_t dot = function.name().find('.'); dot != std::string::npos) {
                const std::string receiver = function.name().substr(0, dot);
                const std::string canonical = semantics.canonicalTypeName(receiver);
                if (findStruct(structs, receiver) == nullptr && normalize(canonical) != normalize(receiver)) {
                    signatures.emplace(normalize(canonical + function.name().substr(dot)), signature);
                }
            }
            signatures.emplace(normalizedName, std::move(signature));
        }
    }

    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::FunctionDeclaration) {
            continue;
        }

        const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
        if (!equalsIgnoreCase(function.name(), "Main")) {
            const auto signature = signatures.find(normalize(function.name()));
            emitFunction(functionOutput, function, signature->second, signatures, structs, moduleContext, semantics, stateGlobals, usesStringCleanup);
        }
    }

    if (mainFunction == nullptr) {
        throw CodegenError("LLVM emission requires Main");
    }
    const FunctionSignature mainSignature{"main", "i32", {}, {}};
    emitFunction(functionOutput, *mainFunction, mainSignature, signatures, structs, moduleContext, semantics, stateGlobals, usesStringCleanup);

    for (const StateGlobal& state : stateGlobals) {
        output << stateGlobalSymbol(state.name) << " = internal global " << state.llvmType
               << " zeroinitializer\n";
    }
    if (!stateGlobals.empty()) {
        output << '\n';
    }

    for (const auto& [_, structType] : structs) {
        output << structType.llvmName << " = type { ";
        for (std::size_t index = 0; index < structType.fields.size(); ++index) {
            if (index != 0) {
                output << ", ";
            }
            output << structType.fields[index].llvmType;
        }
        output << " }\n";
    }
    if (!structs.empty()) {
        output << '\n';
    }

    output << "@.inox.fmt.i64.nl = private unnamed_addr constant [6 x i8] c\"%lld\\0A\\00\"\n"
           << "@.inox.fmt.i64 = private unnamed_addr constant [5 x i8] c\"%lld\\00\"\n"
           << "@.inox.fmt.f64.nl = private unnamed_addr constant [4 x i8] c\"%f\\0A\\00\"\n"
           << "@.inox.fmt.f64 = private unnamed_addr constant [3 x i8] c\"%f\\00\"\n"
           << "@.inox.fmt.str.nl = private unnamed_addr constant [4 x i8] c\"%s\\0A\\00\"\n"
           << "@.inox.fmt.str = private unnamed_addr constant [3 x i8] c\"%s\\00\"\n"
           << "@.inox.true = private unnamed_addr constant [5 x i8] c\"true\\00\"\n"
           << "@.inox.false = private unnamed_addr constant [6 x i8] c\"false\\00\"\n";
    for (const std::string& global : moduleContext.moduleLines) {
        output << global << '\n';
    }
    output << "declare i32 @printf(ptr, ...)\n";
    output << "declare i32 @getchar()\n";
    output << "declare double @llvm.sqrt.f64(double)\n";
    output << "declare double @llvm.sin.f64(double)\n";
    output << "declare double @llvm.cos.f64(double)\n";
    output << "declare double @llvm.exp.f64(double)\n";
    output << "declare double @llvm.log.f64(double)\n";
    output << "declare double @llvm.log2.f64(double)\n";
    output << "declare double @llvm.log10.f64(double)\n";
    output << "declare double @llvm.pow.f64(double, double)\n";
    output << "declare double @llvm.floor.f64(double)\n";
    output << "declare double @llvm.ceil.f64(double)\n";
    output << "declare double @llvm.fabs.f64(double)\n";
    output << "declare double @cbrt(double)\n";
    output << "declare double @tan(double)\n";
    output << "declare double @asin(double)\n";
    output << "declare double @acos(double)\n";
    output << "declare double @atan(double)\n";
    output << "declare double @atan2(double, double)\n";
    output << "declare double @sinh(double)\n";
    output << "declare double @cosh(double)\n";
    output << "declare double @tanh(double)\n";
    output << "declare double @log1p(double)\n";
    output << "declare double @fmod(double, double)\n";
    output << "declare double @hypot(double, double)\n";
    if (usesExceptions || usesStringCleanup) {
        output << "declare i32 @" << moduleContext.exceptionAbi.personality() << "(...)\n";
        for (const RuntimeFunction function : kExceptionRuntimeFunctions) {
            output << runtimeDeclaration(function) << '\n';
        }
        moduleContext.usesInoxRuntime = true;
    }
    output << '\n';
    output << inputRuntimeHelpers();
    output << mathRuntimeHelpers();
    output << functionOutput.str();
    return LlvmModule{output.str(), moduleContext.usesInoxRuntime};
}

} // namespace inox::compiler::codegen
