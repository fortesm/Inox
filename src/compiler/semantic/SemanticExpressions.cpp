// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: expression typing: operators, indexing, slices, members, side effects.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"
#include "../support/CharLiteral.h"
#include "../support/RealLiteral.h"
#include "../exceptions/ExceptionTypes.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
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

void SemanticAnalyzer::requireStatementExpression(const ast::Expression& expression)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::IdentifierExpression: {
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
        if (equalsIgnoreCase(identifier.name(), "Get") || equalsIgnoreCase(identifier.name(), "GetLn")) {
            return;
        }
        if (genericRoutine(identifier.name()) != nullptr) {
            return;  // analyzed as a call: its types must be known there
        }
        if (moduleIntrinsic(identifier.name()) != nullptr) {
            return;  // Std.Terminal: ClearScreen, Flush, ...
        }
        if (equalsIgnoreCase(identifier.name(), "EndOfInput")) {
            throw SemanticError(discardedResultMessage(identifier));
        }
        if (const FunctionSignature* signature = resolveFunctionSignature(identifier.name())) {
            // `Report` calls a routine without arguments (CANON-7); a routine
            // that takes arguments needs them.
            if (!signature->parameters.empty()) {
                throw SemanticError(identifier.name() + " expects " +
                                    std::to_string(signature->parameters.size()) +
                                    (signature->parameters.size() == 1 ? " argument" : " arguments") +
                                    ", got 0");
            }
            return;
        }
        throw SemanticError("'" + identifier.name() +
                            "' is not a subroutine and cannot be used as a statement");
    }
    case ast::AstNodeKind::LiteralExpression:
    case ast::AstNodeKind::UnaryExpression:
    case ast::AstNodeKind::ConditionalExpression:
        throw SemanticError("expression is not a statement: only assignments and calls may be used as statements");
    case ast::AstNodeKind::BinaryExpression:
        if (static_cast<const ast::BinaryExpression&>(expression).op() != ast::BinaryOperator::Assign) {
            throw SemanticError("expression is not a statement: only assignments and calls may be used as statements");
        }
        return;
    default:
        return;
    }
}

// Whether the UInt64 context of a target reaches into this expression's
// operands (change uint64): only through arithmetic and bitwise operators,
// unary + and bitnot, and the branches of IIf.
// `real`: the context is Float32 (change float-conversions), where a negated
// literal and a quotient of literals are Float32 too.
static bool carriesIntegerContext(const ast::Expression& expression, bool real)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression:
    case ast::AstNodeKind::ConditionalExpression:
        return true;
    // A universal real Const takes a Float32 context (r3.86).
    case ast::AstNodeKind::IdentifierExpression:
        return real;
    case ast::AstNodeKind::UnaryExpression: {
        const ast::UnaryOperator op = static_cast<const ast::UnaryExpression&>(expression).op();
        return op == ast::UnaryOperator::Plus || op == ast::UnaryOperator::BitNot ||
               (real && op == ast::UnaryOperator::Minus);
    }
    case ast::AstNodeKind::BinaryExpression:
        switch (static_cast<const ast::BinaryExpression&>(expression).op()) {
        case ast::BinaryOperator::Divide:
        case ast::BinaryOperator::Add:
        case ast::BinaryOperator::Subtract:
        case ast::BinaryOperator::Multiply:
        case ast::BinaryOperator::Power:
        case ast::BinaryOperator::Modulo:
        case ast::BinaryOperator::Remainder:
        case ast::BinaryOperator::ShiftLeft:
        case ast::BinaryOperator::ShiftRight:
        case ast::BinaryOperator::BitAnd:
        case ast::BinaryOperator::BitOr:
        case ast::BinaryOperator::BitXor:
            return true;
        default:
            return false;
        }
    default:
        return false;
    }
}

// An integer literal whose magnitude does not fit Int64 (only a UInt64 holds it).
static bool isLiteralAboveInt64(const ast::Expression& expression)
{
    if (expression.kind() != ast::AstNodeKind::LiteralExpression) {
        return false;
    }
    const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
    std::uint64_t magnitude = 0;
    return literal.literalKind() == ast::LiteralKind::Integer &&
           parseIntegerLiteralMagnitude(literal.value(), magnitude) && magnitude >= kInt64MagnitudeOfMin;
}

TypeRef SemanticAnalyzer::analyzeWithIntegerContext(const ast::Expression& expression, TypeRef context)
{
    // A UInt64 context makes its integer literals UInt64 (change uint64); a
    // Float32 context makes its real literals Float32 (change
    // float-conversions).
    // A Currency or Crypto context makes its literals exact decimals (change
    // decimals, r3.91).
    const bool keeps = isUInt64Type(context) || (context != nullptr && context->isFloat() && context->bits() == 32) ||
                       (context != nullptr && context->isDecimal());
    const TypeRef previous = std::exchange(integerContext_, keeps ? context : nullptr);
    try {
        const TypeRef type = analyzeExpression(expression);
        integerContext_ = previous;
        return type;
    } catch (...) {
        integerContext_ = previous;
        throw;
    }
}

TypeRef SemanticAnalyzer::analyzeExpression(const ast::Expression& expression)
{
    const TypeRef context = integerContext_;
    if (!carriesIntegerContext(expression, context != nullptr && (context->isFloat() || context->isDecimal()))) {
        integerContext_ = nullptr;
    }
    TypeRef type = nullptr;
    try {
        type = inferExpressionType(expression);
    } catch (...) {
        integerContext_ = context;
        throw;
    }
    integerContext_ = context;
    result_.setExpressionType(expression, type);
    foldConstantExpression(expression);
    // CANON-8: constant overflow is a compile-time error in every integer type.
    if (isNarrowIntegerType(type)) {
        std::int64_t value = 0;
        std::int64_t low = 0;
        std::int64_t high = 0;
        if (constantIntegerValue(expression, value) && integerTypeRange(type, low, high) &&
            (value < low || value > high)) {
            throw SemanticError("constant overflow: " + std::to_string(value) + " does not fit " +
                                type->name());
        }
    }
    return type;
}

