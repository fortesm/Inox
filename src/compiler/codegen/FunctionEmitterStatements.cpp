// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: the statement dispatcher, declarations, assignments, calls.

#include "FunctionEmitter.h"

#include "TirWalk.h"
#include "ExceptionAbi.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "MathBuiltins.h"
#include "Operators.h"
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

// Every statement releases the String temporaries it created.
void FunctionEmitter::emitStatement(const tir::Stmt& statement)
{
    ++statementDepth_;
    const bool outerRunsRoutine = statementRunsRoutine_;
    statementRunsRoutine_ = statementMayRunRoutine(statement);
    emitStatementKind(statement);
    statementRunsRoutine_ = outerRunsRoutine;
    flushStatementTemporaries();
    --statementDepth_;
}

void FunctionEmitter::emitStatementKind(const tir::Stmt& statement)
{
    switch (statement.kind) {
    case tir::StmtKind::Declare:
        emitDeclaration(statement);
        recordDeclaredSlots(statement);
        return;
    case tir::StmtKind::Block:
        // A `var` block: its declarations, one statement.
        for (const tir::Stmt* declaration : statement.body) {
            if (declaration->kind != tir::StmtKind::Declare) {
                throw CodegenUnsupported("unsupported local variable declaration in Integer function");
            }
            emitDeclaration(*declaration);
            recordDeclaredSlots(*declaration);
        }
        return;
    case tir::StmtKind::Evaluate:
        if (statement.discard) {
            // r3.93: `_ := F(X)` evaluates the call as a value and drops it;
            // an owned result (a String, an Own, ...) was held by the call
            // and is released with the statement's temporaries.
            (void)emitValue(*statement.value);
            return;
        }
        emitExpressionStatement(*statement.value);
        return;
    case tir::StmtKind::Assign:
        emitAssignment(statement);
        return;
    case tir::StmtKind::If:
        emitIfMerge(statement);
        return;
    case tir::StmtKind::While:
        emitWhile(statement);
        return;
    case tir::StmtKind::Repeat:
        emitRepeat(statement);
        return;
    case tir::StmtKind::ForIn:
        emitForIn(statement);
        return;
    case tir::StmtKind::Try:
        emitTry(statement);
        return;
    case tir::StmtKind::Raise:
        emitRaise(statement);
        return;
    case tir::StmtKind::Retry:
        emitRetry(statement);
        return;
    case tir::StmtKind::Return:
        emitReturn(statement);
        return;
    case tir::StmtKind::Exit:
        emitExit();
        return;
    case tir::StmtKind::Leave:
        emitLeaveTransfer();
        return;
    case tir::StmtKind::Continue:
        emitContinueTransfer();
        return;
    case tir::StmtKind::With:
        emitWith(statement);
        return;
    case tir::StmtKind::Until:
        emitUntil(statement);
        return;
    case tir::StmtKind::Case:
        emitCase(statement);
        return;
    case tir::StmtKind::Unless:
        emitUnless(statement);
        return;
    }
    throw CodegenUnsupported("LLVM emission does not lower this statement kind yet");
}

// A local variable: `X T`, `X T := V`, or `X := V` (CANON-5 / A6 / A7: of
// the type of V). `X := V` where an earlier scope of the routine already
// made X's storage stores into it.
void FunctionEmitter::emitDeclaration(const tir::Stmt& declaration)
{
    const std::string_view name = declaration.symbol->name;
    if (declaration.inferred) {
        const auto local = locals_.find(normalize(name));
        if (local == locals_.end()) {
            emitLocalVariable(name, *declaration.value);
            return;
        }
        emitStorageGuard(local->first);
        const std::string value = emitValue(*declaration.value);
        emitOwningStore(local->second.slot, local->second.type, local->second.llvmType, value);
        return;
    }
    emitTypedLocalVariable(name, declaration.type, declaration.value);
    if (declaration.value == nullptr && declaration.invariants != nullptr &&
        declaration.invariants->newValue != nullptr) {
        const auto local = locals_.find(normalize(name));
        if (local != locals_.end()) {
            emitInvariantOf(declaration.invariants->newValue, local->second.slot);
        }
    }
}

