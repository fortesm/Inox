// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// `inox --dump-tir`: the typed IR as text. One statement a line, indented by
// nesting; an expression is `(kind Type ...)`, a place `(kind ...)`. Only
// names, types and values are printed, never an address or the order of an
// unordered container, so the output is the same on every platform.

#include "Verify.h"

namespace inox::compiler::tir {

namespace {

class Printer {
public:
    explicit Printer(std::ostream& output) : output_(output) {}

    void program(const Program& program);

private:
    void routine(const Routine& routine);
    void linkedLocals(const char* what, Span<const semantic::Symbol*> locals);
    void statements(const Span<const Stmt*>& statements, int depth);
    void statement(const Stmt& statement, int depth);
    void expression(const Expr* expression);
    void place(const Place* place);
    void invariants(const Invariants* invariants, int depth);
    void indent(int depth);
    void type(TypeRef type);
    void literal(const Expr& expression);
    void operation(const Expr& expression);
    void inlineInvariants(const Invariants& checks);

    std::ostream& output_;
};

void Printer::indent(int depth)
{
    for (int level = 0; level < depth; ++level) {
        output_ << "  ";
    }
}

void Printer::type(TypeRef type)
{
    output_ << types::nameOf(type);
}

std::string_view modeText(semantic::PassMode mode)
{
    switch (mode) {
    case semantic::PassMode::Value: return "";
    case semantic::PassMode::Ref: return "ref ";
    case semantic::PassMode::RefMut: return "ref mut ";
    }
    return "";
}

std::string_view stringFunctionName(semantic::StringFunction function)
{
    for (const semantic::StringFunctionInfo& info : semantic::kStringFunctions) {
        if (info.function == function) {
            return info.name;
        }
    }
    return "?";
}

void Printer::program(const Program& program)
{
    for (const TypeRef structType : program.structTypes) {
        output_ << "struct " << types::nameOf(structType) << " {";
        for (std::size_t index = 0; index < structType->fields().size(); ++index) {
            const types::StructField& field = structType->fields()[index];
            output_ << (index == 0 ? " " : ", ") << field.name << ' ';
            type(field.type);
        }
        output_ << " }\n";
    }
    for (const Constant& constant : program.constants) {
        output_ << "const " << constant.name << ' ';
        type(constant.type);
        output_ << " = ";
        expression(constant.value);
        output_ << '\n';
    }
    for (const StateVariable& state : program.states) {
        output_ << "state " << state.name << ' ';
        type(state.type);
        if (state.initializer != nullptr) {
            output_ << " := ";
            expression(state.initializer);
        }
        output_ << '\n';
    }
    for (const Routine* routine : program.routines) {
        this->routine(*routine);
    }
}

void Printer::routine(const Routine& routine)
{
    output_ << '\n' << (routine.isEntry ? "entry " : "routine ") << routine.name;
    if (!routine.parameters.empty()) {
        output_ << '(';
        for (std::size_t index = 0; index < routine.parameters.size(); ++index) {
            const Parameter& parameter = routine.parameters[index];
            output_ << (index == 0 ? "" : ", ") << modeText(parameter.mode) << parameter.name << ' ';
            type(parameter.type);
            if (parameter.isReceiver) {
                output_ << " receiver";
            }
        }
        output_ << ')';
    }
    if (routine.result != nullptr) {
        output_ << ' ';
        type(routine.result);
    }
    output_ << '\n';
    linkedLocals("linked, generation", routine.generationLocals);
    linkedLocals("linked, on the stack", routine.stackLinkedLocals);
    statements(routine.body, 1);
}

void Printer::linkedLocals(const char* what, Span<const semantic::Symbol*> locals)
{
    if (locals.empty()) {
        return;
    }
    output_ << "  " << what << ':';
    for (const semantic::Symbol* local : locals) {
        output_ << ' ' << local->name;
    }
    output_ << '\n';
}

void Printer::statements(const Span<const Stmt*>& statements, int depth)
{
    for (const Stmt* statement : statements) {
        this->statement(*statement, depth);
    }
}

void Printer::invariants(const Invariants* invariants, int depth)
{
    if (invariants == nullptr) {
        return;
    }
    if (invariants->newValue != nullptr) {
        indent(depth);
        output_ << "invariant-of-new-value " << types::nameOf(invariants->newValue) << '\n';
    }
    for (const InvariantCheck& check : invariants->before) {
        indent(depth);
        output_ << "invariant-before " << types::nameOf(check.type) << ' ';
        place(check.place);
        output_ << '\n';
    }
    for (const InvariantCheck& check : invariants->after) {
        indent(depth);
        output_ << "invariant-after " << types::nameOf(check.type) << ' ';
        place(check.place);
        output_ << '\n';
    }
}

void Printer::statement(const Stmt& statement, int depth)
{
    indent(depth);
    output_ << stmtKindName(statement.kind);
    switch (statement.kind) {
    case StmtKind::Evaluate:
        output_ << (statement.discard ? " discard " : " ");
        expression(statement.value);
        break;
    case StmtKind::Assign:
        output_ << (statement.compound ? " compound " : " ");
        place(statement.target);
        output_ << " := ";
        expression(statement.value);
        break;
    case StmtKind::Declare:
        output_ << ' ' << statement.symbol->name << ' ';
        type(statement.type);
        if (statement.value != nullptr) {
            output_ << " := ";
            expression(statement.value);
        }
        break;
    case StmtKind::If:
    case StmtKind::Unless:
    case StmtKind::While:
    case StmtKind::Until:
    case StmtKind::Retry:
    case StmtKind::Case:
        output_ << ' ';
        expression(statement.value);
        break;
    case StmtKind::ForIn:
        output_ << ' ' << statement.symbol->name << ' ';
        type(statement.type);
        output_ << " in ";
        expression(statement.value);
        if (statement.step != nullptr) {
            output_ << " step ";
            expression(statement.step);
        }
        break;
    case StmtKind::Raise:
        if (statement.type != nullptr) {
            output_ << ' ' << statement.type->name();
        }
        break;
    case StmtKind::Return:
        if (statement.value != nullptr) {
            output_ << ' ';
            expression(statement.value);
        }
        break;
    case StmtKind::With:
        output_ << ' ' << statement.symbol->name << " := ";
        expression(statement.value);
        break;
    case StmtKind::Repeat:
    case StmtKind::Try:
    case StmtKind::Exit:
    case StmtKind::Leave:
    case StmtKind::Continue:
    case StmtKind::Block:
        break;
    }
    output_ << '\n';
    invariants(statement.invariants, depth + 1);
    if (!statement.scopeEnds.empty()) {
        indent(depth + 1);
        output_ << "ends";
        for (const semantic::Symbol* local : statement.scopeEnds) {
            output_ << ' ' << local->name;
        }
        output_ << '\n';
    }
    statements(statement.body, depth + 1);
    for (const Branch& branch : statement.branches) {
        indent(depth);
        output_ << (statement.kind == StmtKind::If ? "elif" : "when");
        for (const Expr* condition : branch.conditions) {
            output_ << ' ';
            expression(condition);
        }
        output_ << '\n';
        statements(branch.body, depth + 1);
    }
    if (!statement.elseBody.empty() || statement.hasElse) {
        indent(depth);
        output_ << "else\n";
        statements(statement.elseBody, depth + 1);
    }
    if (statement.tryParts != nullptr) {
        const TryParts& parts = *statement.tryParts;
        if (parts.hasExcept && parts.plainExcept) {
            indent(depth);
            output_ << "except\n";
            statements(parts.exceptBody, depth + 1);
        }
        for (const Handler& handler : parts.handlers) {
            indent(depth);
            output_ << "on " << handler.type->name();
            if (handler.binding != nullptr) {
                output_ << ' ' << handler.binding->name;
            }
            output_ << '\n';
            statements(handler.body, depth + 1);
        }
        if (!parts.elseBody.empty()) {
            indent(depth);
            output_ << "else\n";
            statements(parts.elseBody, depth + 1);
        }
        if (parts.hasEnsure) {
            indent(depth);
            output_ << "ensure\n";
            statements(parts.ensureBody, depth + 1);
        }
    }
}

void Printer::expression(const Expr* expression)
{
    output_ << '(' << exprKindName(expression->kind) << ' ';
    type(expression->type);
    switch (expression->kind) {
    case ExprKind::Literal:
        literal(*expression);
        break;
    case ExprKind::Read:
        output_ << ' ';
        place(expression->place);
        break;
    case ExprKind::Call:
    case ExprKind::Builtin:
    case ExprKind::String:
    case ExprKind::Intrinsic:
    case ExprKind::Unary:
    case ExprKind::Binary:
    case ExprKind::Check:
    case ExprKind::Slice:
        operation(*expression);
        break;
    case ExprKind::Construct:
        for (const FieldValue& field : expression->fields) {
            output_ << ' ' << expression->type->fields()[field.field].name << '=';
            if (field.value != nullptr) {
                this->expression(field.value);
            } else {
                output_ << "default";
            }
        }
        break;
    case ExprKind::Convert:
    case ExprKind::Conditional:
    case ExprKind::SetLiteral:
    case ExprKind::Pure:
    case ExprKind::RangeCheck:
    case ExprKind::NaturalCheck:
    case ExprKind::TypeValue:
        break;
    }
    // A check's kind and text are printed decoded, after its name.
    const std::size_t printed = expression->kind == ExprKind::Check ? 1 : expression->operands.size();
    for (std::size_t index = 0; index < printed; ++index) {
        output_ << ' ';
        this->expression(expression->operands[index]);
    }
    if (expression->invariants != nullptr) {
        inlineInvariants(*expression->invariants);
    }
    output_ << ')';
}

void Printer::literal(const Expr& expression)
{
    switch (expression.literal) {
    case LiteralKind::String: output_ << " \"" << expression.text << '"'; break;
    case LiteralKind::Char: output_ << " '" << expression.text << '\''; break;
    case LiteralKind::Bool: output_ << (expression.boolean ? " true" : " false"); break;
    case LiteralKind::None: break;
    case LiteralKind::Integer:
    case LiteralKind::Real:
    case LiteralKind::Decimal: output_ << ' ' << expression.text; break;
    }
}

void Printer::operation(const Expr& expression)
{
    switch (expression.kind) {
    case ExprKind::Call: output_ << ' ' << expression.routine->name; break;
    case ExprKind::Builtin:
        output_ << ' ' << semantic::builtinName(expression.op);
        if (expression.folded) {
            output_ << " = " << expression.foldedValue;
        }
        break;
    case ExprKind::String: output_ << ' ' << stringFunctionName(expression.string); break;
    case ExprKind::Intrinsic: output_ << ' ' << expression.intrinsic->name; break;
    case ExprKind::Unary: output_ << ' ' << unaryOpText(expression.unary); break;
    case ExprKind::Binary: output_ << ' ' << binaryOpText(expression.binary); break;
    case ExprKind::Check: output_ << ' ' << expression.checkKind << " \"" << expression.text << '"'; break;
    case ExprKind::Slice:
        output_ << (expression.hasLow ? "" : " no-low") << (expression.hasHigh ? "" : " no-high");
        break;
    default: break;
    }
}

void Printer::inlineInvariants(const Invariants& checks)
{
    if (checks.newValue != nullptr) {
        output_ << " [invariant-of-new-value " << types::nameOf(checks.newValue) << ']';
    }
    for (const InvariantCheck& check : checks.before) {
        output_ << " [invariant-before " << types::nameOf(check.type) << ' ';
        place(check.place);
        output_ << ']';
    }
    for (const InvariantCheck& check : checks.after) {
        output_ << " [invariant-after " << types::nameOf(check.type) << ' ';
        place(check.place);
        output_ << ']';
    }
}

void Printer::place(const Place* place)
{
    output_ << '(' << placeKindName(place->kind);
    switch (place->kind) {
    case PlaceKind::Local:
    case PlaceKind::LoopIterator:
    case PlaceKind::WithBinding:
    case PlaceKind::HandlerBinding:
    case PlaceKind::State:
        output_ << ' ' << place->symbol->name;
        break;
    case PlaceKind::Parameter:
    case PlaceKind::Receiver:
        output_ << ' ' << modeText(place->mode) << place->symbol->name;
        break;
    case PlaceKind::Field:
        output_ << ' ';
        this->place(place->base);
        output_ << ' ' << place->base->type->fields()[place->field].name;
        break;
    case PlaceKind::Element:
        output_ << ' ';
        this->place(place->base);
        for (const Expr* index : place->indices) {
            output_ << ' ';
            expression(index);
        }
        break;
    case PlaceKind::Through:
        output_ << ' ';
        this->place(place->base);
        break;
    case PlaceKind::Value:
        output_ << ' ';
        expression(place->value);
        break;
    }
    if (place->linkAlive) {
        output_ << " alive";  // no generation check (tir/LinkLifetime)
    }
    output_ << ')';
}

} // namespace

void printProgram(const Program& program, std::ostream& output)
{
    Printer(output).program(program);
}

} // namespace inox::compiler::tir
