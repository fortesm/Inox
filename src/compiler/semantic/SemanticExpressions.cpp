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
#include "../support/ArrayType.h"

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

std::string SemanticAnalyzer::analyzeExpression(const ast::Expression& expression)
{
    std::string typeName = inferExpressionType(expression);
    result_.setExpressionType(expression, resolvedType(typeName));
    foldConstantExpression(expression);
    // CANON-8: constant overflow is a compile-time error in every integer type.
    const std::string canonical = canonicalTypeName(typeName);
    if (isNarrowIntegerType(canonical)) {
        std::int64_t value = 0;
        std::int64_t low = 0;
        std::int64_t high = 0;
        if (constantIntegerValue(expression, value) && integerTypeRange(canonical, low, high) &&
            (value < low || value > high)) {
            throw SemanticError("constant overflow: " + std::to_string(value) + " does not fit " +
                                canonical);
        }
    }
    return typeName;
}

std::string SemanticAnalyzer::inferExpressionType(const ast::Expression& expression)
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
            return "Int64";
        }
        case ast::LiteralKind::Float:
            return "Float64";
        case ast::LiteralKind::String:
            return "String";
        case ast::LiteralKind::Char:
            return "Char";
        case ast::LiteralKind::Boolean:
            return "Bool";
        }
        break;
    }
    case ast::AstNodeKind::IdentifierExpression: {
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
        if (equalsIgnoreCase(identifier.name(), "Get") || equalsIgnoreCase(identifier.name(), "GetLn")) {
            const Symbol& symbol = resolveOrThrow(identifier.name());
            result_.bind(identifier, symbol);
            return "Void";
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
        const Symbol& symbol = resolveOrThrow(identifier.name());
        result_.bind(identifier, symbol);
        if (!updateOnlyReads_.contains(&identifier)) {
            readSymbols_.insert(&symbol);
        }
        return symbol.typeName;
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
        const std::string conditionType =
            canonicalTypeName(analyzeExpression(conditional.condition()));
        if (conditionType != "Bool") {
            throw SemanticError("the condition of a conditional expression must be Bool, got " +
                                (conditionType.empty() ? std::string("<unknown>") : conditionType));
        }
        const std::string thenType = analyzeExpression(conditional.thenValue());
        const std::string elseType = analyzeExpression(conditional.elseValue());
        if (!typesMatch(thenType, elseType)) {
            throw SemanticError("the branches of a conditional expression have different types: " +
                                thenType + " and " + elseType + " (ADR-0013)");
        }
        return thenType;
    }
    default:
        break;
    }

    return {};
}

// CANON-13: `A[I]` reads element I of a fixed array. I is an integer; a
// constant index outside Low..High is a compile-time error, any other index
// is checked at run time.
std::string SemanticAnalyzer::analyzeIndexExpression(const ast::CallExpression& call)
{
    const std::string baseType = canonicalTypeName(analyzeExpression(*call.arguments()[0]));
    if (support::isSliceTypeName(baseType)) {
        const std::string indexType = canonicalTypeName(analyzeExpression(*call.arguments()[1]));
        if (!isIntegerType(indexType)) {
            throw SemanticError("an array index must be an integer, not " + indexType);
        }
        return support::sliceElementTypeName(baseType);
    }
    const auto parts = support::splitArrayTypeName(baseType);
    if (!parts) {
        throw SemanticError("only an array can be indexed (CANON-13); this value has type " + baseType);
    }
    const std::string indexType = canonicalTypeName(analyzeExpression(*call.arguments()[1]));
    if (!isIntegerType(indexType)) {
        throw SemanticError("an array index must be an integer, not " + indexType);
    }
    std::int64_t index = 0;
    const std::int64_t low = std::stoll(parts->low);
    const std::int64_t high = std::stoll(parts->high);
    if (constantIntegerValue(*call.arguments()[1], index) && (index < low || index > high)) {
        throw SemanticError("array index out of bounds: " + std::to_string(index) + " is not in " +
                            parts->low + ".." + parts->high);
    }
    return parts->element;
}