TypeRef SemanticAnalyzer::inferExpressionType(const ast::Expression& expression)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression: {
        const auto literalKind = static_cast<const ast::LiteralExpression&>(expression).literalKind();
        switch (literalKind) {
        case ast::LiteralKind::Integer: {
            std::uint64_t magnitude = 0;
            const auto& integerLiteral = static_cast<const ast::LiteralExpression&>(expression);
            if (!parseIntegerLiteralMagnitude(integerLiteral.value(), magnitude)) {
                throw SemanticError("integer literal out of range for Int64: " +
                                    std::string(integerLiteral.value()));
            }
            // Change decimals (r3.91, A2): where a Currency or a Crypto is
            // expected, an integer literal is one, exactly.
            if (integerContext_ != nullptr && integerContext_->isDecimal()) {
                checkDecimalLiteral(integerLiteral.value(), integerContext_);
                return integerContext_;
            }
            // Change uint64 (Q1): where a UInt64 is expected, a literal is a
            // UInt64 and may reach 2^64-1; elsewhere it must fit Int64.
            if (isUInt64Type(integerContext_)) {
                return integerContext_;
            }
            if (magnitude >= kInt64MagnitudeOfMin) {
                throw SemanticError("integer literal out of range for Int64: " +
                                    std::string(integerLiteral.value()) +
                                    " (a literal above Int64.Max is a UInt64 where a UInt64 is expected: "
                                    "X UInt64 := " + std::string(integerLiteral.value()) + ")");
            }
            return types().int64Type();
        }
        case ast::LiteralKind::Float:
            // Change decimals (r3.91, CANON-8): a real literal is read exactly
            // where a Currency or a Crypto is expected.
            if (integerContext_ != nullptr && integerContext_->isDecimal()) {
                checkDecimalLiteral(static_cast<const ast::LiteralExpression&>(expression).value(), integerContext_);
                return integerContext_;
            }
            // A real literal is a Float32 where a Float32 is expected.
            if (integerContext_ != nullptr && integerContext_->isFloat()) {
                const double value =
                    support::parseRealLiteral32(static_cast<const ast::LiteralExpression&>(expression).value());
                if (std::isinf(value)) {
                    throw SemanticError("value out of range for Float32: " +
                                        std::string(static_cast<const ast::LiteralExpression&>(expression).value()));
                }
                return integerContext_;
            }
            // r3.89: an exponent can write a literal beyond Float64.
            if (std::isinf(
                    support::parseRealLiteral(static_cast<const ast::LiteralExpression&>(expression).value()))) {
                throw SemanticError("value out of range for Float: " +
                                    std::string(static_cast<const ast::LiteralExpression&>(expression).value()));
            }
            return types().float64Type();
        case ast::LiteralKind::String:
            return types().stringType();
        case ast::LiteralKind::Char: {
            // CANON-14 (char-values): one Unicode scalar, never a surrogate.
            const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
            const support::DecodedChar decoded = support::decodeCharLiteral(literal.value());
            switch (decoded.status) {
            case support::CharLiteralStatus::Ok:
                break;
            case support::CharLiteralStatus::Empty:
                throw SemanticError("a Char literal holds one character: '' is empty");
            case support::CharLiteralStatus::NotOneChar:
                throw SemanticError("a Char literal holds one character, got " + std::string(literal.value()) +
                                    "; a text is a String, \"...\"");
            case support::CharLiteralStatus::UnknownEscape:
                throw SemanticError("unknown escape in Char literal " + std::string(literal.value()) +
                                    ": the escapes are \\n \\r \\t \\0 \\\\ \\'");
            case support::CharLiteralStatus::Surrogate:
                throw SemanticError("a Char is a Unicode scalar value, never a surrogate (CANON-14): " +
                                    std::string(literal.value()));
            }
            return types().charType();
        }
        case ast::LiteralKind::Boolean:
            return types().boolType();
        }
        break;
    }
    case ast::AstNodeKind::IdentifierExpression: {
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
        if (equalsIgnoreCase(identifier.name(), "Get") || equalsIgnoreCase(identifier.name(), "GetLn")) {
            const Symbol& symbol = resolveOrThrow(identifier.name());
            result_.bind(identifier, symbol);
            return types().voidType();
        }
        if (const ModuleIntrinsic* terminal = moduleIntrinsic(identifier.name())) {
            return analyzeIntrinsicIdentifier(identifier, *terminal);
        }
        // r3.89 (C1): EndOfInput, true when nothing is left to read.
        if (equalsIgnoreCase(identifier.name(), "EndOfInput")) {
            if (const Symbol* symbol = symbols_.currentScope().resolve(identifier.name());
                symbol != nullptr && symbol->kind == SymbolKind::Builtin) {
                result_.bind(identifier, *symbol);
                return types().boolType();
            }
        }
        if (const ast::FunctionDeclaration* generic = genericRoutine(identifier.name())) {
            return analyzeGenericCall(*generic, identifier, nullptr);
        }
        if (const FunctionSignature* signature = resolveFunctionSignature(identifier.name())) {
            if (signature->parameters.empty()) {
                if (const Symbol* symbol = symbols_.currentScope().resolve(identifier.name())) {
                    result_.bind(identifier, *symbol);
                }
                return signature->returnType;
            }
            throw SemanticError("function " + identifier.name() + " requires arguments; use parentheses only when passing arguments");
        }
        // None: the empty Link or the absent owner (safe-pointers).
        if (equalsIgnoreCase(identifier.name(), "None") &&
            symbols_.currentScope().resolve(identifier.name()) == nullptr) {
            return types().noneType();
        }
        const Symbol& symbol = resolveOrThrow(identifier.name());
        result_.bind(identifier, symbol);
        if (!updateOnlyReads_.contains(&identifier)) {
            readSymbols_.insert(&symbol);
        }
        if (symbol.kind == SymbolKind::Variable) {
            requireOwnerUsable(symbol);
        }
        // r3.86: a universal real Const (`Pi`) is a Float32 where a Float32
        // is expected, rounded once from its decimal text.
        if (symbol.kind == SymbolKind::Constant && integerContext_ != nullptr && integerContext_->isFloat()) {
            if (const ConstantValue* constant = result_.constantValueOf(symbol);
                constant != nullptr && constant->kind == ConstantValue::Kind::Real && constant->universal) {
                if (std::isinf(support::parseRealLiteral32(constant->text))) {
                    throw SemanticError("value out of range for Float32: " + identifier.name());
                }
                return integerContext_;
            }
        }
        return symbol.type;
    }
    case ast::AstNodeKind::BinaryExpression:
        return analyzeBinaryExpression(static_cast<const ast::BinaryExpression&>(expression));
    case ast::AstNodeKind::UnaryExpression:
        return analyzeUnaryExpression(static_cast<const ast::UnaryExpression&>(expression));
    case ast::AstNodeKind::CallExpression:
        return analyzeCallExpression(static_cast<const ast::CallExpression&>(expression));
    case ast::AstNodeKind::ConditionalExpression: {
        // ADR-0013: the condition is Bool; both branches have compatible
        // types, which is the type of the expression.
        const auto& conditional = static_cast<const ast::ConditionalExpression&>(expression);
        const TypeRef conditionType = analyzeWithIntegerContext(conditional.condition(), nullptr);
        if (conditionType == nullptr || !conditionType->isBool()) {
            throw SemanticError("the condition of a conditional expression must be Bool, got " +
                                (conditionType == nullptr ? std::string("<unknown>") : conditionType->name()));
        }
        const TypeRef thenType = analyzeExpression(conditional.thenValue());
        // A UInt64 branch makes the other a UInt64 context (change uint64).
        const TypeRef elseType = analyzeWithIntegerContext(
            conditional.elseValue(), integerContext_ != nullptr ? integerContext_ : thenType);
        // `IIf(C, 7, U)`: a constant branch that is not negative is a UInt64
        // next to a UInt64 one.
        std::int64_t constant = 0;
        if (isUInt64Type(elseType) && thenType == types().int64Type() &&
            constantIntegerValue(conditional.thenValue(), constant) && constant >= 0) {
            return elseType;
        }
        // A Range and its base meet in the base (CANON-13).
        const auto base = [](TypeRef type) { return type != nullptr && type->isRange() ? type->rangeBase() : type; };
        if (thenType != elseType && (base(thenType) != thenType || base(elseType) != elseType) &&
            base(thenType) == base(elseType)) {
            return base(thenType);
        }
        if (!typesMatch(thenType, elseType)) {
            throw SemanticError("the branches of a conditional expression have different types: " +
                                types::nameOf(thenType) + " and " + types::nameOf(elseType) + " (ADR-0013)");
        }
        return thenType;
    }
    default:
        break;
    }

    return nullptr;
}

