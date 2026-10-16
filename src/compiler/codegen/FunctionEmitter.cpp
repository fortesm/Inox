// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: construction and the body of a routine.

#include "FunctionEmitter.h"

#include "AstQueries.h"
#include "ExceptionAbi.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "MathBuiltins.h"
#include "Operators.h"
#include "../exceptions/ExceptionTypes.h"
#include "../support/ArrayType.h"
#include "../support/Platform.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace inox::compiler::codegen::detail {

FunctionEmitter::FunctionEmitter(std::ostringstream& output,
                                 const ast::FunctionDeclaration& function,
                                 const FunctionSignature& signature,
                                 const FunctionSignatures& signatures,
                                 const StructDefinitions& structs,
                                 ModuleContext& module,
                                 const semantic::SemanticResult& semantics,
                                 const StateGlobals& stateGlobals)
    : semantics_(semantics),
      stateGlobals_(stateGlobals),
      output_(output),
      function_(function),
      signature_(signature),
      signatures_(signatures),
      structs_(structs),
      module_(module)
{
    // State variables are addressed like locals whose slot is a global.
    for (const StateGlobal& state : stateGlobals_) {
        locals_.emplace(normalize(state.name),
                        LocalInfo{stateGlobalSymbol(state.name), state.inoxType, state.llvmType});
    }
    for (const FunctionParameter& parameter : signature.parameters) {
        if (parameter.byReference) {
            // The pointer itself is the slot: reads and writes of the
            // parameter go to the caller's storage.
            locals_.emplace(normalize(parameter.inoxName),
                            LocalInfo{"%" + parameter.llvmName, parameter.inoxType, parameter.valueType});
            continue;
        }
        // Safe pointers: a parameter of a move-only type (an owner, or a
        // value holding one) is owned by the routine, which frees it at
        // every exit unless it gives it away with Move.
        if (isMoveOnly(parameter.inoxType) &&
            !(findStruct(structs_, parameter.inoxType) != nullptr && parameter.llvmType == "ptr")) {
            const std::string slot = "%" + parameter.llvmName + ".addr";
            output_ << "  " << slot << " = alloca " << parameter.llvmType << "\n";
            registerOwnedSlot(slot, parameter.inoxType, parameter.llvmType);
            output_ << "  store " << parameter.llvmType << " %" << parameter.llvmName << ", ptr " << slot << '\n';
            locals_.emplace(normalize(parameter.inoxName),
                            LocalInfo{slot, parameter.inoxType, parameter.llvmType});
            continue;
        }
        if (const auto layout = arrayLayout(parameter.inoxType)) {
            const std::string slot = "%" + parameter.llvmName + ".addr";
            output_ << "  " << slot << " = alloca " << layout->llvmType << "\n";
            output_ << "  store " << layout->llvmType << " %" << parameter.llvmName
                    << ", ptr " << slot << '\n';
            locals_.emplace(normalize(parameter.inoxName),
                            LocalInfo{slot, parameter.inoxType, layout->llvmType});
            continue;
        }
        if (const StructDefinition* structType = findStruct(structs_, parameter.inoxType)) {
            if (parameter.llvmType == "ptr") {
                locals_.emplace(
                    normalize(parameter.inoxName),
                    LocalInfo{"%" + parameter.llvmName, parameter.inoxType, structType->llvmName});
            } else {
                const std::string slot = "%" + parameter.llvmName + ".addr";
                output_ << "  " << slot << " = alloca " << structType->llvmName << "\n";
                output_ << "  store " << structType->llvmName << " %" << parameter.llvmName
                        << ", ptr " << slot << '\n';
                locals_.emplace(
                    normalize(parameter.inoxName),
                    LocalInfo{slot, parameter.inoxType, structType->llvmName});
            }
        } else {
            parameters_.emplace(normalize(parameter.inoxName), "%" + parameter.llvmName);
            parameterTypes_.emplace(normalize(parameter.inoxName), parameter.llvmType);
        }
    }

    if (signature_.llvmReturnType != "void" && signature_.llvmReturnType != "i32") {
        returnValueSlot_ = "%__inox.return.value";
        output_ << "  " << returnValueSlot_ << " = alloca " << signature_.llvmReturnType << "\n";
    }
}

