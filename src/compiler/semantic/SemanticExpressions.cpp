// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: expression typing: operators, indexing, slices, members, side effects.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"
#include "../exceptions/ExceptionTypes.h"

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

TypeRef SemanticAnalyzer::analyzeExpression(const ast::Expression& expression)
{
    const TypeRef type = inferExpressionType(expression);
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
            if (!parseIntegerLiteralMagnitude(integerLiteral.value(), magnitude) ||
                magnitude >= kInt64MagnitudeOfMin) {
                throw SemanticError("integer literal out of range for Int64: " +
                                    std::string(integerLiteral.value()));
            }
            return types().int64Type();
        }
        case ast::LiteralKind::Float:
            return types().float64Type();
        case ast::LiteralKind::String:
            return types().stringType();
        case ast::LiteralKind::Char:
            return types().charType();
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
        const TypeRef conditionType = analyzeExpression(conditional.condition());
        if (conditionType == nullptr || !conditionType->isBool()) {
            throw SemanticError("the condition of a conditional expression must be Bool, got " +
                                (conditionType == nullptr ? std::string("<unknown>") : conditionType->name()));
        }
        const TypeRef thenType = analyzeExpression(conditional.thenValue());
        const TypeRef elseType = analyzeExpression(conditional.elseValue());
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
    const auto integerFamily = [this](TypeRef type) {
        return type == types().naturalType() ? types().int64Type() : type;
    };
    TypeRef leftType = integerFamily(analyzeExpression(expression.left()));
    TypeRef rightType = integerFamily(analyzeExpression(expression.right()));

    // CANON-8 (universal integer literal, as in Ada): a constant Integer
    // operand next to a fixed-width integer is read in that type and must fit
    // it (`B + 1` with B Byte); a value that does not fit is a compile error.
    const auto adaptConstant = [&](TypeRef narrowType, const ast::Expression& other, TypeRef& otherType) {
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

    const auto requireMatchingTypes = [&] {
        if (!typesMatch(leftType, rightType)) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires matching operand types");
        }
    };
    const auto requireNumericOperands = [&] {
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
        if (isIntegerType(leftType) && isIntegerType(rightType)) {
            throw SemanticError("operator '/' is not supported for Integer operands; use 'div' for integer division");
        }
        requireNumericOperands();
        return leftType;
    case ast::BinaryOperator::IntegerDivide:
    case ast::BinaryOperator::Modulo:
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
        // CANON-8: negating a Natural leaves the subtype; the result is Integer.
        if (expression.op() == ast::UnaryOperator::Minus && operandType == types().naturalType()) {
            return types().int64Type();
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
