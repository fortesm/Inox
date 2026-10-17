// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: try/except/ensure, Raise, Retry, exception capture and rethrow (both ABIs).

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

std::string FunctionEmitter::emitExceptionTypeMatch(std::string_view actualTypeValue,
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

void FunctionEmitter::emitRetry(const ast::RetryStatement& statement)
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

const std::string& FunctionEmitter::currentUnwindTarget() const
{
    if (unwindTargets_.empty()) {
        throw CodegenError("internal error: no active exception unwind target");
    }
    return unwindTargets_.back();
}

std::string FunctionEmitter::newDeadLabel(std::string_view prefix)
{
    return std::string(prefix) + std::to_string(nextLabel_++);
}

void FunctionEmitter::emitExceptionCapture(std::string_view landingPadLabel,
                                           std::string_view stateSlot,
                                           std::string_view nextLabel,
                                           bool replaceExisting)
{
    const std::string state = module_.exceptionAbi.emitCapturePad(
        output_, landingPadLabel,
        [this](std::string_view prefix) { return std::string(prefix) + std::to_string(nextTemporary_++); });
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

void FunctionEmitter::emitReleaseExceptionState(std::string_view stateSlot)
{
    const std::string state = "%eh.release" + std::to_string(nextTemporary_++);
    output_ << "  " << state << " = load ptr, ptr " << stateSlot << "\n";
    output_ << "  call void @__inox_exception_release(ptr " << state << ")\n";
    output_ << "  store ptr null, ptr " << stateSlot << "\n";
}

void FunctionEmitter::emitRethrowState(std::string_view stateSlot)
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

void FunctionEmitter::emitRaise(const ast::RaiseStatement& statement)
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

void FunctionEmitter::emitTry(const ast::TryStatement& statement)
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

} // namespace inox::compiler::codegen::detail
