// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


// Phase 5b: places. A variable by the origin the analysis recorded, a field
// by its recorded index, an element, the object a pointer reaches, a value.

#include "Lowerer.h"

namespace inox::compiler::tir::detail {

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
            return recorded(source, variable(identifier, *symbol));
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
            if (index_ != nullptr) {
                // The field's name, and the `__member` it is written with.
                index_->recordMember(*call.arguments()[1], result);
                index_->recordName(call.callee());
            }
            return recorded(source, result);
        }
        if (meaning != nullptr && meaning->kind == CallKind::Builtin && meaning->op == semantic::BuiltinOp::Value &&
            call.arguments().size() == 1) {
            const Place* pointer = place(*call.arguments()[0]);
            const Place* target = through(pointer);
            if (target == pointer) {
                fail("Value of something that is not a pointer", source);
            }
            if (index_ != nullptr) {
                index_->recordName(call.callee());
            }
            return recorded(source, target);
        }
        if (meaning != nullptr && meaning->kind == CallKind::Index) {
            Place* result = arena_.make<Place>();
            result->kind = PlaceKind::Element;
            result->base = through(place(*call.arguments()[0]));
            result->indices = arena_.array<const Expr*>(1);
            result->indices[0] = expression(*call.arguments()[1]);
            if (index_ != nullptr) {
                index_->recordName(call.callee());
            }
            result->type = typeOf(source) != nullptr ? typeOf(source)
                           : result->base->type != nullptr ? result->base->type->element()
                                                           : nullptr;
            result->isMutable = result->base->isMutable;
            return recorded(source, result);
        }
    }
    Place* result = arena_.make<Place>();
    result->kind = PlaceKind::Value;
    result->value = expression(source);
    result->type = result->value->type;
    return recorded(source, result);
}

// F6 step 2, TRANSITIONAL: the migration index.
const Place* Lowerer::recorded(const ast::Expression& source, const Place* place)
{
    if (index_ != nullptr) {
        index_->recordPlace(source, place);
    }
    return place;
}

void Lowerer::recordCall(const ast::CallExpression& source, Expr* call)
{
    call->symbol = semantics_.symbolOf(source);
    if (index_ == nullptr) {
        return;
    }
    // The name a call is written with stands for the call: the routine's
    // identifier, or the `__member` of a method and its method name.
    const ast::Expression& callee = source.callee();
    index_->recordCallee(callee, call);
    if (callee.kind() == ast::AstNodeKind::CallExpression) {
        const auto& member = static_cast<const ast::CallExpression&>(callee);
        if (member.arguments().size() == 2) {
            index_->recordCallee(*member.arguments()[1], call);
        }
        index_->recordName(member.callee());
    } else if (call->kind == ExprKind::Call && source.arguments().size() == 2 &&
               semantics_.callMeaning(source)->kind == CallKind::MethodNoArguments) {
        index_->recordCallee(*source.arguments()[1], call);
    }
}

void Lowerer::recordDeclaration(const void* declaration, const Stmt* statement)
{
    if (index_ != nullptr) {
        index_->recordDeclaration(declaration, statement);
    }
}

void Lowerer::recordAssignment(const ast::BinaryExpression& assignment, const Stmt* statement, bool declares)
{
    if (index_ == nullptr) {
        return;
    }
    index_->recordStatement(assignment, statement);
    if (declares) {
        index_->recordDeclared(assignment.left(), statement);
    }
}

} // namespace inox::compiler::tir::detail
