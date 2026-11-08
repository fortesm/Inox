// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: safe pointers (OpenSpec change safe-pointers). No
// reference counting, no collector, no null:
//   - `Own<T>` is the single owner of a heap object made by `New`. It is never
//     copied: it is given away with `Move(P)`, and using P afterwards is a
//     compile error until P is assigned again. Its object is freed when the
//     owner ends, is overwritten, or by `Free(P)`.
//   - `Option<Own<T>>` is an owner that may be absent (`None`); an `Own` is
//     wrapped into it without `Some`.
//   - `Link<T>` reaches an owned object, a variable, a field or an element
//     without owning it. It is copied freely, is born empty, and every use
//     checks that its object still exists (`Alive` asks without trapping).
//   - `P.Field`, `P[I]` and `P.Method` go through a pointer with the dot;
//     `Value(P)` is the whole value, read or written.
// A struct or array that holds an owner is move-only, like the owner.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"

#include <string>
#include <iterator>
#include <string_view>

namespace inox::compiler::semantic {

using namespace detail;

namespace {

bool isNone(const ast::Expression& expression)
{
    return expression.kind() == ast::AstNodeKind::IdentifierExpression &&
           equalsIgnoreCase(static_cast<const ast::IdentifierExpression&>(expression).name(), "None");
}

const std::string kNeverCopied =
    "an owner is never copied: give it away with Move(X), or build a new one with New(...)";

} // namespace

// `Own<T>`, `Link<T>` and `Option<Own<T>>` as written.
TypeRef SemanticAnalyzer::resolvePointerType(const ast::TypeExpr& syntax, std::string_view written)
{
    const ast::TypeExpr& argument = syntax.element();
    if (equalsIgnoreCase(syntax.name, "Option")) {
        if (argument.form != ast::TypeExpr::Form::Generic || !equalsIgnoreCase(argument.name, "Own")) {
            throw SemanticError("Option holds an owner that may be absent: write Option<Own<T>> (other Option "
                                "types come with generics), not " + std::string(written));
        }
        return types().optionOwn(resolvePointerType(argument, ast::spell(argument)));
    }
    const bool own = equalsIgnoreCase(syntax.name, "Own");
    if (argument.form == ast::TypeExpr::Form::Generic &&
        (equalsIgnoreCase(argument.name, "Slice") || equalsIgnoreCase(argument.name, "Own") ||
         equalsIgnoreCase(argument.name, "Link") || equalsIgnoreCase(argument.name, "Option"))) {
        if (equalsIgnoreCase(argument.name, "Slice")) {
            throw SemanticError("a pointer to a slice is not supported: " + std::string(written));
        }
        throw SemanticError("a pointer to a pointer is not supported: " + std::string(written));
    }
    const TypeRef target = resolveTypeExpr(argument, ast::spell(argument));
    rejectFileIn(target, "reached by a pointer");
    return own ? types().own(target) : types().link(target);
}

bool SemanticAnalyzer::isMoveOnlyType(TypeRef type)
{
    if (type == nullptr) {
        return false;
    }
    if (type->isOwn() || type->isOptionOwn()) {
        return true;
    }
    if (type->isArray()) {
        return isMoveOnlyType(type->element());
    }
    if (type->isStruct()) {
        for (const types::StructField& field : type->fields()) {
            if (isMoveOnlyType(field.type)) {
                return true;
            }
        }
    }
    return false;
}

std::string SemanticAnalyzer::ownerFieldWithoutDefault(TypeRef type)
{
    if (type == nullptr) {
        return {};
    }
    if (type->isArray()) {
        return ownerFieldWithoutDefault(type->element());
    }
    if (!type->isStruct()) {
        return {};
    }
    for (const types::StructField& field : type->fields()) {
        if (field.type != nullptr && field.type->isOwn()) {
            return type->name() + "." + field.name;
        }
        if (std::string inner = ownerFieldWithoutDefault(field.type); !inner.empty()) {
            return inner;
        }
    }
    return {};
}

// A value that nobody else holds: a new object, an owner just given away, a
// routine's result, a construction, or None.
bool SemanticAnalyzer::isFreshOwnerValue(const ast::Expression& value) const
{
    if (isNone(value)) {
        return true;
    }
    if (value.kind() == ast::AstNodeKind::IdentifierExpression) {
        const Symbol* symbol = result_.symbolOf(static_cast<const ast::IdentifierExpression&>(value));
        return symbol != nullptr && symbol->kind == SymbolKind::Function;
    }
    if (value.kind() != ast::AstNodeKind::CallExpression) {
        return false;
    }
    const auto& call = static_cast<const ast::CallExpression&>(value);
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
        return true;  // a method call
    }
    const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
    if (name == "__index") {
        return false;
    }
    if (name == "__member") {
        const Symbol* symbol = result_.symbolOf(call);
        return symbol != nullptr && symbol->kind == SymbolKind::Function;
    }
    // `Fill(None)` (r3.81): a new array of empty owners.
    if (isFillCall(call)) {
        return true;
    }
    if ((equalsIgnoreCase(name, "New") || equalsIgnoreCase(name, "Move")) &&
        isPointerBuiltinCall(name, call.arguments().size())) {
        return true;
    }
    // A construction (`TPair(...)` builds an instance of the generic TPair) or
    // a conversion `T(X)`.
    if (result_.constructedStruct(call) != nullptr || types().isDeclared(name)) {
        return true;
    }
    const Symbol* symbol = result_.symbolOf(call);
    return symbol != nullptr && symbol->kind == SymbolKind::Function;
}

