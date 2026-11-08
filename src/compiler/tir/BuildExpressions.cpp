// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


// Phase 5b: expressions and calls. A call becomes what its recorded meaning
// says (CallMeaning); an identifier a place, a constant or a call.

#include "Builder.h"

#include <cstdio>
#include <string>

namespace inox::compiler::tir::detail {

BinaryOp binaryOp(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::Add: return BinaryOp::Add;
    case ast::BinaryOperator::Subtract: return BinaryOp::Subtract;
    case ast::BinaryOperator::Multiply: return BinaryOp::Multiply;
    case ast::BinaryOperator::Divide: return BinaryOp::Divide;
    case ast::BinaryOperator::Modulo: return BinaryOp::Modulo;
    case ast::BinaryOperator::Remainder: return BinaryOp::Remainder;
    case ast::BinaryOperator::ShiftLeft: return BinaryOp::ShiftLeft;
    case ast::BinaryOperator::ShiftRight: return BinaryOp::ShiftRight;
    case ast::BinaryOperator::BitAnd: return BinaryOp::BitAnd;
    case ast::BinaryOperator::BitXor: return BinaryOp::BitXor;
    case ast::BinaryOperator::BitOr: return BinaryOp::BitOr;
    case ast::BinaryOperator::Power: return BinaryOp::Power;
    case ast::BinaryOperator::Range: return BinaryOp::Range;
    case ast::BinaryOperator::In: return BinaryOp::In;
    case ast::BinaryOperator::Equal: return BinaryOp::Equal;
    case ast::BinaryOperator::NotEqual: return BinaryOp::NotEqual;
    case ast::BinaryOperator::Less: return BinaryOp::Less;
    case ast::BinaryOperator::Greater: return BinaryOp::Greater;
    case ast::BinaryOperator::LessEqual: return BinaryOp::LessEqual;
    case ast::BinaryOperator::GreaterEqual: return BinaryOp::GreaterEqual;
    case ast::BinaryOperator::And: return BinaryOp::And;
    case ast::BinaryOperator::Xor: return BinaryOp::Xor;
    case ast::BinaryOperator::Or: return BinaryOp::Or;
    case ast::BinaryOperator::Assign: break;
    }
    return BinaryOp::Add;
}

const Expr* Builder::expression(const ast::Expression& source)
{
    Expr* result = nullptr;
    switch (source.kind()) {
    case ast::AstNodeKind::LiteralExpression: {
        const auto& literal = static_cast<const ast::LiteralExpression&>(source);
        result = make(ExprKind::Literal, typeOf(source), source);
        result->text = arena_.text(literal.value());
        switch (literal.literalKind()) {
        case ast::LiteralKind::Integer: result->literal = LiteralKind::Integer; break;
        case ast::LiteralKind::Float: result->literal = LiteralKind::Real; break;
        case ast::LiteralKind::String: result->literal = LiteralKind::String; break;
        case ast::LiteralKind::Char: result->literal = LiteralKind::Char; break;
        case ast::LiteralKind::Boolean:
            result->literal = LiteralKind::Bool;
            result->boolean = !literal.value().empty() && (literal.value()[0] == 't' || literal.value()[0] == 'T');
            break;
        }
        break;
    }
    case ast::AstNodeKind::IdentifierExpression:
        return checked(source, identifier(static_cast<const ast::IdentifierExpression&>(source)));
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(source);
        if (binary.op() == ast::BinaryOperator::Assign) {
            fail("an assignment inside an expression", source);
        }
        result = make(ExprKind::Binary, typeOf(source), source);
        result->binary = binaryOp(binary.op());
        result->operands = arena_.array<const Expr*>(2);
        result->operands[0] = expression(binary.left());
        result->operands[1] = expression(binary.right());
        break;
    }
    case ast::AstNodeKind::UnaryExpression: {
        const auto& unary = static_cast<const ast::UnaryExpression&>(source);
        result = make(ExprKind::Unary, typeOf(source), source);
        switch (unary.op()) {
        case ast::UnaryOperator::Plus: result->unary = UnaryOp::Plus; break;
        case ast::UnaryOperator::Minus: result->unary = UnaryOp::Minus; break;
        case ast::UnaryOperator::Not: result->unary = UnaryOp::Not; break;
        case ast::UnaryOperator::BitNot: result->unary = UnaryOp::BitNot; break;
        case ast::UnaryOperator::Old: fail("`old` outside the contract lowering", source);
        }
        result->operands = arena_.array<const Expr*>(1);
        result->operands[0] = expression(unary.operand());
        break;
    }
    case ast::AstNodeKind::CallExpression: {
        const CallMeaning* meaning = semantics_.callMeaning(source);
        if (meaning == nullptr) {
            fail("a call the analysis gave no meaning", source);
        }
        return checked(source, call(static_cast<const ast::CallExpression&>(source), *meaning));
    }
    case ast::AstNodeKind::ConditionalExpression: {
        const auto& conditional = static_cast<const ast::ConditionalExpression&>(source);
        result = make(ExprKind::Conditional, typeOf(source), source);
        result->operands = arena_.array<const Expr*>(3);
        result->operands[0] = expression(conditional.condition());
        result->operands[1] = expression(conditional.thenValue());
        result->operands[2] = expression(conditional.elseValue());
        break;
    }
    default:
        fail("an expression of this kind", source);
    }
    return checked(source, result);
}

