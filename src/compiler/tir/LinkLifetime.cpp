// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The Link analysis of a routine (LinkLifetime.h). It runs while the
// builder still owns the routine's nodes, before the TIR is verified and
// handed to code generation, and writes only the annotations of the Link
// analysis (`Place::linkAlive`, the two lists of the routine).
//
// Where a linked variable lives. A Link to a local X is "kept" when its
// value is only used to reach X (`Value(L)`, `L.F`, `L[I]`, a method of X
// through L), tested (`Alive`, `Same`), or stored in a local Link variable
// L declared in X's block or in a block nested in it, whose value is in
// turn only used that way. Any other use (returned, passed, stored in a
// field, an element, State or another variable) lets the Link outlive X: X
// gets a generation block, as before. A local whose Links are all kept is
// plain storage of the frame, which outlives every one of them.
//
// Which checks are proven. A Link variable whose every value is a kept
// Link to a local kept on the stack always reaches live storage. Any other
// Link variable is checked at its first use in a block; a later statement
// of the same block that uses it again needs no check when no statement
// between them, nor the statement itself, can free an object: no call of a
// routine, no `Free` or `Move`, no assignment of a value holding owners, no
// discarded result, no invariant check. A block starts with nothing proven
// when it can be entered again (a loop body, a protected region, Retry) or
// after code that may free (a handler, an ensure, the rest of the block
// after a nested block).

#include "LinkLifetime.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace inox::compiler::tir {
namespace {

using semantic::BuiltinOp;
using semantic::Symbol;
using SymbolSet = std::unordered_set<const Symbol*>;

const Expr& bare(const Expr& expression)
{
    const Expr* value = &expression;
    while (value->kind == ExprKind::RangeCheck || value->kind == ExprKind::NaturalCheck) {
        value = value->operands[0];
    }
    return *value;
}

// The annotations are written while the builder owns the nodes.
Place& annotated(const Place& place)
{
    return const_cast<Place&>(place);
}

bool holdsOwner(TypeRef type)
{
    if (type == nullptr) {
        return false;
    }
    if (type->isOwn() || type->isOptionOwn()) {
        return true;
    }
    if (type->isArray()) {
        return holdsOwner(type->element());
    }
    if (type->isStruct()) {
        return std::any_of(type->fields().begin(), type->fields().end(),
                           [](const types::StructField& field) { return holdsOwner(field.type); });
    }
    return false;
}

bool isVariableKind(PlaceKind kind)
{
    switch (kind) {
    case PlaceKind::Local:
    case PlaceKind::Parameter:
    case PlaceKind::Receiver:
    case PlaceKind::LoopIterator:
    case PlaceKind::WithBinding:
    case PlaceKind::HandlerBinding:
    case PlaceKind::State:
        return true;
    case PlaceKind::Field:
    case PlaceKind::Element:
    case PlaceKind::Through:
    case PlaceKind::Value:
        return false;
    }
    return false;
}

// A local or a parameter holding a Link.
bool isLinkVariable(const Place& place)
{
    return (place.kind == PlaceKind::Local || place.kind == PlaceKind::Parameter) && place.type != nullptr &&
           place.type->isLink();
}

// A Link variable only its own routine changes, and only by assigning it by
// name: a local, or a parameter passed by value.
bool isOwnLinkVariable(const Place& place)
{
    return isLinkVariable(place) &&
           (place.kind == PlaceKind::Local || place.mode == semantic::PassMode::Value);
}

// The variable a place lies in when the path crosses no pointer.
const Place* storageRoot(const Place* place)
{
    while (place != nullptr && (place->kind == PlaceKind::Field || place->kind == PlaceKind::Element)) {
        place = place->base->kind == PlaceKind::Through ? nullptr : place->base;
    }
    return place != nullptr && isVariableKind(place->kind) ? place : nullptr;
}

// The argument of `Link(X)` when X lies in a variable (not reached through
// a pointer): the Link carries that variable's generation.
const Place* linkedPlace(const Expr& expression)
{
    const Expr& value = bare(expression);
    if (value.kind != ExprKind::Builtin || value.op != BuiltinOp::Link) {
        return nullptr;
    }
    const Expr& argument = bare(*value.operands[0]);
    if ((argument.type != nullptr && argument.type->isPointer()) || argument.kind != ExprKind::Read) {
        return nullptr;
    }
    return storageRoot(argument.place) != nullptr ? argument.place : nullptr;
}

// Where the Links of a routine go.
class Escapes {
public:
    void run(Routine& routine, Arena& arena)
    {
        body(routine.body);
        std::vector<const Symbol*> generation;
        std::vector<const Symbol*> stack;
        SymbolSet onStack;
        for (const Symbol* root : roots_) {
            if (staysOnStack(root)) {
                stack.push_back(root);
                onStack.insert(root);
            } else {
                generation.push_back(root);
            }
        }
        routine.generationLocals = copy(arena, generation);
        routine.stackLinkedLocals = copy(arena, stack);
        for (const auto& [link, place] : uses_) {
            if (reachesOnlyStack(link, onStack)) {
                annotated(*place).linkAlive = true;
            }
        }
    }

private:
    struct Source {
        const Symbol* link = nullptr;
        const Symbol* root = nullptr;
    };

