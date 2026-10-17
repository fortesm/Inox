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
#include "../types/TypeSyntax.h"

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
TypeRef SemanticAnalyzer::resolvePointerType(const types::TypeSyntax& syntax, std::string_view written)
{
    const types::TypeSyntax& argument = *syntax.argument;
    if (equalsIgnoreCase(syntax.name, "Option")) {
        if (argument.form != types::TypeSyntax::Form::Generic || !equalsIgnoreCase(argument.name, "Own")) {
            throw SemanticError("Option holds an owner that may be absent: write Option<Own<T>> (other Option "
                                "types come with generics), not " + std::string(written));
        }
        return types().optionOwn(resolvePointerType(argument, argument.spelling()));
    }
    const bool own = equalsIgnoreCase(syntax.name, "Own");
    if (argument.form == types::TypeSyntax::Form::Generic) {
        if (equalsIgnoreCase(argument.name, "Slice")) {
            throw SemanticError("a pointer to a slice is not supported: " + std::string(written));
        }
        throw SemanticError("a pointer to a pointer is not supported: " + std::string(written));
    }
    const TypeRef target = resolveTypeSyntax(argument, argument.spelling());
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
    if ((equalsIgnoreCase(name, "New") || equalsIgnoreCase(name, "Move")) &&
        isPointerBuiltinCall(name, call.arguments().size())) {
        return true;
    }
    if (types().isDeclared(name)) {
        return true;  // a construction
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
        if (writable == nullptr && !(isVariable && currentParameters_.contains(
                                         result_.symbolOf(static_cast<const ast::IdentifierExpression&>(argument))))) {
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
    if (owner.kind() != ast::AstNodeKind::IdentifierExpression) {
        return;
    }
    const Symbol* symbol = result_.symbolOf(static_cast<const ast::IdentifierExpression&>(owner));
    if (symbol == nullptr) {
        return;
    }
    for (const Scope* scope = &symbols_.currentScope(); scope != nullptr; scope = scope->parent()) {
        if (scope->containsLocal(symbol->name) && scope->resolve(symbol->name) == symbol) {
            movedOwners_[symbol] = scope;
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

void SemanticAnalyzer::forgetMovedOwnersOf(const Scope* scope)
{
    for (auto it = movedOwners_.begin(); it != movedOwners_.end();) {
        it = it->second == scope ? movedOwners_.erase(it) : std::next(it);
    }
}

} // namespace inox::compiler::semantic