// The checks the analysis asked for on the way into a place.
const Expr* Builder::checked(const ast::Expression& source, const Expr* value)
{
    if (const TypeRef range = semantics_.rangeCheck(source)) {
        Expr* check = make(ExprKind::RangeCheck, range, source);
        check->operands = arena_.array<const Expr*>(1);
        check->operands[0] = value;
        value = check;
    }
    if (semantics_.needsNaturalCheck(source)) {
        Expr* check = make(ExprKind::NaturalCheck, semantics_.types().naturalType(), source);
        check->operands = arena_.array<const Expr*>(1);
        check->operands[0] = value;
        value = check;
    }
    return value;
}

const Expr* Builder::identifier(const ast::IdentifierExpression& source)
{
    if (const CallMeaning* meaning = semantics_.callMeaning(source)) {
        // A call without parentheses: `GetLn`, `EndOfInput`, `ClearScreen`, `Report`.
        Expr* result = nullptr;
        switch (meaning->kind) {
        case CallKind::Routine:
            result = make(ExprKind::Call, nullptr, source);
            result->routine = routineOf(meaning->routine, source);
            break;
        case CallKind::Builtin:
            result = make(ExprKind::Builtin, nullptr, source);
            result->op = meaning->op;
            break;
        case CallKind::Intrinsic:
            result = make(ExprKind::Intrinsic, nullptr, source);
            result->intrinsic = meaning->intrinsic;
            break;
        default:
            fail("an identifier with this call meaning", source);
        }
        const TypeRef type = typeOf(source);
        result->type = type != nullptr ? type : semantics_.types().voidType();
        result->symbol = semantics_.symbolOf(source);
        return result;
    }
    const semantic::Symbol* symbol = semantics_.symbolOf(source);
    if (symbol == nullptr) {
        const TypeRef type = typeOf(source);
        if (type != nullptr && type->isNone()) {
            Expr* none = make(ExprKind::Literal, type, source);
            none->literal = LiteralKind::None;
            return none;
        }
        fail("an identifier without a symbol", source);
    }
    switch (symbol->kind) {
    case semantic::SymbolKind::Constant: {
        const semantic::ConstantValue* value = semantics_.constantValueOf(*symbol);
        if (value == nullptr) {
            unsupported("a constant without a value", source);
        }
        const TypeRef type = typeOf(source);
        Expr* literal = const_cast<Expr*>(constantLiteral(*value, type != nullptr ? type : symbol->type,
                                                          source.position()));
        literal->symbol = symbol;
        literal->constant = value;
            return literal;
    }
    case semantic::SymbolKind::Type: {
        const TypeRef type = typeOf(source);
        Expr* result = make(ExprKind::TypeValue, type != nullptr ? type : symbol->type, source);
        result->symbol = symbol;
        return result;
    }
    case semantic::SymbolKind::Variable:
    case semantic::SymbolKind::LoopIterator:
    case semantic::SymbolKind::State: {
        const Place* read = variable(source, *symbol);
        const TypeRef type = typeOf(source);
        Expr* result = make(ExprKind::Read, type != nullptr ? type : read->type, source);
        result->place = read;
        return result;
    }
    default:
        fail("an identifier of this kind", source);
    }
}

const Expr* Builder::constantLiteral(const semantic::ConstantValue& value, TypeRef type, ast::SourcePosition position)
{
    Expr* result = arena_.make<Expr>();
    result->kind = ExprKind::Literal;
    result->type = type;
    result->position = position;
    switch (value.kind) {
    case semantic::ConstantValue::Kind::Integer:
        result->literal = LiteralKind::Integer;
        result->text = arena_.text(std::to_string(value.integer));
        break;
    case semantic::ConstantValue::Kind::Boolean:
        result->literal = LiteralKind::Bool;
        result->boolean = value.boolean;
        result->text = value.boolean ? "true" : "false";
        break;
    case semantic::ConstantValue::Kind::String:
        result->literal = LiteralKind::String;
        result->text = arena_.text(value.text);
        break;
    case semantic::ConstantValue::Kind::Real: {
        result->literal = LiteralKind::Real;
        if (!value.text.empty()) {
            result->text = arena_.text(value.text);
        } else {
            char buffer[40];
            std::snprintf(buffer, sizeof buffer, "%.17g", value.real);
            result->text = arena_.text(buffer);
        }
        break;
    }
    case semantic::ConstantValue::Kind::Decimal:
        result->literal = LiteralKind::Decimal;
        result->text = arena_.text(value.text);
        break;
    }
    return result;
}