// ADR-0017: `A[L..H]` is a read-only view of elements L..H of a named array
// or slice, keeping their indices. An omitted bound is the base's own.
std::string SemanticAnalyzer::analyzeSliceExpression(const ast::CallExpression& call)
{
    const ast::Expression& base = *call.arguments()[0];
    const std::string baseType = canonicalTypeName(analyzeExpression(base));
    std::string element;
    std::optional<std::pair<std::int64_t, std::int64_t>> staticBounds;
    if (support::isSliceTypeName(baseType)) {
        element = support::sliceElementTypeName(baseType);
    } else if (const auto parts = support::splitArrayTypeName(baseType)) {
        element = parts->element;
        staticBounds = std::make_pair(std::stoll(parts->low), std::stoll(parts->high));
    } else {
        throw SemanticError("only an array or a slice can be sliced (ADR-0017); this value has type " + baseType);
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
        const std::string boundType = canonicalTypeName(analyzeExpression(bound));
        if (!isIntegerType(boundType)) {
            throw SemanticError("a slice bound must be an integer, not " + boundType);
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
    return support::makeSliceTypeName(element);
}

void SemanticAnalyzer::requireBoolCondition(const ast::Expression& expression)
{
    const std::string typeName = analyzeExpression(expression);
    if (!typeName.empty() && typeName != "Bool") {
        throw SemanticError("condition must be Bool");
    }
}

std::string SemanticAnalyzer::analyzeBinaryExpression(const ast::BinaryExpression& expression)
{
    const ast::BinaryOperator op = expression.op();

    if (op == ast::BinaryOperator::Assign) {
        std::string targetType;

        if (expression.left().kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression.left());

            // CANON-5 / A6 + A7: first appearance of a name is a DECLARATION.
            // `Name := Expr` where Name is not yet visible in any scope is an
            // inline declaration with the type INFERRED from the initializer
            // (Ada/SPARK safe inference). A subsequent `Name := Expr` is a plain
            // assignment, handled by the existing path below.
            if (symbols_.currentScope().resolve(identifier.name()) == nullptr) {
                const std::string inferredType = analyzeExpression(expression.right());
                if (inferredType.empty()) {
                    throw SemanticError(
                        "cannot infer type for declaration: " + identifier.name());
                }
                if (isFileType(inferredType)) {
                    requireFreshFile(expression.right());
                }
                declareOrThrow(
                    identifier.name(),
                    SymbolKind::Variable,
                    inferredType,
                    true);
                trackLocal(identifier.name());
                const Symbol& declared = resolveOrThrow(identifier.name());
                result_.bind(identifier, declared);
                result_.setExpressionType(identifier, resolvedType(declared.typeName));
                return inferredType;
            }

            const Symbol& target = resolveOrThrow(identifier.name());
            result_.bind(identifier, target);
            result_.setExpressionType(identifier, resolvedType(target.typeName));
            if (target.kind == SymbolKind::LoopIterator) {
                throw SemanticError("cannot assign to read-only loop iterator: " + identifier.name());
            }
            if ((target.kind != SymbolKind::Variable && target.kind != SymbolKind::State) ||
                !target.isMutable) {
                throw SemanticError("assignment target is not mutable: " + identifier.name());
            }
            if (support::isSliceTypeName(target.typeName)) {
                throw SemanticError("a slice is bound once and cannot be reassigned: " + identifier.name() +
                                    " (ADR-0017)");
            }
            targetType = target.typeName;
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
            if (rootSymbol == nullptr ||
                (rootSymbol->kind != SymbolKind::Variable && rootSymbol->kind != SymbolKind::State) ||
                !rootSymbol->isMutable) {
                throw SemanticError("array element assignment requires a mutable array variable");
            }
            targetType = analyzeExpression(expression.left());
            if (!rootWasRead) {
                readSymbols_.erase(rootSymbol);
            }
            // ADR-0017: a slice is a read-only view.
            for (const ast::Expression* base = &expression.left();
                 base->kind() == ast::AstNodeKind::CallExpression;
                 base = static_cast<const ast::CallExpression&>(*base).arguments()[0].get()) {
                const ast::Expression& inner = *static_cast<const ast::CallExpression&>(*base).arguments()[0];
                if (const ResolvedType* type = result_.typeOf(inner);
                    type != nullptr && support::isSliceTypeName(type->name)) {
                    throw SemanticError("a slice is read-only: its elements cannot be assigned (ADR-0017)");
                }
            }
        } else {
            throw SemanticError("assignment target must be an identifier, field or array element");
        }

        const std::string rightType = analyzeExpression(expression.right());
        if (!canAssignValue(targetType, rightType, expression.right())) {
            throw SemanticError("cannot assign " + rightType + " to " + targetType);
        }
        return targetType;
    }

    // CANON-8 (v3.39): Natural is a subtype of Integer; arithmetic and
    // comparisons on it belong to the Integer family (`A - B` may be negative).
    const auto integerFamily = [](std::string type) {
        return type == "Natural" ? std::string("Int64") : type;
    };
    std::string leftType = integerFamily(analyzeExpression(expression.left()));
    std::string rightType = integerFamily(analyzeExpression(expression.right()));

    // CANON-8 (universal integer literal, as in Ada): a constant Integer
    // operand next to a fixed-width integer is read in that type and must fit
    // it (`B + 1` with B Byte); a value that does not fit is a compile error.
    const auto adaptConstant = [&](const std::string& narrowType, const ast::Expression& other,
                                   std::string& otherType) {
        const std::string canonical = canonicalTypeName(narrowType);
        if (!isNarrowIntegerType(canonical) || otherType != "Int64") {
            return;
        }
        std::int64_t value = 0;
        std::int64_t low = 0;
        std::int64_t high = 0;
        if (!constantIntegerValue(other, value) || !integerTypeRange(canonical, low, high)) {
            return;
        }
        if (value < low || value > high) {
            throw SemanticError("value out of range for " + canonical + ": " + std::to_string(value));
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
        if ((!leftType.empty() && !isNumericType(leftType)) ||
            (!rightType.empty() && !isNumericType(rightType))) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires numeric operands");
        }
        requireMatchingTypes();
    };
    const auto requireIntegerOperands = [&] {
        if ((!leftType.empty() && !isIntegerType(leftType)) ||
            (!rightType.empty() && !isIntegerType(rightType))) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires integer operands");
        }
        requireMatchingTypes();
    };

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
        requireMatchingTypes();
        if (support::isArrayTypeName(leftType) || support::isSliceTypeName(leftType)) {
            throw SemanticError("arrays and slices cannot be compared with '" +
                                std::string(binaryOperatorName(op)) + "' (CANON-13)");
        }
        // v3.57: numbers, Strings, Char and Bool (false < true) are compared
        // and ordered; structs are never compared, as in Pascal.
        if (!leftType.empty() && resolveStruct(leftType) != nullptr) {
            throw SemanticError("structs cannot be compared with '" + std::string(binaryOperatorName(op)) +
                                "'; compare their fields");
        }
        if (isFileType(leftType) || isFileType(rightType)) {
            throw SemanticError("Files cannot be compared with '" + std::string(binaryOperatorName(op)) + "'");
        }
        return "Bool";
    case ast::BinaryOperator::And:
    case ast::BinaryOperator::Xor:
    case ast::BinaryOperator::Or:
        requireMatchingTypes();
        if ((!leftType.empty() && leftType != "Bool") ||
            (!rightType.empty() && rightType != "Bool")) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires Bool operands");
        }
        return "Bool";
    case ast::BinaryOperator::Range:
        requireIntegerOperands();
        return leftType;
    case ast::BinaryOperator::In:
        return "Bool";
    case ast::BinaryOperator::Assign:
        break;
    }

    return {};
}

