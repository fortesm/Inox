// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Phase 5b: the analyzed AST and the SemanticResult become a tir::Program.
// Every decision comes from the analysis: a call's meaning (CallMeaning),
// a variable's origin (VariableOrigin), the symbol a declaration made, a
// field's index, a constant's value, the checks it asked for. The lowering
// tests the shape of the AST it walks, never a name.
//
// Stack (condition 5): the recursive functions (expression, place, call,
// statement, statements) are small and hold no strings or vectors; an error
// is built out of line (fail). tools/stack_frames.py keeps their budget.

#include "Lower.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <unordered_map>

namespace inox::compiler::tir {

namespace {

using semantic::CallKind;
using semantic::CallMeaning;

class Lowerer {
public:
    Lowerer(Program& program, const semantic::SemanticResult& semantics)
        : program_(program), arena_(program.arena), semantics_(semantics)
    {
    }

    void lowerModule(const ast::ModuleNode& module);

private:
    // Expressions.
    const Expr* expression(const ast::Expression& expression);
    const Expr* identifier(const ast::IdentifierExpression& identifier);
    const Expr* call(const ast::CallExpression& call, const CallMeaning& meaning);
    const Expr* construction(const ast::CallExpression& call);
    Expr* slice(const ast::CallExpression& call, const CallMeaning& meaning, TypeRef type);
    Expr* check(const ast::CallExpression& call);
    Expr* builtin(const ast::CallExpression& call, const CallMeaning& meaning, TypeRef type);
    semantic::StringFunction stringFunction(const ast::CallExpression& call) const;
    const Expr* constantLiteral(std::int64_t value, TypeRef type, ast::SourcePosition position);
    const Expr* checked(const ast::Expression& source, const Expr* value);
    Span<const Expr*> expressions(const std::vector<ast::ExpressionPtr>& expressions, std::size_t first,
                                  const ast::Expression* receiver);
    const Place* place(const ast::Expression& expression);
    const Place* variable(const ast::IdentifierExpression& identifier, const semantic::Symbol& symbol);
    const Place* through(const Place* base);
    const Invariants* invariants(const ast::AstNode& site, const ast::AstNode* other);

    // Statements.
    const Stmt* statement(const ast::Statement& statement);
    Span<const Stmt*> statements(const std::vector<ast::StatementPtr>& statements);
    const Stmt* expressionStatement(const ast::ExpressionStatement& statement);
    const Stmt* declaration(const ast::VarStatement& declaration);
    const Stmt* ifStatement(const ast::IfStatement& statement);
    const Stmt* caseStatement(const ast::CaseStatement& statement);
    const Stmt* tryStatement(const ast::TryStatement& statement);
    const Stmt* withStatement(const ast::WithStatement& statement);
    const Stmt* forStatement(const ast::ForInStatement& statement);
    const Stmt* raiseStatement(const ast::RaiseStatement& statement);

    // Routines and module declarations.
    Routine* routineShell(const ast::FunctionDeclaration& function, bool entry);
    void routineBody(Routine& routine);
    const Routine* routineOf(const semantic::FunctionSignature* signature, const ast::AstNode& site);
    const Expr* constantLiteral(const semantic::ConstantValue& value, TypeRef type, ast::SourcePosition position);

    Expr* make(ExprKind kind, TypeRef type, const ast::AstNode& source);
    Stmt* make(StmtKind kind, const ast::AstNode& source);
    TypeRef typeOf(const ast::Expression& expression) const;

    [[noreturn]] void fail(const char* what, const ast::AstNode& node) const;