// CANON-13: `A[I]` reads element I of a fixed array. I is an integer; a
// constant index outside Low..High is a compile-time error, any other index
// is checked at run time.
TypeRef SemanticAnalyzer::analyzeIndexExpression(const ast::CallExpression& call)
{
    // Through a pointer, `P[I]` is an element of its array (safe-pointers).
    const TypeRef baseType = typeThroughPointer(analyzeExpression(*call.arguments()[0]));
    if (baseType != nullptr && baseType->isSlice()) {
        const TypeRef indexType = analyzeExpression(*call.arguments()[1]);
        if (!isIntegerType(indexType)) {
            throw SemanticError("an array index must be an integer, not " + types::nameOf(indexType));
        }
        return baseType->element();
    }
    if (baseType != nullptr && baseType->isString()) {
        throw SemanticError("S[I] on a String is reserved (CANON-14): use Substr(S, I, 1) of Std.Strings");
    }
    if (baseType == nullptr || !baseType->isArray()) {
        throw SemanticError("only an array can be indexed (CANON-13); this value has type " +
                            types::nameOf(baseType));
    }
    const TypeRef indexType = analyzeExpression(*call.arguments()[1]);
    if (!isIntegerType(indexType)) {
        throw SemanticError("an array index must be an integer, not " + types::nameOf(indexType));
    }
    std::int64_t index = 0;
    const std::int64_t low = baseType->low();
    const std::int64_t high = baseType->high();
    if (constantIntegerValue(*call.arguments()[1], index) && (index < low || index > high)) {
        throw SemanticError("array index out of bounds: " + std::to_string(index) + " is not in " +
                            std::to_string(low) + ".." + std::to_string(high));
    }
    return baseType->element();
}

