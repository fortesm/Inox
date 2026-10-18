// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: the statement dispatcher, declarations, assignments, calls.

#include "FunctionEmitter.h"

#include "AstQueries.h"
#include "ExceptionAbi.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "MathBuiltins.h"
#include "Operators.h"
#include "../exceptions/ExceptionTypes.h"
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
void FunctionEmitter::emitStatement(const ast::Statement& statement)
{
    ++statementDepth_;
    const bool outerRunsRoutine = statementRunsRoutine_;
    statementRunsRoutine_ = statementMayRunRoutine(statement);
    emitStatementKind(statement);
    statementRunsRoutine_ = outerRunsRoutine;
    flushStatementTemporaries();
    --statementDepth_;
}

void FunctionEmitter::emitStatementKind(const ast::Statement& statement)
{
    if (statement.kind() == ast::AstNodeKind::VarStatement) {
        const auto& variable = static_cast<const ast::VarStatement&>(statement);
        if (variable.initializer() == nullptr) {
            if (variable.typeName().empty()) {
                throw CodegenUnsupported(
                    "LLVM emission currently requires local variable initialization");
            }
            emitTypedLocalVariable(variable.name(), semantics_.declaredType(variable));
            emitNewValueInvariant(variable);
        } else if (!variable.typeName().empty()) {
            emitTypedLocalVariable(variable.name(), semantics_.declaredType(variable), variable.initializer());
        } else {
            emitLocalVariable(variable.name(), *variable.initializer());
        }
        return;
    }

    if (statement.kind() == ast::AstNodeKind::VarBlockStatement) {
        const auto& block = static_cast<const ast::VarBlockStatement&>(statement);
        for (const auto& declaration : block.declarations()) {
            emitVarBlockDeclaration(*declaration);
        }
        return;
    }

    if (statement.kind() == ast::AstNodeKind::ExpressionStatement) {
        const auto& expression =
            static_cast<const ast::ExpressionStatement&>(statement).expression();
        if (isAssignmentExpression(expression)) {
            emitLocalAssignment(expression);
        } else {
            emitExpressionStatement(expression);
        }
        return;
    }

    if (statement.kind() == ast::AstNodeKind::IfStatement) {
        emitIfMerge(static_cast<const ast::IfStatement&>(statement));
        return;
    }

    if (statement.kind() == ast::AstNodeKind::WhileStatement) {
        emitWhile(static_cast<const ast::WhileStatement&>(statement));
        return;
    }

    if (statement.kind() == ast::AstNodeKind::RepeatStatement) {
        emitRepeat(static_cast<const ast::RepeatStatement&>(statement));
        return;
    }

    if (statement.kind() == ast::AstNodeKind::ForInStatement) {
        emitForIn(static_cast<const ast::ForInStatement&>(statement));
        return;
    }

    if (statement.kind() == ast::AstNodeKind::TryStatement) {
        emitTry(static_cast<const ast::TryStatement&>(statement));
        return;
    }

    if (statement.kind() == ast::AstNodeKind::RaiseStatement) {
        emitRaise(static_cast<const ast::RaiseStatement&>(statement));
        return;
    }

    if (statement.kind() == ast::AstNodeKind::RetryStatement) {
        emitRetry(static_cast<const ast::RetryStatement&>(statement));
        return;
    }

    if (statement.kind() == ast::AstNodeKind::ReturnStatement) {
        emitReturn(static_cast<const ast::ReturnStatement&>(statement));
        return;
    }

    if (statement.kind() == ast::AstNodeKind::ExitStatement) {
        emitExit();
        return;
    }

    if (statement.kind() == ast::AstNodeKind::LeaveStatement) {
        emitLeaveTransfer();
        return;
    }

    if (statement.kind() == ast::AstNodeKind::ContinueStatement) {
        emitContinueTransfer();
        return;
    }

    if (statement.kind() == ast::AstNodeKind::WithStatement) {
        emitWith(static_cast<const ast::WithStatement&>(statement));
        return;
    }

    if (statement.kind() == ast::AstNodeKind::UntilStatement) {
        emitUntil(static_cast<const ast::UntilStatement&>(statement));
        return;
    }

    if (statement.kind() == ast::AstNodeKind::CaseStatement) {
        emitCase(static_cast<const ast::CaseStatement&>(statement));
        return;
    }
    if (statement.kind() == ast::AstNodeKind::UnlessStatement) {
        emitUnless(static_cast<const ast::UnlessStatement&>(statement));
        return;
    }
    throw CodegenUnsupported("LLVM emission does not lower this statement kind yet");
}