std::optional<bool> SemanticAnalyzer::canAssignPointer(TypeRef targetType, TypeRef valueType,
                                                       const ast::Expression& value) const
{
    const bool targetIsPointer = targetType != nullptr && targetType->isPointer();
    const bool valueIsPointer = valueType != nullptr && valueType->isPointer();
    const bool valueIsNone = valueType != nullptr && valueType->isNone();
    if (!targetIsPointer && !valueIsPointer && !valueIsNone && !isMoveOnlyType(targetType)) {
        return std::nullopt;
    }
    if (valueIsNone) {
        return targetType != nullptr && (targetType->isLink() || targetType->isOptionOwn());
    }
    if (targetType != nullptr && targetType->isLink()) {
        return targetType == valueType;
    }
    const bool sameType = targetType == valueType;
    const bool wraps = targetType != nullptr && targetType->isOptionOwn() && valueType != nullptr &&
                       valueType->isOwn() && targetType->element() == valueType;
    if (!sameType && !wraps) {
        return false;
    }
    if (!isFreshOwnerValue(value)) {
        if (targetIsPointer) {
            throw SemanticError(kNeverCopied);
        }
        throw SemanticError(types::nameOf(targetType) +
                            " holds an owner and is never copied: give it away with Move(X)");
    }
    return true;
}

bool SemanticAnalyzer::isPointerBuiltinCall(std::string_view name, std::size_t argumentCount) const
{
    const bool known = ((equalsIgnoreCase(name, "New") || equalsIgnoreCase(name, "Free") ||
                         equalsIgnoreCase(name, "Move") || equalsIgnoreCase(name, "Value") ||
                         equalsIgnoreCase(name, "Link") || equalsIgnoreCase(name, "Alive")) &&
                        argumentCount == 1) ||
                       (equalsIgnoreCase(name, "Same") && argumentCount == 2);
    // A routine of the program with the same name is called instead.
    return known && resolveFunctionSignature(name) == nullptr;
}

TypeRef SemanticAnalyzer::typeThroughPointer(TypeRef type)
{
    if (type != nullptr && type->isOptionOwn()) {
        throw SemanticError("an Option<Own<T>> may be None: reach it through Link(X) and test it with Alive");
    }
    if (type != nullptr && (type->isOwn() || type->isLink())) {
        return type->pointerTarget();
    }
    return type;
}