// ADR-0017: `A[L..H]` is a read-only view of elements L..H of a named array
// or slice, keeping their indices. An omitted bound is the base's own.
TypeRef SemanticAnalyzer::analyzeSliceExpression(const ast::CallExpression& call)
{
    const ast::Expression& base = *call.arguments()[0];
    const TypeRef baseType = analyzeExpression(base);
    TypeRef element = nullptr;
    std::optional<std::pair<std::int64_t, std::int64_t>> staticBounds;
    if (baseType != nullptr && baseType->isSlice()) {
        element = baseType->element();
    } else if (baseType != nullptr && baseType->isArray()) {
        element = baseType->element();
        staticBounds = std::make_pair(baseType->low(), baseType->high());
    } else {
        throw SemanticError("only an array or a slice can be sliced (ADR-0017); this value has type " +
                            types::nameOf(baseType));
    }
    // The view must not outlive its storage: the base is a named variable or
    // parameter (or a field or element of one), never a temporary.
    const ast::Expression* root = &base;
    while (root->kind() == ast::AstNodeKind::CallExpression) {
        const auto& inner = static_cast<const ast::CallExpression&>(*root);
        if (inner.callee().kind() != ast::AstNodeKind::IdentifierExpression || inner.arguments().empty()) {
            break;
        }
        const std::string& name = static_cast<const ast::IdentifierExpression&>(inner.callee()).name();
        if (name != "__index" && name != "__member" && name != "__slice") {
            break;
        }
        root = inner.arguments()[0].get();
    }
    const Symbol* rootSymbol = root->kind() == ast::AstNodeKind::IdentifierExpression
        ? symbols_.currentScope().resolve(static_cast<const ast::IdentifierExpression&>(*root).name())
        : nullptr;
    if (rootSymbol == nullptr ||
        (rootSymbol->kind != SymbolKind::Variable && rootSymbol->kind != SymbolKind::State)) {
        throw SemanticError("only a named array can be sliced, not a temporary value (ADR-0017)");
    }
    std::int64_t low = 0;
    std::int64_t high = 0;
    bool lowKnown = false;
    bool highKnown = false;
    for (int which = 1; which <= 2; ++which) {
        const ast::Expression& bound = *call.arguments()[which];
        if (bound.kind() == ast::AstNodeKind::IdentifierExpression &&
            (static_cast<const ast::IdentifierExpression&>(bound).name() == "__low" ||
             static_cast<const ast::IdentifierExpression&>(bound).name() == "__high")) {
            if (staticBounds) {
                (which == 1 ? low : high) = which == 1 ? staticBounds->first : staticBounds->second;
                (which == 1 ? lowKnown : highKnown) = true;
            }
            continue;
        }
        const TypeRef boundType = analyzeExpression(bound);
        if (!isIntegerType(boundType)) {
            throw SemanticError("a slice bound must be an integer, not " + types::nameOf(boundType));
        }
        std::int64_t value = 0;
        if (constantIntegerValue(bound, value)) {
            (which == 1 ? low : high) = value;
            (which == 1 ? lowKnown : highKnown) = true;
        }
    }
    if (staticBounds) {
        if (lowKnown && low < staticBounds->first) {
            throw SemanticError("slice bounds out of range: " + std::to_string(low) + " is below " +
                                std::to_string(staticBounds->first));
        }
        if (highKnown && high > staticBounds->second) {
            throw SemanticError("slice bounds out of range: " + std::to_string(high) + " is above " +
                                std::to_string(staticBounds->second));
        }
    }
    if (lowKnown && highKnown && low > high) {
        throw SemanticError("slice bounds out of range: " + std::to_string(low) + ".." +
                            std::to_string(high));
    }
    return types().slice(element);
}

void SemanticAnalyzer::requireBoolCondition(const ast::Expression& expression)
{
    const TypeRef type = analyzeExpression(expression);
    if (type != nullptr && !type->isBool()) {
        throw SemanticError("condition must be Bool");
    }
}

