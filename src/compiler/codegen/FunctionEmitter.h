// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Lowers one Inox routine to LLVM IR text (OpenSpec change
// compiler-modularization, step 2). The member functions are defined by
// responsibility in FunctionEmitter*.cpp; see AGENTS.md, "Where things live".

#pragma once

#include "ModuleContext.h"
#include "RuntimeAbi.h"
#include "TypeLayout.h"
#include "../ast/Ast.h"
#include "../semantic/SemanticResult.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace inox::compiler::codegen::detail {

class FunctionEmitter {
public:
    FunctionEmitter(std::ostringstream& output,
                    const ast::FunctionDeclaration& function,
                    const FunctionSignature& signature,
                    const FunctionSignatures& signatures,
                    const StructDefinitions& structs,
                    ModuleContext& module,
                    const semantic::SemanticResult& semantics,
                    const StateGlobals& stateGlobals);

    bool usesStringCleanup() const;

    void emit();

    void emitBody();

private:
    const semantic::SemanticResult& semantics_;
    const StateGlobals& stateGlobals_;
    struct LoopTargets {
        std::string continueTarget;
        std::string leaveTarget;
    };

    struct RetryContext {
        std::string counterSlot;
        std::string actionSlot;
        std::string cleanupTarget;
        std::string rethrowRequestTarget;
    };

    struct CleanupContext {
        std::string actionSlot;
        std::string returnRequestTarget;
        std::string exitRequestTarget;
        std::string leaveRequestTarget;
        std::string continueRequestTarget;
        std::size_t loopDepthAtEntry = 0;
        std::string leaveDestination;
        std::string continueDestination;
        // `until` exits the nearest repeat enclosing this try, possibly across
        // intermediate loops. Empty when the try is not inside a repeat.
        std::string untilRequestTarget;
        std::size_t untilTargetDepth = 0;
        std::string untilDestination;
    };

    struct LocalInfo {
        std::string slot;
        std::string inoxType;
        std::string llvmType;
    };

    struct FieldAddress {
        std::string pointer;
        std::string llvmType;
    };

    void emitIfReturn(const ast::IfStatement& statement);

    void emitReturn(const ast::ReturnStatement& statement);

    void emitExit();

    const CleanupContext* cleanupForLoopTransfer() const;

    void emitReleaseHandlersLeftBy(std::size_t targetLoopDepth);

    void emitLeaveTransfer();

    void emitContinueTransfer();

    void emitIfMerge(const ast::IfStatement& statement);

    void emitUnless(const ast::UnlessStatement& statement);

    void emitCase(const ast::CaseStatement& statement);

    void emitAssignmentBranch(const std::vector<ast::StatementPtr>& statements);

    void emitWhile(const ast::WhileStatement& statement);

    void emitRepeat(const ast::RepeatStatement& statement);

    void emitForIn(const ast::ForInStatement& statement);

    bool emitLoopStatements(const std::vector<ast::StatementPtr>& statements);

    void emitUntil(const ast::UntilStatement& statement);

    const LoopTargets& currentLoopTargets() const;

    void emitWith(const ast::WithStatement& statement);

    std::string emitExceptionTypeMatch(std::string_view actualTypeValue,
                                       std::string_view expectedTypeName);

    void emitRetry(const ast::RetryStatement& statement);

    const std::string& currentUnwindTarget() const;

    std::string newDeadLabel(std::string_view prefix);

    void emitExceptionCapture(std::string_view landingPadLabel,
                              std::string_view stateSlot,
                              std::string_view nextLabel,
                              bool replaceExisting = false);

    void emitReleaseExceptionState(std::string_view stateSlot);

    void emitRethrowState(std::string_view stateSlot);

    void emitRaise(const ast::RaiseStatement& statement);

    void emitTry(const ast::TryStatement& statement);

    void emitStatement(const ast::Statement& statement);

    void emitStatementKind(const ast::Statement& statement);

    void emitVarBlockDeclaration(const ast::Statement& statement);

    std::string newSlot(const std::string& normalizedName);

    void emitLocalVariable(std::string_view name, const ast::Expression& initializer);

