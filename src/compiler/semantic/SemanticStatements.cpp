// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: routine bodies, statements, Var blocks, case (ADR-0011).

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"
#include "../exceptions/ExceptionTypes.h"
#include "../support/ArrayType.h"
#include "../support/PointerType.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

namespace inox::compiler::semantic {

using namespace detail;

void SemanticAnalyzer::analyzeFunction(const ast::FunctionDeclaration& function)
{
    const std::string previousReturnType = std::move(currentFunctionReturnType_);
    const bool previousSawReturn = currentFunctionSawReturn_;
    const std::string previousRoutine = std::exchange(currentRoutine_, function.name());
    const FunctionSignature* signature = resolveFunctionSignature(function.name());
    currentFunctionReturnType_ = signature != nullptr ? signature->returnType : std::string{};
    currentFunctionSawReturn_ = false;
    symbols_.pushScope();
    const auto previousParameters = std::exchange(currentParameters_, {});
    if (signature != nullptr) {
        for (const FunctionParameter& parameter : signature->parameters) {
            declareOrThrow(parameter.name, SymbolKind::Variable, parameter.typeName,
                           parameter.mode == PassMode::RefMut);
            currentParameters_.insert(symbols_.currentScope().resolve(parameter.name));
        }
    }
    analyzeStatements(function.body(), false);
    currentParameters_ = previousParameters;
    leaveScope();
    if (!currentFunctionReturnType_.empty() && !currentFunctionSawReturn_) {
        throw SemanticError("function " + function.name() + " must return a value");
    }
    if (!currentFunctionReturnType_.empty() && !cannotFallThrough(function.body())) {
        throw SemanticError("function " + function.name() +
                            " may reach its end without returning a value; every path must end "
                            "in Return or Raise (CANON-12)");
    }
    currentFunctionReturnType_ = previousReturnType;
    currentFunctionSawReturn_ = previousSawReturn;
    currentRoutine_ = previousRoutine;
}

bool SemanticAnalyzer::cannotFallThrough(const std::vector<ast::StatementPtr>& statements)
{
    for (const auto& statement : statements) {
        if (statement && cannotFallThrough(*statement)) {
            return true;
        }
    }
    return false;
}

// Loops are treated as "may fall through" (a loop may run zero times or exit
// through leave). This never accepts a function that can fall through; it may
// reject one whose only exit is inside a loop, which then needs a final Return.
bool SemanticAnalyzer::cannotFallThrough(const ast::Statement& statement)
{
    switch (statement.kind()) {
    case ast::AstNodeKind::ReturnStatement:
    case ast::AstNodeKind::RaiseStatement:
    case ast::AstNodeKind::RetryStatement:
        return true;
    case ast::AstNodeKind::BlockStatement:
        return cannotFallThrough(static_cast<const ast::BlockStatement&>(statement).statements());
    case ast::AstNodeKind::WithStatement:
        return cannotFallThrough(static_cast<const ast::WithStatement&>(statement).body());
    case ast::AstNodeKind::IfStatement: {
        const auto& ifStatement = static_cast<const ast::IfStatement&>(statement);
        if (ifStatement.elseBody().empty() || !cannotFallThrough(ifStatement.thenBody()) ||
            !cannotFallThrough(ifStatement.elseBody())) {
            return false;
        }
        for (const ast::ElseIfClause& clause : ifStatement.elseIfClauses()) {
            if (!cannotFallThrough(clause.body)) {
                return false;
            }
        }
        return true;
    }
    case ast::AstNodeKind::CaseStatement: {
        const auto& caseStatement = static_cast<const ast::CaseStatement&>(statement);
        // ADR-0011: a case accepted without `otherwise` covers every value of
        // its selector, so only the explicit arms decide. With `otherwise`
        // (even an empty one) that arm must not fall through either.
        if (caseStatement.hasOtherwise() &&
            (caseStatement.otherwiseBody().empty() ||
             !cannotFallThrough(caseStatement.otherwiseBody()))) {
            return false;
        }
        if (caseStatement.arms().empty()) {
            return false;
        }
        for (const ast::CaseArm& arm : caseStatement.arms()) {
            if (!cannotFallThrough(arm.body)) {
                return false;
            }
        }
        return true;
    }
    case ast::AstNodeKind::TryStatement: {
        const auto& tryStatement = static_cast<const ast::TryStatement&>(statement);
        if (tryStatement.hasEnsure() && cannotFallThrough(tryStatement.ensureBody())) {
            return true;
        }
        if (!cannotFallThrough(tryStatement.body())) {
            return false;
        }
        if (tryStatement.hasPlainExcept() && !cannotFallThrough(tryStatement.exceptBody())) {
            return false;
        }
        for (const ast::ExceptionHandler& handler : tryStatement.handlers()) {
            if (!cannotFallThrough(handler.body)) {
                return false;
            }
        }
        if (!tryStatement.elseBody().empty() && !cannotFallThrough(tryStatement.elseBody())) {
            return false;
        }
        return true;
    }
    default:
        return false;
    }
}

void SemanticAnalyzer::analyzeStatements(const std::vector<ast::StatementPtr>& statements, bool createScope)
{
    if (createScope) {
        symbols_.pushScope();
    }

    for (const auto& statement : statements) {
        analyzeStatement(*statement);
    }

    if (createScope) {
        leaveScope();
    }
}

void SemanticAnalyzer::analyzeStatement(const ast::Statement& statement)
{
    switch (statement.kind()) {
    case ast::AstNodeKind::ExitStatement:
        // CANON-12: `Exit` is valid only in subroutines without a return value
        // and in Main; it is FORBIDDEN in functions. Found by mutation fuzzing:
        // previously only the LLVM backend noticed.
        if (!currentFunctionReturnType_.empty()) {
            throw SemanticError("Exit is not valid in a function that returns a value; use Return (CANON-12)");
        }
        break;
    case ast::AstNodeKind::BlockStatement: {
        const auto& block = static_cast<const ast::BlockStatement&>(statement);
        analyzeStatements(block.statements(), true);
        break;
    }
    case ast::AstNodeKind::ExpressionStatement: {
        const auto& expressionStatement = static_cast<const ast::ExpressionStatement&>(statement);
        const ast::Expression& statementExpression = expressionStatement.expression();
        requireStatementExpression(statementExpression);
        // ADR-0012 / OPEN-4: `X += R` is stored as `X := X + R`. The copy of
        // X on the right only serves the update, so it is not a read of X
        // (assigning is not reading); a read of X inside R still counts.
        if (expressionStatement.isCompoundAssignment()) {
            const auto& assignment = static_cast<const ast::BinaryExpression&>(statementExpression);
            // ADR-0012: an element target is read and written through the
            // same indices, so they must have no side effects. A bare name
            // can be a parameterless call, which only symbols can tell.
            if (isIndexTarget(assignment.left()) && !isSideEffectFree(assignment.left())) {
                throw SemanticError(
                    "a compound assignment target must be a variable, a field or an array "
                    "element whose indices have no side effects (ADR-0012)");
            }
            const ast::Expression* root =
                &static_cast<const ast::BinaryExpression&>(assignment.right()).left();
            while (root->kind() == ast::AstNodeKind::CallExpression &&
                   isMemberCall(static_cast<const ast::CallExpression&>(*root))) {
                root = static_cast<const ast::CallExpression&>(*root).arguments()[0].get();
            }
            if (root->kind() == ast::AstNodeKind::IdentifierExpression) {
                const auto& identifier = static_cast<const ast::IdentifierExpression&>(*root);
                if (symbols_.currentScope().resolve(identifier.name()) == nullptr) {
                    throw SemanticError("compound assignment needs an existing variable: " +
                                        identifier.name() + " (ADR-0012)");
                }
                updateOnlyReads_.insert(&identifier);
            }
        }
        analyzeExpression(statementExpression);
        if (statementExpression.kind() == ast::AstNodeKind::CallExpression) {
            const auto& call = static_cast<const ast::CallExpression&>(statementExpression);
            if (isMemberCall(call) && result_.symbolOf(call) == nullptr) {
                throw SemanticError("field access is not a statement: only assignments and calls may be used as statements");
            }
        }
        break;
    }
    case ast::AstNodeKind::VarStatement: {
        const auto& var = static_cast<const ast::VarStatement&>(statement);
        std::string typeName = var.typeName().empty() ? std::string{} : canonicalTypeName(var.typeName());
        if (!var.typeName().empty()) {
            resolveTypeOrThrow(var.typeName());
            if (support::isArrayTypeName(var.typeName()) || support::isGenericTypeName(var.typeName())) {
                result_.setCanonicalTypeName(var.typeName(), typeName);
            }
        }
        if (var.initializer() != nullptr) {
            const std::string initializerType = analyzeExpression(*var.initializer());
            if (typeName.empty()) {
                typeName = initializerType;
                if (isFileType(typeName)) {
                    requireFreshFile(*var.initializer());
                }
                if (typeName == "None") {
                    throw SemanticError("the type of None is not known here: write it, " + var.name() +
                                        " Link<T> := None");
                }
                if (isMoveOnlyType(typeName) && !isFreshOwnerValue(*var.initializer())) {
                    throw SemanticError("an owner is never copied: give it away with Move(X), or build a "
                                        "new one with New(...)");
                }
            } else if (!canAssignValue(typeName, initializerType, *var.initializer())) {
                throw SemanticError("cannot initialize " + typeName + " with " + initializerType);
            }
        }
        if (typeName.empty()) {
            throw SemanticError("variable requires a type or initializer: " + var.name());
        }
        // CANON-5 rule 3/4: every variable is born with a value. Only structs and
        // aggregates may omit ":=" (type-default initialization); scalars must be
        // initialized explicitly.
        // A Link and an Option<Own<T>> are born empty; an Own is born with
        // New(...) (safe-pointers).
        const bool bornEmpty = support::isLinkTypeName(typeName) || support::isOptionOwnTypeName(typeName);
        if (var.initializer() == nullptr && resolveStruct(typeName) == nullptr &&
            !support::isArrayTypeName(typeName) && !bornEmpty) {
            if (support::isOwnTypeName(typeName)) {
                throw SemanticError("an owner is born with New(...) or Move(X): " + var.name());
            }
            throw SemanticError("scalar declaration requires initializer: " + var.name());
        }
        if (var.initializer() == nullptr) {
            if (const std::string owner = ownerFieldWithoutDefault(typeName); !owner.empty()) {
                throw SemanticError(owner + " is an owner without default: build " + var.name() +
                                    " with its owners given");
            }
        }
        // CANON-24: a struct born with its defaults satisfies its invariant.
        if (var.initializer() == nullptr && hasInvariant(typeName) && invariantChecksOn()) {
            result_.requireNewValueInvariant(var, resolveStruct(typeName)->name);
        }
        declareOrThrow(var.name(), SymbolKind::Variable, std::move(typeName), true);
        trackLocal(var.name());
        break;
    }
    case ast::AstNodeKind::VarBlockStatement:
        analyzeVarBlock(static_cast<const ast::VarBlockStatement&>(statement));
        break;
    case ast::AstNodeKind::IfStatement: {
        // Safe pointers: an owner given away on any branch is given away
        // after the if (the analysis follows every path).
        const auto& ifStatement = static_cast<const ast::IfStatement&>(statement);
        requireBoolCondition(ifStatement.condition());
        const auto start = movedOwners_;
        analyzeStatements(ifStatement.thenBody(), true);
        auto merged = movedOwners_;
        for (const auto& clause : ifStatement.elseIfClauses()) {
            movedOwners_ = start;
            requireBoolCondition(*clause.condition);
            analyzeStatements(clause.body, true);
            merged.insert(movedOwners_.begin(), movedOwners_.end());
        }
        movedOwners_ = start;
        analyzeStatements(ifStatement.elseBody(), true);
        merged.insert(movedOwners_.begin(), movedOwners_.end());
        movedOwners_ = std::move(merged);
        break;
    }
    case ast::AstNodeKind::UnlessStatement: {
        const auto& unlessStatement = static_cast<const ast::UnlessStatement&>(statement);
        requireBoolCondition(unlessStatement.condition());
        const auto start = movedOwners_;
        analyzeStatements(unlessStatement.body(), true);
        movedOwners_.insert(start.begin(), start.end());
        break;
    }
    case ast::AstNodeKind::WhileStatement: {
        const auto& whileStatement = static_cast<const ast::WhileStatement&>(statement);
        requireBoolCondition(whileStatement.condition());
        const auto start = movedOwners_;
        ++loopDepth_;
        analyzeStatements(whileStatement.body(), true);
        --loopDepth_;
        requireOwnersBackAfterLoop(start);
        break;
    }
    case ast::AstNodeKind::RepeatStatement: {
        const auto& repeatStatement = static_cast<const ast::RepeatStatement&>(statement);
        const auto start = movedOwners_;
        ++loopDepth_;
        ++repeatDepth_;
        analyzeStatements(repeatStatement.body(), true);
        --repeatDepth_;
        --loopDepth_;
        requireOwnersBackAfterLoop(start);
        break;
    }
    case ast::AstNodeKind::UntilStatement: {
        const auto& untilStatement = static_cast<const ast::UntilStatement&>(statement);
        if (repeatDepth_ == 0) {
            throw SemanticError("until outside repeat");
        }
        requireBoolCondition(untilStatement.condition());
        break;
    }
    case ast::AstNodeKind::ForInStatement: {
        const auto& forStatement = static_cast<const ast::ForInStatement&>(statement);
        // ADR-0010: `for I in A..N(2)` was the old step form. Now `N(2)` is a
        // call; when N is a value, say how the step is written.
        if (forStatement.iterable().kind() == ast::AstNodeKind::BinaryExpression) {
            const auto& range = static_cast<const ast::BinaryExpression&>(forStatement.iterable());
            if (range.op() == ast::BinaryOperator::Range &&
                range.right().kind() == ast::AstNodeKind::CallExpression) {
                const auto& call = static_cast<const ast::CallExpression&>(range.right());
                if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
                    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
                    const Symbol* symbol = symbols_.currentScope().resolve(callee.name());
                    if (symbol != nullptr && symbol->kind != SymbolKind::Function &&
                        symbol->kind != SymbolKind::Builtin && symbol->kind != SymbolKind::Type) {
                        throw SemanticError(
                            "'" + callee.name() + "(...)' calls a value: the for-loop step "
                            "is written 'step S' (ADR-0010), e.g. 'for I in A.." +
                            callee.name() + " step S'");
                    }
                }
            }
        }
        analyzeExpression(forStatement.iterable());
        if (forStatement.step() != nullptr) {
            const std::string stepType = canonicalTypeName(analyzeExpression(*forStatement.step()));
            if (!isIntegerType(stepType)) {
                throw SemanticError("for-loop step must be an Integer expression, got " +
                                    (stepType.empty() ? std::string("<unknown>") : stepType));
            }
            std::int64_t constantStep = 0;
            if (constantIntegerValue(*forStatement.step(), constantStep) && constantStep <= 0) {
                throw SemanticError("for-loop step must be a positive integer");
            }
        }
        symbols_.pushScope();
        if (symbols_.currentScope().containsLocal(forStatement.iterator()) ||
            symbols_.currentScope().containsInAncestors(forStatement.iterator())) {
            leaveScope();
            throw SemanticError(
                "loop iterator conflicts with existing symbol: " + forStatement.iterator());
        }
        declareOrThrow(forStatement.iterator(), SymbolKind::LoopIterator, "Int64", false);
        const auto start = movedOwners_;
        ++loopDepth_;
        analyzeStatements(forStatement.body(), false);
        --loopDepth_;
        leaveScope();
        requireOwnersBackAfterLoop(start);
        break;
    }
    case ast::AstNodeKind::CaseStatement:
        analyzeCaseStatement(static_cast<const ast::CaseStatement&>(statement));
        break;
    case ast::AstNodeKind::TryStatement: {
        const auto& tryStatement = static_cast<const ast::TryStatement&>(statement);

        // Safe pointers: a handler may start after any part of the body, so
        // it sees the owners given away before the try and in the body.
        const auto startOfTry = movedOwners_;
        analyzeStatements(tryStatement.body(), true);
        auto handlerStart = startOfTry;
        handlerStart.insert(movedOwners_.begin(), movedOwners_.end());
        auto afterTry = movedOwners_;
        const auto joinHandler = [&] {
            afterTry.insert(movedOwners_.begin(), movedOwners_.end());
        };

        if (tryStatement.hasPlainExcept()) {
            // Bare Raise is meaningful here, but Retry is intentionally not:
            // Retry belongs to an explicit On/Else handler for this try.
            ++exceptionHandlerDepth_;
            movedOwners_ = handlerStart;
            analyzeStatements(tryStatement.exceptBody(), true);
            joinHandler();
            --exceptionHandlerDepth_;
        }

        bool catchesAll = false;
        std::unordered_map<std::string, bool> seenHandlers;
        std::vector<std::string> precedingHandlerTypes;
        for (const auto& handler : tryStatement.handlers()) {
            if (!isExceptionType(handler.typeName)) {
                throw SemanticError("unknown exception type in On handler: " + handler.typeName);
            }
            const std::string normalizedType = normalizeName(handler.typeName);
            if (seenHandlers.contains(normalizedType)) {
                throw SemanticError("duplicate exception handler: " + handler.typeName);
            }
            if (catchesAll) {
                throw SemanticError("unreachable exception handler after On Exception: " + handler.typeName);
            }
            for (const std::string& precedingType : precedingHandlerTypes) {
                if (exceptions::isSubtypeOf(handler.typeName, precedingType)) {
                    throw SemanticError(
                        "unreachable exception handler: " + handler.typeName +
                        " is already matched by earlier On " + precedingType);
                }
            }
            seenHandlers.emplace(normalizedType, true);
            precedingHandlerTypes.push_back(handler.typeName);
            catchesAll = exceptions::isSubtypeOf("Exception", handler.typeName);

            symbols_.pushScope();
            if (!handler.bindingName.empty()) {
                declareOrThrow(handler.bindingName, SymbolKind::Variable, handler.typeName, false);
            }
            ++exceptionHandlerDepth_;
            ++retryHandlerDepth_;
            movedOwners_ = handlerStart;
            analyzeStatements(handler.body, false);
            joinHandler();
            --retryHandlerDepth_;
            --exceptionHandlerDepth_;
            leaveScope();
        }

        if (!tryStatement.elseBody().empty()) {
            if (catchesAll) {
                throw SemanticError("unreachable Else after On Exception");
            }
            ++exceptionHandlerDepth_;
            ++retryHandlerDepth_;
            movedOwners_ = handlerStart;
            analyzeStatements(tryStatement.elseBody(), true);
            joinHandler();
            --retryHandlerDepth_;
            --exceptionHandlerDepth_;
        }

        movedOwners_ = afterTry;
        if (tryStatement.hasEnsure()) {
            movedOwners_.insert(handlerStart.begin(), handlerStart.end());
            ++ensureDepth_;
            analyzeStatements(tryStatement.ensureBody(), true);
            --ensureDepth_;
        }
        break;
    }
    case ast::AstNodeKind::RaiseStatement: {
        const auto& raiseStatement = static_cast<const ast::RaiseStatement&>(statement);
        if (raiseStatement.expression() == nullptr) {
            if (exceptionHandlerDepth_ == 0) {
                throw SemanticError("bare Raise is only allowed inside an exception handler");
            }
            break;
        }

        const ast::Expression& expression = *raiseStatement.expression();
        if (expression.kind() != ast::AstNodeKind::IdentifierExpression) {
            throw SemanticError("Raise currently requires an exception type name");
        }
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
        if (!isExceptionType(identifier.name())) {
            throw SemanticError("Raise requires an exception type: " + identifier.name());
        }
        break;
    }
    case ast::AstNodeKind::RetryStatement: {
        const auto& retryStatement = static_cast<const ast::RetryStatement&>(statement);
        if (retryHandlerDepth_ == 0 || ensureDepth_ != 0) {
            throw SemanticError("Retry is only allowed inside an active On/Else exception handler");
        }
        const std::string countType = analyzeExpression(retryStatement.count());
        if (!isIntegerType(countType)) {
            throw SemanticError("Retry count must be an Integer expression");
        }
        if (retryStatement.count().kind() == ast::AstNodeKind::UnaryExpression) {
            const auto& unary = static_cast<const ast::UnaryExpression&>(retryStatement.count());
            if (unary.op() == ast::UnaryOperator::Minus &&
                unary.operand().kind() == ast::AstNodeKind::LiteralExpression) {
                const auto& literal = static_cast<const ast::LiteralExpression&>(unary.operand());
                if (literal.literalKind() == ast::LiteralKind::Integer) {
                    throw SemanticError("Retry count cannot be negative");
                }
            }
        }
        break;
    }
    case ast::AstNodeKind::ReturnStatement: {
        currentFunctionSawReturn_ = true;
        const ast::Expression& returned =
            static_cast<const ast::ReturnStatement&>(statement).expression();
        const std::string typeName = analyzeExpression(returned);
        if (currentFunctionReturnType_.empty()) {
            throw SemanticError("subroutine without return type cannot return a value");
        }
        if (!canAssignValue(currentFunctionReturnType_, typeName, returned)) {
            throw SemanticError(
                "cannot return " + typeName + " from function returning " +
                currentFunctionReturnType_);
        }
        break;
    }
    case ast::AstNodeKind::LeaveStatement:
        if (loopDepth_ == 0) {
            throw SemanticError("leave outside loop");
        }
        break;
    case ast::AstNodeKind::ContinueStatement:
        if (loopDepth_ == 0) {
            throw SemanticError("continue outside loop");
        }
        break;
    case ast::AstNodeKind::WithStatement: {
        const auto& withStatement = static_cast<const ast::WithStatement&>(statement);
        const std::string targetType = analyzeExpression(withStatement.target());
        symbols_.pushScope();
        // Declare the synthetic binding directly — no shadowing check needed
        // because the name (__with_N) is compiler-generated and never user-visible.
        // The binding is as writable as its target (CANON-9): `with P`
        // on a parameter or a read-only Self only reads.
        symbols_.currentScope().declare(
            withStatement.bindingName(), SymbolKind::Variable, targetType,
            writableRoot(withStatement.target()) != nullptr);
        analyzeStatements(withStatement.body(), false);
        leaveScope();
        break;
    }
    default:
        break;
    }
}