TypeRef SemanticAnalyzer::analyzePointerBuiltin(std::string_view name, const ast::CallExpression& call)
{
    const ast::Expression& argument = *call.arguments()[0];
    const std::string written(name);
    const auto noValue = [&](TypeRef type) {
        return type == nullptr || type->isVoid() || type->isNone();
    };
    const auto isPointer = [](TypeRef type) { return type != nullptr && type->isPointer(); };
    if (equalsIgnoreCase(name, "New")) {
        TypeRef target = nullptr;
        if (argument.kind() == ast::AstNodeKind::IdentifierExpression &&
            types().isDeclared(static_cast<const ast::IdentifierExpression&>(argument).name())) {
            // New(TName): a new object with the defaults of its type.
            const auto& typeName = static_cast<const ast::IdentifierExpression&>(argument);
            target = types().lookup(typeName.name());
            if (!target->isStruct()) {
                throw SemanticError("New(" + typeName.name() + ") builds a struct with its defaults; for a " +
                                    target->name() + " give a value: New(0)");
            }
            if (const std::string owner = ownerFieldWithoutDefault(target); !owner.empty()) {
                throw SemanticError(owner + " is an owner without default: build the value, New(" + target->name() +
                                    "(... := New(...)))");
            }
            requireBuildableHere(target);
            if (const std::string field = requiredFieldWithoutDefault(target); !field.empty()) {
                throw SemanticError("New(" + target->name() + ") has no value for " + field +
                                    ", which has no default (r3.82): build the value, New(" + target->name() +
                                    "(... := ...))");
            }
            result_.bind(typeName, resolveOrThrow(typeName.name()));
            result_.setExpressionType(typeName, target);
        } else {
            target = analyzeExpression(argument);
            if (noValue(target)) {
                throw SemanticError("New takes a value or a type: New(TName(...)) or New(TName)");
            }
            if (target->isSlice() || target->isPointer()) {
                throw SemanticError("New of a pointer or a slice is not supported: " + target->name());
            }
            rejectFileIn(target, "reached by a pointer");
            if (isMoveOnlyType(target) && !isFreshOwnerValue(argument)) {
                throw SemanticError(target->name() + " holds an owner and is never copied: give it away with Move(X)");
            }
        }
        return types().own(target);
    }

    const TypeRef type = analyzeExpression(argument);
    if (equalsIgnoreCase(name, "Same")) {
        const TypeRef other = analyzeExpression(*call.arguments()[1]);
        if (!isPointer(type) || !isPointer(other) || type->pointerTarget() != other->pointerTarget()) {
            throw SemanticError("Same compares two pointers to the same type, got " + types::nameOf(type) + " and " +
                                types::nameOf(other));
        }
        return types().boolType();
    }
    if (equalsIgnoreCase(name, "Alive")) {
        if (!isPointer(type)) {
            throw SemanticError("Alive asks a pointer (Own, Option<Own> or Link) whether its object exists, got " +
                                types::nameOf(type));
        }
        return types().boolType();
    }
    if (equalsIgnoreCase(name, "Value")) {
        if (type != nullptr && type->isOptionOwn()) {
            throw SemanticError("an Option<Own<T>> may be None: reach it through Link(X) and test it with Alive");
        }
        if (type == nullptr || (!type->isOwn() && !type->isLink())) {
            throw SemanticError("Value takes a pointer (Own or Link), got " + types::nameOf(type));
        }
        return type->pointerTarget();
    }
    if (equalsIgnoreCase(name, "Link")) {
        // A Link is a capability to write: never made from a place that is
        // read-only through its owners (change read-only-through-owners).
        if (const Symbol* owner = readOnlyOwnerRoot(argument, type != nullptr && type->isOwn())) {
            throwReadOnlyOwner("Link would allow writing a read-only place: ", *owner);
        }
        if (isPointer(type)) {
            return types().link(type->pointerTarget());
        }
        if (noValue(type)) {
            throw SemanticError("Link takes a variable, a field or an element");
        }
        if (type->isSlice() || isFileType(type)) {
            throw SemanticError("a Link to a " + type->name() + " is not supported");
        }
        // The place must be a variable of the routine, a State variable, or
        // a field or element of one, or a place reached through a pointer.
        const ast::Expression* root = &argument;
        bool throughPointer = false;
        while (root->kind() == ast::AstNodeKind::CallExpression) {
            const auto& inner = static_cast<const ast::CallExpression&>(*root);
            if (inner.callee().kind() != ast::AstNodeKind::IdentifierExpression || inner.arguments().empty()) {
                break;
            }
            const std::string& innerName = static_cast<const ast::IdentifierExpression&>(inner.callee()).name();
            if (innerName != "__member" && innerName != "__index") {
                break;
            }
            root = inner.arguments()[0].get();
            if (isPointer(result_.typeOf(*root))) {
                throughPointer = true;
                break;
            }
        }
        if (!throughPointer) {
            const Symbol* symbol = root->kind() == ast::AstNodeKind::IdentifierExpression
                                       ? symbols_.currentScope().resolve(
                                             static_cast<const ast::IdentifierExpression&>(*root).name())
                                       : nullptr;
            if (symbol == nullptr || (symbol->kind != SymbolKind::Variable && symbol->kind != SymbolKind::State)) {
                throw SemanticError("Link takes a variable, a field or an element of one, not a temporary value");
            }
            if (currentParameters_.contains(symbol)) {
                throw SemanticError("a Link to the parameter " + symbol->name +
                                    " is not supported yet: copy it into a variable first");
            }
            if (symbol->name.rfind("__", 0) == 0) {
                throw SemanticError("Link takes a variable, a field or an element of one");
            }
        }
        // The place is read twice (its address and its storage's header).
        if (!isSideEffectFree(argument)) {
            throw SemanticError("Link of a place whose index calls a routine: store the index in a variable "
                                "first");
        }
        return types().link(type);
    }

    // Free and Move.
    const bool isFree = equalsIgnoreCase(name, "Free");
    const bool ownerOrOption = type != nullptr && (type->isOwn() || type->isOptionOwn());
    if (isFree ? !ownerOrOption : !isMoveOnlyType(type)) {
        throw SemanticError(written + " takes an owner (Own or Option<Own>), got " + types::nameOf(type));
    }
    if (type->isOptionOwn()) {
        // The place becomes None.
        const bool isVariable = argument.kind() == ast::AstNodeKind::IdentifierExpression;
        const Symbol* writable = writableRoot(argument);
        if (writable == nullptr) {
            if (const Symbol* owner = readOnlyOwnerRoot(argument)) {
                throwReadOnlyOwner(written + " changes its argument, which is read-only: ", *owner);
            }
        }
        // A value parameter belongs to the routine; a `ref` parameter only
        // reads the caller's variable (change read-only-through-owners).
        const Symbol* parameter =
            isVariable ? result_.symbolOf(static_cast<const ast::IdentifierExpression&>(argument)) : nullptr;
        if (writable == nullptr && parameter != nullptr && currentParameters_.contains(parameter) &&
            parameter->deepReadOnly) {
            throw SemanticError(written + " changes its argument " + parameter->name +
                                ", a ref parameter, which only reads the caller's variable; declare ref mut " +
                                parameter->name + " (ADR-0016)");
        }
        if (writable == nullptr && !(parameter != nullptr && currentParameters_.contains(parameter))) {
            throw SemanticError(written + " changes its argument: give a mutable variable, a field or an element");
        }
        return isFree ? types().voidType() : type;
    }
    // An owner or a struct that holds one: only a variable gives it away,
    // and it cannot be used again until it is assigned.
    if (argument.kind() != ast::AstNodeKind::IdentifierExpression) {
        throw SemanticError("only a variable gives its owner away with " + written +
                            "; a field that may be emptied is Option<Own<T>>");
    }
    const Symbol* symbol = result_.symbolOf(static_cast<const ast::IdentifierExpression&>(argument));
    if (symbol == nullptr || symbol->kind != SymbolKind::Variable) {
        throw SemanticError("only a variable gives its owner away with " + written);
    }
    noteOwnerGivenAway(argument);
    return isFree ? types().voidType() : type;
}