TypeRef SemanticAnalyzer::analyzeBinaryExpression(const ast::BinaryExpression& expression)
{
    const ast::BinaryOperator op = expression.op();

    // CANON-8 (r3.77): `Max<Integer>(3)` reads as `Max < Integer > (3)`; a
    // generic routine or struct compared with a type is written type arguments.
    if (op == ast::BinaryOperator::Less && expression.left().kind() == ast::AstNodeKind::IdentifierExpression &&
        expression.right().kind() == ast::AstNodeKind::IdentifierExpression) {
        const std::string& name = static_cast<const ast::IdentifierExpression&>(expression.left()).name();
        const std::string& argument = static_cast<const ast::IdentifierExpression&>(expression.right()).name();
        if ((genericRoutine(name) != nullptr || genericStruct(name) != nullptr) &&
            (types().lookup(argument) != nullptr || genericStruct(argument) != nullptr)) {
            throw SemanticError("type arguments are not written in an expression (CANON-8): they come from the "
                                "arguments, " + name + "(...), or from the type of the target");
        }
    }

    if (op == ast::BinaryOperator::Assign) {
        TypeRef targetType = nullptr;

        if (expression.left().kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression.left());

            // CANON-5 / A6 + A7: first appearance of a name is a DECLARATION.
            // `Name := Expr` where Name is not yet visible in any scope is an
            // inline declaration with the type INFERRED from the initializer
            // (Ada/SPARK safe inference). A subsequent `Name := Expr` is a plain
            // assignment, handled by the existing path below.
            if (symbols_.currentScope().resolve(identifier.name()) == nullptr) {
                const TypeRef inferredType = analyzeExpression(expression.right());
                if (inferredType == nullptr) {
                    throw SemanticError(
                        "cannot infer type for declaration: " + identifier.name());
                }
                if (isFileType(inferredType)) {
                    requireFreshFile(expression.right());
                }
                if (inferredType->isNone()) {
                    throw SemanticError("the type of None is not known here: write it, " + identifier.name() +
                                        " Link<T> := None or " + identifier.name() + " Option<Own<T>> := None");
                }
                if (isMoveOnlyType(inferredType) && !isFreshOwnerValue(expression.right())) {
                    throw SemanticError("an owner is never copied: give it away with Move(X), or build a new "
                                        "one with New(...)");
                }
                declareOrThrow(
                    identifier.name(),
                    SymbolKind::Variable,
                    inferredType,
                    true);
                trackLocal(identifier.name());
                const Symbol& declared = resolveOrThrow(identifier.name());
                result_.bind(identifier, declared);
                result_.setExpressionType(identifier, declared.type);
                return inferredType;
            }

            const Symbol& target = resolveOrThrow(identifier.name());
            result_.bind(identifier, target);
            result_.setExpressionType(identifier, target.type);
            if (target.kind == SymbolKind::LoopIterator) {
                throw SemanticError("cannot assign to read-only loop iterator: " + identifier.name());
            }
            if ((target.kind != SymbolKind::Variable && target.kind != SymbolKind::State) ||
                !target.isMutable) {
                throw SemanticError("assignment target is not mutable: " + identifier.name());
            }
            if (target.type != nullptr && target.type->isSlice()) {
                throw SemanticError("a slice is bound once and cannot be reassigned: " + identifier.name() +
                                    " (ADR-0017)");
            }
            targetType = target.type;
        } else if (expression.left().kind() == ast::AstNodeKind::CallExpression &&
                   isMemberCall(static_cast<const ast::CallExpression&>(expression.left()))) {
            // OPEN-4: writing a field (`P.X := 10`) is not reading P. The
            // target is analyzed as usual, then the read it recorded for the
            // root variable is withdrawn unless P had been read before.
            const ast::Expression* root = &expression.left();
            while (root->kind() == ast::AstNodeKind::CallExpression &&
                   isMemberCall(static_cast<const ast::CallExpression&>(*root))) {
                root = static_cast<const ast::CallExpression&>(*root).arguments()[0].get();
            }
            const Symbol* rootSymbol = nullptr;
            bool rootWasRead = false;
            if (root->kind() == ast::AstNodeKind::IdentifierExpression) {
                rootSymbol = symbols_.currentScope().resolve(
                    static_cast<const ast::IdentifierExpression&>(*root).name());
                rootWasRead = rootSymbol != nullptr && readSymbols_.contains(rootSymbol);
            }
            targetType = analyzeMemberExpression(
                static_cast<const ast::CallExpression&>(expression.left()));
            if (rootSymbol != nullptr && !rootWasRead) {
                readSymbols_.erase(rootSymbol);
            }
            // Parameters are immutable and `Self` only reads its receiver:
            // a field is written only through a mutable variable, `Self mut`
            // or a `ref mut` parameter (CANON-9, ADR-0016).
            if (writableRoot(expression.left()) == nullptr) {
                throw SemanticError(
                    "assignment target is not mutable: " +
                    (root->kind() == ast::AstNodeKind::IdentifierExpression
                         ? (static_cast<const ast::IdentifierExpression&>(*root).name().rfind("__with", 0) == 0
                                ? std::string("the target of 'with' is not a mutable variable")
                                : static_cast<const ast::IdentifierExpression&>(*root).name()) +
                               (equalsIgnoreCase(static_cast<const ast::IdentifierExpression&>(*root).name(), "Self")
                                    ? " (a method that changes its receiver declares Self mut)"
                                    : "")
                         : std::string("a field of a value that is not a variable")));
            }
            // CANON-24: a write from outside the struct's own methods keeps
            // its invariant (and the invariants of the structs around it).
            recordInvariantPlaces(expression,
                                  *static_cast<const ast::CallExpression&>(expression.left()).arguments()[0], false,
                                  true);
        } else if (isIndexTarget(expression.left())) {
            // CANON-13: `A[I] := X` writes an element. The root must be a
            // mutable variable; writing it is not reading it (OPEN-4).
            const ast::Expression* root = &expression.left();
            while (root->kind() == ast::AstNodeKind::CallExpression) {
                root = static_cast<const ast::CallExpression&>(*root).arguments()[0].get();
            }
            const Symbol* rootSymbol = nullptr;
            bool rootWasRead = false;
            if (root->kind() == ast::AstNodeKind::IdentifierExpression) {
                rootSymbol = symbols_.currentScope().resolve(
                    static_cast<const ast::IdentifierExpression&>(*root).name());
                rootWasRead = rootSymbol != nullptr && readSymbols_.contains(rootSymbol);
            }
            targetType = analyzeExpression(expression.left());
            if (writableRoot(expression.left()) == nullptr) {
                throw SemanticError("array element assignment requires a mutable array variable");
            }
            if (rootSymbol != nullptr && !rootWasRead) {
                readSymbols_.erase(rootSymbol);
            }
            recordInvariantPlaces(expression,
                                  *static_cast<const ast::CallExpression&>(expression.left()).arguments()[0], false,
                                  true);
            // ADR-0017: a slice is a read-only view.
            for (const ast::Expression* base = &expression.left();
                 base->kind() == ast::AstNodeKind::CallExpression;
                 base = static_cast<const ast::CallExpression&>(*base).arguments()[0].get()) {
                const ast::Expression& inner = *static_cast<const ast::CallExpression&>(*base).arguments()[0];
                if (const TypeRef type = result_.typeOf(inner); type != nullptr && type->isSlice()) {
                    throw SemanticError("a slice is read-only: its elements cannot be assigned (ADR-0017)");
                }
            }
        } else if (expression.left().kind() == ast::AstNodeKind::CallExpression &&
                   static_cast<const ast::CallExpression&>(expression.left()).callee().kind() ==
                       ast::AstNodeKind::IdentifierExpression &&
                   equalsIgnoreCase(static_cast<const ast::IdentifierExpression&>(
                                        static_cast<const ast::CallExpression&>(expression.left()).callee())
                                        .name(),
                                    "Value") &&
                   isPointerBuiltinCall("Value", static_cast<const ast::CallExpression&>(expression.left())
                                                     .arguments()
                                                     .size())) {
            // `Value(P) := X` replaces the whole object (safe-pointers).
            targetType = analyzeExpression(expression.left());
        } else {
            throw SemanticError("assignment target must be an identifier, field or array element");
        }

        const TypeRef rightType = analyzeExpressionFor(expression.right(), targetType);
        if (!canAssignValue(targetType, rightType, expression.right())) {
            throw SemanticError("cannot assign " + types::nameOf(rightType) + " to " + types::nameOf(targetType));
        }
        if (expression.left().kind() == ast::AstNodeKind::IdentifierExpression) {
            if (const Symbol* target =
                    result_.symbolOf(static_cast<const ast::IdentifierExpression&>(expression.left()))) {
                noteOwnerAssigned(*target);
            }
        }
        return targetType;
    }

    // CANON-8 (v3.39): Natural is a subtype of Integer; arithmetic and
    // comparisons on it belong to the Integer family (`A - B` may be negative).
    // A Range computes in its base type too (CANON-13).
    const auto integerFamily = [this](TypeRef type) {
        if (type != nullptr && type->isRange()) {
            return type->rangeBase();
        }
        return type == types().naturalType() ? types().int64Type() : type;
    };
    // CANON-13: `X in S` and `X in A..B`; a set literal compared with a set
    // takes its type.
    if (op == ast::BinaryOperator::In) {
        return analyzeMembership(expression);
    }
    if ((isSetLiteral(expression.left()) || isSetLiteral(expression.right())) &&
        (op == ast::BinaryOperator::Equal || op == ast::BinaryOperator::NotEqual ||
         op == ast::BinaryOperator::LessEqual || op == ast::BinaryOperator::GreaterEqual)) {
        const bool literalLeft = isSetLiteral(expression.left());
        const TypeRef other = analyzeExpression(literalLeft ? expression.right() : expression.left());
        analyzeSetOperand(literalLeft ? expression.left() : expression.right(), other);
        return types().boolType();
    }

    // Change uint64: a UInt64 operand makes the other one a UInt64 context,
    // so `U = 18446744073709551615` and `U + 1` read the literal as a UInt64.
    TypeRef leftType = nullptr;
    TypeRef rightType = nullptr;
    if (isLiteralAboveInt64(expression.left()) && !isUInt64Type(integerContext_)) {
        rightType = integerFamily(analyzeExpression(expression.right()));
        leftType = integerFamily(analyzeWithIntegerContext(expression.left(), rightType));
    } else {
        leftType = integerFamily(analyzeExpression(expression.left()));
        rightType = integerFamily(analyzeWithIntegerContext(
            expression.right(), integerContext_ != nullptr ? integerContext_ : leftType));
    }

    // CANON-8 (universal integer literal, as in Ada): a constant Integer
    // operand next to a fixed-width integer is read in that type and must fit
    // it (`B + 1` with B Byte); a value that does not fit is a compile error.
    const auto adaptConstant = [&](TypeRef narrowType, const ast::Expression& other, TypeRef& otherType) {
        if (isUInt64Type(narrowType) && otherType == types().int64Type()) {
            std::int64_t value = 0;
            if (constantIntegerValue(other, value)) {
                if (value < 0) {
                    throw SemanticError("value out of range for UInt64: " + std::to_string(value));
                }
                otherType = narrowType;
            }
            return;
        }
        if (!isNarrowIntegerType(narrowType) || otherType != types().int64Type()) {
            return;
        }
        std::int64_t value = 0;
        std::int64_t low = 0;
        std::int64_t high = 0;
        if (!constantIntegerValue(other, value) || !integerTypeRange(narrowType, low, high)) {
            return;
        }
        if (value < low || value > high) {
            throw SemanticError("value out of range for " + narrowType->name() + ": " + std::to_string(value));
        }
        otherType = narrowType;
    };
    adaptConstant(leftType, expression.right(), rightType);
    adaptConstant(rightType, expression.left(), leftType);

    // Change decimals (r3.91, A2): a numeric literal next to a Currency or a
    // Crypto is read exactly in that type (`3 * Price`); a variable of
    // another type is converted explicitly.
    if (leftType != nullptr && rightType != nullptr && leftType != rightType) {
        if (rightType->isDecimal() && !leftType->isDecimal() && isNumericLiteral(expression.left())) {
            leftType = analyzeWithIntegerContext(expression.left(), rightType);
        } else if (leftType->isDecimal() && !rightType->isDecimal() && isNumericLiteral(expression.right())) {
            rightType = analyzeWithIntegerContext(expression.right(), leftType);
        }
        if (leftType != rightType && (leftType->isDecimal() || rightType->isDecimal()) &&
            (isNumericType(leftType) && isNumericType(rightType))) {
            const TypeRef decimal = leftType->isDecimal() ? leftType : rightType;
            const TypeRef other = leftType->isDecimal() ? rightType : leftType;
            // The names a program writes: Integer and Float.
            const std::string otherName = other == types().int64Type()     ? std::string("Integer")
                                          : other == types().float64Type() ? std::string("Float")
                                                                           : other->name();
            throw SemanticError(
                other->isDecimal()
                    ? leftType->name() + " and " + rightType->name() +
                          " do not mix (CANON-8): write Crypto(X) (exact) or Currency(X) (rounded half to even)"
                    : decimal->name() + " and " + otherName + " do not mix (CANON-8): write " + decimal->name() +
                          "(X) to compute in " + decimal->name() + ", or " + otherName + "(" +
                          decimal->name().substr(0, 1) + ")");
        }
    }
    if (op == ast::BinaryOperator::Power &&
        ((leftType != nullptr && leftType->isDecimal()) || (rightType != nullptr && rightType->isDecimal()))) {
        throw SemanticError("'^' is not defined for " +
                            (leftType != nullptr && leftType->isDecimal() ? leftType : rightType)->name() +
                            " (CANON-8): multiply, or compute in Float");
    }

    const auto requireMatchingTypes = [&] {
        // Change uint64 (Q3): UInt64 and another integer type never mix; the
        // conversion is written.
        if (!typesMatch(leftType, rightType) && (isUInt64Type(leftType) || isUInt64Type(rightType)) &&
            isIntegerType(leftType) && isIntegerType(rightType)) {
            const bool leftUnsigned = isUInt64Type(leftType);
            const TypeRef other = leftUnsigned ? rightType : leftType;
            throw SemanticError("operator '" + std::string(binaryOperatorName(op)) + "' mixes UInt64 and " +
                                other->name() + ": write the conversion, UInt64(X) or " + other->name() +
                                "(U) (a conversion outside the target's range stops the program)");
        }
        if (!typesMatch(leftType, rightType)) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires matching operand types");
        }
    };
    const auto requireNumericOperands = [&] {
        if ((leftType != nullptr && leftType->isEnum()) || (rightType != nullptr && rightType->isEnum())) {
            throw SemanticError("an enum has no arithmetic: Succ(E), Pred(E) and Ord(E) step and measure it "
                                "(CANON-13)");
        }
        if ((leftType != nullptr && leftType->isChar()) || (rightType != nullptr && rightType->isChar())) {
            throw SemanticError("a Char has no arithmetic (CANON-14): compute on its code point, "
                                "Char(Integer(C) + 1)");
        }
        if ((leftType != nullptr && !isNumericType(leftType)) ||
            (rightType != nullptr && !isNumericType(rightType))) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires numeric operands");
        }
        requireMatchingTypes();
    };
    const auto requireIntegerOperands = [&] {
        if ((leftType != nullptr && !isIntegerType(leftType)) ||
            (rightType != nullptr && !isIntegerType(rightType))) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires integer operands");
        }
        requireMatchingTypes();
    };

    // CANON-14 (std-strings): `+` joins two Strings, never a String and a
    // value of another type (no implicit conversion).
    if (op == ast::BinaryOperator::Add && ((leftType != nullptr && leftType->isString()) ||
                                           (rightType != nullptr && rightType->isString()))) {
        const TypeRef other = leftType != nullptr && leftType->isString() ? rightType : leftType;
        if (other == nullptr || !other->isString()) {
            throw SemanticError("'+' joins two Strings (CANON-14), and the other operand is " +
                                types::nameOf(other) + ": there is no implicit conversion to String");
        }
        return types().stringType();
    }

    switch (op) {
    case ast::BinaryOperator::Add:
    case ast::BinaryOperator::Subtract:
    case ast::BinaryOperator::Multiply:
    case ast::BinaryOperator::Power:
        requireNumericOperands();
        return leftType;
    case ast::BinaryOperator::Divide:
        // r3.95, as in Ada: `/` of two integers is the integer division,
        // truncated toward zero; of two reals or two decimals, theirs. The
        // operands are never mixed.
        if (isIntegerType(leftType) && isIntegerType(rightType)) {
            requireIntegerOperands();
            return leftType;
        }
        requireNumericOperands();
        return leftType;
    // r3.96, as in Ada: `mod` has the sign of the divisor, `rem` the sign
    // of the dividend (A = (A / B) * B + A rem B).
    case ast::BinaryOperator::Modulo:
    case ast::BinaryOperator::Remainder:
    case ast::BinaryOperator::ShiftLeft:
    case ast::BinaryOperator::ShiftRight:
    case ast::BinaryOperator::BitAnd:
    case ast::BinaryOperator::BitXor:
    case ast::BinaryOperator::BitOr:
        requireIntegerOperands();
        return leftType;
    case ast::BinaryOperator::Equal:
    case ast::BinaryOperator::NotEqual:
    case ast::BinaryOperator::Less:
    case ast::BinaryOperator::Greater:
    case ast::BinaryOperator::LessEqual:
    case ast::BinaryOperator::GreaterEqual:
        // `1 < X < 10` reads as `(1 < X) < 10`, a Bool against a number:
        // comparisons are not chained as in mathematics (CANON-20).
        if (expression.left().kind() == ast::AstNodeKind::BinaryExpression && leftType != nullptr &&
            leftType->isBool() && rightType != nullptr && !rightType->isBool()) {
            const ast::BinaryOperator inner = static_cast<const ast::BinaryExpression&>(expression.left()).op();
            if (inner == ast::BinaryOperator::Equal || inner == ast::BinaryOperator::NotEqual ||
                inner == ast::BinaryOperator::Less || inner == ast::BinaryOperator::Greater ||
                inner == ast::BinaryOperator::LessEqual || inner == ast::BinaryOperator::GreaterEqual) {
                throw SemanticError("comparisons are not chained: A < B < C compares the Bool A < B with C; "
                                    "write A < B and B < C, or B in A..C");
            }
        }
        if ((leftType != nullptr && (leftType->isPointer() || leftType->isNone())) ||
            (rightType != nullptr && (rightType->isPointer() || rightType->isNone()))) {
            throw SemanticError("pointers are not compared with '" + std::string(binaryOperatorName(op)) +
                                "': Same(A, B) asks whether they reach one object, Alive(X) whether it "
                                "exists, and Value(A) = Value(B) compares the values");
        }
        requireMatchingTypes();
        if (leftType != nullptr && (leftType->isArray() || leftType->isSlice())) {
            throw SemanticError("arrays and slices cannot be compared with '" +
                                std::string(binaryOperatorName(op)) + "' (CANON-13)");
        }
        // CANON-13: a set is a subset (<=) or a superset (>=) of another; it
        // has no order.
        if (leftType != nullptr && leftType->isSet() &&
            (op == ast::BinaryOperator::Less || op == ast::BinaryOperator::Greater)) {
            throw SemanticError("sets are compared with <= (subset) and >= (superset), not '" +
                                std::string(binaryOperatorName(op)) + "' (CANON-13)");
        }
        // v3.57: numbers, Strings, Char and Bool (false < true) are compared
        // and ordered; structs are never compared, as in Pascal.
        if (leftType != nullptr && leftType->isStruct()) {
            throw SemanticError("structs cannot be compared with '" + std::string(binaryOperatorName(op)) +
                                "'; compare their fields");
        }
        if (isFileType(leftType) || isFileType(rightType)) {
            throw SemanticError("Files cannot be compared with '" + std::string(binaryOperatorName(op)) + "'");
        }
        return types().boolType();
    case ast::BinaryOperator::And:
    case ast::BinaryOperator::Xor:
    case ast::BinaryOperator::Or:
        requireMatchingTypes();
        if ((leftType != nullptr && !leftType->isBool()) ||
            (rightType != nullptr && !rightType->isBool())) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires Bool operands");
        }
        return types().boolType();
    case ast::BinaryOperator::Range:
        // `Club..Heart`: a range of literals of one enum (CANON-13).
        if (leftType != nullptr && leftType->isEnum() && leftType == rightType) {
            return leftType;
        }
        // `'a'..'z'`: a range of Chars (change char-values), of the Range
        // type when both ends have it.
        if (leftType != nullptr && rightType != nullptr && leftType->isChar() && rightType->isChar()) {
            return leftType == rightType ? leftType : types().charType();
        }
        if ((leftType != nullptr && leftType->isChar()) || (rightType != nullptr && rightType->isChar())) {
            throw SemanticError("a range of Chars has two Chars, 'a'..'z' (CANON-14)");
        }
        requireIntegerOperands();
        return leftType;
    case ast::BinaryOperator::In:
        return types().boolType();
    case ast::BinaryOperator::Assign:
        break;
    }

    return nullptr;
}

