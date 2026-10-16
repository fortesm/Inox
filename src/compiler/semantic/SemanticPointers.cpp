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
#include "../support/ArrayType.h"
#include "../support/PointerType.h"

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

std::string SemanticAnalyzer::canonicalPointerTypeName(std::string_view name) const
{
    if (support::isOptionTypeName(name)) {
        const std::string inner = support::optionInnerTypeName(name);
        if (!support::isOwnTypeName(inner)) {
            throw SemanticError("Option holds an owner that may be absent: write Option<Own<T>> (other Option "
                                "types come with generics), not " + std::string(name));
        }
        return "Option<" + canonicalPointerTypeName(inner) + ">";
    }
    const bool own = support::isOwnTypeName(name);
    const std::string target = support::pointerTargetTypeName(name);
    if (support::isGenericTypeName(target) && !support::isSliceTypeName(target)) {
        throw SemanticError("a pointer to a pointer is not supported: " + std::string(name));
    }
    if (support::isSliceTypeName(target)) {
        throw SemanticError("a pointer to a slice is not supported: " + std::string(name));
    }
    resolveTypeOrThrow(target);
    const std::string canonical = canonicalTypeName(target);
    rejectFileIn(canonical, "reached by a pointer");
    return own ? support::makeOwnTypeName(canonical) : support::makeLinkTypeName(canonical);
}

bool SemanticAnalyzer::isMoveOnlyType(std::string_view typeName) const
{
    if (support::isOwnTypeName(typeName) || support::isOptionOwnTypeName(typeName)) {
        return true;
    }
    if (support::isLinkTypeName(typeName) || support::isSliceTypeName(typeName)) {
        return false;
    }
    if (const auto parts = support::splitArrayTypeName(typeName)) {
        return isMoveOnlyType(parts->element);
    }
    if (const StructType* structType = resolveStruct(typeName)) {
        for (const StructField& field : structType->fields) {
            if (isMoveOnlyType(field.typeName)) {
                return true;
            }
        }
    }
    return false;
}