Span<const Expr*> Builder::expressions(const std::vector<ast::ExpressionPtr>& sources, std::size_t first,
                                       const ast::Expression* receiver)
{
    const std::size_t extra = receiver != nullptr ? 1 : 0;
    Span<const Expr*> result = arena_.array<const Expr*>(sources.size() - first + extra);
    if (receiver != nullptr) {
        result[0] = expression(*receiver);
    }
    for (std::size_t index = first; index < sources.size(); ++index) {
        result[index - first + extra] = expression(*sources[index]);
    }
    return result;
}

const Routine* Builder::routineOf(const semantic::FunctionSignature* signature, const ast::AstNode& site)
{
    if (signature == nullptr || signature->declaration == nullptr) {
        fail("a call of a routine without a declaration", site);
    }
    const auto found = routines_.find(signature->declaration);
    if (found == routines_.end()) {
        fail("a call of a routine that is not compiled", site);
    }
    return found->second;
}

const Expr* Builder::call(const ast::CallExpression& source, const CallMeaning& meaning)
{
    const TypeRef type = typeOf(source) != nullptr ? typeOf(source) : semantics_.types().voidType();
    Expr* result = nullptr;
    switch (meaning.kind) {
    case CallKind::Routine:
        result = make(ExprKind::Call, type, source);
        result->routine = routineOf(meaning.routine, source);
        result->operands = expressions(source.arguments(), 0, nullptr);
        break;
    case CallKind::Method: {
        if (source.callee().kind() != ast::AstNodeKind::CallExpression) {
            fail("a method call without a receiver", source);
        }
        const auto& member = static_cast<const ast::CallExpression&>(source.callee());
        result = make(ExprKind::Call, type, source);
        result->routine = routineOf(meaning.routine, source);
        result->operands = expressions(source.arguments(), 0, member.arguments()[0].get());
        break;
    }
    case CallKind::MethodNoArguments:
        result = make(ExprKind::Call, type, source);
        result->routine = routineOf(meaning.routine, source);
        result->operands = arena_.array<const Expr*>(1);
        result->operands[0] = expression(*source.arguments()[0]);
        break;
    case CallKind::Field:
    case CallKind::Index:
        result = make(ExprKind::Read, type, source);
        result->place = place(source);
        break;
    case CallKind::Slice:
        result = slice(source, meaning, type);
        break;
    case CallKind::SetLiteral:
        result = make(ExprKind::SetLiteral, type, source);
        result->operands = expressions(source.arguments(), 0, nullptr);
        break;
    case CallKind::Check:
        result = check(source);
        break;
    case CallKind::Pure:
        result = make(ExprKind::Pure, type, source);
        result->operands = arena_.array<const Expr*>(1);
        result->operands[0] = expression(*source.arguments()[0]);
        break;
    case CallKind::Construction:
        return construction(source);
    case CallKind::Conversion:
        result = make(ExprKind::Convert, type, source);
        result->operands = expressions(source.arguments(), 0, nullptr);
        break;
    case CallKind::Builtin:
        result = builtin(source, meaning, type);
        if (meaning.constant) {
            // A constant of the type (CANON-13); the operand is built too,
            // the type is its.
            result->folded = true;
            result->foldedValue = *meaning.constant;
        }
        break;
    case CallKind::StringFunction:
        result = make(ExprKind::String, type, source);
        result->string = stringFunction(source);
        result->operands = expressions(source.arguments(), 0, nullptr);
        break;
    case CallKind::Intrinsic:
        result = make(ExprKind::Intrinsic, type, source);
        result->intrinsic = meaning.intrinsic;
        result->operands = expressions(source.arguments(), 0, nullptr);
        break;
    }
    result->invariants = invariants(source, nullptr);
    recordCall(source, result);
    return result;
}

Expr* Builder::slice(const ast::CallExpression& source, const CallMeaning& meaning, TypeRef type)
{
    Expr* result = make(ExprKind::Slice, type, source);
    result->hasLow = !meaning.lowOmitted;
    result->hasHigh = !meaning.highOmitted;
    result->operands = arena_.array<const Expr*>(1 + (result->hasLow ? 1 : 0) + (result->hasHigh ? 1 : 0));
    result->operands[0] = expression(*source.arguments()[0]);
    if (result->hasLow) {
        result->operands[1] = expression(*source.arguments()[1]);
    }
    if (result->hasHigh) {
        result->operands[result->hasLow ? 2 : 1] = expression(*source.arguments()[2]);
    }
    return result;
}