TypeRef SemanticAnalyzer::analyzeUnaryExpression(const ast::UnaryExpression& expression)
{
    // `-9223372036854775808` is the one literal whose magnitude only fits when
    // it is directly negated.
    if (expression.op() == ast::UnaryOperator::Minus &&
        expression.operand().kind() == ast::AstNodeKind::LiteralExpression) {
        const auto& literal = static_cast<const ast::LiteralExpression&>(expression.operand());
        std::uint64_t magnitude = 0;
        if (literal.literalKind() == ast::LiteralKind::Integer &&
            parseIntegerLiteralMagnitude(literal.value(), magnitude) &&
            magnitude == kInt64MagnitudeOfMin) {
            result_.setExpressionType(literal, types().int64Type());
            constants_[&expression] = kInt64Min;
            return types().int64Type();
        }
    }
    const TypeRef operandType = analyzeExpression(expression.operand());

    switch (expression.op()) {
    case ast::UnaryOperator::Plus:
    case ast::UnaryOperator::Minus:
        if (operandType != nullptr && !isNumericType(operandType)) {
            throw SemanticError("unary arithmetic operator requires a numeric operand");
        }
        // Change uint64 (Q2): no signed type holds every negated UInt64.
        if (expression.op() == ast::UnaryOperator::Minus && isUInt64Type(operandType)) {
            throw SemanticError("unary minus is not defined for UInt64: write -Integer(U), which stops the "
                                "program when U is above Int64.Max");
        }
        // CANON-8: negating a Natural or a Range leaves the subtype; the
        // result is Integer.
        if (expression.op() == ast::UnaryOperator::Minus && operandType == types().naturalType()) {
            return types().int64Type();
        }
        if (operandType != nullptr && operandType->isRange()) {
            return operandType->rangeBase();
        }
        return operandType;
    case ast::UnaryOperator::Not:
        if (operandType != nullptr && !operandType->isBool()) {
            throw SemanticError("operator 'not' requires a Bool operand");
        }
        return types().boolType();
    case ast::UnaryOperator::BitNot:
        if (operandType != nullptr && !isIntegerType(operandType)) {
            throw SemanticError("operator 'bitnot' requires an integer operand");
        }
        if (operandType == types().naturalType()) {
            return types().int64Type();
        }
        if (operandType != nullptr && operandType->isRange()) {
            return operandType->rangeBase();
        }
        return operandType;
    case ast::UnaryOperator::Old:
        // The contract pass turns every `old X` of a `pos` into a value
        // saved on entry; one left here is outside a pos (CANON-24).
        throw SemanticError("'old' is valid only in a 'pos' clause (CANON-24)");
    }

    return nullptr;
}