void SemanticAnalyzer::analyzeVarBlock(const ast::VarBlockStatement& statement)
{
    for (const auto& declaration : statement.declarations()) {
        if (declaration->kind() == ast::AstNodeKind::VarStatement) {
            analyzeStatement(*declaration);
            continue;
        }

        if (declaration->kind() == ast::AstNodeKind::ExpressionStatement) {
            const auto& expressionStatement = static_cast<const ast::ExpressionStatement&>(*declaration);
            const auto& expression = expressionStatement.expression();
            if (expression.kind() == ast::AstNodeKind::BinaryExpression) {
                const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
                if (binary.op() == ast::BinaryOperator::Assign &&
                    binary.left().kind() == ast::AstNodeKind::IdentifierExpression) {
                    const auto& identifier = static_cast<const ast::IdentifierExpression&>(binary.left());
                    const std::string typeName = analyzeExpression(binary.right());
                    if (typeName == "None" ||
                        (isMoveOnlyType(typeName) && !isFreshOwnerValue(binary.right()))) {
                        throw SemanticError(typeName == "None"
                                                ? "the type of None is not known here: write it"
                                                : "an owner is never copied: give it away with Move(X)");
                    }
                    declareOrThrow(identifier.name(), SymbolKind::Variable, typeName, true);
                    trackLocal(identifier.name());
                    const Symbol& symbol = resolveOrThrow(identifier.name());
                    result_.bind(identifier, symbol);
                    result_.setExpressionType(identifier, resolvedType(typeName));
                    result_.setExpressionType(binary, resolvedType(typeName));
                    continue;
                }
            }
        }

        analyzeStatement(*declaration);
    }
}