void SemanticAnalyzer::noteOwnerGivenAway(const ast::Expression& owner)
{
    if (owner.kind() != ast::AstNodeKind::IdentifierExpression || !ownerPathReachable_) {
        return;
    }
    const Symbol* symbol = result_.symbolOf(static_cast<const ast::IdentifierExpression&>(owner));
    if (symbol == nullptr) {
        return;
    }
    for (const Scope* scope = &symbols_.currentScope(); scope != nullptr; scope = scope->parent()) {
        if (scope->containsLocal(symbol->name) && scope->resolve(symbol->name) == symbol) {
            movedOwners_[symbol] = scope;
            // An exception may leave from any point of a try: its handlers
            // and its `ensure` see every owner given away in it.
            for (OwnerFrame& frame : ownerFrames_) {
                if (frame.kind == OwnerFrameKind::Try) {
                    frame.givenAway[symbol] = scope;
                    if (frame.inEnsure) {
                        frame.givenAwayInEnsure[symbol] = scope;
                    }
                }
            }
            return;
        }
    }
}

void SemanticAnalyzer::noteOwnerAssigned(const Symbol& symbol)
{
    movedOwners_.erase(&symbol);
}

void SemanticAnalyzer::requireOwnerUsable(const Symbol& symbol) const
{
    if (movedOwners_.contains(&symbol)) {
        throw SemanticError(symbol.name + " was given away (Move or Free) on a path that reaches here: "
                            "assign it again before using it");
    }
}

