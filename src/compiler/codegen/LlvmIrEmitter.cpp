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
                  ModuleContext& moduleContext,
                  const semantic::SemanticResult& semantics,
                  const tir::AstNodeMap& astNodes,
                  const StateGlobals& stateGlobals,
                  bool& usesStringCleanup)
{
    std::ostringstream body;
    FunctionEmitter emitter(body, function, signature, signatures, moduleContext, semantics, astNodes, stateGlobals);
    emitter.emit();
    usesStringCleanup = usesStringCleanup || emitter.usesStringCleanup();

    output << "define " << signature.llvmReturnType << " @" << signature.llvmName << '(';
    for (std::size_t index = 0; index < signature.parameters.size(); ++index) {
        if (index != 0) {
            output << ", ";
        }
        output << signature.parameters[index].llvmType << " %" << signature.parameters[index].llvmName;
        if (signature.parameters[index].callerStorage) {
            output << ", ptr %" << signature.parameters[index].llvmName << ".storage.header, i64 %"
                   << signature.parameters[index].llvmName << ".storage.generation";
        }
    }
    // D4 (r3.92): Main receives the command line and gives it to libinoxrt.
    if (signature.llvmName == "main") {
        output << "i32 %__inox.argc, ptr %__inox.argv";
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

std::string LlvmIrEmitter::emit(const ast::ModuleNode& module, const semantic::SemanticResult& semantics,
                                const tir::AstNodeMap& astNodes, const tir::Program& program) const
{
    return emitModule(module, semantics, astNodes, program).ir;
}

LlvmModule LlvmIrEmitter::emitModule(const ast::ModuleNode& module, const semantic::SemanticResult& semantics,
                                     const tir::AstNodeMap& astNodes, const tir::Program& program) const
{
    const ast::FunctionDeclaration* mainFunction = nullptr;
    FunctionSignatures signatures;
    std::ostringstream functionOutput;
    std::ostringstream output;
    ModuleContext moduleContext(hostExceptionAbi());
    const bool usesExceptions = moduleUsesExceptions(module);
    bool usesStringCleanup = false;

    // The struct types of the typed IR: declaration order, then the
    // instances of generic structs in the order the analysis made them, so
    // the IR is the same with every standard library. State globals in
    // source order.
    const std::vector<TypeRef> structTypes(program.structTypes.begin(), program.structTypes.end());
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
            const TypeRef type = astNodes.declaredType(declaration);
            const std::string llvmType = llvmValueType(type);
            if (llvmType.empty()) {
                throw CodegenUnsupported("State variables of type " + types::nameOf(type) + " are not lowered yet");
            }
            stateGlobals.push_back(StateGlobal{declaration.name, type, llvmType, declaration.value.get()});
        }
    }

    // The routines to compile, those of the typed IR: the program's own (a
    // generic routine is a template, never compiled), then the instances of
    // generic routines; the entry last.
    std::vector<const ast::FunctionDeclaration*> routines;
    for (const tir::Routine* routine : program.routines) {
        if (routine == program.entry) {
            mainFunction = routine->origin;
        } else {
            routines.push_back(routine->origin);
        }
    }

    for (const ast::FunctionDeclaration* routine : routines) {
        const ast::FunctionDeclaration& function = *routine;
        const semantic::FunctionSignature* analyzed = semantics.signatureOf(function);
        if (analyzed == nullptr) {
            throw CodegenError("routine without an analyzed signature: " + function.name());
        }
        signatures.emplace(&function, lowerSignature(function, *analyzed));
    }

    for (const ast::FunctionDeclaration* routine : routines) {
        const auto signature = signatures.find(routine);
        emitFunction(functionOutput, *routine, signature->second, signatures, moduleContext, semantics, astNodes, stateGlobals,
                     usesStringCleanup);
    }

    if (mainFunction == nullptr) {
        throw CodegenError("LLVM emission requires Main");
    }
    const FunctionSignature mainSignature{"main", "i32", {}, nullptr};
    emitFunction(functionOutput, *mainFunction, mainSignature, signatures, moduleContext, semantics, astNodes, stateGlobals,
                 usesStringCleanup);

    for (const StateGlobal& state : stateGlobals) {
        output << stateGlobalSymbol(state.name) << " = internal global " << state.llvmType
               << " zeroinitializer\n";
    }
    if (!stateGlobals.empty()) {
        output << '\n';
    }

    for (const TypeRef structType : structTypes) {
        output << llvmStructName(structType) << " = type { ";
        for (std::size_t index = 0; index < structType->fields().size(); ++index) {
            if (index != 0) {
                output << ", ";
            }
            output << fieldLayout(structType, index).llvmType;
        }
        output << " }\n";
    }
    if (!structTypes.empty()) {
        output << '\n';
    }

    for (const std::string& global : moduleContext.moduleLines) {
        output << global << '\n';
    }
    // E1 (change console-abi-terminal): the console is libinoxrt's, and every
    // program links it.
    for (const RuntimeFunction function : kConsoleRuntimeFunctions) {
        output << runtimeDeclaration(function) << '\n';
    }
    moduleContext.usesInoxRuntime = true;
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
