// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: if, unless, case, loops, Return, Exit, leave, continue, until, with.

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

void FunctionEmitter::emitIfReturn(const ast::IfStatement& statement)
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

void FunctionEmitter::emitReturn(const ast::ReturnStatement& statement)
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

void FunctionEmitter::emitExit()
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

const FunctionEmitter::CleanupContext* FunctionEmitter::cleanupForLoopTransfer() const
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
void FunctionEmitter::emitReleaseHandlersLeftBy(std::size_t targetLoopDepth)
{
    for (auto it = handlerRegions_.rbegin(); it != handlerRegions_.rend(); ++it) {
        if (it->loopDepthAtEntry >= targetLoopDepth) {
            emitReleaseExceptionState(it->stateSlot);
        }
    }
}

void FunctionEmitter::emitLeaveTransfer()
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

void FunctionEmitter::emitContinueTransfer()
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

void FunctionEmitter::emitIfMerge(const ast::IfStatement& statement)
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
void FunctionEmitter::emitUnless(const ast::UnlessStatement& statement)
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
void FunctionEmitter::emitCase(const ast::CaseStatement& statement)
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

void FunctionEmitter::emitAssignmentBranch(const std::vector<ast::StatementPtr>& statements)
{
    for (const auto& statement : statements) {
        emitStatement(*statement);
    }
}

void FunctionEmitter::emitWhile(const ast::WhileStatement& statement)
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

void FunctionEmitter::emitRepeat(const ast::RepeatStatement& statement)
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

void FunctionEmitter::emitForIn(const ast::ForInStatement& statement)
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

bool FunctionEmitter::emitLoopStatements(const std::vector<ast::StatementPtr>& statements)
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
void FunctionEmitter::emitUntil(const ast::UntilStatement& statement)
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

const FunctionEmitter::LoopTargets& FunctionEmitter::currentLoopTargets() const
{
    if (loopTargets_.empty()) {
        throw CodegenError(
            "LLVM emission supports leave and continue only inside loops");
    }
    return loopTargets_.back();
}

void FunctionEmitter::emitWith(const ast::WithStatement& statement)
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

} // namespace inox::compiler::codegen::detail