    static Span<const Symbol*> copy(Arena& arena, const std::vector<const Symbol*>& symbols)
    {
        Span<const Symbol*> span = arena.array<const Symbol*>(symbols.size());
        std::copy(symbols.begin(), symbols.end(), span.begin());
        return span;
    }

    bool nestedIn(int inner, int outer) const
    {
        for (int body = inner; body >= 0; body = parents_[static_cast<std::size_t>(body)]) {
            if (body == outer) {
                return true;
            }
        }
        return false;
    }

    bool staysOnStack(const Symbol* root) const
    {
        if (!candidates_.contains(root) || escaped_.contains(root)) {
            return false;
        }
        const auto rootBody = declaredIn_.find(root);
        if (rootBody == declaredIn_.end()) {
            return false;
        }
        for (const Source& source : sources_) {
            if (source.root != root) {
                continue;
            }
            const auto linkBody = declaredIn_.find(source.link);
            if (escapedLinks_.contains(source.link) || linkBody == declaredIn_.end() ||
                !nestedIn(linkBody->second, rootBody->second)) {
                return false;
            }
        }
        return true;
    }

    bool reachesOnlyStack(const Symbol* link, const SymbolSet& onStack) const
    {
        if (foreign_.contains(link)) {
            return false;
        }
        bool any = false;
        for (const Source& source : sources_) {
            if (source.link == link) {
                if (!onStack.contains(source.root)) {
                    return false;
                }
                any = true;
            }
        }
        return any;
    }

    void body(Span<const Stmt*> statements)
    {
        const int outer = current_;
        current_ = static_cast<int>(parents_.size());
        parents_.push_back(outer);
        for (const Stmt* statement : statements) {
            this->statement(*statement);
        }
        current_ = outer;
    }

    void root(const Place& linked, bool escapes)
    {
        const Place* variable = storageRoot(&linked);
        const Symbol* symbol = variable->symbol;
        if (variable->kind == PlaceKind::State) {
            place(linked);
            return;  // a State variable never dies
        }
        if (std::find(roots_.begin(), roots_.end(), symbol) == roots_.end()) {
            roots_.push_back(symbol);
        }
        if (variable->kind == PlaceKind::Local && !variable->type->isSlice()) {
            candidates_.insert(symbol);
        }
        if (escapes) {
            escaped_.insert(symbol);
        }
        place(linked);
    }

    // `L := Link(X)` into a local Link variable L.
    void source(const Symbol* link, const Expr& value)
    {
        if (const Place* linked = linkedPlace(value)) {
            const Place* variable = storageRoot(linked);
            if (variable->kind != PlaceKind::State) {
                sources_.push_back(Source{link, variable->symbol});
            }
            root(*linked, false);
            if (variable->kind == PlaceKind::State) {
                foreign_.insert(link);
            }
            return;
        }
        foreign_.insert(link);
        expression(bare(value));
    }

    void statement(const Stmt& statement)
    {
        switch (statement.kind) {
        case StmtKind::Declare:
            declaredIn_[statement.symbol] = current_;
            if (statement.value == nullptr) {
                foreign_.insert(statement.symbol);
            } else if (statement.symbol->type != nullptr && statement.symbol->type->isLink()) {
                source(statement.symbol, *statement.value);
            } else {
                expression(*statement.value);
            }
            break;
        case StmtKind::Assign:
            if (statement.target->kind == PlaceKind::Local && isLinkVariable(*statement.target) &&
                !statement.compound) {
                source(statement.target->symbol, *statement.value);
            } else {
                place(*statement.target);
                expression(*statement.value);
            }
            break;
        default:
            if (statement.value != nullptr) {
                expression(*statement.value);
            }
            if (statement.step != nullptr) {
                expression(*statement.step);
            }
            break;
        }
        invariants(statement.invariants);
        body(statement.body);
        for (const Branch& branch : statement.branches) {
            for (const Expr* condition : branch.conditions) {
                expression(*condition);
            }
            body(branch.body);
        }
        body(statement.elseBody);
        if (const TryParts* parts = statement.tryParts) {
            body(parts->exceptBody);
            for (const Handler& handler : parts->handlers) {
                body(handler.body);
            }
            body(parts->elseBody);
            body(parts->ensureBody);
        }
    }