std::string SemanticAnalyzer::ownerFieldWithoutDefault(std::string_view typeName) const
{
    if (const auto parts = support::splitArrayTypeName(typeName)) {
        return ownerFieldWithoutDefault(parts->element);
    }
    const StructType* structType = resolveStruct(typeName);
    if (structType == nullptr) {
        return {};
    }
    for (const StructField& field : structType->fields) {
        if (support::isOwnTypeName(field.typeName)) {
            return structType->name + "." + field.name;
        }
        if (std::string inner = ownerFieldWithoutDefault(field.typeName); !inner.empty()) {
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
    if (types_.resolve(name) != nullptr) {
        return true;  // a construction
    }
    const Symbol* symbol = result_.symbolOf(call);
    return symbol != nullptr && symbol->kind == SymbolKind::Function;
}

std::optional<bool> SemanticAnalyzer::canAssignPointer(std::string_view targetType, std::string_view valueType,
                                                       const ast::Expression& value) const
{
    const bool valueIsNone = valueType == "None";
    if (!support::isPointerTypeName(targetType) && !support::isPointerTypeName(valueType) && !valueIsNone &&
        !isMoveOnlyType(targetType)) {
        return std::nullopt;
    }
    if (valueIsNone) {
        return support::isLinkTypeName(targetType) || support::isOptionOwnTypeName(targetType);
    }
    if (support::isLinkTypeName(targetType)) {
        return targetType == valueType;
    }
    const bool sameType = targetType == valueType;
    const bool wraps = support::isOptionOwnTypeName(targetType) && support::isOwnTypeName(valueType) &&
                       support::optionInnerTypeName(targetType) == valueType;
    if (!sameType && !wraps) {
        return false;
    }
    if (!isFreshOwnerValue(value)) {
        if (support::isPointerTypeName(targetType)) {
            throw SemanticError(kNeverCopied);
        }
        throw SemanticError(std::string(targetType) +
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

std::string SemanticAnalyzer::typeThroughPointer(const std::string& typeName) const
{
    if (support::isOptionOwnTypeName(typeName)) {
        throw SemanticError("an Option<Own<T>> may be None: reach it through Link(X) and test it with Alive");
    }
    if (support::isOwnTypeName(typeName) || support::isLinkTypeName(typeName)) {
        return support::pointerTargetTypeName(typeName);
    }
    return typeName;
}

std::string SemanticAnalyzer::analyzePointerBuiltin(std::string_view name, const ast::CallExpression& call)
{
    const ast::Expression& argument = *call.arguments()[0];
    const std::string written(name);
    if (equalsIgnoreCase(name, "New")) {
        std::string target;
        if (argument.kind() == ast::AstNodeKind::IdentifierExpression &&
            types_.resolve(static_cast<const ast::IdentifierExpression&>(argument).name()) != nullptr) {
            // New(TName): a new object with the defaults of its type.
            const auto& typeName = static_cast<const ast::IdentifierExpression&>(argument);
            target = canonicalTypeName(typeName.name());
            if (resolveStruct(target) == nullptr) {
                throw SemanticError("New(" + typeName.name() + ") builds a struct with its defaults; for a " +
                                    target + " give a value: New(0)");
            }
            if (const std::string owner = ownerFieldWithoutDefault(target); !owner.empty()) {
                throw SemanticError(owner + " is an owner without default: build the value, New(" + target +
                                    "(... := New(...)))");
            }
            result_.bind(typeName, resolveOrThrow(typeName.name()));
            result_.setExpressionType(typeName, resolvedType(target));
        } else {
            target = canonicalTypeName(analyzeExpression(argument));
            if (target.empty() || target == "Void" || target == "None") {
                throw SemanticError("New takes a value or a type: New(TName(...)) or New(TName)");
            }
            if (support::isGenericTypeName(target)) {
                throw SemanticError("New of a pointer or a slice is not supported: " + target);
            }
            rejectFileIn(target, "reached by a pointer");
            if (isMoveOnlyType(target) && !isFreshOwnerValue(argument)) {
                throw SemanticError(target + " holds an owner and is never copied: give it away with Move(X)");
            }
        }
        return support::makeOwnTypeName(target);
    }

    const std::string type = canonicalTypeName(analyzeExpression(argument));
    if (equalsIgnoreCase(name, "Same")) {
        const std::string other = canonicalTypeName(analyzeExpression(*call.arguments()[1]));
        if (!support::isPointerTypeName(type) || !support::isPointerTypeName(other) ||
            support::pointerTargetTypeName(type) != support::pointerTargetTypeName(other)) {
            throw SemanticError("Same compares two pointers to the same type, got " + type + " and " + other);
        }
        return "Bool";
    }
    if (equalsIgnoreCase(name, "Alive")) {
        if (!support::isPointerTypeName(type)) {
            throw SemanticError("Alive asks a pointer (Own, Option<Own> or Link) whether its object exists, got " +
                                type);
        }
        return "Bool";
    }
    if (equalsIgnoreCase(name, "Value")) {
        if (support::isOptionOwnTypeName(type)) {
            throw SemanticError("an Option<Own<T>> may be None: reach it through Link(X) and test it with Alive");
        }
        if (!support::isOwnTypeName(type) && !support::isLinkTypeName(type)) {
            throw SemanticError("Value takes a pointer (Own or Link), got " + type);
        }
        return support::pointerTargetTypeName(type);
    }
    if (equalsIgnoreCase(name, "Link")) {
        if (support::isPointerTypeName(type)) {
            return support::makeLinkTypeName(support::pointerTargetTypeName(type));
        }
        if (type.empty() || type == "Void" || type == "None") {
            throw SemanticError("Link takes a variable, a field or an element");
        }
        if (support::isSliceTypeName(type) || isFileType(type)) {
            throw SemanticError("a Link to a " + type + " is not supported");
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
            if (const ResolvedType* baseType = result_.typeOf(*root);
                baseType != nullptr && support::isPointerTypeName(baseType->name)) {
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
        return support::makeLinkTypeName(type);
    }

    // Free and Move.
    const bool isFree = equalsIgnoreCase(name, "Free");
    if (isFree ? !(support::isOwnTypeName(type) || support::isOptionOwnTypeName(type)) : !isMoveOnlyType(type)) {
        throw SemanticError(written + " takes an owner (Own or Option<Own>), got " + type);
    }
    if (support::isOptionOwnTypeName(type)) {
        // The place becomes None.
        const bool isVariable = argument.kind() == ast::AstNodeKind::IdentifierExpression;
        const Symbol* writable = writableRoot(argument);
        if (writable == nullptr && !(isVariable && currentParameters_.contains(
                                         result_.symbolOf(static_cast<const ast::IdentifierExpression&>(argument))))) {
            throw SemanticError(written + " changes its argument: give a mutable variable, a field or an element");
        }
        return isFree ? "Void" : type;
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
    return isFree ? "Void" : type;
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
