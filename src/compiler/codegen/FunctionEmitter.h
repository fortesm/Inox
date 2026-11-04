// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Lowers one Inox routine to LLVM IR text (OpenSpec change
// compiler-modularization). The member functions are defined by
// responsibility in FunctionEmitter*.cpp; see AGENTS.md, "Where things live".

#pragma once

#include "ModuleContext.h"
#include "RuntimeAbi.h"
#include "TypeLayout.h"
#include "TirWalk.h"
#include "../semantic/StdIntrinsics.h"
#include "../tir/Tir.h"
#include "../types/TypeContext.h"

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
                    const tir::Routine& routine,
                    const FunctionSignature& signature,
                    const FunctionSignatures& signatures,
                    ModuleContext& module,
                    const types::TypeContext& types,
                    const StateGlobals& stateGlobals);

    bool usesStringCleanup() const;

    void emit();

    void emitBody();

private:
    const types::TypeContext& types_;
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

    void emitIfReturn(const tir::Stmt& statement);

    void emitReturn(const tir::Stmt& statement);

    void emitExit();

    const CleanupContext* cleanupForLoopTransfer() const;

    void emitReleaseHandlersLeftBy(std::size_t targetLoopDepth);

    void emitLeaveTransfer();

    void emitContinueTransfer();

    void emitIfMerge(const tir::Stmt& statement);
    std::string emitCaseChoice(const tir::Expr& choice, const std::string& selector, const std::string& type,
                               bool textual, bool unsignedCompare);
    void emitIfArm(const tir::Expr& condition, const TirStatements& body, std::size_t label, std::size_t armIndex,
                   bool isLastConditionalArm, bool hasElse);

    void emitUnless(const tir::Stmt& statement);

    void emitCase(const tir::Stmt& statement);

    void emitAssignmentBranch(const TirStatements& statements);

    void emitWhile(const tir::Stmt& statement);

    void emitRepeat(const tir::Stmt& statement);

    void emitForIn(const tir::Stmt& statement);

    bool emitLoopStatements(const TirStatements& statements);

    void emitUntil(const tir::Stmt& statement);

    const LoopTargets& currentLoopTargets() const;

    void emitWith(const tir::Stmt& statement);
    void bindWithTarget(const std::string& binding, const tir::Expr& target);

    std::string emitExceptionTypeMatch(std::string_view actualTypeValue,
                                       std::string_view expectedTypeName);

    void emitRetry(const tir::Stmt& statement);

    const std::string& currentUnwindTarget() const;

    std::string newDeadLabel(std::string_view prefix);

    void emitExceptionCapture(std::string_view landingPadLabel,
                              std::string_view stateSlot,
                              std::string_view nextLabel,
                              bool replaceExisting = false);

    void emitReleaseExceptionState(std::string_view stateSlot);

    void emitRethrowState(std::string_view stateSlot);

    void emitRaise(const tir::Stmt& statement);

    void emitTry(const tir::Stmt& statement);

    void emitStatement(const tir::Stmt& statement);

    void emitStatementKind(const tir::Stmt& statement);

    void emitDeclaration(const tir::Stmt& declaration);

    std::string newSlot(const std::string& normalizedName);

    void emitLocalVariable(std::string_view name, const tir::Expr& initializer);

    void emitTypedLocalVariable(std::string_view name, TypeRef type, const tir::Expr* initializer = nullptr);

    void emitExpressionStatement(const tir::Expr& expression);

    std::string emitReferenceArgument(const tir::Expr& argument, const FunctionParameter& parameter);

    std::vector<std::string> emitArguments(const FunctionSignature& signature,
                                           const tir::Span<const tir::Expr*>& arguments,
                                           std::size_t parameterOffset,
                                           std::vector<std::string>* storage = nullptr);

    std::string emitUserCall(const FunctionSignature& signature,
                             const std::vector<std::string>& arguments,
                             bool requireValue);
    std::string emitUserCall(const FunctionSignature& signature,
                             const std::vector<std::string>& arguments,
                             bool requireValue,
                             const std::vector<std::string>& storage);

    void emitInputCallSequence(tir::Span<const tir::Expr*> arguments, bool consumeRestOfLine);

    void emitInputReadLine(const tir::Expr& argument);

    void emitInputReadInteger(const tir::Expr& argument);

    void emitInputDiscardToken();

    void emitInputDiscardLine();

    void emitOutputCallSequence(tir::Span<const tir::Expr*> arguments, bool newline);

    static std::string inputName(const tir::Expr& argument);
    const LocalInfo& inputTarget(const tir::Expr& argument) const;
    std::string emitInputReadCharacter(const tir::Expr& argument);
    void emitInputReadReal(const tir::Expr& argument);
    void emitInputReadDecimal(const tir::Expr& argument);
    std::string emitNumberString(const std::string& value, TypeRef source, const std::string& llvmType);
    TypeRef textConversionTarget(const tir::Expr& conversion) const;
    std::string emitTextToNumber(const tir::Expr& conversion, TypeRef target);
    // Currency and Crypto (change decimals).
    std::pair<std::string, std::string> decimalHalves(const std::string& value, TypeRef type);
    std::string newDecimalSlot();
    std::string loadDecimal(const std::string& slot, TypeRef type);
    void emitDecimalStatus(const std::string& status, int firstKind, int secondKind);
    std::string emitDecimalArithmetic(tir::BinaryOp op, TypeRef type, const std::string& left,
                                      const std::string& right);
    std::string emitDecimalConversion(const tir::Expr& conversion, TypeRef target);
    std::string emitDecimalToInteger(const std::string& low, const std::string& high, TypeRef source, TypeRef target,
                                     bool round);
    std::string emitRounding(const tir::Expr& call);
    void emitOutputValue(const tir::Expr& argument);
    void emitOutputCall(const tir::Expr& argument, bool newline);

    std::string emitStringLiteral(std::string_view value);

    bool isBoolExpression(const tir::Expr& expression) const;

    // ---- String ownership (CANON-14, OpenSpec change string-runtime) ----
    // Every String expression yields a borrowed pointer, valid until the end
    // of its statement. Owned storage (variables, fields, elements, State,
    // statement temporaries) holds one reference each; a routine releases its
    // owned slots at every exit and, on an exception, in one cleanup pad.

    bool typeOwnsReferences(TypeRef type) const;

    void requireModuleLine(const std::string& line);
    // Enums (change enum-range-set; FunctionEmitterEnums.cpp).
    static TypeRef enumTypeNamed(const tir::Expr& expression);
    static bool isOrdinalBuiltin(const tir::Expr& call);
    std::string emitOrdinalBuiltin(const tir::Expr& call);
    std::string enumNameTable(TypeRef enumType);
    std::string emitEnumName(const std::string& value, TypeRef enumType);
    // Sets and `in` (change enum-range-set; FunctionEmitterSets.cpp).
    static bool isSetCall(const tir::Expr& call);
    std::string emitSetMask(const std::string& value, TypeRef setType);
    std::string emitSetCall(const tir::Expr& call);
    // `Fill(Value)` (r3.81): the target array with every element a copy.
    std::string emitFill(const tir::Expr& call);
    // Change char-values: Char(I) checked, and String(C).
    std::string emitCharCheck(const std::string& value, TypeRef target);
    std::string emitCharString(const std::string& value);
    // Change float-conversions: conversions to and from Float.
    TypeRef realConversionTarget(const tir::Expr& conversion) const;
    std::string emitRealConversion(const tir::Expr& conversion, TypeRef target);
    void emitTrapUnless(const std::string& condition, int kind);
    void emitArrayFillDefault(const std::string& pointer, TypeRef arrayType, const types::FieldDefault& fill);
    static std::string defaultConstant(const types::FieldDefault& value, TypeRef type, const std::string& llvmType);
    std::string emitSetComparison(const tir::Expr& expression);
    std::string emitMembership(const tir::Expr& expression);
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

    std::string emitConstruction(const tir::Expr& construction);
    static std::vector<const tir::FieldValue*> constructionOrder(const tir::Expr& construction);

    // The storage of a place: a variable, a field of a place or an element.
    // Any other value is copied into a temporary so its fields can be read.
    struct Place {
        std::string pointer;
        TypeRef type = nullptr;
    };

    Place emitPlace(const tir::Place& place, const tir::Expr* value = nullptr);
    Place emitPlace(const tir::Expr& expression);
    Place emitPlaceCopy(const tir::Expr& expression);
    Place storePlaceTemporary(const std::string& value, TypeRef type, const std::string& llvmType);
    std::string emitPlaceRead(const tir::Place& place);
    std::string emitPlaceValue(const tir::Place& place);
    static FieldLayout fieldOf(const tir::Place& field);

    const FunctionSignature* routineSignature(const tir::Routine* routine) const;
    std::string emitReceiver(const FunctionSignature& signature, const tir::Expr& receiver);
    // A call of a routine of the program (a method: the receiver first).
    std::string emitCall(const tir::Expr& call, bool requireValue);
    static bool isMethodCall(const tir::Expr& call);
    std::string emitMethodCall(const tir::Expr& call, const FunctionSignature& signature, bool requireValue);
    std::string emitMethodCallWith(const FunctionSignature& signature, const tir::Expr& call,
                                   const std::string& receiverPointer, bool guarded, bool requireValue);

    // Design by contract (CANON-24; FunctionEmitterContracts.cpp).
    void emitContractCheck(const tir::Expr& check);
    void emitInvariantChecks(const tir::Invariants* invariants, bool after);
    void emitInvariantOf(TypeRef structType, const std::string& pointer);

    void emitAssignment(const tir::Stmt& assignment);
    void emitCompoundAssignment(const tir::Stmt& assignment);

    bool isNaturalConversion(const tir::Expr& conversion) const;

    TypeRef conversionTarget(const tir::Expr& conversion) const;

    static std::string valueLlvmType(TypeRef type);

    struct ElementAddress {
        std::string pointer;
        std::string storage;          // LLVM storage type of the element
        TypeRef elementType = nullptr;
    };

    std::string emitArrayBaseAddress(const tir::Place& base, const ArrayLayout& layout);
    std::string emitArrayBaseAddress(const tir::Expr& base, const ArrayLayout& layout);
    std::string storeArrayTemporary(const std::string& value, const ArrayLayout& layout);

    struct SliceParts {
        std::string data;
        std::string low;
        std::string high;
    };

    SliceParts extractSlice(const std::string& value);

    SliceParts emitSliceSource(const tir::Expr& base, std::string& storage, TypeRef& elementType);
    SliceParts emitSliceSource(const tir::Place& base, std::string& storage, TypeRef& elementType);
    SliceParts arraySliceParts(const std::string& pointer, const ArrayLayout& layout);

    std::string emitSlice(const tir::Expr& slice);

    ElementAddress emitElementAddress(const tir::Place& element);

    std::string loadElement(const ElementAddress& element);

    std::string narrowForStorage(const std::string& value, const ElementAddress& element);

    std::string emitRangeCheck(const std::string& value, IntegerRange range, int faultKind);

    // ---- The typed IR (F6 step 5) ----
    // An expression without the RangeCheck and NaturalCheck around it.
    static const tir::Expr& bare(const tir::Expr& expression);
    // The value of `expression`, its checks included.
    std::string emitValue(const tir::Expr& expression);
    // The value of `expression` (not a check), protected and narrowed.
    std::string emitValueKind(const tir::Expr& expression);
    bool isBorrowedRead(const tir::Expr& expression) const;
    bool placeMayChange(const tir::Place& place) const;
    bool isBorrowedPlace(TypeRef type, const tir::Place& place) const;
    std::string protectBorrowed(TypeRef type, const std::string& value);
    static bool isRangeCheckedArithmetic(const tir::Expr& expression);
    std::string emitLiteral(const tir::Expr& literal);
    static bool isVariablePlace(const tir::Place& place);
    std::string emitVariableRead(const tir::Place& place);
    std::string emitUnary(const tir::Expr& unary);
    std::string emitBuiltin(const tir::Expr& call);
    std::string emitConversion(const tir::Expr& conversion);
    std::string emitConditional(const tir::Expr& expression);
    std::string expressionLlvmType(const tir::Expr& expression) const;
    bool isStringConcatenation(const tir::Expr& expression) const;

    std::string emitMathBuiltinCall(semantic::BuiltinOp op, tir::Span<const tir::Expr*> arguments);

    // ---- Safe pointers (OpenSpec change safe-pointers; FunctionEmitterPointers.cpp) ----
    // An owner (Own<T>, Option<Own<T>>) is its object's address in a block
    // of libinoxrt; a Link is { object, block header, generation }. A value
    // of a move-only type (an owner, or an aggregate holding one) is moved:
    // every fresh one (New, Move, a call's result, a construction) is held
    // by a statement temporary until a store, an argument or a Return takes
    // it, which disarms the temporary.
    static bool isMoveOnly(TypeRef type);
    static bool isPointerBuiltin(semantic::BuiltinOp op);
    std::string emitPointerBuiltin(const tir::Expr& call);
    // CANON-11 with CANON-25: a `with` whose target is reached through a
    // pointer keeps the header and generation of the object holding it, and
    // every `.Member` checks them, so a Free inside the body is detected.
    static const void* lastPointerOnPath(const tir::Place& place);
    static const void* pointerKey(const tir::Expr& pointer);
    void emitStorageGuard(const std::string& normalizedName);
    // The address of the object a pointer expression reaches, checked: an
    // owner given away traps (17), a dead or empty Link traps (16).
    std::string emitPointerTarget(const tir::Place& pointer);
    std::string emitPointerTarget(const tir::Expr& pointer);
    std::string emitPointerTargetOf(const void* key, TypeRef type, const std::string& value);
    struct LinkParts {
        std::string target;
        std::string header;
        std::string generation;
    };
    LinkParts extractLink(const std::string& link);
    std::string buildLink(const LinkParts& parts);
    void emitLinkCheck(const LinkParts& parts);
    std::string emitLinkAlive(const LinkParts& parts);
    std::string emitLinkOf(const tir::Expr& argument);
    LinkParts emitHeaderOfPlace(const tir::Place& place);
    LinkParts emitHeaderOfPointer(const tir::Place& pointer);
    LinkParts emitHeaderOfPointerValue(TypeRef type, const std::string& value);
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
    // CANON-25: the names whose storage lies in an object that may be freed
    // while they are in use, with that object's header and generation,
    // checked at every use (emitStorageGuard): a `with` binding reached
    // through a pointer, a parameter that is the caller's storage.
    std::unordered_map<std::string, LinkParts> storageGuards_;
    // CANON-25: a place whose object, reached through a pointer, a later
    // index or slice bound may free (it may run a routine): the object's
    // header and generation, captured when the pointer is reached, checked
    // before the place is used (FunctionEmitterPointers.cpp).
    struct PlaceGuard {
        const void* pointer = nullptr;  // a Place, or the Expr of a value (pointerKey)
        std::optional<LinkParts> reached;
    };
    std::vector<PlaceGuard> placeGuards_;
    // Each element or slice access opens one guard (empty when it needs
    // none) and closes it: a stack, never a local of the recursive frames.
    void beginPlaceGuard(const tir::Place& base, bool laterRunsRoutine);
    void beginPlaceGuard(const tir::Expr& base, bool laterRunsRoutine);
    void beginSliceGuard(const tir::Expr& slice);
    void beginElementGuard(const tir::Place& element);
    std::size_t beginReferenceGuard(tir::Span<const tir::Expr*> arguments, std::size_t index);
    // The header and generation a parameter that is the caller's storage
    // receives for `place`: those of the object holding it, or of the
    // immortal header for storage no owner frees.
    void beginCallerStorage(const tir::Expr& place, bool receiver);
    std::string endCallerStorage(const tir::Expr& place);
    const LinkParts* rootStorageGuard(const tir::Expr& place) const;
    std::string immortalStorage();
    void bindStorageGuard(const std::string& binding, const tir::Expr& target);
    void emitArgument(std::vector<std::string>& values, const tir::Span<const tir::Expr*>& arguments,
                      std::size_t index, const FunctionParameter& parameter, std::vector<std::string>* storage,
                      std::size_t& guards);
    std::string emitReferenceParameterArgument(tir::Span<const tir::Expr*> arguments, std::size_t index,
                                               const FunctionParameter& parameter, std::vector<std::string>* storage,
                                               std::size_t& guards);
    bool beginReceiverGuard(const tir::Expr& call, const FunctionSignature& signature);
    void endPlaceGuard();
    void capturePlaceGuards(const void* pointer, const std::string& object, const LinkParts* link);
    const void* withGuardPointer_ = nullptr;
    std::optional<LinkParts> withGuardCaptured_;
    std::unordered_set<std::string> tombstoneLocals_;
    std::unordered_map<std::string, std::string> tombstoneSlots_;

    // ---- Strings: `+` and Std.Strings / Std.StrUtils (change std-strings;
    // FunctionEmitterStringFunctions.cpp) ----
    std::string emitConcatenation(const tir::Expr& expression);
    // The operator of `binary` applied to two values of its operands' types.
    std::string emitBinaryValues(const tir::Expr& binary, const std::string& result,
                                 const std::string& left, std::string right);
    // The checks the value of `expression` takes on its way out (range,
    // Natural, a borrowed read held).
    std::string finishValue(const tir::Expr& expression, std::string value);
    // `S := S + X ...`: false when the assignment is not of that form.
    bool emitStringAppend(const tir::Stmt& assignment);
    std::string emitStringCall(const tir::Expr& call);
    std::string emitStringParts(const std::vector<const tir::Expr*>& parts);
    std::string newStringStatus();
    void emitStringStatusCheck(const std::string& status);
    static bool samePlace(const tir::Place& first, const tir::Place& second, bool path);
    static bool sameValue(const tir::Expr& first, const tir::Expr& second);
    bool mayRunRoutine(const tir::Expr& expression) const;

    // ---- Borrowed values a routine could free (FunctionEmitterStrings.cpp) ----
    // A String read from storage is borrowed until the end of its statement.
    // When the statement may run a routine of the program, and the storage is
    // one a routine can change (State, a ref parameter, a path through a
    // pointer, a with binding), the value read is retained into a statement
    // temporary: the routine cannot free it while the statement still uses it.
    static bool runsRoutine(const tir::Expr& node);
    static bool isCallResult(const tir::Expr& expression);
    bool statementMayRunRoutine(const tir::Stmt& statement) const;
    bool statementRunsRoutine_ = false;
    std::unordered_set<std::string> referenceParameters_;

    // ---- Std.IOUtils (OpenSpec change file-io; FunctionEmitterFiles.cpp) ----
    // A call to a file function (and not to a routine of the program).
    static bool isFileCall(semantic::BuiltinOp op);
    // Std.Terminal (FunctionEmitterIntrinsics.cpp, change console-abi-terminal).
    std::string emitIntrinsicCall(const tir::Expr& expression);
    std::string emitTerminalStruct(TypeRef type, const std::vector<std::pair<std::string, std::string>>& fields);
    std::string narrowFromInt64(const std::string& value, const std::string& llvmType);
    std::string emitInt64Argument(const tir::Expr& argument);
    std::string emitPlainIntrinsic(RuntimeFunction function, const std::string& returnType, const std::string& symbol,
                                   const std::vector<std::string>& arguments);
    // Std.Debug Assert (FunctionEmitterDebug.cpp): a branch to a trap.
    void emitAssertCall(const tir::Expr& call);
    // The LLVM type of a file function's result ("void" when it has none).
    static std::string fileCallLlvmType(semantic::BuiltinOp op);
    std::string emitFileCall(const tir::Expr& call);
    // Calls a libinoxrt function with `invoke` inside a protected region, so
    // an exception it raises reaches the handlers and cleanup pads.
    std::string emitRuntimeCall(RuntimeFunction function, std::string_view returnType, std::string_view symbol,
                                const std::string& arguments);

    std::ostringstream& output_;
    const tir::Routine& routine_;
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