    void invariants(const Invariants* checks)
    {
        if (checks == nullptr) {
            return;
        }
        for (const InvariantCheck& check : checks->before) {
            place(*check.place, false);
        }
        for (const InvariantCheck& check : checks->after) {
            place(*check.place, false);
        }
    }

    // A use of a place: reaching an object through a Link variable keeps
    // the Link (`use`: the place may be proven alive).
    void place(const Place& place, bool use = true)
    {
        switch (place.kind) {
        case PlaceKind::Through:
            if (isLinkVariable(*place.base)) {
                if (use) {
                    uses_.emplace_back(place.base->symbol, place.base);
                }
            } else {
                this->place(*place.base, use);
            }
            break;
        case PlaceKind::Field:
            this->place(*place.base, use);
            break;
        case PlaceKind::Element:
            this->place(*place.base, use);
            for (const Expr* index : place.indices) {
                expression(*index);
            }
            break;
        case PlaceKind::Value:
            expression(*place.value);
            break;
        default:
            break;
        }
    }

    // `tested`: the value only feeds Alive or Same.
    void expression(const Expr& expression, bool tested = false)
    {
        switch (expression.kind) {
        case ExprKind::Read:
            if (isLinkVariable(*expression.place)) {
                if (!tested) {
                    escapedLinks_.insert(expression.place->symbol);
                }
            } else {
                place(*expression.place);
            }
            break;
        case ExprKind::Builtin:
            builtin(expression, tested);
            break;
        case ExprKind::Call: {
            std::size_t first = 0;
            const Routine* routine = expression.routine;
            if (routine != nullptr && !routine->parameters.empty() && routine->parameters[0].isReceiver &&
                !routine->parameters[0].type->isLink() && !expression.operands.empty()) {
                // A method of the object a Link variable reaches.
                const Expr& receiver = bare(*expression.operands[0]);
                if (receiver.kind == ExprKind::Read && isLinkVariable(*receiver.place)) {
                    uses_.emplace_back(receiver.place->symbol, receiver.place);
                    first = 1;
                }
            }
            for (std::size_t index = first; index < expression.operands.size(); ++index) {
                this->expression(*expression.operands[index]);
            }
            break;
        }
        default:
            for (const Expr* operand : expression.operands) {
                this->expression(*operand);
            }
            break;
        }
        for (const FieldValue& field : expression.fields) {
            if (field.value != nullptr) {
                this->expression(*field.value);
            }
        }
        invariants(expression.invariants);
    }

    void builtin(const Expr& call, bool tested)
    {
        if (call.op == BuiltinOp::Link) {
            if (const Place* linked = linkedPlace(call)) {
                root(*linked, !tested);
                return;
            }
        }
        const bool test = call.op == BuiltinOp::Alive || call.op == BuiltinOp::Same;
        for (const Expr* operand : call.operands) {
            expression(bare(*operand), test);
        }
    }

    std::vector<int> parents_;
    int current_ = -1;
    std::unordered_map<const Symbol*, int> declaredIn_;
    std::vector<const Symbol*> roots_;
    SymbolSet candidates_;
    SymbolSet escaped_;
    SymbolSet escapedLinks_;
    SymbolSet foreign_;
    std::vector<Source> sources_;
    std::vector<std::pair<const Symbol*, const Place*>> uses_;
};

// Checks an earlier statement of the same block already made.
class ProvenChecks {
public:
    void run(const Routine& routine)
    {
        SymbolSet checked;
        body(routine.body, checked);
    }

private:
    struct Use {
        const Symbol* link = nullptr;
        const Place* place = nullptr;
        bool always = false;  // runs whenever the statement runs
    };
    struct Effects {
        bool mayFree = false;
        std::vector<Use> uses;
    };

    void body(Span<const Stmt*> statements, SymbolSet checked)
    {
        for (const Stmt* statement : statements) {
            this->statement(*statement, checked);
        }
    }

    static void apply(const Effects& effects, SymbolSet& checked)
    {
        if (effects.mayFree) {
            checked.clear();
            return;
        }
        for (const Use& use : effects.uses) {
            if (checked.contains(use.link)) {
                annotated(*use.place).linkAlive = true;
            }
        }
        for (const Use& use : effects.uses) {
            if (use.always) {
                checked.insert(use.link);
            }
        }
    }

