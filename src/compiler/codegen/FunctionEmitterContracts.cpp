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

bool FunctionEmitter::isContractCheck(const ast::CallExpression& call) const
{
    return call.callee().kind() == ast::AstNodeKind::IdentifierExpression &&
           static_cast<const ast::IdentifierExpression&>(call.callee()).name() == "__check";
}

void FunctionEmitter::emitContractCheck(const ast::CallExpression& call)
{
    const auto& arguments = call.arguments();
    const std::string condition = emitExpression(*arguments[1]);
    const auto& text = static_cast<const ast::LiteralExpression&>(*arguments[2]).value();
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

void FunctionEmitter::emitInvariantChecks(const ast::AstNode& site, bool after)
{
    const auto* checks = semantics_.invariantChecks(site, after);
    if (checks == nullptr) {
        return;
    }
    for (const semantic::SemanticResult::InvariantCheck& check : *checks) {
        emitInvariantOf(check.typeName, emitPlace(*check.place).pointer);
    }
}

// The invariant of `typeName` at `pointer`: its own clauses, then those of
// every struct it reuses (CANON-9; Eiffel: the parents' invariants apply).
void FunctionEmitter::emitInvariantOf(const std::string& typeName, const std::string& pointer)
{
    const auto own = signatures_.find(normalize(typeName + ".__invariant"));
    if (own != signatures_.end()) {
        emitUserCall(own->second, {pointer}, false);
    }
    const StructDefinition* structType = findStruct(structs_, typeName);
    if (structType == nullptr) {
        return;
    }
    for (const StructFieldInfo& field : structType->fields) {
        if (!field.isPart) {
            continue;
        }
        const std::string partPointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << partPointer << " = getelementptr " << structType->llvmName << ", ptr " << pointer
                << ", i32 0, i32 " << field.index << '\n';
        emitInvariantOf(field.inoxType, partPointer);
    }
}

void FunctionEmitter::emitNewValueInvariant(const ast::VarStatement& declaration)
{
    if (const std::string* typeName = semantics_.newValueInvariant(declaration)) {
        const auto local = locals_.find(normalize(declaration.name()));
        if (local != locals_.end()) {
            emitInvariantOf(*typeName, local->second.slot);
        }
    }
}

} // namespace inox::compiler::codegen::detail