// `__check(Kind, Condition, "text")`, written by the contract lowering.
Expr* Builder::check(const ast::CallExpression& source)
{
    Expr* result = make(ExprKind::Check, semantics_.types().voidType(), source);
    const auto& kind = static_cast<const ast::LiteralExpression&>(*source.arguments()[0]);
    result->checkKind = static_cast<std::int32_t>(std::stol(kind.value()));
    result->text = arena_.text(static_cast<const ast::LiteralExpression&>(*source.arguments()[2]).value());
    // The condition, then the literals of the kind and the text, which
    // `checkKind` and `text` carry decoded.
    result->operands = arena_.array<const Expr*>(3);
    result->operands[0] = expression(*source.arguments()[1]);
    result->operands[1] = expression(kind);
    result->operands[2] = expression(*source.arguments()[2]);
    return result;
}

Expr* Builder::builtin(const ast::CallExpression& source, const CallMeaning& meaning, TypeRef type)
{
    if (meaning.op == semantic::BuiltinOp::Value) {
        // `Value(P)` is the object P points at: a place (safe-pointers).
        Expr* result = make(ExprKind::Read, type, source);
        result->place = place(source);
        return result;
    }
    Expr* result = make(ExprKind::Builtin, type, source);
    result->op = meaning.op;
    result->operands = expressions(source.arguments(), 0, nullptr);
    if (meaning.op == semantic::BuiltinOp::Fill) {
        result->unit = semantics_.fillUnit(source);
    }
    return result;
}

semantic::StringFunction Builder::stringFunction(const ast::CallExpression& source) const
{
    const semantic::StringFunction* function = semantics_.stringCall(source);
    if (function == nullptr) {
        fail("a string call without its function", source);
    }
    return *function;
}

const Expr* Builder::constantLiteral(std::int64_t integer, TypeRef type, ast::SourcePosition position)
{
    semantic::ConstantValue value;
    value.integer = integer;
    return constantLiteral(value, type, position);
}

const Expr* Builder::construction(const ast::CallExpression& source)
{
    const TypeRef type = semantics_.constructedStruct(source);
    if (type == nullptr) {
        fail("a construction without its struct", source);
    }
    Expr* result = make(ExprKind::Construct, type, source);
    result->fields = arena_.array<FieldValue>(type->fields().size());
    for (std::size_t index = 0; index < result->fields.size(); ++index) {
        result->fields[index].field = static_cast<std::uint32_t>(index);
    }
    std::uint32_t written = 0;
    for (const auto& argument : source.arguments()) {
        const CallMeaning* meaning = semantics_.callMeaning(*argument);
        if (meaning == nullptr || meaning->kind != CallKind::Field ||
            argument->kind() != ast::AstNodeKind::BinaryExpression || meaning->field >= result->fields.size()) {
            fail("a construction argument without its field", *argument);
        }
        const auto& named = static_cast<const ast::BinaryExpression&>(*argument);
        result->fields[meaning->field].value = expression(named.right());
        result->fields[meaning->field].written = written++;
    }
    result->invariants = invariants(source, nullptr);
    recordCall(source, result);
    return result;
}

const Invariants* Builder::invariants(const ast::AstNode& site, const ast::AstNode* other)
{
    const auto* before = semantics_.invariantChecks(site, false);
    const auto* after = semantics_.invariantChecks(site, true);
    const TypeRef newValue = semantics_.newValueInvariant(site);
    const auto* otherBefore = other != nullptr ? semantics_.invariantChecks(*other, false) : nullptr;
    const auto* otherAfter = other != nullptr ? semantics_.invariantChecks(*other, true) : nullptr;
    if (before == nullptr && after == nullptr && newValue == nullptr && otherBefore == nullptr &&
        otherAfter == nullptr) {
        return nullptr;
    }
    Invariants* result = arena_.make<Invariants>();
    result->newValue = newValue;
    const auto copy = [this](const auto* first, const auto* second) {
        const std::size_t count = (first != nullptr ? first->size() : 0) + (second != nullptr ? second->size() : 0);
        Span<InvariantCheck> checks = arena_.array<InvariantCheck>(count);
        std::size_t index = 0;
        for (const auto* list : {first, second}) {
            if (list == nullptr) {
                continue;
            }
            for (const semantic::SemanticResult::InvariantCheck& check : *list) {
                checks[index].place = check.through ? through(place(*check.place)) : place(*check.place);
                checks[index].type = check.type;
                ++index;
            }
        }
        return checks;
    };
    result->before = copy(before, otherBefore);
    result->after = copy(after, otherAfter);
    return result;
}

} // namespace inox::compiler::tir::detail