    Program& program_;
    Arena& arena_;
    const semantic::SemanticResult& semantics_;
    std::unordered_map<const ast::FunctionDeclaration*, Routine*> routines_;
    std::unordered_map<const semantic::Symbol*, const Stmt*> withs_;
};

void Lowerer::fail(const char* what, const ast::AstNode& node) const
{
    std::string message = "valid Inox, not yet lowered to the typed IR: ";
    message += what;
    if (node.position().known()) {
        message += " (line " + std::to_string(node.position().line) + ")";
    }
    throw LoweringError(message);
}

TypeRef Lowerer::typeOf(const ast::Expression& expression) const
{
    return semantics_.typeOf(expression);
}

Expr* Lowerer::make(ExprKind kind, TypeRef type, const ast::AstNode& source)
{
    Expr* expression = arena_.make<Expr>();
    expression->kind = kind;
    expression->type = type;
    expression->position = source.position();
    return expression;
}

Stmt* Lowerer::make(StmtKind kind, const ast::AstNode& source)
{
    Stmt* statement = arena_.make<Stmt>();
    statement->kind = kind;
    statement->position = source.position();
    return statement;
}

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

// ---------------------------------------------------------------- expressions

const Expr* Lowerer::expression(const ast::Expression& source)
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
const Expr* Lowerer::checked(const ast::Expression& source, const Expr* value)
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

const Expr* Lowerer::identifier(const ast::IdentifierExpression& source)
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
            fail("a constant without a value", source);
        }
        const TypeRef type = typeOf(source);
        return constantLiteral(*value, type != nullptr ? type : symbol->type, source.position());
    }
    case semantic::SymbolKind::Type: {
        const TypeRef type = typeOf(source);
        return make(ExprKind::TypeValue, type != nullptr ? type : symbol->type, source);
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

const Expr* Lowerer::constantLiteral(const semantic::ConstantValue& value, TypeRef type, ast::SourcePosition position)
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

Span<const Expr*> Lowerer::expressions(const std::vector<ast::ExpressionPtr>& sources, std::size_t first,
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

const Routine* Lowerer::routineOf(const semantic::FunctionSignature* signature, const ast::AstNode& site)
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

const Expr* Lowerer::call(const ast::CallExpression& source, const CallMeaning& meaning)
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
        if (meaning.constant) {
            return constantLiteral(*meaning.constant, type, source.position());
        }
        result = builtin(source, meaning, type);
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
    return result;
}

Expr* Lowerer::slice(const ast::CallExpression& source, const CallMeaning& meaning, TypeRef type)
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
Expr* Lowerer::check(const ast::CallExpression& source)
{
    Expr* result = make(ExprKind::Check, semantics_.types().voidType(), source);
    const auto& kind = static_cast<const ast::LiteralExpression&>(*source.arguments()[0]);
    result->checkKind = static_cast<std::int32_t>(std::stol(kind.value()));
    result->text = arena_.text(static_cast<const ast::LiteralExpression&>(*source.arguments()[2]).value());
    result->operands = arena_.array<const Expr*>(1);
    result->operands[0] = expression(*source.arguments()[1]);
    return result;
}

Expr* Lowerer::builtin(const ast::CallExpression& source, const CallMeaning& meaning, TypeRef type)
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
    return result;
}

semantic::StringFunction Lowerer::stringFunction(const ast::CallExpression& source) const
{
    const semantic::StringFunction* function = semantics_.stringCall(source);
    if (function == nullptr) {
        fail("a string call without its function", source);
    }
    return *function;
}

const Expr* Lowerer::constantLiteral(std::int64_t integer, TypeRef type, ast::SourcePosition position)
{
    semantic::ConstantValue value;
    value.integer = integer;
    return constantLiteral(value, type, position);
}

const Expr* Lowerer::construction(const ast::CallExpression& source)
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
    for (const auto& argument : source.arguments()) {
        const CallMeaning* meaning = semantics_.callMeaning(*argument);
        if (meaning == nullptr || meaning->kind != CallKind::Field ||
            argument->kind() != ast::AstNodeKind::BinaryExpression || meaning->field >= result->fields.size()) {
            fail("a construction argument without its field", *argument);
        }
        const auto& named = static_cast<const ast::BinaryExpression&>(*argument);
        result->fields[meaning->field].value = expression(named.right());
    }
    result->invariants = invariants(source, nullptr);
    return result;
}

