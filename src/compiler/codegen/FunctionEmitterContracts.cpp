// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: design by contract (CANON-24). A `__check(Kind,
// Condition, "text")` branches on the condition, marked as expected to hold
// with `llvm.expect.i1`, so the optimizer keeps the failure path cold; the
// failure calls the `cold noreturn` @__inox_contract_fault with the text.
// LLVM has no contract primitive; these are the ones that help: expect for
// the branch weight, cold/noreturn for the failure path. (llvm.assume is not
// used: it would let the optimizer remove the very check it describes.)
// A struct invariant is the hidden method `TName.__invariant(Self)` plus the
// invariants of the structs TName reuses, called at the sites the analyzer
// recorded.

#include "FunctionEmitter.h"

#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "RuntimeHelpersIr.h"

#include <string>

namespace inox::compiler::codegen::detail {

// CANON-24: a contract check written by the contract lowering, its
// condition and the message of its failure.
void FunctionEmitter::emitContractCheck(const tir::Expr& check)
{
    const std::string condition = emitValue(*check.operands[0]);
    const std::string_view text = check.text;
    requireModuleLine("declare i1 @llvm.expect.i1(i1, i1)");
    requireModuleLine(contractMessageHelper());
    const std::size_t label = nextLabel_++;
    const std::string expected = "%contract.expect" + std::to_string(label);
    const std::string passed = "contract.ok" + std::to_string(label);
    const std::string failed = "contract.fail" + std::to_string(label);
    output_ << "  " << expected << " = call i1 @llvm.expect.i1(i1 " << condition << ", i1 true)\n";
    output_ << "  br i1 " << expected << ", label %" << passed << ", label %" << failed << "\n\n";
    output_ << failed << ":\n";
    const std::string message = emitStringValue(text);
    output_ << "  call void @__inox_contract_fault(ptr " << message << ")\n";
    output_ << "  unreachable\n\n";
    output_ << passed << ":\n";
}

// CANON-24: the invariants of the structs a call or an assignment may have
// changed, before or after it.
void FunctionEmitter::emitInvariantChecks(const tir::Invariants* invariants, bool after)
{
    if (invariants == nullptr) {
        return;
    }
    for (const tir::InvariantCheck& check : after ? invariants->after : invariants->before) {
        emitInvariantOf(check.type, emitPlace(*check.place).pointer);
    }
}

// The invariant of a struct at `pointer`: its own clauses, then those of
// every struct it reuses (CANON-9; Eiffel: the parents' invariants apply).
void FunctionEmitter::emitInvariantOf(TypeRef structType, const std::string& pointer)
{
    if (const auto own = module_.invariantRoutines.find(structType); own != module_.invariantRoutines.end()) {
        emitUserCall(*routineSignature(own->second), {pointer}, false);
    }
    if (!structType->isStruct()) {
        return;
    }
    for (std::size_t index = 0; index < structType->fields().size(); ++index) {
        const types::StructField& field = structType->fields()[index];
        if (!field.isPart) {
            continue;
        }
        const std::string partPointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << partPointer << " = getelementptr " << llvmStructName(structType) << ", ptr " << pointer
                << ", i32 0, i32 " << index << '\n';
        emitInvariantOf(field.type, partPointer);
    }
}

} // namespace inox::compiler::codegen::detail
