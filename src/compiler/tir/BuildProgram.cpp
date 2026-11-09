// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


// Phase 5b, construction of the typed IR: the analyzed AST and the
// SemanticResult become a tir::Program.
// Every decision comes from the analysis: a call's meaning (CallMeaning),
// a variable's origin (VariableOrigin), the symbol a declaration made, a
// field's index, a constant's value, the checks it asked for. The builder
// tests the shape of the AST it walks, never a name.
//
// Stack: the recursive functions (expression, place, call,
// statement, statements) are small and hold no strings or vectors; an error
// is built out of line (fail). tools/stack_frames.py keeps their budget.

#include "Builder.h"
#include "LinkLifetime.h"

#include <algorithm>
#include <string>

namespace inox::compiler::tir {

namespace detail {

namespace {

std::string located(const char* prefix, const char* what, const ast::AstNode& node)
{
    std::string message = prefix;
    message += what;
    if (node.position().known()) {
        message += " (line " + std::to_string(node.position().line) + ")";
    }
    return message;
}

} // namespace

void Builder::fail(const char* what, const ast::AstNode& node) const
{
    throw InvariantError(located("internal error: typed IR: ", what, node));
}

void Builder::unsupported(const char* what, const ast::AstNode& node) const
{
    throw UnsupportedConstructError(located("typed IR: ", what, node));
}

TypeRef Builder::typeOf(const ast::Expression& expression) const
{
    return semantics_.typeOf(expression);
}

Expr* Builder::make(ExprKind kind, TypeRef type, const ast::AstNode& source)
{
    Expr* expression = arena_.make<Expr>();
    expression->kind = kind;
    expression->type = type;
    expression->position = source.position();
    return expression;
}

Stmt* Builder::make(StmtKind kind, const ast::AstNode& source)
{
    Stmt* statement = arena_.make<Stmt>();
    statement->kind = kind;
    statement->position = source.position();
    return statement;
}

Routine* Builder::routineShell(const ast::FunctionDeclaration& function, bool entry)
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
        Parameter& built = routine->parameters[index];
        built.name = arena_.text(parameter.name);
        built.type = parameter.type;
        built.mode = parameter.mode;
        built.isReceiver = index < function.parameters().size() && function.parameters()[index].isReceiver;
        built.symbol =
            index < function.parameters().size() ? semantics_.declaredSymbol(&function.parameters()[index]) : nullptr;
    }
    routines_[&function] = routine;
    return routine;
}

void Builder::routineBody(Routine& routine)
{
    routine.body = statements(routine.origin->body());
    analyzeLinks(routine, arena_);
}

void Builder::buildModule(const ast::ModuleNode& module)
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
        throw InvariantError("internal error: typed IR: a program without Main");
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
            throw InvariantError("internal error: typed IR: the Const " + declaration.name + " without its symbol");
        }
        constant.type = constant.symbol->type;
        const semantic::ConstantValue* value = semantics_.constantValueOf(*constant.symbol);
        if (value == nullptr) {
            // The analysis computes no value for some Consts yet
            // (`Const Mask := UInt8($FF)`): a known gap of the backend.
            throw UnsupportedConstructError("typed IR: the Const " + declaration.name + " without its value");
        }
        constant.value = constantLiteral(*value, constant.type, declaration.position);
    }

    program_.routines = arena_.array<Routine*>(routines.size());
    std::copy(routines.begin(), routines.end(), program_.routines.begin());
    program_.entry = entryRoutine;
    std::vector<InvariantRoutine> invariants;
    for (const TypeRef type : structTypes) {
        if (const ast::FunctionDeclaration* checker = semantics_.invariantRoutine(type)) {
            const auto found = routines_.find(checker);
            if (found == routines_.end()) {
                throw InvariantError("internal error: typed IR: the invariant of " + type->name() + " is not compiled");
            }
            invariants.push_back(InvariantRoutine{type, found->second});
        }
    }
    program_.invariants = arena_.array<InvariantRoutine>(invariants.size());
    std::copy(invariants.begin(), invariants.end(), program_.invariants.begin());
    for (Routine* routine : routines) {
        routineBody(*routine);
    }
}

} // namespace detail

std::unique_ptr<Program> buildProgram(const ast::ModuleNode& module, const semantic::SemanticResult& semantics)
{
    auto program = std::make_unique<Program>();
    detail::Builder(*program, semantics).buildModule(module);
    return program;
}

} // namespace inox::compiler::tir
