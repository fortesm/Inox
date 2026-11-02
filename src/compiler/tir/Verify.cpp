// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The verifier of the typed IR (F6 step 1): every node has a type and a
// meaning. It runs after every lowering in Debug builds and with
// `--dump-tir`.

#include "Verify.h"

#include <unordered_set>

namespace inox::compiler::tir {

namespace {

class Verifier {
public:
    std::string run(const Program& program);

private:
    void routine(const Routine& routine);
    void statements(const Span<const Stmt*>& statements);
    void statement(const Stmt& statement);
    void expression(const Expr* expression);
    void place(const Place* place);
    void invariants(const Invariants* invariants);
    void declare(const semantic::Symbol* symbol, const char* what);
    void problem(const char* what);
    void untyped(const Expr& expression);
    void notAPlace(const Place* target);

    const Routine* routine_ = nullptr;
    std::unordered_set<const semantic::Symbol*> declared_;
    std::string problem_;
};

void Verifier::problem(const char* what)
{
    if (!problem_.empty()) {
        return;
    }
    problem_ = what;
    if (routine_ != nullptr) {
        problem_ += " (in ";
        problem_ += routine_->name;
        problem_ += ")";
    }
}

// Out of line: the recursive functions hold no strings (stack budget).
void Verifier::untyped(const Expr& expression)
{
    std::string what = "an expression without a type: ";
    what += exprKindName(expression.kind);
    if (expression.kind == ExprKind::Binary) {
        what += ' ';
        what += binaryOpText(expression.binary);
    }
    problem(what.c_str());
}

void Verifier::notAPlace(const Place* target)
{
    std::string what = "an assignment to something that is not a place";
    if (target != nullptr && target->value != nullptr) {
        what += ": ";
        what += exprKindName(target->value->kind);
    }
    problem(what.c_str());
}

void Verifier::declare(const semantic::Symbol* symbol, const char* what)
{
    if (symbol == nullptr) {
        problem(what);
        return;
    }
    declared_.insert(symbol);
}

std::string Verifier::run(const Program& program)
{
    if (program.entry == nullptr || !program.entry->isEntry) {
        problem("the program has no entry routine");
    }
    for (const TypeRef type : program.structTypes) {
        if (type == nullptr || !type->isStruct()) {
            problem("a struct type that is not a struct");
        }
    }
    for (const Constant& constant : program.constants) {
        if (constant.symbol == nullptr || constant.value == nullptr || constant.value->kind != ExprKind::Literal) {
            problem("a Const without its symbol or its value");
        }
        expression(constant.value);
    }
    for (const StateVariable& state : program.states) {
        if (state.symbol == nullptr || state.symbol->kind != semantic::SymbolKind::State || state.type == nullptr) {
            problem("a State variable without its symbol or its type");
        }
        if (state.initializer != nullptr) {
            expression(state.initializer);
        }
    }
    for (const Routine* routine : program.routines) {
        this->routine(*routine);
    }
    return problem_;
}

void Verifier::routine(const Routine& routine)
{
    routine_ = &routine;
    declared_.clear();
    if (routine.signature == nullptr || routine.origin == nullptr) {
        problem("a routine without its signature");
    }
    for (const Parameter& parameter : routine.parameters) {
        declare(parameter.symbol, "a parameter without its symbol");
        if (parameter.type == nullptr) {
            problem("a parameter without a type");
        }
    }
    statements(routine.body);
    routine_ = nullptr;
}

void Verifier::statements(const Span<const Stmt*>& statements)
{
    for (const Stmt* statement : statements) {
        if (statement == nullptr) {
            problem("a missing statement");
            continue;
        }
        this->statement(*statement);
    }
}

void Verifier::statement(const Stmt& statement)
{
    invariants(statement.invariants);
    switch (statement.kind) {
    case StmtKind::Evaluate:
    case StmtKind::Until:
    case StmtKind::Retry:
        expression(statement.value);
        break;
    case StmtKind::Assign:
        if (statement.target == nullptr || statement.target->kind == PlaceKind::Value) {
            notAPlace(statement.target);
        }
        expression(statement.value);
        place(statement.target);
        break;
    case StmtKind::Declare:
        if (statement.type == nullptr) {
            problem("a declaration without a type");
        }
        if (statement.value != nullptr) {
            expression(statement.value);
        }
        declare(statement.symbol, "a declaration without its symbol");
        break;
    case StmtKind::If:
        expression(statement.value);
        statements(statement.body);
        for (const Branch& branch : statement.branches) {
            if (branch.conditions.size() != 1) {
                problem("an elif without one condition");
            }
            for (const Expr* condition : branch.conditions) {
                expression(condition);
            }
            statements(branch.body);
        }
        statements(statement.elseBody);
        break;
    case StmtKind::Unless:
    case StmtKind::While:
        expression(statement.value);
        statements(statement.body);
        break;
    case StmtKind::Repeat:
    case StmtKind::Block:
        statements(statement.body);
        break;
    case StmtKind::ForIn:
        expression(statement.value);
        if (statement.step != nullptr) {
            expression(statement.step);
        }
        declare(statement.symbol, "a loop without its iterator");
        statements(statement.body);
        break;
    case StmtKind::Case:
        expression(statement.value);
        for (const Branch& branch : statement.branches) {
            if (branch.conditions.empty()) {
                problem("a case arm without choices");
            }
            for (const Expr* choice : branch.conditions) {
                expression(choice);
            }
            statements(branch.body);
        }
        statements(statement.elseBody);
        break;
    case StmtKind::Try:
        statements(statement.body);
        if (statement.tryParts == nullptr) {
            problem("a try without its parts");
            break;
        }
        statements(statement.tryParts->exceptBody);
        for (const Handler& handler : statement.tryParts->handlers) {
            if (handler.typeName.empty()) {
                problem("a handler without its exception");
            }
            if (handler.binding != nullptr) {
                declared_.insert(handler.binding);
            }
            statements(handler.body);
        }
        statements(statement.tryParts->elseBody);
        statements(statement.tryParts->ensureBody);
        break;
    case StmtKind::Return:
        if (routine_ != nullptr && routine_->result != nullptr && statement.value == nullptr) {
            problem("a Return without the routine's result");
        }
        if (statement.value != nullptr) {
            expression(statement.value);
        }
        break;
    case StmtKind::With:
        expression(statement.value);
        declare(statement.symbol, "a `with` without its binding");
        statements(statement.body);
        break;
    case StmtKind::Raise:
    case StmtKind::Exit:
    case StmtKind::Leave:
    case StmtKind::Continue:
        break;
    }
}

void Verifier::invariants(const Invariants* invariants)
{
    if (invariants == nullptr) {
        return;
    }
    for (const auto* checks : {&invariants->before, &invariants->after}) {
        for (const InvariantCheck& check : *checks) {
            if (check.type == nullptr || !check.type->isStruct()) {
                problem("an invariant check of something that is not a struct");
            }
            place(check.place);
        }
    }
}

void Verifier::expression(const Expr* expression)
{
    if (expression == nullptr) {
        problem("a missing expression");
        return;
    }
    if (expression->type == nullptr) {
        untyped(*expression);
    }
    invariants(expression->invariants);
    for (const Expr* operand : expression->operands) {
        this->expression(operand);
    }
    switch (expression->kind) {
    case ExprKind::Read:
        place(expression->place);
        break;
    case ExprKind::Call: {
        const Routine* routine = expression->routine;
        if (routine == nullptr) {
            problem("a call without its routine");
            break;
        }
        if (routine->parameters.size() != expression->operands.size()) {
            problem("a call without one argument per parameter");
            break;
        }
        for (std::size_t index = 0; index < routine->parameters.size(); ++index) {
            const Expr* argument = expression->operands[index];
            if (routine->parameters[index].mode != semantic::PassMode::Value &&
                (argument == nullptr || argument->kind != ExprKind::Read || argument->place == nullptr ||
                 argument->place->kind == PlaceKind::Value)) {
                problem("a reference argument that is not a place");
            }
        }
        break;
    }
    case ExprKind::Builtin:
        if (expression->op == semantic::BuiltinOp::None) {
            problem("a builtin without its operation");
        }
        break;
    case ExprKind::Intrinsic:
        if (expression->intrinsic == nullptr) {
            problem("an intrinsic without its function");
        }
        break;
    case ExprKind::Unary:
    case ExprKind::Pure:
    case ExprKind::RangeCheck:
    case ExprKind::NaturalCheck:
    case ExprKind::Check:
        if (expression->operands.size() != 1) {
            problem("an operation without its one operand");
        }
        break;
    case ExprKind::Binary:
        if (expression->operands.size() != 2) {
            problem("a binary operation without its two operands");
        }
        break;
    case ExprKind::Conditional:
        if (expression->operands.size() != 3) {
            problem("a conditional without condition and branches");
        }
        break;
    case ExprKind::Construct:
        if (!expression->type->isStruct() || expression->fields.size() != expression->type->fields().size()) {
            problem("a construction without one value per field");
            break;
        }
        for (const FieldValue& field : expression->fields) {
            if (field.value != nullptr) {
                this->expression(field.value);
            }
        }
        break;
    case ExprKind::Slice:
        if (expression->operands.empty()) {
            problem("a slice without its base");
        }
        break;
    case ExprKind::Literal:
    case ExprKind::String:
    case ExprKind::Convert:
    case ExprKind::SetLiteral:
    case ExprKind::TypeValue:
        break;
    }
}

void Verifier::place(const Place* place)
{
    if (place == nullptr) {
        problem("a missing place");
        return;
    }
    if (place->type == nullptr) {
        problem("a place without a type");
    }
    switch (place->kind) {
    case PlaceKind::Local:
    case PlaceKind::Parameter:
    case PlaceKind::Receiver:
    case PlaceKind::LoopIterator:
    case PlaceKind::HandlerBinding:
    case PlaceKind::WithBinding:
        if (place->symbol == nullptr || !declared_.contains(place->symbol)) {
            problem("a variable read before its declaration in its routine");
        }
        if (place->kind == PlaceKind::WithBinding && (place->with == nullptr || place->with->symbol != place->symbol)) {
            problem("a `with` binding without its statement");
        }
        break;
    case PlaceKind::State:
        if (place->symbol == nullptr || place->symbol->kind != semantic::SymbolKind::State) {
            problem("a State place without its State variable");
        }
        break;
    case PlaceKind::Field:
        this->place(place->base);
        if (place->base != nullptr && place->base->type != nullptr &&
            (!place->base->type->isStruct() || place->field >= place->base->type->fields().size())) {
            problem("a field of something that is not a struct, or past its fields");
        }
        break;
    case PlaceKind::Element:
        this->place(place->base);
        if (place->indices.empty()) {
            problem("an element without its index");
        }
        for (const Expr* index : place->indices) {
            expression(index);
        }
        break;
    case PlaceKind::Through:
        this->place(place->base);
        if (place->base != nullptr && place->base->type != nullptr && !place->base->type->isPointer()) {
            problem("a place reached through something that is not a pointer");
        }
        break;
    case PlaceKind::Value:
        expression(place->value);
        break;
    }
}

} // namespace

std::string verifyProgram(const Program& program)
{
    return Verifier().run(program);
}

} // namespace inox::compiler::tir