bool SemanticAnalyzer::looksLikeIdentifier(std::string_view text)
{
    if (text.empty()) {
        return false;
    }
    const auto isIdentifierStart = [](char ch) {
        return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
    };
    const auto isIdentifierPart = [](char ch) {
        return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
    };

    if (!isIdentifierStart(text.front())) {
        return false;
    }
    for (char ch : text.substr(1)) {
        if (!isIdentifierPart(ch)) {
            return false;
        }
    }
    return true;
}

bool SemanticAnalyzer::isMemberCall(const ast::CallExpression& expression)
{
    if (expression.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
        return false;
    }
    const auto& callee = static_cast<const ast::IdentifierExpression&>(expression.callee());
    return equalsIgnoreCase(callee.name(), "__member");
}

bool SemanticAnalyzer::isSideEffectFree(const ast::Expression& expression) const
{
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression:
        return true;
    case ast::AstNodeKind::IdentifierExpression: {
        const Symbol* symbol =
            symbols_.currentScope().resolve(static_cast<const ast::IdentifierExpression&>(expression).name());
        return symbol == nullptr || symbol->kind != SymbolKind::Function;
    }
    case ast::AstNodeKind::UnaryExpression:
        return isSideEffectFree(static_cast<const ast::UnaryExpression&>(expression).operand());
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        return isSideEffectFree(binary.left()) && isSideEffectFree(binary.right());
    }
    case ast::AstNodeKind::CallExpression: {
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
            return false;
        }
        const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
        if (name == "__member") {
            return isSideEffectFree(*call.arguments()[0]);
        }
        if (name != "__index") {
            return false;
        }
        return isSideEffectFree(*call.arguments()[0]) && isSideEffectFree(*call.arguments()[1]);
    }
    default:
        return false;
    }
}