// The owners of a scope die with it, on every path recorded for a target
// outside it.
void SemanticAnalyzer::forgetMovedOwnersOf(const Scope* scope)
{
    const auto forget = [scope](MovedOwners& owners) {
        std::erase_if(owners, [scope](const auto& entry) { return entry.second == scope; });
    };
    forget(movedOwners_);
    for (OwnerFrame& frame : ownerFrames_) {
        forget(frame.exits);
        forget(frame.nextRound);
        forget(frame.givenAway);
        forget(frame.givenAwayInEnsure);
        for (OwnerDeparture& departure : frame.departures) {
            forget(departure.moved);
        }
        for (OwnerDeparture& retry : frame.retries) {
            forget(retry.moved);
        }
    }
}

SemanticAnalyzer::OwnerState SemanticAnalyzer::ownerState() const
{
    return {movedOwners_, ownerPathReachable_};
}

void SemanticAnalyzer::setOwnerState(OwnerState state)
{
    movedOwners_ = std::move(state.moved);
    ownerPathReachable_ = state.reachable;
}

// Paths that meet: a path that reaches the meeting point brings its set.
void SemanticAnalyzer::joinCurrentOwnerState(OwnerState& into) const
{
    if (ownerPathReachable_) {
        into.moved.insert(movedOwners_.begin(), movedOwners_.end());
        into.reachable = true;
    }
}

// After `leave`, `continue`, `Return`, `Raise`, `Exit` or `Retry` no path
// reaches the next statement; the set was recorded for its target first.
void SemanticAnalyzer::endOwnerPath()
{
    movedOwners_.clear();
    ownerPathReachable_ = false;
}