void FunctionEmitter::emitVarBlockDeclaration(const ast::Statement& statement)
{
    if (statement.kind() == ast::AstNodeKind::VarStatement) {
        const auto& variable = static_cast<const ast::VarStatement&>(statement);
        if (variable.initializer() == nullptr) {
            emitTypedLocalVariable(variable.name(), semantics_.declaredType(variable));
            emitNewValueInvariant(variable);
        } else if (!variable.typeName().empty()) {
            emitTypedLocalVariable(variable.name(), semantics_.declaredType(variable), variable.initializer());
        } else {
            emitLocalVariable(variable.name(), *variable.initializer());
        }
        return;
    }

    if (statement.kind() != ast::AstNodeKind::ExpressionStatement) {
        throw CodegenUnsupported(
            "unsupported local variable declaration in Integer function");
    }

    const auto& expression =
        static_cast<const ast::ExpressionStatement&>(statement).expression();
    if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
        throw CodegenUnsupported(
            "unsupported local variable declaration in Integer function");
    }

    const auto& assignment = static_cast<const ast::BinaryExpression&>(expression);
    if (assignment.op() != ast::BinaryOperator::Assign ||
        assignment.left().kind() != ast::AstNodeKind::IdentifierExpression) {
        throw CodegenUnsupported(
            "unsupported local variable declaration in Integer function");
    }

    const auto& identifier =
        static_cast<const ast::IdentifierExpression&>(assignment.left());
    emitLocalVariable(identifier.name(), assignment.right());
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

void FunctionEmitter::emitLocalVariable(std::string_view name, const ast::Expression& initializer)
{
    const std::string normalizedName = normalize(name);
    const std::string llvmType = expressionLlvmType(initializer);
    const TypeRef type = typeOf(initializer);
    const std::string slot = newSlot(normalizedName);
    output_ << "  " << slot << " = alloca " << llvmType << "\n";
    if (typeOwnsReferences(type)) {
        registerOwnedSlot(slot, type, llvmType);
    }
    const std::string value = emitExpression(initializer);
    emitOwningStore(slot, type, llvmType, value);
    locals_.insert_or_assign(normalizedName, LocalInfo{slot, type, llvmType});
    declareTombstone(normalizedName);
}

void FunctionEmitter::emitTypedLocalVariable(std::string_view name, TypeRef type, const ast::Expression* initializer)
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
            const std::string value = emitExpression(*initializer);
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
            emitOwningStore(slot, type, llvmType, emitExpression(*initializer));
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
    const std::string value = initializer != nullptr ? emitExpression(*initializer) : zero;
    emitOwningStore(slot, type, llvmType, value);
    locals_.insert_or_assign(normalizedName, LocalInfo{slot, type, llvmType});
    declareTombstone(normalizedName);
}

bool FunctionEmitter::isAssignmentExpression(const ast::Expression& expression)
{
    if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
        return false;
    }

    const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
    return binary.op() == ast::BinaryOperator::Assign;
}

void FunctionEmitter::emitAssignmentOrCallStatement(const ast::Expression& expression)
{
    if (isAssignmentExpression(expression)) {
        emitLocalAssignment(expression);
        return;
    }
    emitExpressionStatement(expression);
}

void FunctionEmitter::emitExpressionStatement(const ast::Expression& expression)
{
    if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
        if (equalsIgnoreCase(identifier.name(), "Get")) {
            emitInputDiscardToken();
            return;
        }
        if (equalsIgnoreCase(identifier.name(), "GetLn")) {
            emitInputDiscardLine();
            return;
        }
        emitNoArgumentSubroutineCall(identifier);
        return;
    }

    if (expression.kind() != ast::AstNodeKind::CallExpression) {
        throw CodegenUnsupported(
            "LLVM emission currently supports only assignments and calls as statements");
    }

    const auto& call = static_cast<const ast::CallExpression&>(expression);
    if (isMemberAccessCall(call)) {
        emitNoArgumentMethodCall(call, false);
        return;
    }
    if (isMemberAccessCall(call.callee())) {
        emitMethodCall(call, false);
        return;
    }
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
        throw CodegenUnsupported(
            "LLVM emission currently supports only direct calls or method calls as statements");
    }

    const auto& callee =
        static_cast<const ast::IdentifierExpression&>(call.callee());
    if (isAssertCall(call)) {
        emitAssertCall(call);
        return;
    }
    if (isContractCheck(call)) {
        emitContractCheck(call);
        return;
    }
    if (isPointerBuiltin(call)) {
        emitPointerBuiltin(call);
        return;
    }
    if (isFileCall(call)) {
        emitFileCall(call);
        return;
    }
    const bool isPut = equalsIgnoreCase(callee.name(), "Put");
    const bool isPutLn = equalsIgnoreCase(callee.name(), "PutLn");
    if (isPut || isPutLn) {
        emitOutputCallSequence(call.arguments(), isPutLn);
        return;
    }

    const bool isGet = equalsIgnoreCase(callee.name(), "Get");
    const bool isGetLn = equalsIgnoreCase(callee.name(), "GetLn");
    if (isGet || isGetLn) {
        emitInputCallSequence(call.arguments(), isGetLn);
        return;
    }

    emitSubroutineCall(call, callee);
}

