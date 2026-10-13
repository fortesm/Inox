// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "LlvmIrEmitter.h"

#include "AstQueries.h"
#include "ExceptionSupport.h"
#include "LlvmText.h"
#include "MathBuiltins.h"
#include "Operators.h"
#include "RuntimeHelpersIr.h"
#include "TypeLayout.h"
#include "../exceptions/ExceptionTypes.h"
#include "../support/ArrayType.h"
#include "../support/Platform.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace inox::compiler::codegen {

using namespace detail;

namespace {

class FunctionEmitter {
public:
    FunctionEmitter(std::ostringstream& output,
                    const ast::FunctionDeclaration& function,
                    const FunctionSignature& signature,
                    const FunctionSignatures& signatures,
                    const StructDefinitions& structs,
                    std::vector<std::string>& stringGlobals,
                    std::size_t& nextStringLiteral,
                    const semantic::SemanticResult& semantics,
                    const StateGlobals& stateGlobals)
        : semantics_(semantics),
          stateGlobals_(stateGlobals),
          output_(output),
          function_(function),
          signature_(signature),
          signatures_(signatures),
          structs_(structs),
          stringGlobals_(stringGlobals),
          nextStringLiteral_(nextStringLiteral)
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
    bool usesStringCleanup() const
    {
        return ownsStrings_;
    }