    void statement(const Stmt& statement, SymbolSet& checked)
    {
        Effects effects;
        effects.mayFree = statement.discard || hasChecks(statement.invariants);
        switch (statement.kind) {
        case StmtKind::Declare:
        case StmtKind::Assign:
        case StmtKind::Evaluate:
        case StmtKind::Return:
        case StmtKind::Raise:
        case StmtKind::Until:
        case StmtKind::Retry:
            if (statement.value != nullptr) {
                expression(*statement.value, true, effects);
            }
            if (statement.kind == StmtKind::Assign) {
                // A Link written through another name (a `with` binding, a
                // reference parameter) may be one this block checked.
                effects.mayFree = effects.mayFree || holdsOwner(statement.target->type) ||
                                  (statement.target->type != nullptr && statement.target->type->isLink() &&
                                   !isOwnLinkVariable(*statement.target));
                place(*statement.target, true, effects);
            }
            apply(effects, checked);
            if (statement.kind == StmtKind::Declare) {
                checked.erase(statement.symbol);
            } else if (statement.kind == StmtKind::Assign && isVariableKind(statement.target->kind)) {
                checked.erase(statement.target->symbol);
            }
            return;
        case StmtKind::If:
        case StmtKind::Unless:
        case StmtKind::Case:
        case StmtKind::With:
        case StmtKind::Block:
            if (statement.value != nullptr) {
                expression(*statement.value, true, effects);
            }
            for (const Branch& branch : statement.branches) {
                for (const Expr* condition : branch.conditions) {
                    expression(*condition, false, effects);
                }
            }
            apply(effects, checked);
            // A `with` body may write a Link through its binding.
            body(statement.body, statement.kind == StmtKind::With ? SymbolSet{} : checked);
            for (const Branch& branch : statement.branches) {
                body(branch.body, checked);
            }
            body(statement.elseBody, checked);
            break;
        case StmtKind::ForIn:
            if (statement.value != nullptr) {
                expression(*statement.value, true, effects);
            }
            if (statement.step != nullptr) {
                expression(*statement.step, true, effects);
            }
            apply(effects, checked);
            body(statement.body, {});
            break;
        case StmtKind::While:
        case StmtKind::Repeat:
        case StmtKind::Try:
            body(statement.body, {});
            for (const Branch& branch : statement.branches) {
                body(branch.body, {});
            }
            body(statement.elseBody, {});
            if (const TryParts* parts = statement.tryParts) {
                body(parts->exceptBody, {});
                for (const Handler& handler : parts->handlers) {
                    body(handler.body, {});
                }
                body(parts->elseBody, {});
                body(parts->ensureBody, {});
            }
            break;
        case StmtKind::Exit:
        case StmtKind::Leave:
        case StmtKind::Continue:
            return;
        }
        // The end of a nested block may free what it declared.
        checked.clear();
    }

    static bool hasChecks(const Invariants* checks)
    {
        return checks != nullptr && (!checks->before.empty() || !checks->after.empty() || checks->newValue != nullptr);
    }

    void place(const Place& place, bool always, Effects& effects)
    {
        switch (place.kind) {
        case PlaceKind::Through:
            if (isOwnLinkVariable(*place.base)) {
                effects.uses.push_back(Use{place.base->symbol, place.base, always});
            } else {
                this->place(*place.base, always, effects);
            }
            break;
        case PlaceKind::Field:
            this->place(*place.base, always, effects);
            break;
        case PlaceKind::Element:
            this->place(*place.base, always, effects);
            for (const Expr* index : place.indices) {
                expression(*index, always, effects);
            }
            break;
        case PlaceKind::Value:
            expression(*place.value, always, effects);
            break;
        default:
            break;
        }
    }

    void expression(const Expr& expression, bool always, Effects& effects)
    {
        switch (expression.kind) {
        case ExprKind::Call:
        case ExprKind::Intrinsic:
            effects.mayFree = true;
            break;
        case ExprKind::Builtin:
            effects.mayFree = effects.mayFree || expression.op == BuiltinOp::Free || expression.op == BuiltinOp::Move;
            break;
        case ExprKind::Read:
            place(*expression.place, always, effects);
            break;
        default:
            break;
        }
        // A node bound to a routine of the program runs it.
        effects.mayFree = effects.mayFree || hasChecks(expression.invariants) ||
                          (expression.symbol != nullptr && expression.symbol->kind == semantic::SymbolKind::Function &&
                           expression.kind != ExprKind::Literal && expression.kind != ExprKind::TypeValue);
        for (std::size_t index = 0; index < expression.operands.size(); ++index) {
            const bool conditional = expression.kind == ExprKind::Check || expression.kind == ExprKind::Pure ||
                                     (expression.kind == ExprKind::Conditional && index > 0) ||
                                     (expression.kind == ExprKind::Binary && index > 0 &&
                                      (expression.binary == BinaryOp::And || expression.binary == BinaryOp::Or));
            this->expression(*expression.operands[index], always && !conditional, effects);
        }
        for (const FieldValue& field : expression.fields) {
            if (field.value != nullptr) {
                this->expression(*field.value, always, effects);
            }
        }
    }
};

} // namespace

void analyzeLinks(Routine& routine, Arena& arena)
{
    Escapes().run(routine, arena);
    ProvenChecks().run(routine);
}

} // namespace inox::compiler::tir