const Invariants* Lowerer::invariants(const ast::AstNode& site, const ast::AstNode* other)
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
                checks[index].place = place(*check.place);
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

// ---------------------------------------------------------------- places

const Place* Lowerer::variable(const ast::IdentifierExpression& source, const semantic::Symbol& symbol)
{
    Place* result = arena_.make<Place>();
    result->symbol = &symbol;
    result->type = symbol.type;
    result->isMutable = symbol.isMutable;
    if (symbol.kind == semantic::SymbolKind::State) {
        result->kind = PlaceKind::State;
        return result;
    }
    if (symbol.kind == semantic::SymbolKind::LoopIterator) {
        result->kind = PlaceKind::LoopIterator;
        return result;
    }
    switch (symbol.origin) {
    case semantic::VariableOrigin::Local: result->kind = PlaceKind::Local; break;
    case semantic::VariableOrigin::Parameter: result->kind = PlaceKind::Parameter; break;
    case semantic::VariableOrigin::RefParameter:
        result->kind = PlaceKind::Parameter;
        result->mode = semantic::PassMode::Ref;
        break;
    case semantic::VariableOrigin::RefMutParameter:
        result->kind = PlaceKind::Parameter;
        result->mode = semantic::PassMode::RefMut;
        break;
    case semantic::VariableOrigin::Receiver: result->kind = PlaceKind::Receiver; break;
    case semantic::VariableOrigin::MutReceiver:
        result->kind = PlaceKind::Receiver;
        result->mode = semantic::PassMode::RefMut;
        break;
    case semantic::VariableOrigin::WithBinding: {
        result->kind = PlaceKind::WithBinding;
        const auto found = withs_.find(&symbol);
        if (found == withs_.end()) {
            fail("a `with` binding outside its statement", source);
        }
        result->with = found->second;
        break;
    }
    case semantic::VariableOrigin::HandlerBinding: result->kind = PlaceKind::HandlerBinding; break;
    case semantic::VariableOrigin::None: fail("a variable without an origin", source);
    }
    return result;
}

const Place* Lowerer::through(const Place* base)
{
    if (base->type == nullptr || !base->type->isPointer()) {
        return base;
    }
    Place* result = arena_.make<Place>();
    result->kind = PlaceKind::Through;
    result->base = base;
    result->type = base->type->pointerTarget();
    result->isMutable = base->isMutable;
    return result;
}

const Place* Lowerer::place(const ast::Expression& source)
{
    if (source.kind() == ast::AstNodeKind::IdentifierExpression && semantics_.callMeaning(source) == nullptr) {
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(source);
        const semantic::Symbol* symbol = semantics_.symbolOf(identifier);
        if (symbol != nullptr && (symbol->kind == semantic::SymbolKind::Variable ||
                                  symbol->kind == semantic::SymbolKind::LoopIterator ||
                                  symbol->kind == semantic::SymbolKind::State)) {
            return variable(identifier, *symbol);
        }
    }
    if (source.kind() == ast::AstNodeKind::CallExpression) {
        const CallMeaning* meaning = semantics_.callMeaning(source);
        const auto& call = static_cast<const ast::CallExpression&>(source);
        if (meaning != nullptr && meaning->kind == CallKind::Field) {
            Place* result = arena_.make<Place>();
            result->kind = PlaceKind::Field;
            result->base = through(place(*call.arguments()[0]));
            result->field = meaning->field;
            // The field's type, as its struct declares it (a written target
            // is not an analyzed expression).
            const TypeRef structType = result->base->type;
            result->type = typeOf(source) != nullptr ? typeOf(source)
                           : structType != nullptr && structType->isStruct() && meaning->field < structType->fields().size()
                               ? structType->fields()[meaning->field].type
                               : nullptr;
            result->isMutable = result->base->isMutable;
            return result;
        }
        if (meaning != nullptr && meaning->kind == CallKind::Builtin && meaning->op == semantic::BuiltinOp::Value &&
            call.arguments().size() == 1) {
            const Place* pointer = place(*call.arguments()[0]);
            const Place* target = through(pointer);
            if (target == pointer) {
                fail("Value of something that is not a pointer", source);
            }
            return target;
        }
        if (meaning != nullptr && meaning->kind == CallKind::Index) {
            Place* result = arena_.make<Place>();
            result->kind = PlaceKind::Element;
            result->base = through(place(*call.arguments()[0]));
            result->indices = arena_.array<const Expr*>(1);
            result->indices[0] = expression(*call.arguments()[1]);
            result->type = typeOf(source) != nullptr ? typeOf(source)
                           : result->base->type != nullptr ? result->base->type->element()
                                                           : nullptr;
            result->isMutable = result->base->isMutable;
            return result;
        }
    }
    Place* result = arena_.make<Place>();
    result->kind = PlaceKind::Value;
    result->value = expression(source);
    result->type = result->value->type;
    return result;
}