    void emitTypedLocalVariable(std::string_view name, std::string_view writtenTypeName, const ast::Expression* initializer = nullptr);

    static bool isAssignmentExpression(const ast::Expression& expression);

    void emitAssignmentOrCallStatement(const ast::Expression& expression);

    void emitExpressionStatement(const ast::Expression& expression);

    std::string emitReferenceArgument(const ast::Expression& argument, const FunctionParameter& parameter);

    std::vector<std::string> emitArguments(const FunctionSignature& signature,
                                           const std::vector<ast::ExpressionPtr>& arguments,
                                           std::size_t parameterOffset);

    std::string emitUserCall(const FunctionSignature& signature,
                             const std::vector<std::string>& arguments,
                             bool requireValue);

    void emitNoArgumentSubroutineCall(std::string_view calleeName);

    void emitSubroutineCall(const ast::CallExpression& call, std::string_view calleeName);

    void emitInputCallSequence(const std::vector<ast::ExpressionPtr>& arguments, bool consumeRestOfLine);

    void emitInputReadLine(const ast::Expression& argument);

    void emitInputReadInteger(const ast::Expression& argument);

    void emitInputDiscardToken();

    void emitInputDiscardLine();

    void emitOutputCallSequence(const std::vector<ast::ExpressionPtr>& arguments, bool newline);

    void emitOutputCall(const ast::Expression& argument, bool newline);

    std::string emitStringLiteral(std::string_view value);

    bool isBoolExpression(const ast::Expression& expression) const;

    // ---- String ownership (CANON-14, OpenSpec change string-runtime) ----
    // Every String expression yields a borrowed pointer, valid until the end
    // of its statement. Owned storage (variables, fields, elements, State,
    // statement temporaries) holds one reference each; a routine releases its
    // owned slots at every exit and, on an exception, in one cleanup pad.

    static bool isStringType(std::string_view inoxType);

    bool typeOwnsStrings(std::string_view inoxType) const;

    void requireModuleLine(const std::string& line);

    // Declares a libinoxrt function once and marks the module as needing
    // the runtime library.
    void requireRuntime(RuntimeFunction function);

    void requireStringRuntime();

    std::string emptyStringValue();

    std::string emitStringValue(std::string_view text);

    void forEachStringAt(const std::string& pointer, std::string_view inoxType,
                         const std::function<void(const std::string&)>& action);

    void emitStringCallAt(const std::string& pointer, std::string_view inoxType, std::string_view function);

    void emitRetainAt(const std::string& pointer, std::string_view inoxType);

    void emitReleaseAt(const std::string& pointer, std::string_view inoxType);

    void emitOwningStore(const std::string& pointer, std::string_view inoxType, const std::string& llvmType,
                         const std::string& value);

    void registerOwnedSlot(const std::string& slot, std::string_view inoxType, const std::string& llvmType,
                           bool initialize = true);

    void holdOwnedResult(const std::string& value, std::string_view inoxType, const std::string& llvmType);

    void emitReleaseOwnedSlots();

    void flushStatementTemporaries();

    void emitRetainResult(const std::string& value);

    bool scanOwnsStrings() const;

    bool structHasDefaults(const StructDefinition& structType) const;

    static bool stringArray(std::string_view inoxType);

    const StructDefinition* defaultedStruct(std::string_view inoxType) const;

    void emitStructDefaults(const std::string& pointer, const StructDefinition& structType);

    void emitStructFieldDefaults(const std::string& pointer, const StructDefinition& structType);

    void emitArrayElementDefaults(const std::string& pointer, std::string_view arrayType);

    std::string emitConstruction(const ast::CallExpression& call, const StructDefinition& structType);

    // The storage of a place: a variable, a field of a place or an element.
    // Any other value is copied into a temporary so its fields can be read.
    struct Place {
        std::string pointer;
        std::string inoxType;
    };

    Place emitPlace(const ast::Expression& expression);

    std::optional<std::pair<const StructDefinition*, const StructFieldInfo*>>
    memberField(const ast::CallExpression& member) const;

    std::string placeTypeName(const ast::Expression& expression) const;

    FieldAddress emitMemberAddress(const ast::CallExpression& call);