// ADR-0016: a reference argument passes the address of the caller's
// place: a variable's slot, a field or an element; a read-only reference
// to any other value gets a temporary.
std::string FunctionEmitter::emitReferenceArgument(const ast::Expression& argument, const FunctionParameter& parameter)
{
    if (argument.kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto local = locals_.find(normalize(static_cast<const ast::IdentifierExpression&>(argument).name()));
        if (local != locals_.end()) {
            return local->second.slot;
        }
    }
    if (isIndexCall(argument)) {
        const ElementAddress element = emitElementAddress(static_cast<const ast::CallExpression&>(argument));
        if (element.storage != parameter.valueType) {
            throw CodegenUnsupported(
                "a narrow-integer array element cannot be passed by reference yet");
        }
        return element.pointer;
    }
    if (isMemberAccessCall(argument)) {
        return emitMemberAddress(static_cast<const ast::CallExpression&>(argument)).pointer;
    }
    const std::string value = emitExpression(argument);
    const std::string slot = "%reftmp" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca " << parameter.valueType << '\n';
    output_ << "  store " << parameter.valueType << ' ' << value << ", ptr " << slot << '\n';
    return slot;
}

std::vector<std::string> FunctionEmitter::emitArguments(const FunctionSignature& signature,
                                                        const std::vector<ast::ExpressionPtr>& arguments,
                                                        std::size_t parameterOffset)
{
    std::vector<std::string> values;
    values.reserve(arguments.size());
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const FunctionParameter& parameter = signature.parameters[index + parameterOffset];
        values.push_back(parameter.byReference ? emitReferenceArgument(*arguments[index], parameter)
                                               : emitExpression(*arguments[index]));
        // Safe pointers: a move-only argument is given to the routine,
        // which owns its parameter.
        if (!parameter.byReference && isMoveOnly(parameter.type)) {
            disarmFresh(values.back());
        }
    }
    return values;
}

std::string FunctionEmitter::emitUserCall(const FunctionSignature& signature,
                                          const std::vector<std::string>& arguments,
                                          bool requireValue)
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

void FunctionEmitter::emitNoArgumentSubroutineCall(const ast::IdentifierExpression& callee)
{
    const FunctionSignature* routine = calledRoutine(callee);
    if (routine == nullptr) {
        throw CodegenUnsupported(
            "LLVM emission could not resolve zero-argument subroutine call: " + callee.name());
    }
    if (!routine->parameters.empty()) {
        throw CodegenError(
            "subroutine requires arguments; use parentheses only when passing arguments");
    }
    if (routine->llvmReturnType != "void") {
        // The same backend gap as a call with arguments whose result is not
        // used (`Twice(2)` as a statement); whether Inox allows discarding a
        // function result is an open question (revisions.md).
        throw CodegenUnsupported(
            "LLVM emission currently supports only subroutine calls as expression statements");
    }
    emitUserCall(*routine, {}, false);
}

void FunctionEmitter::emitSubroutineCall(const ast::CallExpression& call, const ast::IdentifierExpression& callee)
{
    const FunctionSignature* routine = calledRoutine(callee);
    if (routine == nullptr) {
        throw CodegenUnsupported(
            "LLVM emission currently supports only Put/PutLn and user subroutine calls as statements");
    }
    if (routine->llvmReturnType != "void") {
        throw CodegenUnsupported(
            "LLVM emission currently supports only subroutine calls as expression statements");
    }
    if (call.arguments().size() != routine->parameters.size()) {
        throw CodegenUnsupported("unsupported subroutine argument count: " + function_.name());
    }

    const std::vector<std::string> arguments = emitArguments(*routine, call.arguments(), 0);
    emitUserCall(*routine, arguments, false);
    emitInvariantChecks(call, true);
}

} // namespace inox::compiler::codegen::detail