std::string SemanticAnalyzer::analyzeUnaryExpression(const ast::UnaryExpression& expression)
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
            result_.setExpressionType(literal, resolvedType("Int64"));
            constants_[&expression] = kInt64Min;
            return "Int64";
        }
    }
    const std::string operandType = analyzeExpression(expression.operand());

    switch (expression.op()) {
    case ast::UnaryOperator::Plus:
    case ast::UnaryOperator::Minus:
        if (!operandType.empty() && !isNumericType(operandType)) {
            throw SemanticError("unary arithmetic operator requires a numeric operand");
        }
        // CANON-8: negating a Natural leaves the subtype; the result is Integer.
        if (expression.op() == ast::UnaryOperator::Minus && operandType == "Natural") {
            return "Int64";
        }
        return operandType;
    case ast::UnaryOperator::Not:
        if (!operandType.empty() && operandType != "Bool") {
            throw SemanticError("operator 'not' requires a Bool operand");
        }
        return "Bool";
    case ast::UnaryOperator::BitNot:
        if (!operandType.empty() && !isIntegerType(operandType)) {
            throw SemanticError("operator 'bitnot' requires an integer operand");
        }
        if (operandType == "Natural") {
            return "Int64";
        }
        return operandType;
    }

    return {};
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
        if (name != "__index" && name != "__member") {
            return nullptr;
        }
        root = call.arguments()[0].get();
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