// The variable a writable place is rooted at (`X`, `P.F`, `A[I]`), when the
// place can be written: a mutable local, a State variable or a `ref mut`
// parameter. Null otherwise.
const Symbol* SemanticAnalyzer::writableRoot(const ast::Expression& expression) const
{
    const ast::Expression* root = &expression;
    while (root->kind() == ast::AstNodeKind::CallExpression) {
        const auto& call = static_cast<const ast::CallExpression&>(*root);
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().empty()) {
            return nullptr;
        }
        const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
        // The object of a pointer is written through it (safe-pointers).
        if (equalsIgnoreCase(name, "Value") && call.arguments().size() == 1 &&
            isPointerBuiltinCall(name, 1)) {
            return &pointerTargetRoot_;
        }
        if (name != "__index" && name != "__member") {
            return nullptr;
        }
        root = call.arguments()[0].get();
        if (const TypeRef baseType = result_.typeOf(*root);
            baseType != nullptr && (baseType->isOwn() || baseType->isLink())) {
            return &pointerTargetRoot_;
        }
    }
    if (root->kind() != ast::AstNodeKind::IdentifierExpression) {
        return nullptr;
    }
    const Symbol* symbol =
        symbols_.currentScope().resolve(static_cast<const ast::IdentifierExpression&>(*root).name());
    if (symbol == nullptr || !symbol->isMutable ||
        (symbol->kind != SymbolKind::Variable && symbol->kind != SymbolKind::State)) {
        return nullptr;
    }
    return symbol;
}

} // namespace inox::compiler::semantic