// The current set goes to the frame `target`; a try with `ensure` between
// holds it until `ensure` has run (CANON-15).
void SemanticAnalyzer::sendOwnersTo(std::size_t target, OwnerDepartureKind kind, std::size_t from,
                                    const ast::SourcePosition& position)
{
    // A routine exit crosses every try with `ensure` around it.
    const std::size_t stop = kind == OwnerDepartureKind::RoutineExit ? 0 : target + 1;
    for (std::size_t index = from; index > stop; --index) {
        OwnerFrame& frame = ownerFrames_[index - 1];
        if (frame.kind == OwnerFrameKind::Try && frame.hasEnsure && !frame.inEnsure) {
            frame.departures.push_back({target, kind, movedOwners_, position});
            return;
        }
    }
    if (kind == OwnerDepartureKind::RoutineExit) {
        requireRefMutOwnersBack(&position);
        return;
    }
    OwnerFrame& frame = ownerFrames_[target];
    switch (kind) {
    case OwnerDepartureKind::LoopExit:
        frame.exits.insert(movedOwners_.begin(), movedOwners_.end());
        break;
    case OwnerDepartureKind::NextRound:
        frame.nextRound.insert(movedOwners_.begin(), movedOwners_.end());
        break;
    case OwnerDepartureKind::Retry:
        frame.retries.push_back({target, kind, movedOwners_, position});
        break;
    case OwnerDepartureKind::RoutineExit:
    case OwnerDepartureKind::ExceptionExit:
        break;
    }
}

// `Return` and `Exit`: the routine's normal exits, after every `ensure`
// they cross.
void SemanticAnalyzer::sendOwnersOutOfRoutine(const ast::SourcePosition& position)
{
    if (ownerPathReachable_ && !refMutParameters_.empty()) {
        sendOwnersTo(0, OwnerDepartureKind::RoutineExit, ownerFrames_.size(), position);
    }
}

// CANON-25: a routine gives back every owner it received as `ref mut`
// before it returns, so the caller's owner is valid after the call.
void SemanticAnalyzer::requireRefMutOwnersBack(const ast::SourcePosition* position, bool byException)
{
    for (const Symbol* parameter : refMutParameters_) {
        if (movedOwners_.contains(parameter)) {
            if (position != nullptr) {
                notePosition(*position);
            }
            if (byException) {
                throw SemanticError(parameter->name + " is received as ref mut and given away (Move or Free) "
                                    "where an exception may leave the routine: assign it again first");
            }
            throw SemanticError(parameter->name + " is received as ref mut and given away (Move or Free) on a "
                                "path that returns: assign it again before the routine returns");
        }
    }
}

// CANON-25: a routine gives back an owner it received as `ref mut` before
// an exception leaves it. At a point where an exception may leave the
// statement, the current set goes where the exception goes.
void SemanticAnalyzer::noteRaisingPoint(TypeRef raised, const ast::SourcePosition& position)
{
    // An `ensure` the exception crosses may also give the owner away.
    if (ownerPathReachable_ && !refMutParameters_.empty()) {
        routeOwnersByException(ownerFrames_.size(), raised, position);
    }
}

// An invariant check (CANON-24) may call a routine that raises.
void SemanticAnalyzer::noteInvariantRaisingPoint(const ast::AstNode& site)
{
    noteRaisingPoint(nullptr, site.position().known() ? site.position() : diagnosticPlace().position);
}

// A call that may raise (CallMeaning.h, callMayRaise): a routine, a method,
// a text-to-number conversion, a Std.Files operation.
void SemanticAnalyzer::noteRaisingCall(const ast::Expression& expression)
{
    if (!ownerPathReachable_ || refMutParameters_.empty()) {
        return;
    }
    const CallMeaning* meaning = result_.callMeaning(expression);
    if (meaning == nullptr) {
        return;
    }
    bool textSource = false;
    if (meaning->kind == CallKind::Conversion && expression.kind() == ast::AstNodeKind::CallExpression) {
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        const TypeRef source = call.arguments().empty() ? nullptr : result_.typeOf(*call.arguments()[0]);
        textSource = source != nullptr && source->isString();
    }
    if (callMayRaise(meaning->kind, meaning->op, textSource)) {
        // An expression without a position of its own reports its statement.
        noteRaisingPoint(nullptr, expression.position().known() ? expression.position()
                                                                : diagnosticPlace().position);
    }
}