void FunctionEmitter::recordDeclaredSlots(const tir::Stmt& declaration)
{
    const std::string name = normalize(declaration.symbol->name);
    DeclaredSlots slots;
    if (const auto local = locals_.find(name); local != locals_.end() && isMoveOnly(local->second.type)) {
        slots.value = local->second.slot;
        slots.type = local->second.type;
        slots.llvmType = local->second.llvmType;
    }
    if (const auto tombstone = tombstoneSlots_.find(name); tombstone != tombstoneSlots_.end()) {
        slots.tombstone = tombstone->second;
    }
    if (!slots.value.empty() || !slots.tombstone.empty()) {
        declaredSlots_.insert_or_assign(declaration.symbol, std::move(slots));
    }
}

// Returns a slot name that is unique inside the current function, so sibling
// scopes (two `for` loops over the same iterator, the same local in two `if`
// bodies, ...) never produce a duplicate LLVM definition.
std::string FunctionEmitter::newSlot(const std::string& normalizedName)
{
    const std::string base = "%" + safeLlvmName(normalizedName);
    std::string candidate = base;
    while (!usedSlots_.insert(candidate).second) {
        candidate = base + "." + std::to_string(slotCounter_++);
    }
    return candidate;
}

void FunctionEmitter::emitLocalVariable(std::string_view name, const tir::Expr& initializer)
{
    const std::string normalizedName = normalize(name);
    const std::string llvmType = expressionLlvmType(initializer);
    const TypeRef type = bare(initializer).type;
    const std::string slot = newSlot(normalizedName);
    output_ << "  " << slot << " = alloca " << llvmType << "\n";
    if (typeOwnsReferences(type)) {
        registerOwnedSlot(slot, type, llvmType);
    }
    const std::string value = emitValue(initializer);
    emitOwningStore(slot, type, llvmType, value);
    locals_.insert_or_assign(normalizedName, LocalInfo{slot, type, llvmType});
    declareTombstone(normalizedName);
}

void FunctionEmitter::emitTypedLocalVariable(std::string_view name, TypeRef type, const tir::Expr* initializer)
{
    const std::string llvmType = llvmValueType(type);
    if (llvmType.empty()) {
        throw CodegenUnsupported("unsupported local variable type for LLVM emission");
    }

    const std::string normalizedName = normalize(name);
    const std::string slot = newSlot(normalizedName);
    output_ << "  " << slot << " = alloca " << llvmType << "\n";
    const bool owned = typeOwnsReferences(type);
    if (owned) {
        // A declaration that runs again (in a loop) first releases the
        // value of the previous run.
        registerOwnedSlot(slot, type, llvmType);
    }

    if (type->isStruct()) {
        // CANON-9: `Q T := Expr` copies the value; `P T` takes the type's
        // field defaults (rule 4).
        if (initializer != nullptr) {
            const std::string value = emitValue(*initializer);
            emitOwningStore(slot, type, llvmType, value);
        } else {
            if (owned) {
                emitReleaseAt(slot, type);
            }
            emitStructDefaults(slot, type);
        }
        locals_.insert_or_assign(normalizedName, LocalInfo{slot, type, llvmType});
        declareTombstone(normalizedName);
        return;
    }

    if (type->isArray()) {
        // CANON-13: an array is a value; without an initializer every
        // element is its type's zero, or its struct type's defaults.
        if (initializer != nullptr) {
            emitOwningStore(slot, type, llvmType, emitValue(*initializer));
        } else {
            if (owned) {
                emitReleaseAt(slot, type);
            }
            output_ << "  store " << llvmType << " zeroinitializer, ptr " << slot << '\n';
            emitArrayElementDefaults(slot, type);
        }
        locals_.insert_or_assign(normalizedName, LocalInfo{slot, type, llvmType});
        declareTombstone(normalizedName);
        return;
    }

    // A declared-but-uninitialized scalar starts at its type's zero. LLVM
    // requires a floating-point literal for float and double, so `0` is not
    // valid there; a Link and an Option are born empty (zeroinitializer).
    const std::string zero = type->isPointer() ? "zeroinitializer" : isFloatLlvmType(llvmType) ? "0.0" : "0";
    const std::string value = initializer != nullptr ? emitValue(*initializer) : zero;
    emitOwningStore(slot, type, llvmType, value);
    locals_.insert_or_assign(normalizedName, LocalInfo{slot, type, llvmType});
    declareTombstone(normalizedName);
}