    void emit()
    {
        ownsStrings_ = scanOwnsStrings();
        if (!ownsStrings_) {
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
                if (typeOwnsStrings(state.inoxType)) {
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

    void emitBody()
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

    void emitIfReturn(const ast::IfStatement& statement)
    {
        if (signature_.llvmReturnType != "i64") {
            throw CodegenUnsupported(
                "LLVM emission currently supports if/else only in Integer functions");
        }
        if (statement.elseBody().empty()) {
            throw CodegenUnsupported(
                "LLVM emission currently requires else for direct-return if chains");
        }
        if (statement.thenBody().size() != 1 ||
            statement.thenBody().front()->kind() != ast::AstNodeKind::ReturnStatement ||
            statement.elseBody().size() != 1 ||
            statement.elseBody().front()->kind() != ast::AstNodeKind::ReturnStatement) {
            throw CodegenUnsupported(
                "LLVM emission currently requires a single Return in each if branch");
        }
        for (const auto& clause : statement.elseIfClauses()) {
            if (clause.body.size() != 1 ||
                clause.body.front()->kind() != ast::AstNodeKind::ReturnStatement) {
                throw CodegenUnsupported(
                    "LLVM emission currently requires a single Return in each elif branch");
            }
        }

        const std::size_t label = nextLabel_++;
        const bool hasElseIf = !statement.elseIfClauses().empty();
        const std::string condition = emitExpression(statement.condition());
        output_ << "  br i1 " << condition
                << ", label %then" << label
                << ", label %" << (hasElseIf ? "elifcond" : "else") << label;
        if (hasElseIf) {
            output_ << "_0";
        }
        output_ << "\n\n";

        output_ << "then" << label << ":\n";
        emitReturn(static_cast<const ast::ReturnStatement&>(*statement.thenBody().front()));

        for (std::size_t index = 0; index < statement.elseIfClauses().size(); ++index) {
            const auto& clause = statement.elseIfClauses()[index];
            const bool hasNext = index + 1 < statement.elseIfClauses().size();
            output_ << "\nelifcond" << label << '_' << index << ":\n";
            const std::string elseIfCondition = emitExpression(*clause.condition);
            output_ << "  br i1 " << elseIfCondition
                    << ", label %elifthen" << label << '_' << index
                    << ", label %" << (hasNext ? "elifcond" : "else") << label;
            if (hasNext) {
                output_ << '_' << index + 1;
            }
            output_ << "\n\n";

            output_ << "elifthen" << label << '_' << index << ":\n";
            emitReturn(static_cast<const ast::ReturnStatement&>(*clause.body.front()));
        }

        output_ << "\nelse" << label << ":\n";
        emitReturn(static_cast<const ast::ReturnStatement&>(*statement.elseBody().front()));
    }

    void emitReturn(const ast::ReturnStatement& statement)
    {
        const std::string value = emitExpression(statement.expression());
        emitRetainResult(value);
        emitReleaseHandlersLeftBy(0);
        if (!cleanupContexts_.empty()) {
            if (returnValueSlot_.empty()) {
                throw CodegenError("internal error: Return cleanup requires a return-value slot");
            }
            output_ << "  store " << signature_.llvmReturnType << ' ' << value
                    << ", ptr " << returnValueSlot_ << "\n";
            output_ << "  br label %" << cleanupContexts_.back().returnRequestTarget << "\n";
            const std::string dead = newDeadLabel("eh.after.return");
            output_ << "\n" << dead << ":\n";
            return;
        }
        emitReleaseOwnedSlots();
        output_ << "  ret " << signature_.llvmReturnType << ' ' << value << '\n';
    }

    void emitExit()
    {
        emitReleaseHandlersLeftBy(0);
        if (!cleanupContexts_.empty()) {
            output_ << "  br label %" << cleanupContexts_.back().exitRequestTarget << "\n";
            const std::string dead = newDeadLabel("eh.after.exit");
            output_ << "\n" << dead << ":\n";
            return;
        }
        emitReleaseOwnedSlots();
        if (signature_.llvmReturnType == "i32") {
            output_ << "  ret i32 0\n";
        } else if (signature_.llvmReturnType == "void") {
            output_ << "  ret void\n";
        } else {
            throw CodegenError("Exit is not valid in a value-returning function");
        }
    }

    const CleanupContext* cleanupForLoopTransfer() const
    {
        if (loopTargets_.empty()) {
            return nullptr;
        }
        const std::size_t targetDepth = loopTargets_.size();
        for (auto it = cleanupContexts_.rbegin(); it != cleanupContexts_.rend(); ++it) {
            if (it->loopDepthAtEntry >= targetDepth) {
                return &*it;
            }
        }
        return nullptr;
    }

    // A nonlocal transfer (leave, continue, until, Return, Exit) that leaves a
    // handler region abandons the exception that handler caught: release its
    // state before the transfer, innermost first, as a handler that completes
    // normally does. `targetLoopDepth` is the loop depth the transfer exits to;
    // handlers of a try at that depth or deeper are left. Return and Exit pass 0
    // (they leave every handler). The slot is nulled, so an exception raised
    // later by an ensure on the way does not release it again.
    void emitReleaseHandlersLeftBy(std::size_t targetLoopDepth)
    {
        for (auto it = handlerRegions_.rbegin(); it != handlerRegions_.rend(); ++it) {
            if (it->loopDepthAtEntry >= targetLoopDepth) {
                emitReleaseExceptionState(it->stateSlot);
            }
        }
    }

    void emitLeaveTransfer()
    {
        if (loopTargets_.empty()) {
            throw CodegenError("leave outside loop");
        }
        emitReleaseHandlersLeftBy(loopTargets_.size());
        if (const CleanupContext* cleanup = cleanupForLoopTransfer()) {
            output_ << "  br label %" << cleanup->leaveRequestTarget << "\n";
        } else {
            output_ << "  br label %" << currentLoopTargets().leaveTarget << "\n";
        }
        const std::string dead = newDeadLabel("eh.after.leave");
        output_ << "\n" << dead << ":\n";
    }

    void emitContinueTransfer()
    {
        if (loopTargets_.empty()) {
            throw CodegenError("continue outside loop");
        }
        emitReleaseHandlersLeftBy(loopTargets_.size());
        if (const CleanupContext* cleanup = cleanupForLoopTransfer()) {
            output_ << "  br label %" << cleanup->continueRequestTarget << "\n";
        } else {
            output_ << "  br label %" << currentLoopTargets().continueTarget << "\n";
        }
        const std::string dead = newDeadLabel("eh.after.continue");
        output_ << "\n" << dead << ":\n";
    }

    void emitIfMerge(const ast::IfStatement& statement)
    {
        // if / elif* / else? lowered as a chain of conditional branches that all
        // join at one merge block. Each body is lowered by the general statement
        // dispatcher, so any statement (loops, try, leave, continue, nested if)
        // composes inside any branch.
        const std::size_t label = nextLabel_++;
        const std::string endTarget = "endif" + std::to_string(label);
        const auto& clauses = statement.elseIfClauses();
        const bool hasElse = !statement.elseBody().empty();

        auto emitArm = [&](const ast::Expression& condition,
                           const std::vector<ast::StatementPtr>& body,
                           std::size_t armIndex,
                           bool isLastConditionalArm) {
            const std::string suffix = std::to_string(label) + "_" + std::to_string(armIndex);
            const std::string thenTarget = armIndex == 0 ? "then" + std::to_string(label) : "elifthen" + suffix;
            const std::string nextTarget = isLastConditionalArm
                ? (hasElse ? "else" + std::to_string(label) : endTarget)
                : "elifcond" + std::to_string(label) + "_" + std::to_string(armIndex + 1);
            const std::string value = emitExpression(condition);
            output_ << "  br i1 " << value << ", label %" << thenTarget
                    << ", label %" << nextTarget << "\n\n";
            output_ << thenTarget << ":\n";
            emitAssignmentBranch(body);
            output_ << "  br label %" << endTarget << "\n\n";
            if (!isLastConditionalArm) {
                output_ << nextTarget << ":\n";
            }
        };

        emitArm(statement.condition(), statement.thenBody(), 0, clauses.empty());
        for (std::size_t index = 0; index < clauses.size(); ++index) {
            emitArm(*clauses[index].condition, clauses[index].body, index + 1,
                    index + 1 == clauses.size());
        }

        if (hasElse) {
            output_ << "else" << label << ":\n";
            emitAssignmentBranch(statement.elseBody());
            output_ << "  br label %" << endTarget << "\n\n";
        }

        output_ << endTarget << ":\n";
    }

    // `unless C` runs its body when C is false.
    void emitUnless(const ast::UnlessStatement& statement)
    {
        const std::size_t label = nextLabel_++;
        const std::string bodyTarget = "unlessbody" + std::to_string(label);
        const std::string endTarget = "unlessend" + std::to_string(label);
        const std::string value = emitExpression(statement.condition());
        output_ << "  br i1 " << value << ", label %" << endTarget
                << ", label %" << bodyTarget << "\n\n";
        output_ << bodyTarget << ":\n";
        emitAssignmentBranch(statement.body());
        output_ << "  br label %" << endTarget << "\n\n";
        output_ << endTarget << ":\n";
    }

    // ADR-0011: the selector is evaluated once; arms are tested in order and
    // the first arm whose choices contain the value runs. Semantic analysis
    // has already proven that choices do not overlap and, without
    // `otherwise`, that they cover the selector type, so the fall-through
    // edge of a total case is unreachable.
    void emitCase(const ast::CaseStatement& statement)
    {
        const std::size_t label = nextLabel_++;
        const std::string prefix = "case" + std::to_string(label);
        const std::string endTarget = prefix + "end";
        const std::string type = expressionLlvmType(statement.expression());
        const std::string selector = emitExpression(statement.expression());
        const bool unsignedCompare = type == "i1";
        const auto& arms = statement.arms();
        if (arms.empty()) {
            output_ << "  br label %" << prefix << "otherwise\n\n";
        }

        for (std::size_t armIndex = 0; armIndex < arms.size(); ++armIndex) {
            const std::string bodyTarget = prefix + "arm" + std::to_string(armIndex);
            const std::string nextArm = armIndex + 1 < arms.size()
                ? prefix + "test" + std::to_string(armIndex + 1)
                : prefix + "otherwise";
            const auto& choices = arms[armIndex].choices;
            for (std::size_t choiceIndex = 0; choiceIndex < choices.size(); ++choiceIndex) {
                const ast::Expression& choice = *choices[choiceIndex];
                const std::string onMiss = choiceIndex + 1 < choices.size()
                    ? prefix + "test" + std::to_string(armIndex) + "_" + std::to_string(choiceIndex + 1)
                    : nextArm;
                std::string hit;
                if (choice.kind() == ast::AstNodeKind::BinaryExpression &&
                    static_cast<const ast::BinaryExpression&>(choice).op() == ast::BinaryOperator::Range) {
                    const auto& range = static_cast<const ast::BinaryExpression&>(choice);
                    const std::string low = emitExpression(range.left());
                    const std::string high = emitExpression(range.right());
                    const std::string geLow = "%tmp" + std::to_string(nextTemporary_++);
                    const std::string leHigh = "%tmp" + std::to_string(nextTemporary_++);
                    hit = "%tmp" + std::to_string(nextTemporary_++);
                    output_ << "  " << geLow << " = icmp " << (unsignedCompare ? "uge " : "sge ")
                            << type << " " << selector << ", " << low << "\n";
                    output_ << "  " << leHigh << " = icmp " << (unsignedCompare ? "ule " : "sle ")
                            << type << " " << selector << ", " << high << "\n";
                    output_ << "  " << hit << " = and i1 " << geLow << ", " << leHigh << "\n";
                } else {
                    const std::string value = emitExpression(choice);
                    hit = "%tmp" + std::to_string(nextTemporary_++);
                    output_ << "  " << hit << " = icmp eq " << type << " " << selector
                            << ", " << value << "\n";
                }
                output_ << "  br i1 " << hit << ", label %" << bodyTarget
                        << ", label %" << onMiss << "\n\n";
                if (choiceIndex + 1 < choices.size()) {
                    output_ << onMiss << ":\n";
                }
            }
            output_ << bodyTarget << ":\n";
            emitAssignmentBranch(arms[armIndex].body);
            output_ << "  br label %" << endTarget << "\n\n";
            if (armIndex + 1 < arms.size()) {
                output_ << nextArm << ":\n";
            }
        }

        output_ << prefix << "otherwise:\n";
        if (statement.hasOtherwise()) {
            emitAssignmentBranch(statement.otherwiseBody());
            output_ << "  br label %" << endTarget << "\n\n";
        } else {
            output_ << "  unreachable\n\n";
        }
        output_ << endTarget << ":\n";
    }

    void emitAssignmentBranch(const std::vector<ast::StatementPtr>& statements)
    {
        for (const auto& statement : statements) {
            emitStatement(*statement);
        }
    }

    void emitWhile(const ast::WhileStatement& statement)
    {
        const std::size_t label = nextLabel_++;
        const std::string conditionTarget = "whilecond" + std::to_string(label);
        const std::string endTarget = "whileend" + std::to_string(label);
        output_ << "  br label %whilecond" << label << "\n\n";

        output_ << "whilecond" << label << ":\n";
        const std::string condition = emitExpression(statement.condition());
        output_ << "  br i1 " << condition
                << ", label %whilebody" << label
                << ", label %whileend" << label << "\n\n";

        output_ << "whilebody" << label << ":\n";
        loopTargets_.push_back(LoopTargets{conditionTarget, endTarget});
        const bool terminated = emitLoopStatements(statement.body());
        loopTargets_.pop_back();
        if (!terminated) {
            output_ << "  br label %" << conditionTarget << '\n';
        }
        output_ << '\n';

        output_ << "whileend" << label << ":\n";
    }

    void emitRepeat(const ast::RepeatStatement& statement)
    {
        const std::size_t label = nextLabel_++;
        const std::string bodyTarget = "repeatbody" + std::to_string(label);
        const std::string endTarget = "repeatend" + std::to_string(label);
        output_ << "  br label %" << bodyTarget << "\n\n";

        output_ << bodyTarget << ":\n";
        loopTargets_.push_back(LoopTargets{bodyTarget, endTarget});
        repeatLoopDepths_.push_back(loopTargets_.size());
        const bool terminated = emitLoopStatements(statement.body());
        repeatLoopDepths_.pop_back();
        loopTargets_.pop_back();
        if (!terminated) {
            output_ << "  br label %" << bodyTarget << '\n';
        }
        output_ << '\n';

        output_ << endTarget << ":\n";
    }

    void emitForIn(const ast::ForInStatement& statement)
    {
        if (statement.iterable().kind() != ast::AstNodeKind::BinaryExpression) {
            throw CodegenUnsupported(
                "LLVM emission currently supports only range expressions in for loops");
        }

        const auto& range = static_cast<const ast::BinaryExpression&>(statement.iterable());
        if (range.op() != ast::BinaryOperator::Range) {
            throw CodegenUnsupported(
                "LLVM emission currently supports only Start..End for ranges");
        }

        const std::string iteratorName = normalize(statement.iterator());
        // Semantic analysis already rejected iterators that conflict with a visible
        // symbol; an entry left in `locals_` here belongs to a finished sibling scope.
        std::optional<LocalInfo> shadowedLocal;
        if (const auto existing = locals_.find(iteratorName); existing != locals_.end()) {
            shadowedLocal = existing->second;
        }

        const std::size_t label = nextLabel_++;
        const std::string conditionTarget = "forcond" + std::to_string(label);
        const std::string bodyTarget = "forbody" + std::to_string(label);
        const std::string stepTarget = "forstep" + std::to_string(label);
        const std::string endTarget = "forend" + std::to_string(label);
        const std::string slot = newSlot(iteratorName);

        auto temporary = [this]() { return "%tmp" + std::to_string(nextTemporary_++); };

        // SECTION 17 (LOOPS) `for in range`: both endpoints and the step are evaluated ONCE,
        // before the first iteration (start, end, step order). The direction comes
        // from the endpoints: A<B ascending, A>B descending, A=B runs once. The step
        // is a positive magnitude; a step <= 0 traps before the loop starts.
        output_ << "  " << slot << " = alloca i64\n";
        const std::string startValue = emitExpression(range.left());
        const std::string endValue = emitExpression(range.right());
        std::string increment = "1";
        if (statement.step() != nullptr) {
            const std::string rawStep = emitExpression(*statement.step());
            increment = temporary();
            output_ << "  " << increment << " = call i64 @__inox_for_step_i64(i64 " << rawStep << ")\n";
        }
        const std::string descending = temporary();
        output_ << "  " << descending << " = icmp sgt i64 " << startValue << ", " << endValue << '\n';
        output_ << "  store i64 " << startValue << ", ptr " << slot << '\n';
        locals_.insert_or_assign(iteratorName, LocalInfo{slot, "Integer", "i64"});

        output_ << "  br label %" << conditionTarget << "\n\n";

        output_ << conditionTarget << ":\n";
        const std::string iteratorValue = temporary();
        const std::string ascendingCondition = temporary();
        const std::string descendingCondition = temporary();
        const std::string condition = temporary();
        output_ << "  " << iteratorValue << " = load i64, ptr " << slot << '\n';
        output_ << "  " << ascendingCondition << " = icmp sle i64 " << iteratorValue << ", " << endValue << '\n';
        output_ << "  " << descendingCondition << " = icmp sge i64 " << iteratorValue << ", " << endValue << '\n';
        output_ << "  " << condition << " = select i1 " << descending << ", i1 " << descendingCondition
                << ", i1 " << ascendingCondition << '\n';
        output_ << "  br i1 " << condition
                << ", label %" << bodyTarget
                << ", label %" << endTarget << "\n\n";

        output_ << bodyTarget << ":\n";
        loopTargets_.push_back(LoopTargets{stepTarget, endTarget});
        const bool terminated = emitLoopStatements(statement.body());
        loopTargets_.pop_back();
        if (!terminated) {
            output_ << "  br label %" << stepTarget << '\n';
        }
        output_ << '\n';

        // The next iterator value is computed with overflow detection: if it does
        // not fit in Int64 the range is exhausted, so the loop ends instead of
        // wrapping around (a range ending at Int64.Max or Int64.Min must terminate).
        output_ << stepTarget << ":\n";
        const std::string current = temporary();
        const std::string addPair = temporary();
        const std::string addValue = temporary();
        const std::string addOverflow = temporary();
        const std::string subPair = temporary();
        const std::string subValue = temporary();
        const std::string subOverflow = temporary();
        const std::string nextValue = temporary();
        const std::string nextOverflow = temporary();
        output_ << "  " << current << " = load i64, ptr " << slot << '\n';
        output_ << "  " << addPair << " = call { i64, i1 } @llvm.sadd.with.overflow.i64(i64 "
                << current << ", i64 " << increment << ")\n";
        output_ << "  " << addValue << " = extractvalue { i64, i1 } " << addPair << ", 0\n";
        output_ << "  " << addOverflow << " = extractvalue { i64, i1 } " << addPair << ", 1\n";
        output_ << "  " << subPair << " = call { i64, i1 } @llvm.ssub.with.overflow.i64(i64 "
                << current << ", i64 " << increment << ")\n";
        output_ << "  " << subValue << " = extractvalue { i64, i1 } " << subPair << ", 0\n";
        output_ << "  " << subOverflow << " = extractvalue { i64, i1 } " << subPair << ", 1\n";
        output_ << "  " << nextValue << " = select i1 " << descending << ", i64 " << subValue
                << ", i64 " << addValue << '\n';
        output_ << "  " << nextOverflow << " = select i1 " << descending << ", i1 " << subOverflow
                << ", i1 " << addOverflow << '\n';
        output_ << "  store i64 " << nextValue << ", ptr " << slot << '\n';
        output_ << "  br i1 " << nextOverflow << ", label %" << endTarget
                << ", label %" << conditionTarget << "\n\n";

        output_ << endTarget << ":\n";
        if (shadowedLocal.has_value()) {
            locals_.insert_or_assign(iteratorName, *shadowedLocal);
        } else {
            locals_.erase(iteratorName);
        }
    }

    bool emitLoopStatements(const std::vector<ast::StatementPtr>& statements)
    {
        // Loop bodies are ordinary blocks: every statement goes through the
        // general dispatcher. leave/continue end the current block and open an
        // unreachable continuation label, so the caller may always close the body
        // with a branch back to the loop header.
        for (const auto& statement : statements) {
            emitStatement(*statement);
        }
        return false;
    }

    // SECTION 17 (LOOPS) `until Condition` exits the nearest repeat when true, wherever it
    // appears in the repeat body: also inside if/elif/else, try, and loops nested
    // in the repeat. It is a transfer to an explicit target (the repeat), not to
    // the innermost loop: every ensure between the until and that repeat runs,
    // innermost first, and no other.
    void emitUntil(const ast::UntilStatement& statement)
    {
        if (repeatLoopDepths_.empty()) {
            throw CodegenError("until outside repeat");
        }
        const std::size_t targetDepth = repeatLoopDepths_.back();
        const std::size_t label = nextLabel_++;
        const std::string exitTarget = "untilexit" + std::to_string(label);
        const std::string nextTarget = "untilnext" + std::to_string(label);
        const std::string condition = emitExpression(statement.condition());
        output_ << "  br i1 " << condition << ", label %" << exitTarget
                << ", label %" << nextTarget << "\n\n";
        output_ << exitTarget << ":\n";
        emitReleaseHandlersLeftBy(targetDepth);
        const CleanupContext* crossed = nullptr;
        for (auto it = cleanupContexts_.rbegin(); it != cleanupContexts_.rend(); ++it) {
            if (it->loopDepthAtEntry >= targetDepth) {
                crossed = &*it;
                break;
            }
        }
        if (crossed != nullptr) {
            output_ << "  br label %" << crossed->untilRequestTarget << "\n\n";
        } else {
            output_ << "  br label %" << loopTargets_[targetDepth - 1].leaveTarget << "\n\n";
        }
        output_ << nextTarget << ":\n";
    }

    const LoopTargets& currentLoopTargets() const
    {
        if (loopTargets_.empty()) {
            throw CodegenError(
                "LLVM emission supports leave and continue only inside loops");
        }
        return loopTargets_.back();
    }

    void emitWith(const ast::WithStatement& statement)
    {
        // The target must be an identifier naming an already-declared local.
        if (statement.target().kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenUnsupported(
                "LLVM emission currently supports only local variable targets for 'with'");
        }

        const auto& targetIdentifier =
            static_cast<const ast::IdentifierExpression&>(statement.target());
        const auto targetIt = locals_.find(normalize(targetIdentifier.name()));
        if (targetIt == locals_.end()) {
            throw CodegenError(
                "LLVM emission: 'with' target must be a declared local variable: " +
                targetIdentifier.name());
        }

        // Register the synthetic binding as an alias to the same slot/type.
        locals_.insert_or_assign(normalize(statement.bindingName()), targetIt->second);

        // Emit body statements; dot-prefixed members were already expanded by
        // the parser to __member(__with_N, Field), which resolves via the alias.
        for (const auto& bodyStatement : statement.body()) {
            emitStatement(*bodyStatement);
        }
    }

    std::string emitExceptionTypeMatch(std::string_view actualTypeValue,
                                       std::string_view expectedTypeName)
    {
        const auto ids = exceptions::matchingTypeIds(expectedTypeName);
        if (ids.empty()) {
            throw CodegenUnsupported(
                "unknown exception type for handler matching: " +
                std::string(expectedTypeName));
        }

        std::string combined;
        for (const std::uint64_t id : ids) {
            const std::string cmp = "%eh.match" + std::to_string(nextTemporary_++);
            output_ << "  " << cmp << " = icmp eq i64 " << actualTypeValue
                    << ", " << id << "\n";
            if (combined.empty()) {
                combined = cmp;
            } else {
                const std::string joined = "%eh.match.any" + std::to_string(nextTemporary_++);
                output_ << "  " << joined << " = or i1 " << combined << ", " << cmp << "\n";
                combined = joined;
            }
        }
        return combined;
    }

    void emitRetry(const ast::RetryStatement& statement)
    {
        if (retryContexts_.empty()) {
            throw CodegenError("Retry requires an active On/Else exception handler");
        }

        const RetryContext& context = retryContexts_.back();
        const std::string limit = emitExpression(statement.count());
        const std::string current = "%eh.retry.count" + std::to_string(nextTemporary_++);
        const std::string allowed = "%eh.retry.allowed" + std::to_string(nextTemporary_++);
        const std::string allowedLabel = "eh.retry.allow" + std::to_string(nextLabel_++);
        const std::string exhaustedLabel = "eh.retry.exhausted" + std::to_string(nextLabel_++);
        const std::string deadLabel = newDeadLabel("eh.after.retry");

        output_ << "  " << current << " = load i64, ptr " << context.counterSlot << "\n";
        // Signed comparison intentionally makes a dynamic negative N behave as
        // an exhausted budget. Semantic analysis rejects literal negatives.
        output_ << "  " << allowed << " = icmp slt i64 " << current << ", " << limit << "\n";
        output_ << "  br i1 " << allowed << ", label %" << allowedLabel
                << ", label %" << exhaustedLabel << "\n\n";

        output_ << allowedLabel << ":\n";
        const std::string next = "%eh.retry.next" + std::to_string(nextTemporary_++);
        output_ << "  " << next << " = add i64 " << current << ", 1\n";
        output_ << "  store i64 " << next << ", ptr " << context.counterSlot << "\n";
        output_ << "  store i32 2, ptr " << context.actionSlot << "\n";
        output_ << "  br label %" << context.cleanupTarget << "\n\n";

        output_ << exhaustedLabel << ":\n";
        output_ << "  br label %" << context.rethrowRequestTarget << "\n\n";

        output_ << deadLabel << ":\n";
    }

    const std::string& currentUnwindTarget() const
    {
        if (unwindTargets_.empty()) {
            throw CodegenError("internal error: no active exception unwind target");
        }
        return unwindTargets_.back();
    }

    std::string newDeadLabel(std::string_view prefix)
    {
        return std::string(prefix) + std::to_string(nextLabel_++);
    }

    void emitExceptionCapture(std::string_view landingPadLabel,
                              std::string_view stateSlot,
                              std::string_view nextLabel,
                              bool replaceExisting = false)
    {
        if (usesMsvcExceptionAbi()) {
            // MSVC EH funclets are only an ABI bridge. No Inox user statement
            // is emitted inside the catchpad. Capture the active C++ exception,
            // carry the opaque state in SSA, and return to the parent CFG with catchret.
            const std::string catchSwitch =
                "%eh.cs" + std::to_string(nextTemporary_++);
            const std::string catchPad =
                "%eh.cp" + std::to_string(nextTemporary_++);
            const std::string state =
                "%eh.state" + std::to_string(nextTemporary_++);
            const std::string catchPadLabel =
                std::string(landingPadLabel) + ".catch";
            const std::string parentLabel =
                std::string(landingPadLabel) + ".parent";

            output_ << landingPadLabel << ":\n";
            output_ << "  " << catchSwitch
                    << " = catchswitch within none [label %" << catchPadLabel
                    << "] unwind to caller\n\n";
            output_ << catchPadLabel << ":\n";
            output_ << "  " << catchPad
                    << " = catchpad within " << catchSwitch
                    << " [ptr null, i32 64, ptr null]\n";
            output_ << "  " << state
                    << " = call ptr @__inox_exception_capture(ptr null)"
                    << " [ \"funclet\"(token " << catchPad << ") ]\n";
            output_ << "  catchret from " << catchPad
                    << " to label %" << parentLabel << "\n\n";
            output_ << parentLabel << ":\n";

            if (replaceExisting) {
                const std::string old = "%eh.old" + std::to_string(nextTemporary_++);
                const std::string hasOld = "%eh.hasold" + std::to_string(nextTemporary_++);
                const std::string releaseLabel = "eh.release.old" + std::to_string(nextLabel_++);
                const std::string storeLabel = "eh.store.new" + std::to_string(nextLabel_++);
                output_ << "  " << old << " = load ptr, ptr " << stateSlot << "\n";
                output_ << "  " << hasOld << " = icmp ne ptr " << old << ", null\n";
                output_ << "  br i1 " << hasOld << ", label %" << releaseLabel
                        << ", label %" << storeLabel << "\n\n";
                output_ << releaseLabel << ":\n";
                output_ << "  call void @__inox_exception_release(ptr " << old << ")\n";
                output_ << "  br label %" << storeLabel << "\n\n";
                output_ << storeLabel << ":\n";
            }
            output_ << "  store ptr " << state << ", ptr " << stateSlot << "\n";
            output_ << "  br label %" << nextLabel << "\n\n";
            return;
        }

        output_ << landingPadLabel << ":\n";
        const std::string landing = "%eh.lp" + std::to_string(nextTemporary_++);
        const std::string raw = "%eh.raw" + std::to_string(nextTemporary_++);
        const std::string state = "%eh.state" + std::to_string(nextTemporary_++);
        output_ << "  " << landing << " = landingpad { ptr, i32 } catch ptr null\n";
        output_ << "  " << raw << " = extractvalue { ptr, i32 } " << landing << ", 0\n";
        output_ << "  " << state << " = call ptr @__inox_exception_capture(ptr " << raw << ")\n";
        if (replaceExisting) {
            const std::string old = "%eh.old" + std::to_string(nextTemporary_++);
            const std::string hasOld = "%eh.hasold" + std::to_string(nextTemporary_++);
            const std::string releaseLabel = "eh.release.old" + std::to_string(nextLabel_++);
            const std::string storeLabel = "eh.store.new" + std::to_string(nextLabel_++);
            output_ << "  " << old << " = load ptr, ptr " << stateSlot << "\n";
            output_ << "  " << hasOld << " = icmp ne ptr " << old << ", null\n";
            output_ << "  br i1 " << hasOld << ", label %" << releaseLabel << ", label %" << storeLabel << "\n\n";
            output_ << releaseLabel << ":\n";
            output_ << "  call void @__inox_exception_release(ptr " << old << ")\n";
            output_ << "  br label %" << storeLabel << "\n\n";
            output_ << storeLabel << ":\n";
        }
        output_ << "  store ptr " << state << ", ptr " << stateSlot << "\n";
        output_ << "  br label %" << nextLabel << "\n\n";
    }

    void emitReleaseExceptionState(std::string_view stateSlot)
    {
        const std::string state = "%eh.release" + std::to_string(nextTemporary_++);
        output_ << "  " << state << " = load ptr, ptr " << stateSlot << "\n";
        output_ << "  call void @__inox_exception_release(ptr " << state << ")\n";
        output_ << "  store ptr null, ptr " << stateSlot << "\n";
    }

    void emitRethrowState(std::string_view stateSlot)
    {
        const std::string state = "%eh.rethrow.state" + std::to_string(nextTemporary_++);
        output_ << "  " << state << " = load ptr, ptr " << stateSlot << "\n";
        if (!unwindTargets_.empty()) {
            const std::string impossible = newDeadLabel("eh.rethrow.unreachable");
            output_ << "  invoke void @__inox_exception_rethrow(ptr " << state << ") to label %"
                    << impossible << " unwind label %" << currentUnwindTarget() << "\n\n";
            output_ << impossible << ":\n  unreachable\n";
        } else {
            output_ << "  call void @__inox_exception_rethrow(ptr " << state << ")\n";
            output_ << "  unreachable\n";
        }
    }

    void emitRaise(const ast::RaiseStatement& statement)
    {
        if (statement.expression() == nullptr) {
            if (caughtExceptionStates_.empty() || bareRethrowTargets_.empty()) {
                throw CodegenError("bare Raise requires an active exception handler");
            }
            const std::string dead = newDeadLabel("eh.after.rethrow");
            output_ << "  br label %" << bareRethrowTargets_.back() << "\n\n";
            output_ << dead << ":\n";
            return;
        }

        if (statement.expression()->kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenUnsupported("Raise currently requires an exception type name");
        }
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(*statement.expression());
        const std::uint64_t typeId = exceptionTypeId(identifier.name());
        if (!unwindTargets_.empty()) {
            const std::string impossible = newDeadLabel("eh.raise.unreachable");
            const std::string dead = newDeadLabel("eh.after.raise");
            output_ << "  invoke void @__inox_raise(i64 " << typeId << ") to label %" << impossible
                    << " unwind label %" << currentUnwindTarget() << "\n\n";
            output_ << impossible << ":\n  unreachable\n\n";
            output_ << dead << ":\n";
        } else {
            const std::string dead = newDeadLabel("eh.after.raise");
            output_ << "  call void @__inox_raise(i64 " << typeId << ")\n";
            output_ << "  unreachable\n\n" << dead << ":\n";
        }
    }

    void emitTry(const ast::TryStatement& statement)
    {
        const std::size_t id = nextLabel_++;
        const std::string stateSlot = "%eh.state.slot" + std::to_string(id);
        const std::string retryCounterSlot = "%eh.retry.slot" + std::to_string(id);
        const std::string actionSlot = "%eh.action.slot" + std::to_string(id);
        const std::string bodyLabel = "eh.try.body" + std::to_string(id);
        const std::string landing = "eh.lpad" + std::to_string(id);
        const std::string dispatch = "eh.dispatch" + std::to_string(id);
        const std::string handlerUnwind = "eh.handler.lpad" + std::to_string(id);
        const std::string handlerUnwindCaptured = "eh.handler.captured" + std::to_string(id);
        const std::string ensureLabel = "eh.ensure" + std::to_string(id);
        const std::string ensureUnwind = "eh.ensure.lpad" + std::to_string(id);
        const std::string ensureUnwindCaptured = "eh.ensure.captured" + std::to_string(id);
        const std::string afterEnsure = "eh.after.ensure" + std::to_string(id);
        const std::string retryPerform = "eh.retry.perform" + std::to_string(id);
        const std::string rethrowRequest = "eh.rethrow.request" + std::to_string(id);
        const std::string returnRequest = "eh.return.request" + std::to_string(id);
        const std::string exitRequest = "eh.exit.request" + std::to_string(id);
        const std::string leaveRequest = "eh.leave.request" + std::to_string(id);
        const std::string continueRequest = "eh.loop.continue.request" + std::to_string(id);
        const std::string returnPerform = "eh.return.perform" + std::to_string(id);
        const std::string exitPerform = "eh.exit.perform" + std::to_string(id);
        const std::string leavePerform = "eh.leave.perform" + std::to_string(id);
        const std::string continuePerform = "eh.loop.continue.perform" + std::to_string(id);
        const std::string untilRequest = "eh.until.request" + std::to_string(id);
        const std::string untilPerform = "eh.until.perform" + std::to_string(id);
        const std::string continueLabel = "eh.continue" + std::to_string(id);
        const std::string rethrowLabel = "eh.rethrow" + std::to_string(id);
        const std::string handledLabel = statement.hasEnsure() ? ensureLabel : continueLabel;
        const std::string cleanupForRetry = statement.hasEnsure() ? ensureLabel : retryPerform;

        const std::size_t loopDepthAtEntry = loopTargets_.size();
        const std::string leaveDestination = loopTargets_.empty() ? std::string{} : loopTargets_.back().leaveTarget;
        const std::string continueDestination = loopTargets_.empty() ? std::string{} : loopTargets_.back().continueTarget;
        const std::string outerReturnRequest = cleanupContexts_.empty()
            ? std::string{} : cleanupContexts_.back().returnRequestTarget;
        const std::string outerExitRequest = cleanupContexts_.empty()
            ? std::string{} : cleanupContexts_.back().exitRequestTarget;

        std::string outerLeaveRequest;
        std::string outerContinueRequest;
        if (loopDepthAtEntry != 0) {
            for (auto it = cleanupContexts_.rbegin(); it != cleanupContexts_.rend(); ++it) {
                if (it->loopDepthAtEntry >= loopDepthAtEntry) {
                    outerLeaveRequest = it->leaveRequestTarget;
                    outerContinueRequest = it->continueRequestTarget;
                    break;
                }
            }
        }

        const std::size_t untilTargetDepth = repeatLoopDepths_.empty() ? 0 : repeatLoopDepths_.back();
        const std::string untilDestination = untilTargetDepth == 0
            ? std::string{} : loopTargets_[untilTargetDepth - 1].leaveTarget;
        std::string outerUntilRequest;
        if (untilTargetDepth != 0) {
            for (auto it = cleanupContexts_.rbegin(); it != cleanupContexts_.rend(); ++it) {
                if (it->loopDepthAtEntry >= untilTargetDepth) {
                    outerUntilRequest = it->untilRequestTarget;
                    break;
                }
            }
        }

        const CleanupContext cleanupContext{
            actionSlot,
            returnRequest,
            exitRequest,
            leaveRequest,
            continueRequest,
            loopDepthAtEntry,
            leaveDestination,
            continueDestination,
            untilTargetDepth != 0 ? untilRequest : std::string{},
            untilTargetDepth,
            untilDestination};

        output_ << "  " << stateSlot << " = alloca ptr\n";
        output_ << "  " << retryCounterSlot << " = alloca i64\n";
        output_ << "  " << actionSlot << " = alloca i32\n";
        output_ << "  store ptr null, ptr " << stateSlot << "\n";
        output_ << "  store i64 0, ptr " << retryCounterSlot << "\n";
        output_ << "  store i32 0, ptr " << actionSlot << "\n";
        output_ << "  br label %" << bodyLabel << "\n\n";

        if (statement.hasEnsure()) {
            cleanupContexts_.push_back(cleanupContext);
        }

        output_ << bodyLabel << ":\n";
        unwindTargets_.push_back(landing);
        for (const auto& bodyStatement : statement.body()) {
            emitStatement(*bodyStatement);
        }
        unwindTargets_.pop_back();
        output_ << "  store i32 0, ptr " << actionSlot << "\n";
        output_ << "  br label %" << handledLabel << "\n\n";

        emitExceptionCapture(landing, stateSlot, dispatch);

        output_ << dispatch << ":\n";
        if (!statement.hasExcept()) {
            output_ << "  br label %" << rethrowRequest << "\n\n";
        } else if (statement.hasPlainExcept()) {
            const std::string catchAll = "eh.catchall" + std::to_string(id);
            output_ << "  br label %" << catchAll << "\n\n";
            output_ << catchAll << ":\n";
            caughtExceptionStates_.push_back(stateSlot);
            bareRethrowTargets_.push_back(rethrowRequest);
            unwindTargets_.push_back(handlerUnwind);
            handlerRegions_.push_back(HandlerRegion{stateSlot, loopDepthAtEntry});
            for (const auto& st : statement.exceptBody()) emitStatement(*st);
            handlerRegions_.pop_back();
            unwindTargets_.pop_back();
            bareRethrowTargets_.pop_back();
            caughtExceptionStates_.pop_back();
            emitReleaseExceptionState(stateSlot);
            output_ << "  store i32 0, ptr " << actionSlot << "\n";
            output_ << "  br label %" << handledLabel << "\n\n";
        } else {
            const std::string state = "%eh.dispatch.state" + std::to_string(nextTemporary_++);
            const std::string type = "%eh.type" + std::to_string(nextTemporary_++);
            output_ << "  " << state << " = load ptr, ptr " << stateSlot << "\n";
            output_ << "  " << type << " = call i64 @__inox_exception_type(ptr " << state << ")\n";

            std::vector<std::string> labels;
            labels.reserve(statement.handlers().size());
            for (std::size_t index = 0; index < statement.handlers().size(); ++index) {
                labels.push_back("eh.handler" + std::to_string(id) + "_" + std::to_string(index));
            }
            const std::string elseLabel = !statement.elseBody().empty()
                ? "eh.else" + std::to_string(id) : rethrowRequest;

            std::string nextCheck = "eh.check" + std::to_string(id) + "_0";
            if (statement.handlers().empty()) {
                output_ << "  br label %" << elseLabel << "\n\n";
            } else {
                output_ << "  br label %" << nextCheck << "\n\n";
                for (std::size_t index = 0; index < statement.handlers().size(); ++index) {
                    const auto& handler = statement.handlers()[index];
                    output_ << nextCheck << ":\n";
                    const std::string matched = emitExceptionTypeMatch(type, handler.typeName);
                    const bool last = index + 1 == statement.handlers().size();
                    const std::string noMatch = last ? elseLabel
                        : "eh.check" + std::to_string(id) + "_" + std::to_string(index + 1);
                    output_ << "  br i1 " << matched << ", label %" << labels[index]
                            << ", label %" << noMatch << "\n\n";
                    nextCheck = noMatch;
                }
            }

            const RetryContext retryContext{
                retryCounterSlot, actionSlot, cleanupForRetry, rethrowRequest};

            for (std::size_t index = 0; index < statement.handlers().size(); ++index) {
                const auto& handler = statement.handlers()[index];
                output_ << labels[index] << ":\n";
                caughtExceptionStates_.push_back(stateSlot);
                bareRethrowTargets_.push_back(rethrowRequest);
                retryContexts_.push_back(retryContext);
                if (!handler.bindingName.empty()) {
                    exceptionBindings_.emplace(normalize(handler.bindingName), stateSlot);
                }
                unwindTargets_.push_back(handlerUnwind);
                handlerRegions_.push_back(HandlerRegion{stateSlot, loopDepthAtEntry});
                for (const auto& st : handler.body) emitStatement(*st);
                handlerRegions_.pop_back();
                unwindTargets_.pop_back();
                if (!handler.bindingName.empty()) {
                    exceptionBindings_.erase(normalize(handler.bindingName));
                }
                retryContexts_.pop_back();
                bareRethrowTargets_.pop_back();
                caughtExceptionStates_.pop_back();
                emitReleaseExceptionState(stateSlot);
                output_ << "  store i32 0, ptr " << actionSlot << "\n";
                output_ << "  br label %" << handledLabel << "\n\n";
            }

            if (!statement.elseBody().empty()) {
                output_ << elseLabel << ":\n";
                caughtExceptionStates_.push_back(stateSlot);
                bareRethrowTargets_.push_back(rethrowRequest);
                retryContexts_.push_back(retryContext);
                unwindTargets_.push_back(handlerUnwind);
                handlerRegions_.push_back(HandlerRegion{stateSlot, loopDepthAtEntry});
                for (const auto& st : statement.elseBody()) emitStatement(*st);
                handlerRegions_.pop_back();
                unwindTargets_.pop_back();
                retryContexts_.pop_back();
                bareRethrowTargets_.pop_back();
                caughtExceptionStates_.pop_back();
                emitReleaseExceptionState(stateSlot);
                output_ << "  store i32 0, ptr " << actionSlot << "\n";
                output_ << "  br label %" << handledLabel << "\n\n";
            }
        }

        if (statement.hasExcept()) {
            emitExceptionCapture(handlerUnwind, stateSlot, handlerUnwindCaptured, true);
            output_ << handlerUnwindCaptured << ":\n";
            output_ << "  br label %" << rethrowRequest << "\n\n";
        }

        output_ << rethrowRequest << ":\n";
        output_ << "  store i32 1, ptr " << actionSlot << "\n";
        output_ << "  br label %" << (statement.hasEnsure() ? ensureLabel : rethrowLabel) << "\n\n";

        if (statement.hasEnsure()) {
            output_ << returnRequest << ":\n";
            output_ << "  store i32 3, ptr " << actionSlot << "\n";
            output_ << "  br label %" << ensureLabel << "\n\n";

            output_ << exitRequest << ":\n";
            output_ << "  store i32 4, ptr " << actionSlot << "\n";
            output_ << "  br label %" << ensureLabel << "\n\n";

            if (loopDepthAtEntry != 0) {
                output_ << leaveRequest << ":\n";
                output_ << "  store i32 5, ptr " << actionSlot << "\n";
                output_ << "  br label %" << ensureLabel << "\n\n";

                output_ << continueRequest << ":\n";
                output_ << "  store i32 6, ptr " << actionSlot << "\n";
                output_ << "  br label %" << ensureLabel << "\n\n";
            }

            if (untilTargetDepth != 0) {
                output_ << untilRequest << ":\n";
                output_ << "  store i32 7, ptr " << actionSlot << "\n";
                output_ << "  br label %" << ensureLabel << "\n\n";
            }

            cleanupContexts_.pop_back();

            output_ << ensureLabel << ":\n";
            unwindTargets_.push_back(ensureUnwind);
            for (const auto& st : statement.ensureBody()) emitStatement(*st);
            unwindTargets_.pop_back();
            output_ << "  br label %" << afterEnsure << "\n\n";

            emitExceptionCapture(ensureUnwind, stateSlot, ensureUnwindCaptured, true);
            output_ << ensureUnwindCaptured << ":\n";
            output_ << "  br label %" << rethrowLabel << "\n\n";

            output_ << afterEnsure << ":\n";
            const std::string action = "%eh.action" + std::to_string(nextTemporary_++);
            output_ << "  " << action << " = load i32, ptr " << actionSlot << "\n";
            output_ << "  switch i32 " << action << ", label %" << continueLabel << " [\n";
            output_ << "    i32 1, label %" << rethrowLabel << "\n";
            output_ << "    i32 2, label %" << retryPerform << "\n";
            if (!returnValueSlot_.empty()) {
                output_ << "    i32 3, label %" << returnPerform << "\n";
            }
            if (signature_.llvmReturnType == "i32" || signature_.llvmReturnType == "void") {
                output_ << "    i32 4, label %" << exitPerform << "\n";
            }
            if (loopDepthAtEntry != 0) {
                output_ << "    i32 5, label %" << leavePerform << "\n";
                output_ << "    i32 6, label %" << continuePerform << "\n";
            }
            if (untilTargetDepth != 0) {
                output_ << "    i32 7, label %" << untilPerform << "\n";
            }
            output_ << "  ]\n\n";

            if (!returnValueSlot_.empty()) {
                output_ << returnPerform << ":\n";
                if (!outerReturnRequest.empty()) {
                    output_ << "  br label %" << outerReturnRequest << "\n\n";
                } else {
                    const std::string returnValue = "%eh.return.value" + std::to_string(nextTemporary_++);
                    output_ << "  " << returnValue << " = load " << signature_.llvmReturnType
                            << ", ptr " << returnValueSlot_ << "\n";
                    emitReleaseOwnedSlots();
                    output_ << "  ret " << signature_.llvmReturnType << " " << returnValue << "\n\n";
                }
            }

            if (signature_.llvmReturnType == "i32" || signature_.llvmReturnType == "void") {
                output_ << exitPerform << ":\n";
                if (!outerExitRequest.empty()) {
                    output_ << "  br label %" << outerExitRequest << "\n\n";
                } else if (signature_.llvmReturnType == "i32") {
                    emitReleaseOwnedSlots();
                    output_ << "  ret i32 0\n\n";
                } else {
                    emitReleaseOwnedSlots();
                    output_ << "  ret void\n\n";
                }
            }

            if (loopDepthAtEntry != 0) {
                output_ << leavePerform << ":\n";
                if (!outerLeaveRequest.empty()) {
                    output_ << "  br label %" << outerLeaveRequest << "\n\n";
                } else {
                    output_ << "  br label %" << leaveDestination << "\n\n";
                }

                output_ << continuePerform << ":\n";
                if (!outerContinueRequest.empty()) {
                    output_ << "  br label %" << outerContinueRequest << "\n\n";
                } else {
                    output_ << "  br label %" << continueDestination << "\n\n";
                }
            }

            if (untilTargetDepth != 0) {
                output_ << untilPerform << ":\n";
                if (!outerUntilRequest.empty()) {
                    output_ << "  br label %" << outerUntilRequest << "\n\n";
                } else {
                    output_ << "  br label %" << untilDestination << "\n\n";
                }
            }
        }

        output_ << retryPerform << ":\n";
        emitReleaseExceptionState(stateSlot);
        output_ << "  store i32 0, ptr " << actionSlot << "\n";
        output_ << "  br label %" << bodyLabel << "\n\n";

        output_ << rethrowLabel << ":\n";
        emitRethrowState(stateSlot);
        const std::string dead = newDeadLabel("eh.after.unhandled");
        output_ << "\n" << dead << ":\n";
        output_ << "  br label %" << continueLabel << "\n\n";

        output_ << continueLabel << ":\n";
    }

    // Every statement releases the String temporaries it created.
    void emitStatement(const ast::Statement& statement)
    {
        ++statementDepth_;
        emitStatementKind(statement);
        flushStatementTemporaries();
        --statementDepth_;
    }

    void emitStatementKind(const ast::Statement& statement)
    {
        if (statement.kind() == ast::AstNodeKind::VarStatement) {
            const auto& variable = static_cast<const ast::VarStatement&>(statement);
            if (variable.initializer() == nullptr) {
                if (variable.typeName().empty()) {
                    throw CodegenUnsupported(
                        "LLVM emission currently requires local variable initialization");
                }
                emitTypedLocalVariable(variable.name(), variable.typeName());
            } else if (!variable.typeName().empty()) {
                emitTypedLocalVariable(variable.name(), variable.typeName(), variable.initializer());
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

    void emitVarBlockDeclaration(const ast::Statement& statement)
    {
        if (statement.kind() == ast::AstNodeKind::VarStatement) {
            const auto& variable = static_cast<const ast::VarStatement&>(statement);
            if (variable.initializer() == nullptr) {
                emitTypedLocalVariable(variable.name(), variable.typeName());
            } else if (!variable.typeName().empty()) {
                emitTypedLocalVariable(variable.name(), variable.typeName(), variable.initializer());
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
    std::string newSlot(const std::string& normalizedName)
    {
        const std::string base = "%" + safeLlvmName(normalizedName);
        std::string candidate = base;
        while (!usedSlots_.insert(candidate).second) {
            candidate = base + "." + std::to_string(slotCounter_++);
        }
        return candidate;
    }

    void emitLocalVariable(std::string_view name, const ast::Expression& initializer)
    {
        const std::string normalizedName = normalize(name);
        const std::string llvmType = expressionLlvmType(initializer);
        const std::string semanticType = semanticTypeName(initializer);
        const bool aggregate = findStruct(structs_, semanticType) != nullptr || arrayLayout(semanticType);
        const std::string inoxType = aggregate ? semanticType :
                                    isStringType(semanticType) ? "String" :
                                    llvmType == "double" ? "Float64" :
                                    llvmType == "i1" ? "Bool" : "Integer";
        const std::string slot = newSlot(normalizedName);
        output_ << "  " << slot << " = alloca " << llvmType << "\n";
        if (typeOwnsStrings(inoxType)) {
            registerOwnedSlot(slot, inoxType, llvmType);
        }
        const std::string value = emitExpression(initializer);
        emitOwningStore(slot, inoxType, llvmType, value);
        locals_.insert_or_assign(normalizedName, LocalInfo{slot, inoxType, llvmType});
    }

    void emitTypedLocalVariable(std::string_view name, std::string_view writtenTypeName, const ast::Expression* initializer = nullptr)
    {
        const std::string canonicalArray = semantics_.canonicalTypeName(std::string(writtenTypeName));
        const std::string_view typeName =
            support::isArrayTypeName(writtenTypeName) || support::isSliceTypeName(writtenTypeName)
                ? std::string_view(canonicalArray)
                : writtenTypeName;
        const std::string llvmType = llvmTypeForInoxType(typeName, structs_);
        if (llvmType.empty()) {
            throw CodegenUnsupported("unsupported local variable type for LLVM emission");
        }

        const std::string normalizedName = normalize(name);
        const std::string slot = newSlot(normalizedName);
        output_ << "  " << slot << " = alloca " << llvmType << "\n";
        const bool owned = typeOwnsStrings(typeName);
        if (owned) {
            // A declaration that runs again (in a loop) first releases the
            // value of the previous run.
            registerOwnedSlot(slot, typeName, llvmType);
        }

        if (const StructDefinition* structType = findStruct(structs_, typeName)) {
            // CANON-9: `Q T := Expr` copies the value; `P T` takes the type's
            // field defaults (rule 4).
            if (initializer != nullptr) {
                const std::string value = emitExpression(*initializer);
                emitOwningStore(slot, typeName, structType->llvmName, value);
            } else {
                if (owned) {
                    emitReleaseAt(slot, typeName);
                }
                emitStructDefaults(slot, *structType);
            }
            locals_.insert_or_assign(normalizedName,
                                     LocalInfo{slot, std::string(typeName), structType->llvmName});
            return;
        }

        if (support::isArrayTypeName(typeName)) {
            // CANON-13: an array is a value; without an initializer every
            // element is its type's zero, or its struct type's defaults.
            if (initializer != nullptr) {
                emitOwningStore(slot, typeName, llvmType, emitExpression(*initializer));
            } else {
                if (owned) {
                    emitReleaseAt(slot, typeName);
                }
                output_ << "  store " << llvmType << " zeroinitializer, ptr " << slot << '\n';
                emitArrayElementDefaults(slot, typeName);
            }
            locals_.insert_or_assign(normalizedName, LocalInfo{slot, std::string(typeName), llvmType});
            return;
        }

        // A declared-but-uninitialized scalar starts at its type's zero. LLVM
        // requires a floating-point literal for float and double, so `0` is not
        // valid there.
        const std::string zero = isFloatLlvmType(llvmType) ? "0.0" : "0";
        const std::string value = initializer != nullptr ? emitExpression(*initializer) : zero;
        emitOwningStore(slot, typeName, llvmType, value);
        locals_.insert_or_assign(normalizedName, LocalInfo{slot, std::string(typeName), llvmType});
    }

    static bool isAssignmentExpression(const ast::Expression& expression)
    {
        if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
            return false;
        }

        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        return binary.op() == ast::BinaryOperator::Assign;
    }

    void emitAssignmentOrCallStatement(const ast::Expression& expression)
    {
        if (isAssignmentExpression(expression)) {
            emitLocalAssignment(expression);
            return;
        }
        emitExpressionStatement(expression);
    }

    void emitExpressionStatement(const ast::Expression& expression)
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
            emitNoArgumentSubroutineCall(identifier.name());
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

        emitSubroutineCall(call, callee.name());
    }

    // ADR-0016: a reference argument passes the address of the caller's
    // place: a variable's slot, a field or an element; a read-only reference
    // to any other value gets a temporary.
    std::string emitReferenceArgument(const ast::Expression& argument, const FunctionParameter& parameter)
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

    std::vector<std::string> emitArguments(const FunctionSignature& signature,
                                           const std::vector<ast::ExpressionPtr>& arguments,
                                           std::size_t parameterOffset)
    {
        std::vector<std::string> values;
        values.reserve(arguments.size());
        for (std::size_t index = 0; index < arguments.size(); ++index) {
            const FunctionParameter& parameter = signature.parameters[index + parameterOffset];
            values.push_back(parameter.byReference ? emitReferenceArgument(*arguments[index], parameter)
                                                   : emitExpression(*arguments[index]));
        }
        return values;
    }

    std::string emitUserCall(const FunctionSignature& signature,
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
        if (!result.empty() && typeOwnsStrings(signature.returnInoxType)) {
            holdOwnedResult(result, signature.returnInoxType, signature.llvmReturnType);
        }
        return result;
    }

    void emitNoArgumentSubroutineCall(std::string_view calleeName)
    {
        const auto signature = signatures_.find(normalize(calleeName));
        if (signature == signatures_.end()) {
            throw CodegenUnsupported(
                "LLVM emission could not resolve zero-argument subroutine call: " + std::string(calleeName));
        }
        if (!signature->second.parameters.empty()) {
            throw CodegenError(
                "subroutine requires arguments; use parentheses only when passing arguments");
        }
        if (signature->second.llvmReturnType != "void") {
            throw CodegenError(
                "function result cannot be ignored in zero-argument call: " + std::string(calleeName));
        }
        emitUserCall(signature->second, {}, false);
    }

    void emitSubroutineCall(const ast::CallExpression& call, std::string_view calleeName)
    {
        const auto signature = signatures_.find(normalize(calleeName));
        if (signature == signatures_.end()) {
            throw CodegenUnsupported(
                "LLVM emission currently supports only Put/PutLn and user subroutine calls as statements");
        }
        if (signature->second.llvmReturnType != "void") {
            throw CodegenUnsupported(
                "LLVM emission currently supports only subroutine calls as expression statements");
        }
        if (call.arguments().size() != signature->second.parameters.size()) {
            throw CodegenUnsupported("unsupported subroutine argument count: " + function_.name());
        }

        const std::vector<std::string> arguments = emitArguments(signature->second, call.arguments(), 0);
        emitUserCall(signature->second, arguments, false);
    }

    void emitInputCallSequence(const std::vector<ast::ExpressionPtr>& arguments, bool consumeRestOfLine)
    {
        if (arguments.empty()) {
            throw CodegenError("Get/GetLn calls with parentheses require at least one argument");
        }

        bool lastReadLine = false;
        for (const auto& argument : arguments) {
            lastReadLine = isStringType(semanticTypeName(*argument));
            if (lastReadLine) {
                emitInputReadLine(*argument);
            } else {
                emitInputReadInteger(*argument);
            }
        }

        // A String argument already consumed the rest of its line.
        if (consumeRestOfLine && !lastReadLine) {
            emitInputDiscardLine();
        }
    }

    // CANON-17 (v3.50): GetLn(S) reads the rest of the line, without its line
    // end, through the program's getchar (StringRuntime.cpp). Invalid UTF-8 and
    // allocation failure are runtime faults 13 and 12.
    void emitInputReadLine(const ast::Expression& argument)
    {
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(argument);
        const auto local = locals_.find(normalize(identifier.name()));
        if (local == locals_.end()) {
            throw CodegenUnsupported("GetLn LLVM emission currently supports only local or State variables");
        }
        requireStringRuntime();
        requireModuleLine("declare ptr @__inox_str_getln(ptr, ptr) nounwind");
        const std::string status = "%getln.status" + std::to_string(nextTemporary_++);
        const std::string line = "%tmp" + std::to_string(nextTemporary_++);
        const std::string code = "%tmp" + std::to_string(nextTemporary_++);
        const std::string label = std::to_string(nextLabel_++);
        output_ << "  " << status << " = alloca i32\n";
        output_ << "  " << line << " = call ptr @__inox_str_getln(ptr @getchar, ptr " << status << ")\n";
        output_ << "  " << code << " = load i32, ptr " << status << '\n';
        output_ << "  switch i32 " << code << ", label %getln.ok" << label << " [\n"
                << "    i32 1, label %getln.utf8" << label << "\n"
                << "    i32 2, label %getln.memory" << label << "\n  ]\n\n";
        output_ << "getln.utf8" << label << ":\n  call void @__inox_arith_fault(i32 13)\n  unreachable\n\n";
        output_ << "getln.memory" << label << ":\n  call void @__inox_arith_fault(i32 12)\n  unreachable\n\n";
        output_ << "getln.ok" << label << ":\n";
        // The line is owned (+1): it replaces the old value without a retain.
        const std::string old = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << old << " = load ptr, ptr " << local->second.slot << '\n';
        output_ << "  store ptr " << line << ", ptr " << local->second.slot << '\n';
        output_ << "  call void @__inox_str_release(ptr " << old << ")\n";
    }

    void emitInputReadInteger(const ast::Expression& argument)
    {
        if (argument.kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenUnsupported("Get/GetLn LLVM emission requires assignable local variables");
        }

        const auto& identifier = static_cast<const ast::IdentifierExpression&>(argument);
        const auto local = locals_.find(normalize(identifier.name()));
        if (local == locals_.end()) {
            throw CodegenUnsupported("Get/GetLn LLVM emission currently supports only local variables");
        }
        if (local->second.llvmType != "i64") {
            throw CodegenUnsupported("Get/GetLn LLVM emission currently supports only Integer/Int64 variables");
        }

        output_ << "  call void @__inox_read_i64(ptr " << local->second.slot << ")\n";
        if (const auto range = narrowIntegerRange(local->second.inoxType)) {
            const std::string read = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << read << " = load i64, ptr " << local->second.slot << "\n";
            const std::string checked = emitRangeCheck(read, *range, 6);
            output_ << "  store i64 " << checked << ", ptr " << local->second.slot << "\n";
        }
    }

    void emitInputDiscardToken()
    {
        output_ << "  call void @__inox_discard_token()\n";
    }

    void emitInputDiscardLine()
    {
        output_ << "  call void @__inox_discard_line()\n";
    }

    void emitOutputCallSequence(const std::vector<ast::ExpressionPtr>& arguments, bool newline)
    {
        if (arguments.empty()) {
            throw CodegenError("Put/PutLn LLVM emission expects at least one argument");
        }

        for (std::size_t index = 0; index < arguments.size(); ++index) {
            emitOutputCall(*arguments[index], newline && index + 1 == arguments.size());
        }
    }

    void emitOutputCall(const ast::Expression& argument, bool newline)
    {
        if (argument.kind() == ast::AstNodeKind::LiteralExpression) {
            const auto& literal = static_cast<const ast::LiteralExpression&>(argument);
            if (literal.literalKind() == ast::LiteralKind::String) {
                const std::string value = emitStringLiteral(literal.value());
                output_ << "  call i32 (ptr, ...) @printf(ptr "
                        << (newline ? "@.inox.fmt.str.nl" : "@.inox.fmt.str")
                        << ", ptr " << value << ")\n";
                return;
            }
        }

        if (isStringType(semanticTypeName(argument))) {
            // The bytes of the block, printed by the program's printf so the
            // output stays in order with every other Put (StringRuntime.cpp).
            requireModuleLine("@.inox.fmt.strv = private unnamed_addr constant [5 x i8] c\"%.*s\\00\"");
            requireModuleLine("@.inox.fmt.strv.nl = private unnamed_addr constant [6 x i8] c\"%.*s\\0A\\00\"");
            const std::string value = emitExpression(argument);
            const std::string lengthPointer = "%tmp" + std::to_string(nextTemporary_++);
            const std::string length = "%tmp" + std::to_string(nextTemporary_++);
            const std::string length32 = "%tmp" + std::to_string(nextTemporary_++);
            const std::string bytes = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << lengthPointer << " = getelementptr { i64, i64 }, ptr " << value << ", i32 0, i32 1\n";
            output_ << "  " << length << " = load i64, ptr " << lengthPointer << '\n';
            output_ << "  " << length32 << " = trunc i64 " << length << " to i32\n";
            output_ << "  " << bytes << " = getelementptr i8, ptr " << value << ", i64 16\n";
            output_ << "  call i32 (ptr, ...) @printf(ptr " << (newline ? "@.inox.fmt.strv.nl" : "@.inox.fmt.strv")
                    << ", i32 " << length32 << ", ptr " << bytes << ")\n";
            return;
        }

        if (isBoolExpression(argument)) {
            const std::string value = emitExpression(argument);
            const std::string selected = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << selected
                    << " = select i1 " << value
                    << ", ptr @.inox.true, ptr @.inox.false\n";
            output_ << "  call i32 (ptr, ...) @printf(ptr "
                    << (newline ? "@.inox.fmt.str.nl" : "@.inox.fmt.str")
                    << ", ptr " << selected << ")\n";
            return;
        }

        const std::string argumentType = expressionLlvmType(argument);
        const std::string value = emitExpression(argument);
        if (isFloatLlvmType(argumentType)) {
            output_ << "  call i32 (ptr, ...) @printf(ptr "
                    << (newline ? "@.inox.fmt.f64.nl" : "@.inox.fmt.f64")
                    << ", double " << value << ")\n";
            return;
        }

        output_ << "  call i32 (ptr, ...) @printf(ptr "
                << (newline ? "@.inox.fmt.i64.nl" : "@.inox.fmt.i64")
                << ", i64 " << value << ")\n";
    }

    std::string emitStringLiteral(std::string_view value)
    {
        const std::string name = ".inox.str." + std::to_string(nextStringLiteral_++);
        const LlvmStringConstant constant = llvmStringConstant(value);
        std::ostringstream global;
        global << "@" << name << " = private unnamed_addr constant ["
               << constant.size << " x i8] c\"" << constant.bytes << "\"";
        stringGlobals_.push_back(global.str());
        return "@" + name;
    }

    bool isBoolExpression(const ast::Expression& expression) const
    {
        if (expression.kind() == ast::AstNodeKind::LiteralExpression) {
            const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
            return literal.literalKind() == ast::LiteralKind::Boolean;
        }

        if (expression.kind() == ast::AstNodeKind::UnaryExpression) {
            const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
            return unary.op() == ast::UnaryOperator::Not;
        }

        if (expression.kind() == ast::AstNodeKind::BinaryExpression) {
            const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
            return !llvmComparisonPredicate(binary.op()).empty() ||
                   !llvmBooleanOperation(binary.op()).empty();
        }

        return expressionLlvmType(expression) == "i1";
    }

    // ---- String ownership (CANON-14, OpenSpec change string-runtime) ----
    // Every String expression yields a borrowed pointer, valid until the end
    // of its statement. Owned storage (variables, fields, elements, State,
    // statement temporaries) holds one reference each; a routine releases its
    // owned slots at every exit and, on an exception, in one cleanup pad.

    static bool isStringType(std::string_view inoxType)
    {
        return equalsIgnoreCase(inoxType, "String");
    }

    bool typeOwnsStrings(std::string_view inoxType) const
    {
        if (isStringType(inoxType)) {
            return true;
        }
        if (const auto layout = arrayLayout(inoxType)) {
            return typeOwnsStrings(layout->elementType);
        }
        if (const StructDefinition* structType = findStruct(structs_, inoxType)) {
            for (const StructFieldInfo& field : structType->fields) {
                if (typeOwnsStrings(field.inoxType)) {
                    return true;
                }
            }
        }
        return false;
    }

    void requireModuleLine(const std::string& line)
    {
        if (std::find(stringGlobals_.begin(), stringGlobals_.end(), line) == stringGlobals_.end()) {
            stringGlobals_.push_back(line);
        }
    }

    void requireStringRuntime()
    {
        requireModuleLine("declare void @__inox_str_retain(ptr) nounwind");
        requireModuleLine("declare void @__inox_str_release(ptr) nounwind");
    }

    // The shared immortal "" of this module.
    std::string emptyStringValue()
    {
        requireModuleLine("@.inox.strv.empty = private unnamed_addr constant { i64, i64, [1 x i8] } "
                          "{ i64 -1, i64 0, [1 x i8] zeroinitializer }");
        return "@.inox.strv.empty";
    }

    // A String literal as a value: an immortal block in the module.
    std::string emitStringValue(std::string_view text)
    {
        const LlvmStringConstant constant = llvmStringConstant(text);
        if (constant.size == 1) {
            return emptyStringValue();
        }
        const std::string name = "@.inox.strv." + std::to_string(nextStringLiteral_++);
        std::ostringstream global;
        global << name << " = private unnamed_addr constant { i64, i64, [" << constant.size
               << " x i8] } { i64 -1, i64 " << constant.size - 1 << ", [" << constant.size
               << " x i8] c\"" << constant.bytes << "\" }";
        stringGlobals_.push_back(global.str());
        return name;
    }

    // Calls `action` with the address of every String inside a value of
    // `inoxType` stored at `pointer`: the value itself, struct fields and
    // array elements, recursively.
    void forEachStringAt(const std::string& pointer, std::string_view inoxType,
                         const std::function<void(const std::string&)>& action)
    {
        if (isStringType(inoxType)) {
            action(pointer);
            return;
        }
        if (const StructDefinition* structType = findStruct(structs_, inoxType)) {
            for (const StructFieldInfo& field : structType->fields) {
                if (!typeOwnsStrings(field.inoxType)) {
                    continue;
                }
                const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << fieldPointer << " = getelementptr " << structType->llvmName << ", ptr "
                        << pointer << ", i32 0, i32 " << field.index << '\n';
                forEachStringAt(fieldPointer, field.inoxType, action);
            }
            return;
        }
        std::optional<ArrayLayout> layout = arrayLayout(inoxType);
        if (!layout) {
            return;
        }
        std::int64_t count = 1;
        std::string element;
        while (layout) {
            count *= layout->high - layout->low + 1;
            element = layout->elementType;
            layout = arrayLayout(element);
        }
        if (!typeOwnsStrings(element)) {
            return;
        }
        const std::string storage = llvmTypeForInoxType(element, structs_);
        const std::string label = std::to_string(nextLabel_++);
        const std::string counter = "%strwalk.i" + label;
        output_ << "  " << counter << " = alloca i64\n";
        output_ << "  store i64 0, ptr " << counter << '\n';
        output_ << "  br label %strwalk.cond" << label << "\n\n";
        output_ << "strwalk.cond" << label << ":\n";
        const std::string index = "%tmp" + std::to_string(nextTemporary_++);
        const std::string more = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << index << " = load i64, ptr " << counter << '\n';
        output_ << "  " << more << " = icmp slt i64 " << index << ", " << count << '\n';
        output_ << "  br i1 " << more << ", label %strwalk.body" << label << ", label %strwalk.end" << label
                << "\n\n";
        output_ << "strwalk.body" << label << ":\n";
        const std::string elementPointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << elementPointer << " = getelementptr inbounds " << storage << ", ptr " << pointer
                << ", i64 " << index << '\n';
        forEachStringAt(elementPointer, element, action);
        const std::string next = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << next << " = add i64 " << index << ", 1\n";
        output_ << "  store i64 " << next << ", ptr " << counter << '\n';
        output_ << "  br label %strwalk.cond" << label << "\n\n";
        output_ << "strwalk.end" << label << ":\n";
    }

    void emitStringCallAt(const std::string& pointer, std::string_view inoxType, std::string_view function)
    {
        requireStringRuntime();
        forEachStringAt(pointer, inoxType, [&](const std::string& at) {
            const std::string value = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << value << " = load ptr, ptr " << at << '\n';
            output_ << "  call void @" << function << "(ptr " << value << ")\n";
        });
    }

    void emitRetainAt(const std::string& pointer, std::string_view inoxType)
    {
        emitStringCallAt(pointer, inoxType, "__inox_str_retain");
    }

    void emitReleaseAt(const std::string& pointer, std::string_view inoxType)
    {
        emitStringCallAt(pointer, inoxType, "__inox_str_release");
    }

    // A store into owned storage (variable, field, element, State): the new
    // value is retained before the old one is released, so `S := S` is safe.
    void emitOwningStore(const std::string& pointer, std::string_view inoxType, const std::string& llvmType,
                         const std::string& value)
    {
        if (!typeOwnsStrings(inoxType)) {
            output_ << "  store " << llvmType << ' ' << value << ", ptr " << pointer << '\n';
            return;
        }
        requireStringRuntime();
        if (isStringType(inoxType)) {
            const std::string old = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  call void @__inox_str_retain(ptr " << value << ")\n";
            output_ << "  " << old << " = load ptr, ptr " << pointer << '\n';
            output_ << "  store ptr " << value << ", ptr " << pointer << '\n';
            output_ << "  call void @__inox_str_release(ptr " << old << ")\n";
            return;
        }
        const std::string copy = "%strcopy" + std::to_string(nextTemporary_++);
        output_ << "  " << copy << " = alloca " << llvmType << '\n';
        output_ << "  store " << llvmType << ' ' << value << ", ptr " << copy << '\n';
        emitRetainAt(copy, inoxType);
        emitReleaseAt(pointer, inoxType);
        output_ << "  store " << llvmType << ' ' << value << ", ptr " << pointer << '\n';
    }

    // An owned slot starts empty (null) in the entry block, so the exit and
    // cleanup releases are valid on every path, also before its declaration.
    void registerOwnedSlot(const std::string& slot, std::string_view inoxType, const std::string& llvmType,
                           bool initialize = true)
    {
        ownedSlots_.push_back(OwnedSlot{slot, std::string(inoxType), llvmType, statementDepth_});
        if (initialize) {
            output_ << "  store " << llvmType << ' ' << (llvmType == "ptr" ? "null" : "zeroinitializer")
                    << ", ptr " << slot << " ; inox.entry-init\n";
        }
    }

    // A call result that owns Strings (+1) is held in a statement temporary
    // and released at the end of the statement that produced it.
    void holdOwnedResult(const std::string& value, std::string_view inoxType, const std::string& llvmType)
    {
        const std::string slot = "%strtmp" + std::to_string(nextTemporary_++);
        output_ << "  " << slot << " = alloca " << llvmType << '\n';
        registerOwnedSlot(slot, inoxType, llvmType);
        emitReleaseAt(slot, inoxType);
        output_ << "  store " << llvmType << ' ' << value << ", ptr " << slot << '\n';
        pendingTemps_.push_back(ownedSlots_.back());
    }

    void emitReleaseOwnedSlots()
    {
        for (auto it = ownedSlots_.rbegin(); it != ownedSlots_.rend(); ++it) {
            emitReleaseAt(it->pointer, it->inoxType);
        }
    }

    void flushStatementTemporaries()
    {
        while (!pendingTemps_.empty() && pendingTemps_.back().depth >= statementDepth_) {
            const OwnedSlot temp = pendingTemps_.back();
            pendingTemps_.pop_back();
            emitReleaseAt(temp.pointer, temp.inoxType);
            output_ << "  store " << temp.llvmType << ' ' << (temp.llvmType == "ptr" ? "null" : "zeroinitializer")
                    << ", ptr " << temp.pointer << '\n';
        }
    }

    // Return: the result leaves the routine owned (+1).
    void emitRetainResult(const std::string& value)
    {
        if (!typeOwnsStrings(signature_.returnInoxType)) {
            return;
        }
        requireStringRuntime();
        if (isStringType(signature_.returnInoxType)) {
            output_ << "  call void @__inox_str_retain(ptr " << value << ")\n";
            return;
        }
        const std::string copy = "%strret" + std::to_string(nextTemporary_++);
        output_ << "  " << copy << " = alloca " << signature_.llvmReturnType << '\n';
        output_ << "  store " << signature_.llvmReturnType << ' ' << value << ", ptr " << copy << '\n';
        emitRetainAt(copy, signature_.returnInoxType);
    }

    // Whether this routine holds owned Strings: a declaration or assignment of
    // a String-holding type, or a call whose result holds Strings.
    bool scanOwnsStrings() const
    {
        bool owns = false;
        forEachStatement(function_.body(), [&](const ast::Statement& statement) {
            if (owns) {
                return;
            }
            if (statement.kind() == ast::AstNodeKind::VarStatement) {
                const auto& variable = static_cast<const ast::VarStatement&>(statement);
                const std::string type = !variable.typeName().empty()
                                             ? semantics_.canonicalTypeName(variable.typeName())
                                             : (variable.initializer() != nullptr ? semanticTypeName(*variable.initializer())
                                                                                  : std::string{});
                owns = typeOwnsStrings(type);
            }
            forEachExpressionOf(statement, [&](const ast::Expression& expression) {
                if (owns || expression.kind() != ast::AstNodeKind::CallExpression) {
                    if (!owns && expression.kind() == ast::AstNodeKind::BinaryExpression &&
                        static_cast<const ast::BinaryExpression&>(expression).op() == ast::BinaryOperator::Assign) {
                        owns = typeOwnsStrings(semanticTypeName(static_cast<const ast::BinaryExpression&>(expression).right()));
                    }
                    return;
                }
                owns = typeOwnsStrings(semanticTypeName(expression)) && !isMemberAccessCall(expression) &&
                       !isIndexCall(expression);
            });
        });
        if (signature_.llvmName == "main") {
            for (const StateGlobal& state : stateGlobals_) {
                owns = owns || typeOwnsStrings(state.inoxType);
            }
        }
        return owns;
    }

    // CANON-9: whether a struct type, or a struct nested in it, has a field
    // default other than zero (a String field starts as "", not null).
    bool structHasDefaults(const StructDefinition& structType) const
    {
        for (const StructFieldInfo& field : structType.fields) {
            if (field.hasDefault || isStringType(field.inoxType)) {
                return true;
            }
            if (defaultedStruct(field.inoxType) != nullptr || stringArray(field.inoxType)) {
                return true;
            }
        }
        return false;
    }

    // Whether an array type's innermost element is a String.
    static bool stringArray(std::string_view inoxType)
    {
        std::string type(inoxType);
        bool isArray = false;
        while (const auto layout = arrayLayout(type)) {
            type = layout->elementType;
            isArray = true;
        }
        return isArray && isStringType(type);
    }

    // The struct with non-zero defaults that a value of `inoxType` holds: the
    // type itself, or the innermost element of an array type; else null.
    const StructDefinition* defaultedStruct(std::string_view inoxType) const
    {
        std::string type(inoxType);
        while (const auto layout = arrayLayout(type)) {
            type = layout->elementType;
        }
        const StructDefinition* structType = findStruct(structs_, type);
        return structType != nullptr && structHasDefaults(*structType) ? structType : nullptr;
    }

    // Writes the type defaults of a struct at `pointer` (CANON-5 rule 4):
    // zero, then each field default, recursively into nested structs.
    void emitStructDefaults(const std::string& pointer, const StructDefinition& structType)
    {
        output_ << "  store " << structType.llvmName << " zeroinitializer, ptr " << pointer << '\n';
        emitStructFieldDefaults(pointer, structType);
    }

    void emitStructFieldDefaults(const std::string& pointer, const StructDefinition& structType)
    {
        for (const StructFieldInfo& field : structType.fields) {
            const StructDefinition* defaulted = field.hasDefault ? nullptr : defaultedStruct(field.inoxType);
            if (!field.hasDefault && defaulted == nullptr && !isStringType(field.inoxType) &&
                !stringArray(field.inoxType)) {
                continue;
            }
            const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << fieldPointer << " = getelementptr " << structType.llvmName << ", ptr "
                    << pointer << ", i32 0, i32 " << field.index << '\n';
            if (field.isStruct) {
                emitStructFieldDefaults(fieldPointer, *defaulted);
                continue;
            }
            if (defaulted != nullptr || stringArray(field.inoxType)) {
                // An array field of structs or Strings: every element gets
                // the defaults.
                emitArrayElementDefaults(fieldPointer, field.inoxType);
                continue;
            }
            if (isStringType(field.inoxType)) {
                // A default is an immortal literal (kept quoted by the parser),
                // so storing it needs no retain.
                const std::string& text = field.defaultValue;
                const std::string value = field.hasDefault && text.size() >= 2
                                              ? emitStringValue(std::string_view(text).substr(1, text.size() - 2))
                                              : emptyStringValue();
                output_ << "  store ptr " << value << ", ptr " << fieldPointer << '\n';
                continue;
            }
            output_ << "  store " << field.llvmType << ' '
                    << llvmDefaultLiteral(field.defaultValue, field.llvmType) << ", ptr " << fieldPointer
                    << '\n';
        }
    }

    // An array of structs with defaults starts with every element at its
    // struct's defaults. The array was zeroed; nested arrays are contiguous,
    // so the elements are walked as one flat run.
    void emitArrayElementDefaults(const std::string& pointer, std::string_view arrayType)
    {
        std::optional<ArrayLayout> layout = arrayLayout(arrayType);
        std::int64_t count = 1;
        while (layout) {
            count *= layout->high - layout->low + 1;
            const std::string element = layout->elementType;
            if (const auto inner = arrayLayout(element)) {
                layout = inner;
                continue;
            }
            const StructDefinition* structType = findStruct(structs_, element);
            if (!isStringType(element) && (structType == nullptr || !structHasDefaults(*structType))) {
                return;
            }
            const std::string label = std::to_string(nextLabel_++);
            const std::string counter = "%elemdefault.i" + label;
            output_ << "  " << counter << " = alloca i64\n";
            output_ << "  store i64 0, ptr " << counter << '\n';
            output_ << "  br label %elemdefault.cond" << label << "\n\n";
            output_ << "elemdefault.cond" << label << ":\n";
            const std::string index = "%tmp" + std::to_string(nextTemporary_++);
            const std::string more = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << index << " = load i64, ptr " << counter << '\n';
            output_ << "  " << more << " = icmp slt i64 " << index << ", " << count << '\n';
            output_ << "  br i1 " << more << ", label %elemdefault.body" << label
                    << ", label %elemdefault.end" << label << "\n\n";
            output_ << "elemdefault.body" << label << ":\n";
            const std::string elementPointer = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << elementPointer << " = getelementptr inbounds "
                    << (structType != nullptr ? structType->llvmName : std::string("ptr"))
                    << ", ptr " << pointer << ", i64 " << index << '\n';
            if (structType != nullptr) {
                emitStructFieldDefaults(elementPointer, *structType);
            } else {
                output_ << "  store ptr " << emptyStringValue() << ", ptr " << elementPointer << '\n';
            }
            const std::string next = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << next << " = add i64 " << index << ", 1\n";
            output_ << "  store i64 " << next << ", ptr " << counter << '\n';
            output_ << "  br label %elemdefault.cond" << label << "\n\n";
            output_ << "elemdefault.end" << label << ":\n";
            return;
        }
    }

    // CANON-9 named construction `T(Field := Value, ...)`: the type defaults,
    // then each named value in source order.
    std::string emitConstruction(const ast::CallExpression& call, const StructDefinition& structType)
    {
        const std::string slot = "%construct" + std::to_string(nextTemporary_++);
        output_ << "  " << slot << " = alloca " << structType.llvmName << '\n';
        emitStructDefaults(slot, structType);
        for (const auto& argument : call.arguments()) {
            const auto& named = static_cast<const ast::BinaryExpression&>(*argument);
            const auto& fieldName = static_cast<const ast::IdentifierExpression&>(named.left());
            const StructFieldInfo* field = findStructField(structType, fieldName.name());
            if (field == nullptr) {
                throw CodegenError("unknown field in construction: " + fieldName.name());
            }
            const std::string value = emitExpression(named.right());
            const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << fieldPointer << " = getelementptr " << structType.llvmName << ", ptr " << slot
                    << ", i32 0, i32 " << field->index << '\n';
            output_ << "  store " << field->llvmType << ' ' << value << ", ptr " << fieldPointer << '\n';
        }
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = load " << structType.llvmName << ", ptr " << slot << '\n';
        return result;
    }

    // The storage of a place: a variable, a field of a place or an element.
    // Any other value is copied into a temporary so its fields can be read.
    struct Place {
        std::string pointer;
        std::string inoxType;
    };

    Place emitPlace(const ast::Expression& expression)
    {
        if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto local = locals_.find(normalize(static_cast<const ast::IdentifierExpression&>(expression).name()));
            if (local != locals_.end()) {
                return Place{local->second.slot, local->second.inoxType};
            }
        }
        if (isIndexCall(expression)) {
            const ElementAddress element = emitElementAddress(static_cast<const ast::CallExpression&>(expression));
            return Place{element.pointer, element.elementType};
        }
        if (isMemberAccessCall(expression)) {
            const auto& member = static_cast<const ast::CallExpression&>(expression);
            if (const auto field = memberField(member)) {
                const Place base = emitPlace(*member.arguments()[0]);
                const std::string pointer = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << pointer << " = getelementptr " << field->first->llvmName << ", ptr "
                        << base.pointer << ", i32 0, i32 " << field->second->index << '\n';
                return Place{pointer, field->second->inoxType};
            }
        }
        const std::string type = semanticTypeName(expression);
        const std::string llvmType = llvmTypeForInoxType(type, structs_);
        if (llvmType.empty()) {
            throw CodegenUnsupported("this value has no storage in the LLVM backend");
        }
        const std::string value = emitExpression(expression);
        const std::string slot = "%placetmp" + std::to_string(nextTemporary_++);
        output_ << "  " << slot << " = alloca " << llvmType << '\n';
        output_ << "  store " << llvmType << ' ' << value << ", ptr " << slot << '\n';
        return Place{slot, type};
    }

    // The struct and field a `__member(Base, Name)` names, when Base is a
    // struct value and Name one of its fields (otherwise it is a method).
    std::optional<std::pair<const StructDefinition*, const StructFieldInfo*>>
    memberField(const ast::CallExpression& member) const
    {
        if (member.arguments().size() != 2 ||
            member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
            return std::nullopt;
        }
        const StructDefinition* structType = findStruct(structs_, placeTypeName(*member.arguments()[0]));
        if (structType == nullptr) {
            return std::nullopt;
        }
        const auto& fieldName = static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
        const StructFieldInfo* field = findStructField(*structType, fieldName.name());
        if (field == nullptr) {
            return std::nullopt;
        }
        return std::make_pair(structType, field);
    }

    // The Inox type of a place or value: a local's declared type, else the
    // semantic type.
    std::string placeTypeName(const ast::Expression& expression) const
    {
        if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto local = locals_.find(normalize(static_cast<const ast::IdentifierExpression&>(expression).name()));
            if (local != locals_.end()) {
                return local->second.inoxType;
            }
        }
        if (isMemberAccessCall(expression)) {
            if (const auto field = memberField(static_cast<const ast::CallExpression&>(expression))) {
                return field->second->inoxType;
            }
        }
        return semanticTypeName(expression);
    }

    FieldAddress emitMemberAddress(const ast::CallExpression& call)
    {
        if (!memberField(call)) {
            // Not a field: the caller treats it as a zero-argument method.
            throw CodegenError("not a struct field");
        }
        const Place place = emitPlace(call);
        const auto field = memberField(call);
        return FieldAddress{place.pointer, field->second->llvmType};
    }

    static bool isMemberAccessCall(const ast::Expression& expression)
    {
        if (expression.kind() != ast::AstNodeKind::CallExpression) {
            return false;
        }
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
            return false;
        }
        const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
        return equalsIgnoreCase(callee.name(), "__member");
    }

    struct MethodCallTarget {
        const FunctionSignature* signature = nullptr;
        std::string receiverPointer;
    };

    MethodCallTarget resolveMethodCall(const ast::CallExpression& call)
    {
        if (!isMemberAccessCall(call.callee())) {
            throw CodegenError("LLVM emission expected a method call");
        }

        const auto& member = static_cast<const ast::CallExpression&>(call.callee());
        if (member.arguments().size() != 2 ||
            member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenUnsupported("LLVM emission currently supports only local method calls");
        }

        const auto& method =
            static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
        const std::string receiverType = placeTypeName(*member.arguments()[0]);
        if (findStruct(structs_, receiverType) == nullptr) {
            throw CodegenUnsupported("LLVM emission method receiver is not a struct");
        }

        const std::string qualifiedName = receiverType + "." + method.name();
        const auto signature = signatures_.find(normalize(qualifiedName));
        if (signature == signatures_.end()) {
            throw CodegenUnsupported("unknown method for LLVM emission: " + qualifiedName);
        }
        if (signature->second.parameters.empty() ||
            signature->second.parameters.front().llvmType != "ptr") {
            throw CodegenUnsupported("LLVM method emission requires an explicit struct receiver parameter");
        }
        if (call.arguments().size() + 1 != signature->second.parameters.size()) {
            throw CodegenUnsupported("unsupported method argument count: " + qualifiedName);
        }

        return MethodCallTarget{&signature->second, emitPlace(*member.arguments()[0]).pointer};
    }

    MethodCallTarget resolveNoArgumentMethodAccess(const ast::CallExpression& member)
    {
        if (!isMemberAccessCall(member)) {
            throw CodegenError("LLVM emission expected member access");
        }
        if (member.arguments().size() != 2 ||
            member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenUnsupported("LLVM emission currently supports only local zero-argument method calls");
        }

        const auto& method =
            static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
        const std::string receiverType = placeTypeName(*member.arguments()[0]);
        if (findStruct(structs_, receiverType) == nullptr) {
            throw CodegenUnsupported("LLVM emission method receiver is not a struct");
        }

        const std::string qualifiedName = receiverType + "." + method.name();
        const auto signature = signatures_.find(normalize(qualifiedName));
        if (signature == signatures_.end()) {
            throw CodegenUnsupported("unknown field or zero-argument method for LLVM emission: " + qualifiedName);
        }
        if (signature->second.parameters.size() != 1 ||
            signature->second.parameters.front().llvmType != "ptr") {
            throw CodegenUnsupported("LLVM zero-argument method emission requires only an explicit struct receiver parameter");
        }
        return MethodCallTarget{&signature->second, emitPlace(*member.arguments()[0]).pointer};
    }

    std::string emitNoArgumentMethodCall(const ast::CallExpression& member, bool requireValue)
    {
        const MethodCallTarget target = resolveNoArgumentMethodAccess(member);
        const FunctionSignature& signature = *target.signature;

        return emitUserCall(signature, {target.receiverPointer}, requireValue);
    }

    std::string emitMethodCall(const ast::CallExpression& call, bool requireValue)
    {
        const MethodCallTarget target = resolveMethodCall(call);
        const FunctionSignature& signature = *target.signature;

        std::vector<std::string> arguments;
        arguments.reserve(call.arguments().size() + 1);
        arguments.push_back(target.receiverPointer);
        for (std::string& value : emitArguments(signature, call.arguments(), 1)) {
            arguments.push_back(std::move(value));
        }

        return emitUserCall(signature, arguments, requireValue);
    }

    void emitLocalAssignment(const ast::Expression& expression)
    {
        if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
            throw CodegenUnsupported(
                "LLVM emission currently supports only simple local assignments");
        }

        const auto& assignment = static_cast<const ast::BinaryExpression&>(expression);
        if (assignment.op() != ast::BinaryOperator::Assign) {
            throw CodegenUnsupported(
                "LLVM emission currently supports only simple local assignments");
        }

        if (assignment.left().kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto& identifier =
                static_cast<const ast::IdentifierExpression&>(assignment.left());
            const auto local = locals_.find(normalize(identifier.name()));
            if (local == locals_.end()) {
                // CANON-5 / A6 / A7: first appearance of `Name := Expr` is an
                // inline declaration with an inferred type. The semantic layer
                // already validated this; here we allocate a fresh local and
                // store the initializer, mirroring emitLocalVariable.
                emitLocalVariable(identifier.name(), assignment.right());
                return;
            }

            const std::string value = emitExpression(assignment.right());
            emitOwningStore(local->second.slot, local->second.inoxType, local->second.llvmType, value);
            return;
        }

        if (isIndexCall(assignment.left())) {
            const ElementAddress element = emitElementAddress(
                static_cast<const ast::CallExpression&>(assignment.left()));
            const std::string value = emitExpression(assignment.right());
            const std::string stored = narrowForStorage(value, element);
            emitOwningStore(element.pointer, element.elementType, element.storage, stored);
            return;
        }

        if (isMemberAccessCall(assignment.left())) {
            const auto& member = static_cast<const ast::CallExpression&>(assignment.left());
            const FieldAddress field = emitMemberAddress(member);
            const std::string value = emitExpression(assignment.right());
            const auto info = memberField(member);
            emitOwningStore(field.pointer, info ? info->second->inoxType : std::string{}, field.llvmType, value);
            return;
        }

        throw CodegenUnsupported(
            "LLVM emission currently supports only local variable or field assignments");
    }

    // P-C stage 1: a module Const is read from the semantic result, already
    // resolved, never re-parsed here.
    const semantic::ConstantValue* resolvedConstant(const ast::IdentifierExpression& identifier) const
    {
        const semantic::Symbol* symbol = semantics_.symbolOf(identifier);
        if (symbol == nullptr || symbol->kind != semantic::SymbolKind::Constant) {
            return nullptr;
        }
        return semantics_.constantValueOf(*symbol);
    }

    std::string expressionLlvmType(const ast::Expression& expression) const
    {
        switch (expression.kind()) {
        case ast::AstNodeKind::LiteralExpression: {
            const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
            switch (literal.literalKind()) {
            case ast::LiteralKind::Integer:
                return "i64";
            case ast::LiteralKind::Float:
                return "double";
            case ast::LiteralKind::Boolean:
                return "i1";
            case ast::LiteralKind::String:
                return "ptr";
            case ast::LiteralKind::Char:
                return "i32";
            }
            break;
        }
        case ast::AstNodeKind::IdentifierExpression: {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
            const std::string normalizedName = normalize(identifier.name());
            const auto local = locals_.find(normalizedName);
            if (local != locals_.end()) {
                return local->second.llvmType;
            }
            const auto parameter = parameterTypes_.find(normalizedName);
            if (parameter != parameterTypes_.end()) {
                return parameter->second;
            }
            const auto signature = signatures_.find(normalizedName);
            if (signature != signatures_.end()) {
                return signature->second.llvmReturnType;
            }
            if (const semantic::ConstantValue* constant = resolvedConstant(identifier)) {
                return constant->kind == semantic::ConstantValue::Kind::Boolean ? "i1" : "i64";
            }
            break;
        }
        case ast::AstNodeKind::UnaryExpression: {
            const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
            if (unary.op() == ast::UnaryOperator::Not) {
                return "i1";
            }
            return expressionLlvmType(unary.operand());
        }
        case ast::AstNodeKind::BinaryExpression: {
            const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
            if (llvmComparisonPredicate(binary.op()).size() != 0 || fcmpPredicate(binary.op()).size() != 0) {
                return "i1";
            }
            if (llvmBooleanOperation(binary.op()).size() != 0) {
                return "i1";
            }
            return expressionLlvmType(binary.left());
        }
        case ast::AstNodeKind::ConditionalExpression:
            // ADR-0013: semantic analysis guarantees compatible branch types.
            return expressionLlvmType(
                static_cast<const ast::ConditionalExpression&>(expression).thenValue());
        case ast::AstNodeKind::CallExpression: {
            const auto& call = static_cast<const ast::CallExpression&>(expression);
            if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
                const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
                if (equalsIgnoreCase(callee.name(), "Abs") && call.arguments().size() == 1) {
                    return expressionLlvmType(*call.arguments().front());
                }
                if (isMathBuiltin(callee.name())) {
                    return "double";
                }
                if (isNaturalConversion(call)) {
                    return "i64";
                }
                if (callee.name() == "__index" || callee.name() == "__slice") {
                    return valueLlvmType(semanticTypeName(expression));
                }
                if (const StructDefinition* structType = findStruct(structs_, callee.name())) {
                    return structType->llvmName;
                }
                if (equalsIgnoreCase(callee.name(), "__member")) {
                    if (const auto field = memberField(call)) {
                        return field->second->llvmType;
                    }
                }
                if (arrayBoundCall(call)) {
                    return "i64";
                }
                const auto signature = signatures_.find(normalize(callee.name()));
                if (signature != signatures_.end()) {
                    return signature->second.llvmReturnType;
                }
            }
            if (isMemberAccessCall(call.callee())) {
                const auto& member = static_cast<const ast::CallExpression&>(call.callee());
                if (member.arguments().size() == 2 && member.arguments()[0]) {
                    const std::string receiverType = placeTypeName(*member.arguments()[0]);
                    if (member.arguments()[1]->kind() == ast::AstNodeKind::IdentifierExpression) {
                        const auto& method = static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
                        const auto signature = signatures_.find(normalize(receiverType + "." + method.name()));
                        if (signature != signatures_.end()) {
                            return signature->second.llvmReturnType;
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
        }
        return "i64";
    }

    // Natural(X), Integer(X) or Int64(X) applied to one i64 value.
    bool isNaturalConversion(const ast::CallExpression& call) const
    {
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 1) {
            return false;
        }
        const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
        if (!equalsIgnoreCase(callee.name(), "Natural") && !equalsIgnoreCase(callee.name(), "Integer") &&
            !equalsIgnoreCase(callee.name(), "Int64") && !narrowIntegerRange(callee.name())) {
            return false;
        }
        return expressionLlvmType(*call.arguments().front()) == "i64";
    }

    // ADR-0013: `if C then A else B`. Only the chosen branch is evaluated, so
    // this is control flow, never an LLVM `select`. A branch may end in a block
    // other than the one it started in (an `invoke` continuation, a nested
    // conditional's merge), so each branch jumps to its own tail block and the
    // phi names the tails, whose predecessors are known.
    std::string emitConditional(const ast::ConditionalExpression& expression)
    {
        const std::string label = std::to_string(nextLabel_++);
        const std::string type = expressionLlvmType(expression);
        const std::string condition = emitExpression(expression.condition());
        output_ << "  br i1 " << condition << ", label %condthen" << label
                << ", label %condelse" << label << "\n\n";

        output_ << "condthen" << label << ":\n";
        const std::string thenValue = emitExpression(expression.thenValue());
        output_ << "  br label %condthentail" << label << "\n\n";
        output_ << "condthentail" << label << ":\n";
        output_ << "  br label %condend" << label << "\n\n";

        output_ << "condelse" << label << ":\n";
        const std::string elseValue = emitExpression(expression.elseValue());
        output_ << "  br label %condelsetail" << label << "\n\n";
        output_ << "condelsetail" << label << ":\n";
        output_ << "  br label %condend" << label << "\n\n";

        output_ << "condend" << label << ":\n";
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = phi " << type << " [ " << thenValue
                << ", %condthentail" << label << " ], [ " << elseValue
                << ", %condelsetail" << label << " ]\n";
        return result;
    }

    // ADR-0015: an Integer expression stored into a Natural carries an
    // implicit range check, recorded by semantic analysis.
    static bool isIndexCall(const ast::Expression& expression)
    {
        if (expression.kind() != ast::AstNodeKind::CallExpression) {
            return false;
        }
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        return call.callee().kind() == ast::AstNodeKind::IdentifierExpression &&
               static_cast<const ast::IdentifierExpression&>(call.callee()).name() == "__index" &&
               call.arguments().size() == 2;
    }

    // The LLVM type of a value of an Inox type in SSA form: arrays are
    // aggregates; every integer is i64.
    std::string valueLlvmType(const std::string& inoxType) const
    {
        if (const auto layout = arrayLayout(inoxType)) {
            return layout->llvmType;
        }
        if (support::isSliceTypeName(inoxType)) {
            return std::string(kSliceLlvmType);
        }
        if (const StructDefinition* structType = findStruct(structs_, inoxType)) {
            return structType->llvmName;
        }
        const std::string scalar = llvmTypeForScalar(inoxType);
        return scalar.empty() ? std::string("i64") : scalar;
    }

    // Low(A), High(A) and Length(A) of a fixed array are constants (CANON-13).
    std::optional<std::int64_t> arrayBoundCall(const ast::CallExpression& call) const
    {
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 1) {
            return std::nullopt;
        }
        const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
        const bool low = equalsIgnoreCase(callee.name(), "Low");
        const bool high = equalsIgnoreCase(callee.name(), "High");
        if (!low && !high && !equalsIgnoreCase(callee.name(), "Length")) {
            return std::nullopt;
        }
        const auto layout = arrayLayout(semanticTypeName(*call.arguments()[0]));
        if (!layout) {
            return std::nullopt;
        }
        return low ? layout->low : high ? layout->high : layout->high - layout->low + 1;
    }

    struct ElementAddress {
        std::string pointer;
        std::string storage;      // LLVM storage type of the element
        std::string elementType;  // Inox element type
    };

    // The address of an array value: a local's slot, an element of an outer
    // array, or a temporary holding any other array value.
    std::string emitArrayBaseAddress(const ast::Expression& base, const ArrayLayout& layout)
    {
        if (base.kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto local = locals_.find(normalize(static_cast<const ast::IdentifierExpression&>(base).name()));
            if (local != locals_.end()) {
                return local->second.slot;
            }
        }
        if (isIndexCall(base)) {
            return emitElementAddress(static_cast<const ast::CallExpression&>(base)).pointer;
        }
        if (isMemberAccessCall(base) && memberField(static_cast<const ast::CallExpression&>(base))) {
            return emitPlace(base).pointer;
        }
        const std::string value = emitExpression(base);
        const std::string slot = "%arraytmp" + std::to_string(nextTemporary_++);
        output_ << "  " << slot << " = alloca " << layout.llvmType << '\n';
        output_ << "  store " << layout.llvmType << ' ' << value << ", ptr " << slot << '\n';
        return slot;
    }

    // CANON-13: every index is checked against Low..High (fault kind 9).
    struct SliceParts {
        std::string data;
        std::string low;
        std::string high;
    };

    SliceParts extractSlice(const std::string& value)
    {
        SliceParts parts;
        parts.data = "%tmp" + std::to_string(nextTemporary_++);
        parts.low = "%tmp" + std::to_string(nextTemporary_++);
        parts.high = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << parts.data << " = extractvalue " << kSliceLlvmType << ' ' << value << ", 0\n";
        output_ << "  " << parts.low << " = extractvalue " << kSliceLlvmType << ' ' << value << ", 1\n";
        output_ << "  " << parts.high << " = extractvalue " << kSliceLlvmType << ' ' << value << ", 2\n";
        return parts;
    }

    // The base of a slice expression: its data pointer and bounds, and the
    // LLVM storage of one element.
    SliceParts emitSliceSource(const ast::Expression& base, std::string& storage, std::string& elementType)
    {
        const std::string baseType = semanticTypeName(base);
        if (support::isSliceTypeName(baseType)) {
            elementType = support::sliceElementTypeName(baseType);
            storage = elementStorageType(elementType);
            return extractSlice(emitExpression(base));
        }
        const auto layout = arrayLayout(baseType);
        if (!layout) {
            throw CodegenUnsupported("slicing of this value is not lowered yet");
        }
        elementType = layout->elementType;
        storage = layout->elementStorage;
        const std::string pointer = emitArrayBaseAddress(base, *layout);
        const std::string data = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << data << " = getelementptr inbounds " << layout->llvmType << ", ptr " << pointer
                << ", i64 0, i64 0\n";
        return SliceParts{data, std::to_string(layout->low), std::to_string(layout->high)};
    }

    // ADR-0017: `B[L..H]` checks Low(B) <= L <= H <= High(B) (a slice is never
    // empty, like an array), then points at element L.
    std::string emitSlice(const ast::CallExpression& call)
    {
        std::string storage;
        std::string elementType;
        const SliceParts source = emitSliceSource(*call.arguments()[0], storage, elementType);
        const auto bound = [&](const ast::Expression& expression, const std::string& fallback) {
            if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
                const std::string& name = static_cast<const ast::IdentifierExpression&>(expression).name();
                if (name == "__low" || name == "__high") {
                    return fallback;
                }
            }
            return emitExpression(expression);
        };
        const std::string low = bound(*call.arguments()[1], source.low);
        const std::string high = bound(*call.arguments()[2], source.high);
        const std::string lowOk = "%tmp" + std::to_string(nextTemporary_++);
        const std::string highOk = "%tmp" + std::to_string(nextTemporary_++);
        const std::string orderOk = "%tmp" + std::to_string(nextTemporary_++);
        const std::string both = "%tmp" + std::to_string(nextTemporary_++);
        const std::string valid = "%tmp" + std::to_string(nextTemporary_++);
        const std::string flag = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << lowOk << " = icmp sge i64 " << low << ", " << source.low << '\n';
        output_ << "  " << highOk << " = icmp sle i64 " << high << ", " << source.high << '\n';
        output_ << "  " << orderOk << " = icmp sle i64 " << low << ", " << high << '\n';
        output_ << "  " << both << " = and i1 " << lowOk << ", " << highOk << '\n';
        output_ << "  " << valid << " = and i1 " << both << ", " << orderOk << '\n';
        output_ << "  " << flag << " = zext i1 " << valid << " to i64\n";
        (void)emitRangeCheck(flag, IntegerRange{1, 1}, 10);
        const std::string offset = "%tmp" + std::to_string(nextTemporary_++);
        const std::string data = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << offset << " = sub i64 " << low << ", " << source.low << '\n';
        output_ << "  " << data << " = getelementptr inbounds " << storage << ", ptr " << source.data
                << ", i64 " << offset << '\n';
        const std::string withData = "%tmp" + std::to_string(nextTemporary_++);
        const std::string withLow = "%tmp" + std::to_string(nextTemporary_++);
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << withData << " = insertvalue " << kSliceLlvmType << " undef, ptr " << data << ", 0\n";
        output_ << "  " << withLow << " = insertvalue " << kSliceLlvmType << ' ' << withData << ", i64 " << low
                << ", 1\n";
        output_ << "  " << result << " = insertvalue " << kSliceLlvmType << ' ' << withLow << ", i64 " << high
                << ", 2\n";
        return result;
    }

    ElementAddress emitElementAddress(const ast::CallExpression& call)
    {
        const ast::Expression& base = *call.arguments()[0];
        if (support::isSliceTypeName(semanticTypeName(base))) {
            std::string storage;
            std::string elementType;
            const SliceParts slice = emitSliceSource(base, storage, elementType);
            const std::string index = emitExpression(*call.arguments()[1]);
            const std::string checked = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << checked << " = call i64 @__inox_range_i64(i64 " << index << ", i64 "
                    << slice.low << ", i64 " << slice.high << ", i32 9)\n";
            const std::string offset = "%tmp" + std::to_string(nextTemporary_++);
            const std::string pointer = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << offset << " = sub i64 " << checked << ", " << slice.low << '\n';
            output_ << "  " << pointer << " = getelementptr inbounds " << storage << ", ptr " << slice.data
                    << ", i64 " << offset << '\n';
            return ElementAddress{pointer, storage, elementType};
        }
        const auto layout = arrayLayout(semanticTypeName(base));
        if (!layout) {
            throw CodegenUnsupported("indexing of this value is not lowered yet");
        }
        const std::string basePointer = emitArrayBaseAddress(base, *layout);
        const std::string index = emitExpression(*call.arguments()[1]);
        const std::string checked = emitRangeCheck(index, IntegerRange{layout->low, layout->high}, 9);
        std::string offset = checked;
        if (layout->low != 0) {
            offset = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << offset << " = sub i64 " << checked << ", " << layout->low << '\n';
        }
        const std::string pointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << pointer << " = getelementptr inbounds " << layout->llvmType << ", ptr "
                << basePointer << ", i64 0, i64 " << offset << '\n';
        return ElementAddress{pointer, layout->elementStorage, layout->elementType};
    }

    std::string loadElement(const ElementAddress& element)
    {
        const std::string loaded = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << loaded << " = load " << element.storage << ", ptr " << element.pointer << '\n';
        const auto range = narrowIntegerRange(element.elementType);
        if (!range) {
            return loaded;
        }
        const std::string widened = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << widened << " = " << (range->low < 0 ? "sext " : "zext ") << element.storage
                << ' ' << loaded << " to i64\n";
        return widened;
    }

    // A narrow integer is already in range; storing it keeps its low bits.
    std::string narrowForStorage(const std::string& value, const ElementAddress& element)
    {
        if (!narrowIntegerRange(element.elementType)) {
            return value;
        }
        const std::string truncated = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << truncated << " = trunc i64 " << value << " to " << element.storage << '\n';
        return truncated;
    }

    // The semantic type name of an expression, or empty.
    std::string semanticTypeName(const ast::Expression& expression) const
    {
        const semantic::ResolvedType* type = semantics_.typeOf(expression);
        return type != nullptr ? type->name : std::string{};
    }

    // An arithmetic result whose semantic type is a narrow fixed-width integer
    // must stay in that type's range (CANON-8: overflow is an error).
    static bool isRangeCheckedArithmetic(const ast::Expression& expression)
    {
        if (expression.kind() == ast::AstNodeKind::UnaryExpression) {
            return static_cast<const ast::UnaryExpression&>(expression).op() == ast::UnaryOperator::Minus;
        }
        if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
            return false;
        }
        switch (static_cast<const ast::BinaryExpression&>(expression).op()) {
        case ast::BinaryOperator::Add:
        case ast::BinaryOperator::Subtract:
        case ast::BinaryOperator::Multiply:
        case ast::BinaryOperator::Power:
        case ast::BinaryOperator::IntegerDivide:
        case ast::BinaryOperator::Modulo:
        case ast::BinaryOperator::ShiftLeft:
        case ast::BinaryOperator::ShiftRight:
        case ast::BinaryOperator::BitAnd:
        case ast::BinaryOperator::BitOr:
        case ast::BinaryOperator::BitXor:
            return true;
        default:
            return false;
        }
    }

    std::string emitRangeCheck(const std::string& value, IntegerRange range, int faultKind)
    {
        const std::string checked = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << checked << " = call i64 @__inox_range_i64(i64 " << value << ", i64 "
                << range.low << ", i64 " << range.high << ", i32 " << faultKind << ")\n";
        return checked;
    }

    std::string emitExpression(const ast::Expression& expression)
    {
        std::string value = emitExpressionValue(expression);
        if (isRangeCheckedArithmetic(expression)) {
            if (const auto range = narrowIntegerRange(semanticTypeName(expression))) {
                value = emitRangeCheck(value, *range, 1);
            }
        }
        if (!semantics_.needsNaturalCheck(expression)) {
            return value;
        }
        const std::string checked = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << checked << " = call i64 @__inox_natural_i64(i64 " << value << ")\n";
        return checked;
    }

    std::string emitExpressionValue(const ast::Expression& expression)
    {
        switch (expression.kind()) {
        case ast::AstNodeKind::LiteralExpression: {
            const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
            if (literal.literalKind() == ast::LiteralKind::Integer) {
                return llvmIntegerLiteral(literal.value());
            }
            if (literal.literalKind() == ast::LiteralKind::Float) {
                return std::string(literal.value());
            }
            if (literal.literalKind() == ast::LiteralKind::Boolean) {
                return equalsIgnoreCase(literal.value(), "true") ? "1" : "0";
            }
            if (literal.literalKind() == ast::LiteralKind::String) {
                return emitStringValue(literal.value());
            }
            break;
        }
        case ast::AstNodeKind::IdentifierExpression: {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
            const std::string normalizedName = normalize(identifier.name());
            const auto local = locals_.find(normalizedName);
            if (local != locals_.end()) {
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                if (local->second.llvmType.empty()) {
                    throw CodegenError("LLVM emission found local value with unknown LLVM type");
                }
                output_ << "  " << result << " = load " << local->second.llvmType
                        << ", ptr " << local->second.slot << '\n';
                return result;
            }
            const auto parameter = parameters_.find(normalizedName);
            if (parameter != parameters_.end()) {
                return parameter->second;
            }
            const auto signature = signatures_.find(normalizedName);
            if (signature != signatures_.end()) {
                if (!signature->second.parameters.empty()) {
                    throw CodegenError("function requires arguments; use parentheses only when passing arguments: " + identifier.name());
                }
                if (signature->second.llvmReturnType == "void") {
                    throw CodegenError("void function call cannot be used as an expression");
                }
                // Through emitUserCall: an `invoke` inside try (the call may
                // raise) and the ownership of a String result.
                return emitUserCall(signature->second, {}, true);
            }
            if (const semantic::ConstantValue* constant = resolvedConstant(identifier)) {
                if (constant->kind == semantic::ConstantValue::Kind::Boolean) {
                    return constant->boolean ? "1" : "0";
                }
                return std::to_string(constant->integer);
            }
            break;
        }
        case ast::AstNodeKind::BinaryExpression: {
            const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
            const std::string leftType = expressionLlvmType(binary.left());
            const std::string rightType = expressionLlvmType(binary.right());
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            const std::string left = emitExpression(binary.left());
            std::string right = emitExpression(binary.right());

            // CANON-8: the shift count of a fixed-width type is 0..width-1.
            if (binary.op() == ast::BinaryOperator::ShiftLeft ||
                binary.op() == ast::BinaryOperator::ShiftRight) {
                if (const auto range = narrowIntegerRange(semanticTypeName(expression))) {
                    int bits = 0;
                    for (std::int64_t span = range->high; span > 0; span >>= 1) {
                        ++bits;
                    }
                    if (range->low < 0) {
                        ++bits;
                    }
                    right = emitRangeCheck(right, IntegerRange{0, bits - 1}, 3);
                }
            }

            // CANON-14: `=` and `#` compare the UTF-8 bytes of two Strings.
            if (isStringType(semanticTypeName(binary.left())) &&
                (binary.op() == ast::BinaryOperator::Equal || binary.op() == ast::BinaryOperator::NotEqual)) {
                requireModuleLine("declare zeroext i1 @__inox_str_equal(ptr, ptr) nounwind");
                const std::string equal = binary.op() == ast::BinaryOperator::Equal
                                              ? result
                                              : "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << equal << " = call zeroext i1 @__inox_str_equal(ptr " << left << ", ptr " << right
                        << ")\n";
                if (binary.op() == ast::BinaryOperator::NotEqual) {
                    output_ << "  " << result << " = xor i1 " << equal << ", true\n";
                }
                return result;
            }
            // CANON-14 (v3.56): `<` `>` `<=` `>=` order Strings byte by byte.
            if (isStringType(semanticTypeName(binary.left()))) {
                if (const std::string predicate = llvmComparisonPredicate(binary.op()); !predicate.empty()) {
                    requireModuleLine("declare i32 @__inox_str_compare(ptr, ptr) nounwind");
                    const std::string order = "%tmp" + std::to_string(nextTemporary_++);
                    output_ << "  " << order << " = call i32 @__inox_str_compare(ptr " << left << ", ptr " << right
                            << ")\n";
                    output_ << "  " << result << " = icmp " << predicate << " i32 " << order << ", 0\n";
                    return result;
                }
            }

            if (isFloatLlvmType(leftType) || isFloatLlvmType(rightType)) {
                if (const std::string predicate = fcmpPredicate(binary.op()); !predicate.empty()) {
                    output_ << "  " << result << " = fcmp " << predicate << ' ' << leftType
                            << ' ' << left << ", " << right << '\n';
                    return result;
                }
                if (binary.op() == ast::BinaryOperator::Power) {
                    output_ << "  " << result << " = call double @llvm.pow.f64(double "
                            << left << ", double " << right << ")\n";
                    return result;
                }
                output_ << "  " << result << " = " << llvmFloatOperation(binary.op()) << ' '
                        << leftType << ' ' << left << ", " << right << '\n';
                return result;
            }

            if (const std::string predicate = llvmComparisonPredicate(binary.op());
                !predicate.empty()) {
                // Every integer type is carried as i64 (signed compare). Bool
                // is i1 and ordered false < true (v3.57): compared unsigned,
                // since a signed i1 reads true as -1.
                const bool isBool = leftType == "i1";
                std::string boolPredicate = predicate;
                if (isBool && boolPredicate.front() == 's') {
                    boolPredicate.front() = 'u';
                }
                output_ << "  " << result << " = icmp " << (isBool ? boolPredicate : predicate) << ' '
                        << (isBool ? "i1" : "i64") << ' ' << left << ", " << right << '\n';
            } else if (const std::string operation = llvmBooleanOperation(binary.op());
                       !operation.empty()) {
                output_ << "  " << result << " = " << operation << " i1 "
                        << left << ", " << right << '\n';
            } else if (binary.op() == ast::BinaryOperator::Power) {
                output_ << "  " << result << " = call i64 @__inox_ipow_i64(i64 "
                        << left << ", i64 " << right << ")\n";
            } else if (const std::string helper = checkedIntegerHelper(binary.op()); !helper.empty()) {
                output_ << "  " << result << " = call i64 @" << helper << "(i64 "
                        << left << ", i64 " << right << ")\n";
            } else {
                output_ << "  " << result << " = " << llvmOperation(binary.op()) << " i64 "
                        << left << ", " << right << '\n';
            }
            return result;
        }
        case ast::AstNodeKind::UnaryExpression: {
            const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
            if (unary.op() == ast::UnaryOperator::Not) {
                const std::string operand = emitExpression(unary.operand());
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << result << " = xor i1 " << operand << ", true\n";
                return result;
            }
            if (unary.op() == ast::UnaryOperator::Plus) {
                return emitExpression(unary.operand());
            }
            if (unary.op() == ast::UnaryOperator::BitNot) {
                // Two's-complement NOT in the operand's width: for an unsigned
                // narrow type that is `x xor (2^n - 1)`, which stays in range.
                const std::string operand = emitExpression(unary.operand());
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                std::int64_t mask = -1;
                if (const auto range = narrowIntegerRange(semanticTypeName(unary.operand()));
                    range && range->low == 0) {
                    mask = range->high;
                }
                output_ << "  " << result << " = xor i64 " << operand << ", " << mask << '\n';
                return result;
            }
            if (unary.op() == ast::UnaryOperator::Minus) {
                const std::string operandType = expressionLlvmType(unary.operand());
                if (unary.operand().kind() == ast::AstNodeKind::LiteralExpression) {
                    const auto& literal = static_cast<const ast::LiteralExpression&>(unary.operand());
                    if (literal.literalKind() == ast::LiteralKind::Integer) {
                        return "-" + llvmIntegerLiteral(literal.value());
                    }
                    if (literal.literalKind() == ast::LiteralKind::Float) {
                        return "-" + std::string(literal.value());
                    }
                }
                const std::string operand = emitExpression(unary.operand());
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                if (isFloatLlvmType(operandType)) {
                    output_ << "  " << result << " = fsub " << operandType << " -0.0, " << operand << '\n';
                } else {
                    output_ << "  " << result << " = call i64 @__inox_neg_i64(i64 " << operand << ")\n";
                }
                return result;
            }
            break;
        }
        case ast::AstNodeKind::ConditionalExpression:
            return emitConditional(static_cast<const ast::ConditionalExpression&>(expression));
        case ast::AstNodeKind::CallExpression: {
            const auto& call = static_cast<const ast::CallExpression&>(expression);
            if (isMemberAccessCall(call.callee())) {
                return emitMethodCall(call, true);
            }
            if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
                break;
            }

            const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
            if (callee.name() == "__index") {
                const ElementAddress element = emitElementAddress(call);
                return loadElement(element);
            }
            if (callee.name() == "__slice") {
                return emitSlice(call);
            }
            if (const StructDefinition* structType = findStruct(structs_, callee.name())) {
                return emitConstruction(call, *structType);
            }
            if (call.arguments().size() == 1 &&
                support::isSliceTypeName(semanticTypeName(*call.arguments()[0])) &&
                (equalsIgnoreCase(callee.name(), "Low") || equalsIgnoreCase(callee.name(), "High") ||
                 equalsIgnoreCase(callee.name(), "Length"))) {
                const SliceParts slice = extractSlice(emitExpression(*call.arguments()[0]));
                if (equalsIgnoreCase(callee.name(), "Low")) {
                    return slice.low;
                }
                if (equalsIgnoreCase(callee.name(), "High")) {
                    return slice.high;
                }
                const std::string span = "%tmp" + std::to_string(nextTemporary_++);
                const std::string length = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << span << " = sub i64 " << slice.high << ", " << slice.low << '\n';
                output_ << "  " << length << " = add i64 " << span << ", 1\n";
                return length;
            }
            if (const auto bound = arrayBoundCall(call)) {
                return std::to_string(*bound);
            }
            if (equalsIgnoreCase(callee.name(), "__member")) {
                try {
                    const FieldAddress field = emitMemberAddress(call);
                    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                    output_ << "  " << result << " = load " << field.llvmType
                            << ", ptr " << field.pointer << '\n';
                    return result;
                } catch (const CodegenError&) {
                    return emitNoArgumentMethodCall(call, true);
                }
            }

            if (equalsIgnoreCase(callee.name(), "Abs") && call.arguments().size() == 1 &&
                expressionLlvmType(*call.arguments().front()) == "i64") {
                const std::string value = emitExpression(*call.arguments().front());
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << result << " = call i64 @__inox_abs_i64(i64 " << value << ")\n";
                return result;
            }

            if (isMathBuiltin(callee.name())) {
                return emitMathBuiltinCall(callee.name(), call.arguments());
            }

            // CANON-8: Natural(X) checks X >= 0 at run time (fault kind 7);
            // Integer(X) and Int64(X) of an integer value are the identity,
            // because Natural shares Integer's i64 representation.
            if (isNaturalConversion(call)) {
                const std::string value = emitExpression(*call.arguments().front());
                if (const auto range = narrowIntegerRange(callee.name())) {
                    return emitRangeCheck(value, *range, 8);
                }
                if (!equalsIgnoreCase(callee.name(), "Natural")) {
                    return value;
                }
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << result << " = call i64 @__inox_natural_i64(i64 " << value << ")\n";
                return result;
            }

            const auto signature = signatures_.find(normalize(callee.name()));
            if (signature == signatures_.end()) {
                break;
            }
            if (signature->second.llvmReturnType == "void") {
                throw CodegenError("void function call cannot be used as an expression");
            }
            if (call.arguments().size() != signature->second.parameters.size()) {
                throw CodegenUnsupported("unsupported call argument count in function: " + function_.name());
            }

            const std::vector<std::string> arguments = emitArguments(signature->second, call.arguments(), 0);
            return emitUserCall(signature->second, arguments, true);
        }
        default:
            break;
        }

        throw CodegenUnsupported("unsupported expression in function: " + function_.name());
    }

    std::string emitMathBuiltinCall(std::string_view name, const std::vector<ast::ExpressionPtr>& arguments)
    {
        std::vector<std::string> values;
        values.reserve(arguments.size());
        for (const auto& argument : arguments) {
            values.push_back(emitExpression(*argument));
        }
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);

        const auto unaryScale = [&](double factor) {
            output_ << "  " << result << " = fmul double " << values.at(0) << ", " << std::scientific << factor << std::defaultfloat << "\n";
            return result;
        };

        if (equalsIgnoreCase(name, "RadToDeg")) return unaryScale(57.295779513082320876798154814105);
        if (equalsIgnoreCase(name, "DegToRad")) return unaryScale(0.017453292519943295769236907684886);
        if (equalsIgnoreCase(name, "RadToGrad")) return unaryScale(63.661977236758134307553505349005);
        if (equalsIgnoreCase(name, "GradToRad")) return unaryScale(0.015707963267948966192313216916398);
        if (equalsIgnoreCase(name, "RadToCycle")) return unaryScale(0.15915494309189533576888376337251);
        if (equalsIgnoreCase(name, "CycleToRad")) return unaryScale(6.283185307179586476925286766559);

        if (equalsIgnoreCase(name, "LogN")) {
            const std::string logX = "%tmp" + std::to_string(nextTemporary_++);
            const std::string logBase = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << logX << " = call double @llvm.log.f64(double " << values.at(1) << ")\n";
            output_ << "  " << logBase << " = call double @llvm.log.f64(double " << values.at(0) << ")\n";
            output_ << "  " << result << " = fdiv double " << logX << ", " << logBase << "\n";
            return result;
        }
        if (equalsIgnoreCase(name, "Hypot3")) {
            const std::string xy = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << xy << " = call double @hypot(double " << values.at(0) << ", double " << values.at(1) << ")\n";
            output_ << "  " << result << " = call double @hypot(double " << xy << ", double " << values.at(2) << ")\n";
            return result;
        }

        if (const std::string intrinsic = mathIntrinsicName(name); !intrinsic.empty()) {
            output_ << "  " << result << " = call double @" << intrinsic << "(";
        } else if (const std::string libm = mathLibmName(name); !libm.empty()) {
            output_ << "  " << result << " = call double @" << libm << "(";
        } else {
            throw CodegenUnsupported("unsupported math builtin: " + std::string(name));
        }
        for (std::size_t index = 0; index < values.size(); ++index) {
            if (index != 0) output_ << ", ";
            output_ << "double " << values[index];
        }
        output_ << ")\n";
        return result;
    }

    std::ostringstream& output_;
    const ast::FunctionDeclaration& function_;
    const FunctionSignature& signature_;
    const FunctionSignatures& signatures_;
    const StructDefinitions& structs_;
    std::vector<std::string>& stringGlobals_;
    std::size_t& nextStringLiteral_;
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

void emitFunction(std::ostringstream& output,
                  const ast::FunctionDeclaration& function,
                  const FunctionSignature& signature,
                  const FunctionSignatures& signatures,
                  const StructDefinitions& structs,
                  std::vector<std::string>& stringGlobals,
                  std::size_t& nextStringLiteral,
                  const semantic::SemanticResult& semantics,
                  const StateGlobals& stateGlobals,
                  bool& usesStringCleanup)
{
    std::ostringstream body;
    FunctionEmitter emitter(body, function, signature, signatures, structs, stringGlobals, nextStringLiteral,
                            semantics, stateGlobals);
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
        output << " personality ptr @"
               << (usesMsvcExceptionAbi()
                       ? "__CxxFrameHandler3" : "__gxx_personality_v0");
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
    const ast::FunctionDeclaration* mainFunction = nullptr;
    FunctionSignatures signatures;
    StructDefinitions structs;
    std::ostringstream functionOutput;
    std::ostringstream output;
    std::vector<std::string> stringGlobals;
    std::size_t nextStringLiteral = 0;
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
            signatures.emplace(normalizedName, parseFunctionSignature(function, structs, semantics));
        }
    }

    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::FunctionDeclaration) {
            continue;
        }

        const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
        if (!equalsIgnoreCase(function.name(), "Main")) {
            const auto signature = signatures.find(normalize(function.name()));
            emitFunction(functionOutput, function, signature->second, signatures, structs, stringGlobals, nextStringLiteral, semantics, stateGlobals, usesStringCleanup);
        }
    }

    if (mainFunction == nullptr) {
        throw CodegenError("LLVM emission requires Main");
    }
    const FunctionSignature mainSignature{"main", "i32", {}, {}};
    emitFunction(functionOutput, *mainFunction, mainSignature, signatures, structs, stringGlobals, nextStringLiteral, semantics, stateGlobals, usesStringCleanup);

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
    for (const std::string& global : stringGlobals) {
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
        if (usesMsvcExceptionAbi()) {
            output << "declare i32 @__CxxFrameHandler3(...)\n";
        } else {
            output << "declare i32 @__gxx_personality_v0(...)\n";
        }
        output << "declare void @__inox_raise(i64) noreturn\n";
        output << "declare ptr @__inox_exception_capture(ptr) nounwind\n";
        output << "declare i64 @__inox_exception_type(ptr) nounwind\n";
        output << "declare void @__inox_exception_release(ptr) nounwind\n";
        output << "declare void @__inox_exception_rethrow(ptr) noreturn\n";
    }
    output << '\n';
    output << inputRuntimeHelpers();
    output << mathRuntimeHelpers();
    output << functionOutput.str();
    return output.str();
}

} // namespace inox::compiler::codegen
