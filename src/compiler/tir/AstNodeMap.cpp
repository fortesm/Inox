// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The answers code generation takes from the typed IR for the AST nodes it
// walks. See AstNodeMap.h.

#include "AstNodeMap.h"

namespace inox::compiler::tir {

const AstNodeMap::Entry& AstNodeMap::entryOf(const ast::Expression& node, const char* question) const
{
    const auto found = entries_.find(&node);
    if (found == entries_.end()) {
        std::string message = "internal error: typed IR: no TIR node for the ";
        message += question;
        message += " of an expression";
        if (node.position().known()) {
            message += " at line " + std::to_string(node.position().line);
        }
        throw InvariantError(message);
    }
    return found->second;
}

void AstNodeMap::recordWrittenAgain(const ast::Expression& copy, const ast::Expression& original)
{
    if (copy.kind() != original.kind()) {
        throw InvariantError("internal error: typed IR: a copy of a target that is not written as it");
    }
    entries_[&copy] = entryOf(original, "copy");
    switch (copy.kind()) {
    case ast::AstNodeKind::CallExpression: {
        const auto& a = static_cast<const ast::CallExpression&>(copy);
        const auto& b = static_cast<const ast::CallExpression&>(original);
        recordWrittenAgain(a.callee(), b.callee());
        for (std::size_t index = 0; index < a.arguments().size() && index < b.arguments().size(); ++index) {
            recordWrittenAgain(*a.arguments()[index], *b.arguments()[index]);
        }
        break;
    }
    case ast::AstNodeKind::BinaryExpression:
        recordWrittenAgain(static_cast<const ast::BinaryExpression&>(copy).left(),
                           static_cast<const ast::BinaryExpression&>(original).left());
        recordWrittenAgain(static_cast<const ast::BinaryExpression&>(copy).right(),
                           static_cast<const ast::BinaryExpression&>(original).right());
        break;
    case ast::AstNodeKind::UnaryExpression:
        recordWrittenAgain(static_cast<const ast::UnaryExpression&>(copy).operand(),
                           static_cast<const ast::UnaryExpression&>(original).operand());
        break;
    default:
        break;
    }
}

TypeRef AstNodeMap::typeOf(const ast::Expression& node) const
{
    const Entry& entry = entryOf(node, "type");
    if (entry.value != nullptr) {
        return entry.value->type;
    }
    if (entry.place != nullptr) {
        return entry.place->type;
    }
    if (entry.statement != nullptr) {
        // A declaration and its name: the declared type; an assignment: its target's.
        return entry.statement->kind == StmtKind::Declare ? entry.statement->type : entry.statement->target->type;
    }
    if (entry.member != nullptr) {
        return entry.member->type;
    }
    if (entry.construct != nullptr) {
        return entry.construct->type->fields()[entry.field].type;
    }
    if (entry.call != nullptr &&
        (entry.call->kind == ExprKind::Convert || entry.call->kind == ExprKind::Construct)) {
        return entry.call->type;  // `T(...)`: the name is the type T
    }
    return nullptr;  // the name of a routine, or of an encoding, has no value
}

const semantic::Symbol* AstNodeMap::symbolOf(const ast::Expression& node) const
{
    const Entry& entry = entryOf(node, "symbol");
    if (entry.call != nullptr) {
        return entry.call->symbol;
    }
    if (entry.declared) {
        return entry.statement->symbol;
    }
    if (entry.place != nullptr && entry.place->symbol != nullptr) {
        return entry.place->symbol;
    }
    if (entry.value == nullptr) {
        return nullptr;
    }
    if (entry.value->kind == ExprKind::Read && entry.value->place != nullptr) {
        return entry.value->place->symbol;
    }
    return entry.value->symbol;
}

const semantic::ConstantValue* AstNodeMap::constantOf(const ast::Expression& node) const
{
    const Entry& entry = entryOf(node, "constant");
    return entry.value != nullptr ? entry.value->constant : nullptr;
}

bool AstNodeMap::typeAgrees(const ast::Expression& node, TypeRef recorded) const
{
    const TypeRef type = typeOf(node);
    if (type == recorded) {
        return true;
    }
    if (recorded != nullptr) {
        return false;  // the analysis typed the node: only its type agrees
    }
    const Entry& entry = entryOf(node, "type");
    // An operation without a result: Void, where the analysis recorded none.
    if (entry.value != nullptr) {
        const ExprKind kind = entry.value->kind;
        const bool operation = kind == ExprKind::Call || kind == ExprKind::Builtin || kind == ExprKind::String ||
                               kind == ExprKind::Intrinsic;
        return operation && type != nullptr && type->isVoid();
    }
    // A written field or element the analysis did not type: its place's type.
    return entry.place != nullptr &&
           (entry.place->kind == PlaceKind::Field || entry.place->kind == PlaceKind::Element);
}

semantic::BuiltinOp AstNodeMap::builtinOf(const ast::Expression& node) const
{
    const Entry& entry = entryOf(node, "builtin");
    if (entry.value != nullptr && entry.value->kind == ExprKind::Builtin) {
        return entry.value->op;
    }
    if (entry.place != nullptr && entry.place->kind == PlaceKind::Through) {
        return semantic::BuiltinOp::Value;
    }
    return semantic::BuiltinOp::None;
}

std::optional<semantic::CallKind> AstNodeMap::callKindOf(const ast::Expression& node) const
{
    const Entry& entry = entryOf(node, "call kind");
    if (!entry.hasCallKind) {
        return std::nullopt;
    }
    return entry.callKind;
}

const ast::FunctionDeclaration* AstNodeMap::routineOf(const ast::Expression& node) const
{
    const Entry& entry = entryOf(node, "routine");
    const Expr* call = entry.call != nullptr ? entry.call : entry.value;
    if (call == nullptr || call->kind != ExprKind::Call || call->routine == nullptr) {
        return nullptr;
    }
    return call->routine->origin;
}

bool AstNodeMap::booleanOf(const ast::Expression& node) const
{
    const Entry& entry = entryOf(node, "Bool literal");
    if (entry.value == nullptr || entry.value->kind != ExprKind::Literal || entry.value->literal != LiteralKind::Bool) {
        throw InvariantError("internal error: typed IR: a Bool literal that is not one");
    }
    return entry.value->boolean;
}

bool AstNodeMap::isCallName(const ast::Expression& node) const
{
    return entryOf(node, "call name").call != nullptr;
}

bool AstNodeMap::isWithBinding(const ast::Expression& node) const
{
    const Entry& entry = entryOf(node, "with binding");
    const Place* place = entry.place;
    if (place == nullptr && entry.value != nullptr && entry.value->kind == ExprKind::Read) {
        place = entry.value->place;
    }
    return place != nullptr && place->kind == PlaceKind::WithBinding;
}

TypeRef AstNodeMap::typeValueOf(const ast::Expression& node) const
{
    const Entry& entry = entryOf(node, "type value");
    return entry.value != nullptr && entry.value->kind == ExprKind::TypeValue ? entry.value->type : nullptr;
}

const semantic::ModuleIntrinsic* AstNodeMap::intrinsicOf(const ast::Expression& node) const
{
    const Entry& entry = entryOf(node, "intrinsic");
    return entry.value != nullptr && entry.value->kind == ExprKind::Intrinsic ? entry.value->intrinsic : nullptr;
}

bool AstNodeMap::isSlice(const ast::Expression& node) const
{
    const Entry& entry = entryOf(node, "slice");
    return entry.value != nullptr && entry.value->kind == ExprKind::Slice;
}

TypeRef AstNodeMap::declaredType(const ast::VarStatement& declaration) const
{
    const auto found = declarations_.find(&declaration);
    if (found == declarations_.end()) {
        throw InvariantError("internal error: typed IR: no TIR declaration of " + declaration.name());
    }
    return found->second->type;
}

TypeRef AstNodeMap::declaredType(const ast::VariableDeclaration& declaration) const
{
    const auto found = states_.find(&declaration);
    if (found == states_.end()) {
        throw InvariantError("internal error: typed IR: no TIR State variable " + declaration.name);
    }
    return found->second->type;
}

} // namespace inox::compiler::tir