// A call as a statement: a subroutine, a method (its result dropped), or a
// built-in operation.
void FunctionEmitter::emitExpressionStatement(const tir::Expr& expression)
{
    switch (expression.kind) {
    case tir::ExprKind::Intrinsic:
        emitIntrinsicCall(expression);
        return;
    case tir::ExprKind::Check:
        emitContractCheck(expression);
        return;
    case tir::ExprKind::Call: {
        if (isMethodCall(expression)) {
            (void)emitCall(expression, false);
            return;
        }
        const FunctionSignature* routine = routineSignature(expression.routine);
        if (routine == nullptr) {
            throw CodegenUnsupported(
                "LLVM emission currently supports only Put/PutLn and user subroutine calls as statements");
        }
        if (routine->llvmReturnType != "void") {
            // Whether Inox allows discarding a function result without `_ :=`
            // is an open question (revisions.md).
            throw CodegenUnsupported(
                "LLVM emission currently supports only subroutine calls as expression statements");
        }
        (void)emitCall(expression, false);
        return;
    }
    case tir::ExprKind::Builtin:
        break;
    default:
        throw CodegenUnsupported("LLVM emission currently supports only assignments and calls as statements");
    }
    const semantic::BuiltinOp op = expression.op;
    if (op == semantic::BuiltinOp::Assert) {
        emitAssertCall(expression);
        return;
    }
    if (isPointerBuiltin(op)) {
        emitPointerBuiltin(expression);
        return;
    }
    if (isFileCall(op)) {
        emitFileCall(expression);
        return;
    }
    if (op == semantic::BuiltinOp::Put || op == semantic::BuiltinOp::PutLn) {
        emitOutputCallSequence(expression.operands, op == semantic::BuiltinOp::PutLn);
        return;
    }
    if (op == semantic::BuiltinOp::Get || op == semantic::BuiltinOp::GetLn) {
        if (expression.operands.empty()) {
            // `Get` and `GetLn` without parentheses skip a token or a line.
            if (op == semantic::BuiltinOp::Get) {
                emitInputDiscardToken();
            } else {
                emitInputDiscardLine();
            }
            return;
        }
        emitInputCallSequence(expression.operands, op == semantic::BuiltinOp::GetLn);
        return;
    }
    throw CodegenUnsupported("LLVM emission currently supports only Put/PutLn and user subroutine calls as statements");
}

// ADR-0016: a reference argument passes the address of the caller's
// place: a variable's slot, a field or an element; a read-only reference
// to any other value gets a temporary.
std::string FunctionEmitter::emitReferenceArgument(const tir::Expr& argument, const FunctionParameter& parameter)
{
    const tir::Expr& value = bare(argument);
    if (value.kind == tir::ExprKind::Read) {
        const tir::Place& place = *value.place;
        if (isVariablePlace(place)) {
            const auto local = locals_.find(normalize(place.symbol->name));
            if (local != locals_.end()) {
                return local->second.slot;
            }
        } else if (place.kind == tir::PlaceKind::Element) {
            const ElementAddress element = emitElementAddress(place);
            if (element.storage != parameter.valueType) {
                throw CodegenUnsupported("a narrow-integer array element cannot be passed by reference yet");
            }
            return element.pointer;
        } else if (place.kind == tir::PlaceKind::Field) {
            return emitPlace(place).pointer;
        }
    }
    const std::string stored = emitValue(argument);
    const std::string slot = "%reftmp" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca " << parameter.valueType << '\n';
    output_ << "  store " << parameter.valueType << ' ' << stored << ", ptr " << slot << '\n';
    return slot;
}