    static bool isMemberAccessCall(const ast::Expression& expression);

    struct MethodCallTarget {
        const FunctionSignature* signature = nullptr;
        std::string receiverPointer;
    };

    MethodCallTarget resolveMethodCall(const ast::CallExpression& call);

    MethodCallTarget resolveNoArgumentMethodAccess(const ast::CallExpression& member);

    std::string emitNoArgumentMethodCall(const ast::CallExpression& member, bool requireValue);

    std::string emitMethodCall(const ast::CallExpression& call, bool requireValue);

    void emitLocalAssignment(const ast::Expression& expression);

    const semantic::ConstantValue* resolvedConstant(const ast::IdentifierExpression& identifier) const;

    std::string expressionLlvmType(const ast::Expression& expression) const;

    bool isNaturalConversion(const ast::CallExpression& call) const;

    std::string emitConditional(const ast::ConditionalExpression& expression);

    static bool isIndexCall(const ast::Expression& expression);

    std::string valueLlvmType(const std::string& inoxType) const;

    std::optional<std::int64_t> arrayBoundCall(const ast::CallExpression& call) const;

    struct ElementAddress {
        std::string pointer;
        std::string storage;      // LLVM storage type of the element
        std::string elementType;  // Inox element type
    };

    std::string emitArrayBaseAddress(const ast::Expression& base, const ArrayLayout& layout);

    struct SliceParts {
        std::string data;
        std::string low;
        std::string high;
    };

    SliceParts extractSlice(const std::string& value);

    SliceParts emitSliceSource(const ast::Expression& base, std::string& storage, std::string& elementType);

    std::string emitSlice(const ast::CallExpression& call);

    ElementAddress emitElementAddress(const ast::CallExpression& call);

    std::string loadElement(const ElementAddress& element);

    std::string narrowForStorage(const std::string& value, const ElementAddress& element);

    std::string semanticTypeName(const ast::Expression& expression) const;

    static bool isRangeCheckedArithmetic(const ast::Expression& expression);

    std::string emitRangeCheck(const std::string& value, IntegerRange range, int faultKind);

    std::string emitExpression(const ast::Expression& expression);

    std::string emitExpressionValue(const ast::Expression& expression);

    std::string emitMathBuiltinCall(std::string_view name, const std::vector<ast::ExpressionPtr>& arguments);

    std::ostringstream& output_;
    const ast::FunctionDeclaration& function_;
    const FunctionSignature& signature_;
    const FunctionSignatures& signatures_;
    const StructDefinitions& structs_;
    ModuleContext& module_;
    std::unordered_map<std::string, std::string> parameters_;
    std::unordered_map<std::string, std::string> parameterTypes_;
    std::unordered_map<std::string, LocalInfo> locals_;
    std::unordered_set<std::string> usedSlots_;
    std::size_t slotCounter_ = 0;
    std::vector<LoopTargets> loopTargets_;
    std::vector<std::size_t> repeatLoopDepths_;
    // Handler regions (On / Else / plain except bodies) being emitted, innermost last: the slot
    // holding the caught exception state and the loop depth of their try.
    struct HandlerRegion {
        std::string stateSlot;
        std::size_t loopDepthAtEntry = 0;
    };
    std::vector<HandlerRegion> handlerRegions_;
    std::vector<std::string> unwindTargets_;
    std::vector<std::string> caughtExceptionStates_;
    std::vector<std::string> bareRethrowTargets_;
    std::vector<RetryContext> retryContexts_;
    std::vector<CleanupContext> cleanupContexts_;
    std::unordered_map<std::string, std::string> exceptionBindings_;
    std::string returnValueSlot_;
    std::size_t nextTemporary_ = 0;
    std::size_t nextLabel_ = 0;
    struct OwnedSlot {
        std::string pointer;
        std::string inoxType;
        std::string llvmType;
        std::size_t depth = 0;
    };
    std::vector<OwnedSlot> ownedSlots_;
    std::vector<OwnedSlot> pendingTemps_;
    std::size_t statementDepth_ = 0;
    bool ownsStrings_ = false;
};

} // namespace inox::compiler::codegen::detail