// Whether the routine got a String cleanup pad (it needs a personality).
bool FunctionEmitter::usesStringCleanup() const
{
    return ownsReferences_;
}

void FunctionEmitter::emit()
{
    collectTombstoneLocals();
    ownsReferences_ = scanOwnsReferences();
    if (!ownsReferences_) {
        emitBody();
        return;
    }
    // One cleanup pad for the whole routine: an exception that leaves it
    // is captured, the owned Strings are released, and it is rethrown
    // (the capture/rethrow lowering of try/ensure, on both ABIs).
    const std::string pad = "str.cleanup";
    const std::string stateSlot = "%str.cleanup.state";
    output_ << "  " << stateSlot << " = alloca ptr\n";
    if (signature_.llvmName == "main") {
        for (const StateGlobal& state : stateGlobals_) {
            if (typeOwnsReferences(state.inoxType)) {
                registerOwnedSlot(stateGlobalSymbol(state.name), state.inoxType, state.llvmType, false);
            }
        }
    }
    unwindTargets_.push_back(pad);
    emitBody();
    unwindTargets_.pop_back();
    output_ << "\n";
    emitExceptionCapture(pad, stateSlot, "str.cleanup.release");
    output_ << "str.cleanup.release:\n";
    emitReleaseOwnedSlots();
    const std::string state = "%str.cleanup.rethrow" + std::to_string(nextTemporary_++);
    output_ << "  " << state << " = load ptr, ptr " << stateSlot << "\n";
    output_ << "  call void @__inox_exception_rethrow(ptr " << state << ")\n";
    output_ << "  unreachable\n";
}

void FunctionEmitter::emitBody()
{
    if (signature_.llvmName == "main") {
        // State is initialized in source order before Main's body: an
        // initializer, or the type defaults of a struct or array.
        for (const StateGlobal& state : stateGlobals_) {
            const std::string global = stateGlobalSymbol(state.name);
            if (state.initializer != nullptr) {
                const std::string value = emitExpression(*state.initializer);
                emitOwningStore(global, state.inoxType, state.llvmType, value);
                flushStatementTemporaries();
            } else if (const StructDefinition* structType = findStruct(structs_, state.inoxType)) {
                emitStructDefaults(global, *structType);
            } else {
                emitArrayElementDefaults(global, state.inoxType);
            }
        }
    }
    if (signature_.llvmReturnType == "i32" || signature_.llvmReturnType == "void") {
        for (const auto& statement : function_.body()) {
            emitStatement(*statement);
        }
        emitReleaseOwnedSlots();
        if (signature_.llvmReturnType == "i32") {
            output_ << "  ret i32 0\n";
        } else {
            output_ << "  ret void\n";
        }
        return;
    }

    if (function_.body().size() == 1 &&
        function_.body().front()->kind() == ast::AstNodeKind::IfStatement) {
        emitIfReturn(
            static_cast<const ast::IfStatement&>(*function_.body().front()));
        return;
    }

    if (function_.body().empty()) {
        throw CodegenUnsupported(
            "LLVM emission currently requires a final Return expression");
    }
    if (function_.body().back()->kind() != ast::AstNodeKind::ReturnStatement) {
        // Semantic analysis proved that every path returns (for example a
        // total `case` whose arms all return), so the end is unreachable.
        for (const auto& statement : function_.body()) {
            emitStatement(*statement);
        }
        output_ << "  unreachable\n";
        return;
    }

    for (std::size_t index = 0; index + 1 < function_.body().size(); ++index) {
        emitStatement(*function_.body()[index]);
    }

    emitReturn(static_cast<const ast::ReturnStatement&>(*function_.body().back()));
}

} // namespace inox::compiler::codegen::detail