void SemanticAnalyzer::analyzeCaseStatement(const ast::CaseStatement& statement)
{
    // Safe pointers: an owner given away in any arm is given away after it.
    const auto caseStart = movedOwners_;
    auto caseMerged = movedOwners_;
    const std::string selectorType =
        canonicalTypeName(analyzeExpression(statement.expression()));
    CaseDomain domain;
    if (!caseDomainFor(selectorType, domain)) {
        throw SemanticError(
            "case selector must be discrete (Integer family, Char, Bool, Enum or Range), got " +
            (selectorType.empty() ? std::string("<unknown>") : selectorType) + " (ADR-0011)");
    }

    struct Interval {
        std::int64_t low;
        std::int64_t high;
    };
    std::vector<Interval> covered;

    for (const ast::CaseArm& arm : statement.arms()) {
        for (const auto& choice : arm.choices) {
            Interval interval{};
            const bool isRange =
                choice->kind() == ast::AstNodeKind::BinaryExpression &&
                static_cast<const ast::BinaryExpression&>(*choice).op() == ast::BinaryOperator::Range;
            if (isRange) {
                const auto& range = static_cast<const ast::BinaryExpression&>(*choice);
                interval.low = caseChoiceValue(range.left(), selectorType);
                interval.high = caseChoiceValue(range.right(), selectorType);
                if (interval.low > interval.high) {
                    throw SemanticError("empty range in case choice: " +
                                        caseValueText(interval.low, domain) + ".." +
                                        caseValueText(interval.high, domain) +
                                        " (the first bound must not exceed the second)");
                }
            } else {
                interval.low = caseChoiceValue(*choice, selectorType);
                interval.high = interval.low;
            }
            if (interval.low < domain.low || interval.high > domain.high) {
                throw SemanticError("case choice out of range for " + selectorType + ": " +
                                    caseValueText(interval.low < domain.low ? interval.low : interval.high, domain));
            }
            for (const Interval& seen : covered) {
                if (interval.low <= seen.high && seen.low <= interval.high) {
                    const std::int64_t shared = std::max(interval.low, seen.low);
                    throw SemanticError("overlapping case choices: " +
                                        caseValueText(shared, domain) +
                                        " is covered more than once (ADR-0011)");
                }
            }
            covered.push_back(interval);
        }
        movedOwners_ = caseStart;
        analyzeStatements(arm.body, true);
        caseMerged.insert(movedOwners_.begin(), movedOwners_.end());
    }

    movedOwners_ = caseStart;
    if (statement.hasOtherwise()) {
        analyzeStatements(statement.otherwiseBody(), true);
    }
    caseMerged.insert(movedOwners_.begin(), movedOwners_.end());
    movedOwners_ = std::move(caseMerged);
    if (statement.hasOtherwise()) {
        return;
    }

    // Without `otherwise` the choices must cover every value of the selector
    // type (Marcelo Fortes, 2026-10-09: "otherwise deve ser obrigatório sempre
    // que os braços não cobrirem todos os valores possíveis").
    if (!domain.provable) {
        throw SemanticError("case on " + selectorType +
                            " cannot be proven to cover every value; add 'otherwise' (ADR-0011)");
    }
    std::sort(covered.begin(), covered.end(),
              [](const Interval& a, const Interval& b) { return a.low < b.low; });
    std::int64_t next = domain.low;
    bool done = false;
    for (const Interval& interval : covered) {
        if (domain.isChar && next >= 0xD800 && next <= 0xDFFF) {
            next = 0xE000;  // surrogates are not Unicode scalar values
        }
        if (interval.low > next) {
            break;
        }
        if (interval.high >= domain.high) {
            done = true;
            break;
        }
        next = std::max(next, interval.high + 1);
    }
    if (!done && domain.isChar && next >= 0xD800 && next <= 0xDFFF) {
        next = 0xE000;
    }
    if (!done) {
        throw SemanticError("case does not cover every value of " + selectorType +
                            " (first missing: " + caseValueText(next, domain) +
                            "); add 'otherwise' (ADR-0011)");
    }
}

} // namespace inox::compiler::semantic

namespace inox::compiler::semantic {

// Safe pointers: a loop's next round starts where its body ends, so an
// owner declared before the loop and given away in the body must be given
// a new value before the body ends. After the loop, the owners given away
// before it still are.
void SemanticAnalyzer::requireOwnersBackAfterLoop(const std::unordered_map<const Symbol*, const Scope*>& start)
{
    for (const auto& [symbol, scope] : movedOwners_) {
        if (!start.contains(symbol)) {
            throw SemanticError(symbol->name + " is given away (Move or Free) inside the loop: assign it again "
                                "before the next round");
        }
    }
    movedOwners_.insert(start.begin(), start.end());
}

} // namespace inox::compiler::semantic