std::vector<std::string> FunctionEmitter::emitArguments(const FunctionSignature& signature,
                                                        const tir::Span<const tir::Expr*>& arguments,
                                                        std::size_t parameterOffset,
                                                        std::vector<std::string>* storage)
{
    std::vector<std::string> values;
    values.reserve(arguments.size());
    std::size_t guards = 0;
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        emitArgument(values, arguments, index, signature.parameters[index + parameterOffset], storage, guards);
    }
    for (; guards != 0; --guards) {
        endPlaceGuard();
    }
    return values;
}

// One argument: a reference's place, or a value; a move-only value is given
// to the routine, which owns its parameter (safe pointers).
void FunctionEmitter::emitArgument(std::vector<std::string>& values, const tir::Span<const tir::Expr*>& arguments,
                                   std::size_t index, const FunctionParameter& parameter,
                                   std::vector<std::string>* storage, std::size_t& guards)
{
    if (parameter.byReference) {
        values.push_back(emitReferenceParameterArgument(arguments, index, parameter, storage, guards));
        return;
    }
    values.push_back(emitValue(*arguments[index]));
    if (isMoveOnly(parameter.type)) {
        disarmFresh(values.back());
    }
}

std::string FunctionEmitter::emitUserCall(const FunctionSignature& signature,
                                          const std::vector<std::string>& arguments,
                                          bool requireValue)
{
    static const std::vector<std::string> noStorage;
    return emitUserCall(signature, arguments, requireValue, noStorage);
}

// A reference argument (ADR-0016): its place, the guard a later argument
// asks for (CANON-25, counted in `guards` for emitArguments to close), and
// the header and generation the parameter receives, in `storage` at the
// argument's index.
std::string FunctionEmitter::emitReferenceParameterArgument(tir::Span<const tir::Expr*> arguments,
                                                            std::size_t index, const FunctionParameter& parameter,
                                                            std::vector<std::string>* storage, std::size_t& guards)
{
    guards += beginReferenceGuard(arguments, index);
    beginCallerStorage(*arguments[index], false);
    std::string value = emitReferenceArgument(*arguments[index], parameter);
    std::string guard = endCallerStorage(*arguments[index]);
    if (storage != nullptr) {
        storage->resize(index + 1);
        (*storage)[index] = std::move(guard);
    }
    return value;
}

// `storage`, when given, holds for each parameter that is the caller's
// storage the header and generation of its object (`ptr H, i64 G`); an
// empty one is storage no owner frees.
std::string FunctionEmitter::emitUserCall(const FunctionSignature& signature,
                                          const std::vector<std::string>& arguments,
                                          bool requireValue,
                                          const std::vector<std::string>& storage)
{
    if (arguments.size() != signature.parameters.size()) {
        throw CodegenError("internal error: LLVM user-call argument count mismatch");
    }
    if (requireValue && signature.llvmReturnType == "void") {
        throw CodegenError("void function call cannot be used as an expression");
    }

    std::string result;
    if (signature.llvmReturnType != "void") {
        result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = ";
    } else {
        output_ << "  ";
    }

    output_ << (unwindTargets_.empty() ? "call " : "invoke ")
            << signature.llvmReturnType << " @" << signature.llvmName << '(';
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        if (index != 0) output_ << ", ";
        output_ << signature.parameters[index].llvmType << ' ' << arguments[index];
        if (signature.parameters[index].callerStorage) {
            output_ << ", " << (index < storage.size() && !storage[index].empty() ? storage[index]
                                                                                  : immortalStorage());
        }
    }
    output_ << ')';

    if (unwindTargets_.empty()) {
        output_ << "\n";
    } else {
        const std::string continuation = "eh.invoke.cont" + std::to_string(nextLabel_++);
        output_ << " to label %" << continuation
                << " unwind label %" << currentUnwindTarget() << "\n\n";
        output_ << continuation << ":\n";
    }
    if (!result.empty() && typeOwnsReferences(signature.returnType)) {
        holdOwnedResult(result, signature.returnType, signature.llvmReturnType);
    }
    return result;
}

} // namespace inox::compiler::codegen::detail
