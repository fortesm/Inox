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
        TypeRef type = nullptr;
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

    void emitTypedLocalVariable(std::string_view name, TypeRef type, const ast::Expression* initializer = nullptr);

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

    void emitNoArgumentSubroutineCall(const ast::IdentifierExpression& callee);

    void emitSubroutineCall(const ast::CallExpression& call, const ast::IdentifierExpression& callee);

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

    bool typeOwnsReferences(TypeRef type) const;

    void requireModuleLine(const std::string& line);
    // Enums (change enum-range-set; FunctionEmitterEnums.cpp).
    TypeRef enumTypeNamed(const ast::Expression& expression) const;
    bool isOrdinalBuiltin(const ast::CallExpression& call) const;
    std::string emitOrdinalBuiltin(const ast::CallExpression& call);
    std::string enumNameTable(TypeRef enumType);
    std::string emitEnumName(const std::string& value, TypeRef enumType);
    // Sets and `in` (change enum-range-set; FunctionEmitterSets.cpp).
    bool isSetCall(const ast::CallExpression& call) const;
    std::string emitSetMask(const std::string& value, TypeRef setType);
    std::string emitSetCall(const ast::CallExpression& call);
    // `Fill(Value)` (r3.81): the target array with every element a copy.
    std::string emitFill(const ast::CallExpression& call);
    std::string emitSetComparison(const ast::BinaryExpression& expression);
    std::string emitMembership(const ast::BinaryExpression& expression);
    // The UInt64 helpers of RuntimeHelpersIr (change uint64), once per module.
    void requireUnsignedHelpers();
    // `T(X)` between integer types where X or T is a UInt64 (change uint64).
    std::string emitUnsignedConversion(const std::string& value, TypeRef source, TypeRef target);

    // Declares a libinoxrt function once and marks the module as needing
    // the runtime library.
    void requireRuntime(RuntimeFunction function);

    void requireStringRuntime();

    std::string emptyStringValue();

    std::string emitStringValue(std::string_view text);

    void forEachStringAt(const std::string& pointer, TypeRef type,
                         const std::function<void(const std::string&)>& action);

    void emitStringCallAt(const std::string& pointer, TypeRef type, std::string_view function);

    // Retains or releases the File stored at pointer.
    void emitFileCallAt(const std::string& pointer, RuntimeFunction function);

    void emitRetainAt(const std::string& pointer, TypeRef type);

    void emitReleaseAt(const std::string& pointer, TypeRef type);

    void emitOwningStore(const std::string& pointer, TypeRef type, const std::string& llvmType,
                         const std::string& value);

    void registerOwnedSlot(const std::string& slot, TypeRef type, const std::string& llvmType,
                           bool initialize = true);

    void holdOwnedResult(const std::string& value, TypeRef type, const std::string& llvmType);

    void emitReleaseOwnedSlots();

    void flushStatementTemporaries();

    void emitRetainResult(const std::string& value);

    bool scanOwnsReferences() const;

    static bool structHasDefaults(TypeRef structType);

    static bool stringArray(TypeRef type);

    static TypeRef defaultedStruct(TypeRef type);

    void emitStructDefaults(const std::string& pointer, TypeRef structType);

    void emitStructFieldDefaults(const std::string& pointer, TypeRef structType);

    void emitArrayElementDefaults(const std::string& pointer, TypeRef arrayType);

    std::string emitConstruction(const ast::CallExpression& call, TypeRef structType);

    // The storage of a place: a variable, a field of a place or an element.
    // Any other value is copied into a temporary so its fields can be read.
    struct Place {
        std::string pointer;
        TypeRef type = nullptr;
    };

    Place emitPlace(const ast::Expression& expression);

    std::optional<FieldLayout> memberField(const ast::CallExpression& member) const;

    // The struct a construction `T(Field := Value, ...)` builds, or null.
    TypeRef constructedStruct(const ast::CallExpression& call) const;

    FieldAddress emitMemberAddress(const ast::CallExpression& call);

    static bool isMemberAccessCall(const ast::Expression& expression);

    struct MethodCallTarget {
        const FunctionSignature* signature = nullptr;
        std::string receiverPointer;
    };

    MethodCallTarget resolveMethodCall(const ast::CallExpression& call);

    // Design by contract (CANON-24; FunctionEmitterContracts.cpp).
    bool isContractCheck(const ast::CallExpression& call) const;
    void emitContractCheck(const ast::CallExpression& call);
    void emitInvariantChecks(const ast::AstNode& site, bool after);
    void emitInvariantOf(TypeRef structType, const std::string& pointer);
    void emitNewValueInvariant(const ast::VarStatement& declaration);
    // The routine the analysis bound a call site to (a method, a function).
    const FunctionSignature* boundSignature(const ast::CallExpression& site) const;
    // The routine a callee names: the one the analysis bound it to (a call of
    // a generic routine names its instance, `Max` is `Max<Int64>`), else the
    // routine of that name; null when it is not a routine of the program.
    const FunctionSignature* calledRoutine(const ast::IdentifierExpression& callee) const;
    std::string emitReceiver(const FunctionSignature& signature, const ast::Expression& receiver);

    MethodCallTarget resolveNoArgumentMethodAccess(const ast::CallExpression& member);

    std::string emitNoArgumentMethodCall(const ast::CallExpression& member, bool requireValue);

    std::string emitMethodCall(const ast::CallExpression& call, bool requireValue);

    void emitLocalAssignment(const ast::Expression& expression);

    const semantic::ConstantValue* resolvedConstant(const ast::IdentifierExpression& identifier) const;

    std::string expressionLlvmType(const ast::Expression& expression) const;

    bool isNaturalConversion(const ast::CallExpression& call) const;

    TypeRef conversionTarget(const ast::CallExpression& call) const;

    std::string emitConditional(const ast::ConditionalExpression& expression);

    static bool isIndexCall(const ast::Expression& expression);

    static std::string valueLlvmType(TypeRef type);

    std::optional<std::int64_t> arrayBoundCall(const ast::CallExpression& call) const;

    struct ElementAddress {
        std::string pointer;
        std::string storage;          // LLVM storage type of the element
        TypeRef elementType = nullptr;
    };

    std::string emitArrayBaseAddress(const ast::Expression& base, const ArrayLayout& layout);

    struct SliceParts {
        std::string data;
        std::string low;
        std::string high;
    };

    SliceParts extractSlice(const std::string& value);

    SliceParts emitSliceSource(const ast::Expression& base, std::string& storage, TypeRef& elementType);

    std::string emitSlice(const ast::CallExpression& call);

    ElementAddress emitElementAddress(const ast::CallExpression& call);

    std::string loadElement(const ElementAddress& element);

    std::string narrowForStorage(const std::string& value, const ElementAddress& element);

    // The type the analysis gave an expression; null when it has none.
    TypeRef typeOf(const ast::Expression& expression) const;

    static bool isRangeCheckedArithmetic(const ast::Expression& expression);

    std::string emitRangeCheck(const std::string& value, IntegerRange range, int faultKind);

    std::string emitExpression(const ast::Expression& expression);

    std::string emitExpressionValue(const ast::Expression& expression);

    std::string emitMathBuiltinCall(std::string_view name, const std::vector<ast::ExpressionPtr>& arguments);

    // ---- Safe pointers (OpenSpec change safe-pointers; FunctionEmitterPointers.cpp) ----
    // An owner (Own<T>, Option<Own<T>>) is its object's address in a block
    // of libinoxrt; a Link is { object, block header, generation }. A value
    // of a move-only type (an owner, or an aggregate holding one) is moved:
    // every fresh one (New, Move, a call's result, a construction) is held
    // by a statement temporary until a store, an argument or a Return takes
    // it, which disarms the temporary.
    static bool isMoveOnly(TypeRef type);
    bool isPointerBuiltin(const ast::CallExpression& call) const;
    std::string emitPointerBuiltin(const ast::CallExpression& call);
    // CANON-11 with CANON-25: a `with` whose target is reached through a
    // pointer keeps the header and generation of the object holding it, and
    // every `.Member` checks them, so a Free inside the body is detected.
    const ast::Expression* lastPointerOnPath(const ast::Expression& place) const;
    void emitWithGuard(const std::string& normalizedName);
    // The address of the object a pointer expression reaches, checked: an
    // owner given away traps (17), a dead or empty Link traps (16).
    std::string emitPointerTarget(const ast::Expression& pointer);
    struct LinkParts {
        std::string target;
        std::string header;
        std::string generation;
    };
    LinkParts extractLink(const std::string& link);
    std::string buildLink(const LinkParts& parts);
    void emitLinkCheck(const LinkParts& parts);
    std::string emitLinkAlive(const LinkParts& parts);
    std::string emitLinkOf(const ast::Expression& argument);
    LinkParts emitHeaderOfPlace(const ast::Expression& place);
    LinkParts emitHeaderOfPointer(const ast::Expression& pointer);
    std::string emitOwnerNullCheck(const std::string& owner);
    std::string dropFunctionFor(TypeRef targetType);
    void emitOwnerFreesAt(const std::string& pointer, TypeRef type);
    void emitTombstoneFree(const std::string& pointer);
    void disarmFresh(const std::string& value);
    std::string emitNewTombstone();
    void collectTombstoneLocals();
    void declareTombstone(const std::string& normalizedName);
    void forEachLeafAt(const std::string& pointer, TypeRef type, const std::function<bool(TypeRef)>& isLeaf,
                       const std::function<void(const std::string&, TypeRef)>& action);
    struct FreshTemp {
        std::string slot;
        std::string llvmType;
    };
    std::unordered_map<std::string, FreshTemp> freshTemps_;
    // with bindings reached through a pointer (see emitWithGuard).
    std::unordered_map<std::string, LinkParts> withGuards_;
    const ast::Expression* withGuardPointer_ = nullptr;
    std::optional<LinkParts> withGuardCaptured_;
    std::unordered_set<std::string> tombstoneLocals_;
    std::unordered_map<std::string, std::string> tombstoneSlots_;

    // ---- Strings: `+` and Std.Strings / Std.StrUtils (change std-strings;
    // FunctionEmitterStringFunctions.cpp) ----
    bool isStringConcatenation(const ast::Expression& expression) const;
    std::string emitConcatenation(const ast::BinaryExpression& expression);
    // `S := S + X ...`: false when the assignment is not of that form.
    bool emitStringAppend(const ast::BinaryExpression& assignment);
    std::string emitStringCall(const ast::CallExpression& call, semantic::StringFunction function);
    std::string emitStringParts(const std::vector<const ast::Expression*>& parts);
    std::string newStringStatus();
    void emitStringStatusCheck(const std::string& status);
    bool samePlace(const ast::Expression& first, const ast::Expression& second) const;
    bool mayRunRoutine(const ast::Expression& expression) const;

    // ---- Borrowed values a routine could free (FunctionEmitterStrings.cpp) ----
    // A String read from storage is borrowed until the end of its statement.
    // When the statement may run a routine of the program, and the storage is
    // one a routine can change (State, a ref parameter, a path through a
    // pointer, a with binding), the value read is retained into a statement
    // temporary: the routine cannot free it while the statement still uses it.
    bool runsRoutine(const ast::Expression& node) const;
    bool statementMayRunRoutine(const ast::Statement& statement) const;
    bool placeMayChange(const ast::Expression& place) const;
    bool isBorrowedRead(const ast::Expression& expression) const;
    std::string protectBorrowed(const ast::Expression& expression, const std::string& value);
    bool statementRunsRoutine_ = false;
    std::unordered_set<std::string> referenceParameters_;

    // ---- Std.IOUtils (OpenSpec change file-io; FunctionEmitterFiles.cpp) ----
    // A call to a file function (and not to a routine of the program).
    bool isFileCall(const ast::CallExpression& call) const;
    // Std.Debug Assert (FunctionEmitterDebug.cpp): a branch to a trap.
    bool isAssertCall(const ast::CallExpression& call) const;
    void emitAssertCall(const ast::CallExpression& call);
    // The LLVM type of a file function's result ("void" when it has none).
    static std::string fileCallLlvmType(const ast::CallExpression& call);
    std::string emitFileCall(const ast::CallExpression& call);
    // Calls a libinoxrt function with `invoke` inside a protected region, so
    // an exception it raises reaches the handlers and cleanup pads.
    std::string emitRuntimeCall(RuntimeFunction function, std::string_view returnType, std::string_view symbol,
                                const std::string& arguments);

    std::ostringstream& output_;
    const ast::FunctionDeclaration& function_;
    const FunctionSignature& signature_;
    const FunctionSignatures& signatures_;
    ModuleContext& module_;
    std::unordered_map<std::string, std::string> parameters_;
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
    // Storage the routine owns: a value that owns references, or the
    // tombstone of a variable some Link points to (FunctionEmitterPointers.cpp).
    struct OwnedSlot {
        std::string pointer;
        TypeRef type = nullptr;
        std::string llvmType;
        std::size_t depth = 0;
        bool tombstone = false;
    };
    void emitReleaseSlot(const OwnedSlot& slot);
    std::vector<OwnedSlot> ownedSlots_;
    std::vector<OwnedSlot> pendingTemps_;
    std::size_t statementDepth_ = 0;
    bool ownsReferences_ = false;
};

} // namespace inox::compiler::codegen::detail