// ---------------------------------------------------------------- statements

Span<const Stmt*> Lowerer::statements(const std::vector<ast::StatementPtr>& sources)
{
    Span<const Stmt*> result = arena_.array<const Stmt*>(sources.size());
    for (std::size_t index = 0; index < sources.size(); ++index) {
        result[index] = statement(*sources[index]);
    }
    return result;
}

const Stmt* Lowerer::statement(const ast::Statement& source)
{
    Stmt* result = nullptr;
    switch (source.kind()) {
    case ast::AstNodeKind::BlockStatement:
        result = make(StmtKind::Block, source);
        result->body = statements(static_cast<const ast::BlockStatement&>(source).statements());
        break;
    case ast::AstNodeKind::VarBlockStatement:
        result = make(StmtKind::Block, source);
        result->body = statements(static_cast<const ast::VarBlockStatement&>(source).declarations());
        break;
    case ast::AstNodeKind::ExpressionStatement:
        return expressionStatement(static_cast<const ast::ExpressionStatement&>(source));
    case ast::AstNodeKind::VarStatement:
        return declaration(static_cast<const ast::VarStatement&>(source));
    case ast::AstNodeKind::IfStatement:
        return ifStatement(static_cast<const ast::IfStatement&>(source));
    case ast::AstNodeKind::UnlessStatement: {
        const auto& unless = static_cast<const ast::UnlessStatement&>(source);
        result = make(StmtKind::Unless, source);
        result->value = expression(unless.condition());
        result->body = statements(unless.body());
        break;
    }
    case ast::AstNodeKind::WhileStatement: {
        const auto& loop = static_cast<const ast::WhileStatement&>(source);
        result = make(StmtKind::While, source);
        result->value = expression(loop.condition());
        result->body = statements(loop.body());
        break;
    }
    case ast::AstNodeKind::RepeatStatement:
        result = make(StmtKind::Repeat, source);
        result->body = statements(static_cast<const ast::RepeatStatement&>(source).body());
        break;
    case ast::AstNodeKind::UntilStatement:
        result = make(StmtKind::Until, source);
        result->value = expression(static_cast<const ast::UntilStatement&>(source).condition());
        break;
    case ast::AstNodeKind::ForInStatement:
        return forStatement(static_cast<const ast::ForInStatement&>(source));
    case ast::AstNodeKind::CaseStatement:
        return caseStatement(static_cast<const ast::CaseStatement&>(source));
    case ast::AstNodeKind::TryStatement:
        return tryStatement(static_cast<const ast::TryStatement&>(source));
    case ast::AstNodeKind::RaiseStatement:
        return raiseStatement(static_cast<const ast::RaiseStatement&>(source));
    case ast::AstNodeKind::RetryStatement:
        result = make(StmtKind::Retry, source);
        result->value = expression(static_cast<const ast::RetryStatement&>(source).count());
        break;
    case ast::AstNodeKind::ReturnStatement:
        result = make(StmtKind::Return, source);
        result->value = expression(static_cast<const ast::ReturnStatement&>(source).expression());
        break;
    case ast::AstNodeKind::ExitStatement:
        result = make(StmtKind::Exit, source);
        break;
    case ast::AstNodeKind::LeaveStatement:
        result = make(StmtKind::Leave, source);
        break;
    case ast::AstNodeKind::ContinueStatement:
        result = make(StmtKind::Continue, source);
        break;
    case ast::AstNodeKind::WithStatement:
        return withStatement(static_cast<const ast::WithStatement&>(source));
    default:
        fail("a statement of this kind (phase 2b removes the written forms)", source);
    }
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Lowerer::expressionStatement(const ast::ExpressionStatement& source)
{
    const ast::Expression& expression = source.expression();
    if (expression.kind() == ast::AstNodeKind::BinaryExpression &&
        static_cast<const ast::BinaryExpression&>(expression).op() == ast::BinaryOperator::Assign) {
        const auto& assignment = static_cast<const ast::BinaryExpression&>(expression);
        if (const semantic::Symbol* declared = semantics_.declaredSymbol(&assignment)) {
            Stmt* result = make(StmtKind::Declare, source);
            result->symbol = declared;
            result->type = declared->type;
            result->value = this->expression(assignment.right());
            result->invariants = invariants(source, &assignment);
            return result;
        }
        Stmt* result = make(StmtKind::Assign, source);
        result->target = place(assignment.left());
        result->value = this->expression(assignment.right());
        result->compound = source.isCompoundAssignment();
        result->invariants = invariants(source, &assignment);
        return result;
    }
    Stmt* result = make(StmtKind::Evaluate, source);
    result->value = this->expression(expression);
    result->discard = source.isDiscard();
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Lowerer::declaration(const ast::VarStatement& source)
{
    const semantic::Symbol* symbol = semantics_.declaredSymbol(&source);
    if (symbol == nullptr) {
        fail("a declaration without its symbol", source);
    }
    Stmt* result = make(StmtKind::Declare, source);
    result->symbol = symbol;
    result->type = semantics_.declaredType(source);
    if (source.initializer() != nullptr) {
        result->value = expression(*source.initializer());
    }
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Lowerer::ifStatement(const ast::IfStatement& source)
{
    Stmt* result = make(StmtKind::If, source);
    result->value = expression(source.condition());
    result->body = statements(source.thenBody());
    result->branches = arena_.array<Branch>(source.elseIfClauses().size());
    for (std::size_t index = 0; index < source.elseIfClauses().size(); ++index) {
        const ast::ElseIfClause& clause = source.elseIfClauses()[index];
        result->branches[index].conditions = arena_.array<const Expr*>(1);
        result->branches[index].conditions[0] = expression(*clause.condition);
        result->branches[index].body = statements(clause.body);
    }
    result->elseBody = statements(source.elseBody());
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Lowerer::caseStatement(const ast::CaseStatement& source)
{
    Stmt* result = make(StmtKind::Case, source);
    result->value = expression(source.expression());
    result->branches = arena_.array<Branch>(source.arms().size());
    for (std::size_t index = 0; index < source.arms().size(); ++index) {
        const ast::CaseArm& arm = source.arms()[index];
        result->branches[index].conditions = expressions(arm.choices, 0, nullptr);
        result->branches[index].body = statements(arm.body);
    }
    result->hasElse = source.hasElse();
    result->elseBody = statements(source.elseBody());
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Lowerer::tryStatement(const ast::TryStatement& source)
{
    Stmt* result = make(StmtKind::Try, source);
    result->body = statements(source.body());
    TryParts* parts = arena_.make<TryParts>();
    parts->hasExcept = source.hasExcept();
    parts->plainExcept = source.hasPlainExcept();
    parts->exceptBody = statements(source.exceptBody());
    parts->handlers = arena_.array<Handler>(source.handlers().size());
    for (std::size_t index = 0; index < source.handlers().size(); ++index) {
        const ast::ExceptionHandler& handler = source.handlers()[index];
        parts->handlers[index].typeName = arena_.text(handler.typeName);
        parts->handlers[index].binding = semantics_.declaredSymbol(&handler);
        parts->handlers[index].body = statements(handler.body);
    }
    parts->elseBody = statements(source.elseBody());
    parts->hasEnsure = source.hasEnsure();
    parts->ensureBody = statements(source.ensureBody());
    result->tryParts = parts;
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Lowerer::withStatement(const ast::WithStatement& source)
{
    const semantic::Symbol* binding = semantics_.declaredSymbol(&source);
    if (binding == nullptr) {
        fail("a `with` without its binding", source);
    }
    Stmt* result = make(StmtKind::With, source);
    result->symbol = binding;
    const Place* target = place(source.target());
    if (target->kind == PlaceKind::Value) {
        result->value = target->value;
    } else {
        Expr* read = make(ExprKind::Read, target->type, source.target());
        read->place = target;
        result->value = read;
    }
    withs_[binding] = result;
    result->body = statements(source.body());
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Lowerer::forStatement(const ast::ForInStatement& source)
{
    const semantic::Symbol* iterator = semantics_.declaredSymbol(&source);
    if (iterator == nullptr) {
        fail("a loop without its iterator", source);
    }
    Stmt* result = make(StmtKind::ForIn, source);
    result->symbol = iterator;
    result->type = iterator->type;
    result->value = expression(source.iterable());
    if (source.step() != nullptr) {
        result->step = expression(*source.step());
    }
    result->body = statements(source.body());
    result->invariants = invariants(source, nullptr);
    return result;
}

const Stmt* Lowerer::raiseStatement(const ast::RaiseStatement& source)
{
    Stmt* result = make(StmtKind::Raise, source);
    // F7 gives exceptions their types; until then the name is carried as
    // written (design of typed-ir, "Relation to the other steps").
    if (source.expression() != nullptr) {
        if (source.expression()->kind() != ast::AstNodeKind::IdentifierExpression) {
            fail("a Raise of this form", source);
        }
        result->text = arena_.text(static_cast<const ast::IdentifierExpression&>(*source.expression()).name());
    }
    return result;
}

// ---------------------------------------------------------------- the program

Routine* Lowerer::routineShell(const ast::FunctionDeclaration& function, bool entry)
{
    const semantic::FunctionSignature* signature = semantics_.signatureOf(function);
    if (signature == nullptr) {
        fail("a routine without an analyzed signature", function);
    }
    Routine* routine = arena_.make<Routine>();
    routine->name = arena_.text(function.name());
    routine->origin = &function;
    routine->signature = signature;
    routine->isEntry = entry;
    routine->result = signature->returnType;
    routine->parameters = arena_.array<Parameter>(signature->parameters.size());
    for (std::size_t index = 0; index < signature->parameters.size(); ++index) {
        const semantic::FunctionParameter& parameter = signature->parameters[index];
        Parameter& lowered = routine->parameters[index];
        lowered.name = arena_.text(parameter.name);
        lowered.type = parameter.type;
        lowered.mode = parameter.mode;
        lowered.isReceiver = index < function.parameters().size() && function.parameters()[index].isReceiver;
        lowered.symbol =
            index < function.parameters().size() ? semantics_.declaredSymbol(&function.parameters()[index]) : nullptr;
    }
    routines_[&function] = routine;
    return routine;
}

void Lowerer::routineBody(Routine& routine)
{
    routine.body = statements(routine.origin->body());
}

void Lowerer::lowerModule(const ast::ModuleNode& module)
{
    const ast::FunctionDeclaration* entry = semantics_.entry();
    std::vector<TypeRef> structTypes;
    std::vector<const ast::VariableDeclaration*> states;
    std::vector<const ast::VariableDeclaration*> constants;
    std::vector<Routine*> routines;
    for (const auto& item : module.items()) {
        if (item->kind() == ast::AstNodeKind::SectionDeclaration) {
            const auto& section = static_cast<const ast::SectionDeclaration&>(*item);
            for (const ast::TypeDeclaration& declaration : section.types()) {
                if (declaration.kind == ast::TypeDeclarationKind::Struct && declaration.typeParameters.empty()) {
                    const TypeRef type = semantics_.declaredType(declaration);
                    if (type == nullptr) {
                        fail("a struct without its type", *item);
                    }
                    structTypes.push_back(type);
                }
            }
            for (const ast::VariableDeclaration& declaration : section.variables()) {
                (section.sectionKind() == ast::SectionKind::State ? states : constants).push_back(&declaration);
            }
        } else if (item->kind() == ast::AstNodeKind::FunctionDeclaration) {
            const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
            if (&function != entry && !function.isGeneric()) {
                routines.push_back(routineShell(function, false));
            }
        }
    }
    for (const auto& instance : semantics_.instances()) {
        routines.push_back(routineShell(*instance, false));
    }
    if (entry == nullptr) {
        throw LoweringError("a program without Main");
    }
    Routine* entryRoutine = routineShell(*entry, true);
    routines.push_back(entryRoutine);
    for (const TypeRef instance : semantics_.structInstances()) {
        structTypes.push_back(instance);
    }

    program_.structTypes = arena_.array<TypeRef>(structTypes.size());
    std::copy(structTypes.begin(), structTypes.end(), program_.structTypes.begin());

    program_.states = arena_.array<StateVariable>(states.size());
    for (std::size_t index = 0; index < states.size(); ++index) {
        const ast::VariableDeclaration& declaration = *states[index];
        StateVariable& state = program_.states[index];
        state.symbol = semantics_.declaredSymbol(&declaration);
        state.name = arena_.text(declaration.name);
        state.type = semantics_.declaredType(declaration);
        if (declaration.value != nullptr) {
            state.initializer = expression(*declaration.value);
        }
    }

    program_.constants = arena_.array<Constant>(constants.size());
    for (std::size_t index = 0; index < constants.size(); ++index) {
        const ast::VariableDeclaration& declaration = *constants[index];
        Constant& constant = program_.constants[index];
        constant.symbol = semantics_.declaredSymbol(&declaration);
        constant.name = arena_.text(declaration.name);
        if (constant.symbol == nullptr) {
            throw LoweringError("valid Inox, not yet lowered to the typed IR: the Const " + declaration.name +
                                " without its symbol");
        }
        constant.type = constant.symbol->type;
        const semantic::ConstantValue* value = semantics_.constantValueOf(*constant.symbol);
        if (value == nullptr) {
            throw LoweringError("valid Inox, not yet lowered to the typed IR: the Const " + declaration.name +
                                " without its value");
        }
        constant.value = constantLiteral(*value, constant.type, declaration.position);
    }

    program_.routines = arena_.array<Routine*>(routines.size());
    std::copy(routines.begin(), routines.end(), program_.routines.begin());
    program_.entry = entryRoutine;
    for (Routine* routine : routines) {
        routineBody(*routine);
    }
}

} // namespace

std::unique_ptr<Program> lowerProgram(const ast::ModuleNode& module, const semantic::SemanticResult& semantics)
{
    auto program = std::make_unique<Program>();
    Lowerer(*program, semantics).lowerModule(module);
    return program;
}

} // namespace inox::compiler::tir