// The way out of an exception from frame `from` inward: a try whose body
// holds the point catches it when it catches everything or provably the
// raised type; a handler's or an `ensure`'s exception leaves its own try;
// a try with `ensure` holds the path until `ensure` has run; at the
// routine's edge every `ref mut` owner must be back.
void SemanticAnalyzer::routeOwnersByException(std::size_t from, TypeRef raised, const ast::SourcePosition& position)
{
    for (std::size_t index = from; index > 0; --index) {
        OwnerFrame& frame = ownerFrames_[index - 1];
        if (frame.kind != OwnerFrameKind::Try || frame.inEnsure) {
            continue;
        }
        if (!frame.inHandler) {
            if (frame.catchesAll) {
                return;
            }
            if (raised != nullptr) {
                for (const TypeRef handlerType : frame.handlerTypes) {
                    if (raised->isExceptionOf(handlerType)) {
                        return;
                    }
                }
            }
        }
        if (frame.hasEnsure) {
            frame.departures.push_back({0, OwnerDepartureKind::ExceptionExit, movedOwners_, position, raised});
            return;
        }
    }
    requireRefMutOwnersBack(&position, true);
}

// CANON-25: an exception may leave the callee between giving a `ref mut`
// owner away and assigning it again, so the handlers and the `ensure` of
// the enclosing trys see it as given away; after a normal return it is
// valid.
void SemanticAnalyzer::noteOwnerLentToCall(const ast::Expression& argument)
{
    if (argument.kind() != ast::AstNodeKind::IdentifierExpression || !ownerPathReachable_) {
        return;
    }
    const Symbol* symbol = result_.symbolOf(static_cast<const ast::IdentifierExpression&>(argument));
    if (symbol == nullptr || !isMoveOnlyType(symbol->type)) {
        return;
    }
    for (const Scope* scope = &symbols_.currentScope(); scope != nullptr; scope = scope->parent()) {
        if (scope->containsLocal(symbol->name) && scope->resolve(symbol->name) == symbol) {
            for (OwnerFrame& frame : ownerFrames_) {
                if (frame.kind == OwnerFrameKind::Try) {
                    frame.givenAway[symbol] = scope;
                }
            }
            return;
        }
    }
}

// `leave` and `continue` act on the innermost loop, `until` on the
// innermost repeat.
void SemanticAnalyzer::sendOwnersOutOfLoop(OwnerDepartureKind kind, bool repeatOnly,
                                           const ast::SourcePosition& position)
{
    if (!ownerPathReachable_) {
        return;
    }
    for (std::size_t index = ownerFrames_.size(); index > 0; --index) {
        const OwnerFrameKind frameKind = ownerFrames_[index - 1].kind;
        if (frameKind == OwnerFrameKind::Repeat || (!repeatOnly && frameKind == OwnerFrameKind::Loop)) {
            sendOwnersTo(index - 1, kind, ownerFrames_.size(), position);
            return;
        }
    }
}

void SemanticAnalyzer::enterLoopOwners(OwnerFrameKind kind)
{
    OwnerFrame frame;
    frame.kind = kind;
    frame.loopStart = ownerState();
    ownerFrames_.push_back(std::move(frame));
}

std::size_t SemanticAnalyzer::enterTryOwners(const ast::TryStatement& tryStatement)
{
    OwnerFrame frame;
    frame.kind = OwnerFrameKind::Try;
    frame.hasEnsure = tryStatement.hasEnsure();
    // CANON-15: a plain except catches every Inox exception, Else every one
    // the On handlers miss, On Exception every one.
    frame.catchesAll = tryStatement.hasPlainExcept() || !tryStatement.elseBody().empty();
    for (const auto& handler : tryStatement.handlers()) {
        if (const TypeRef type = exceptionTypeNamed(handler.typeName)) {
            frame.handlerTypes.push_back(type);
            frame.catchesAll = frame.catchesAll || type->exceptionParent() == nullptr;
        }
    }
    frame.startOfTry = movedOwners_;
    ownerFrames_.push_back(std::move(frame));
    return ownerFrames_.size() - 1;
}

// A handler may start after any operation of the body: it sees the owners
// given away before the try and anywhere in its body.
void SemanticAnalyzer::endTryBodyOwners(std::size_t tryIndex)
{
    OwnerFrame& frame = ownerFrames_[tryIndex];
    joinCurrentOwnerState(frame.afterTry);
    frame.handlerStart = frame.startOfTry;
    frame.handlerStart.insert(frame.givenAway.begin(), frame.givenAway.end());
}

void SemanticAnalyzer::enterHandlerOwners(std::size_t tryIndex, bool retryHandler)
{
    OwnerFrame& frame = ownerFrames_[tryIndex];
    frame.inHandler = true;
    frame.inRetryHandler = retryHandler;
    setOwnerState({frame.handlerStart, true});
}

void SemanticAnalyzer::endHandlerOwners(std::size_t tryIndex)
{
    OwnerFrame& frame = ownerFrames_[tryIndex];
    frame.inHandler = false;
    frame.inRetryHandler = false;
    joinCurrentOwnerState(frame.afterTry);
}

// `ensure` is analyzed once, from every set that may reach it.
void SemanticAnalyzer::enterEnsureOwners(std::size_t tryIndex)
{
    OwnerFrame& frame = ownerFrames_[tryIndex];
    frame.inEnsure = true;
    MovedOwners entry = frame.startOfTry;
    entry.insert(frame.givenAway.begin(), frame.givenAway.end());
    setOwnerState({std::move(entry), true});
}

// Every path leaving the try runs `ensure` first: with X the set at the end
// of `ensure` and M the owners it gives away, a path with set S leaves with
// X ∩ (S ∪ M). A Retry then checks it against the try's start.
void SemanticAnalyzer::leaveTryOwners(std::size_t tryIndex)
{
    OwnerFrame frame = std::move(ownerFrames_[tryIndex]);
    ownerFrames_.pop_back();
    if (!frame.hasEnsure) {
        setOwnerState(std::move(frame.afterTry));
    } else {
        const OwnerState ensureEnd = ownerState();
        const auto afterEnsure = [&](const MovedOwners& before) {
            MovedOwners after;
            for (const auto& entry : ensureEnd.moved) {
                if (before.contains(entry.first) || frame.givenAwayInEnsure.contains(entry.first)) {
                    after.insert(entry);
                }
            }
            return after;
        };
        for (OwnerDeparture& departure : frame.departures) {
            departure.moved = afterEnsure(departure.moved);
        }
        for (OwnerDeparture& retry : frame.retries) {
            retry.moved = afterEnsure(retry.moved);
        }
        setOwnerState({afterEnsure(frame.afterTry.moved), frame.afterTry.reachable && ensureEnd.reachable});
        if (ensureEnd.reachable) {
            const OwnerState current = ownerState();
            for (const OwnerDeparture& departure : frame.departures) {
                movedOwners_ = departure.moved;
                if (departure.kind == OwnerDepartureKind::ExceptionExit) {
                    routeOwnersByException(tryIndex, departure.raised, departure.position);
                } else {
                    sendOwnersTo(departure.target, departure.kind, tryIndex, departure.position);
                }
            }
            setOwnerState(current);
        }
    }
    for (const OwnerDeparture& retry : frame.retries) {
        for (const auto& entry : retry.moved) {
            if (!frame.startOfTry.contains(entry.first)) {
                notePosition(retry.position);
                throw SemanticError(entry.first->name + " is given away (Move or Free) before Retry: assign it "
                                    "again before Retry");
            }
        }
    }
}

} // namespace inox::compiler::semantic
