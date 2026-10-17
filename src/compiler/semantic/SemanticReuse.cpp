// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: method reuse (CANON-9; OpenSpec change method-reuse;
// Marcelo Fortes, 2026-10-13: "Aprovo tudo como você sugeriu 1a, 2a, 3b e
// 4a+b").
//
// `TSubscriber Struct (TPerson)` reuses TPerson: the part is a field named
// TPerson, and TSubscriber also offers TPerson's fields and methods directly,
// at every level (TPerson's own parts too). A reused method runs on its part
// (delegation): `S.UpdateName(X)` is `S.TPerson.UpdateName(X)`. Nothing is
// chosen silently: a name reachable by two paths, or a struct's own field or
// method with a reused name, is an error at the struct's declaration, except
// a method marked `Override`, which replaces the reused ones for callers of
// the outer struct. There is no subtyping: a TSubscriber is never a TPerson.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace inox::compiler::semantic {

using namespace detail;

namespace {

std::string pathText(const std::string& outer, const ReusedMember& member)
{
    std::string text = outer;
    for (const std::string& part : member.path) {
        text += "." + part;
    }
    return text + "." + member.name;
}

std::string signatureText(const FunctionSignature& signature)
{
    std::string text = signature.name + "(Self";
    if (signature.parameters.front().mode == PassMode::RefMut) {
        text += " mut";
    }
    for (std::size_t index = 1; index < signature.parameters.size(); ++index) {
        const FunctionParameter& parameter = signature.parameters[index];
        text += ", ";
        if (parameter.mode == PassMode::Ref) {
            text += "ref ";
        } else if (parameter.mode == PassMode::RefMut) {
            text += "ref mut ";
        }
        text += types::nameOf(parameter.type);
    }
    text += ")";
    if (signature.returnType != nullptr) {
        text += " " + signature.returnType->name();
    }
    return text;
}

bool sameSignature(const FunctionSignature& left, const FunctionSignature& right)
{
    if (left.parameters.size() != right.parameters.size() || left.returnType != right.returnType ||
        left.parameters.front().mode != right.parameters.front().mode) {
        return false;
    }
    for (std::size_t index = 1; index < left.parameters.size(); ++index) {
        if (left.parameters[index].mode != right.parameters[index].mode ||
            left.parameters[index].type != right.parameters[index].type) {
            return false;
        }
    }
    return true;
}

} // namespace

std::vector<std::string> SemanticAnalyzer::ownMethodNames(std::string_view typeName) const
{
    const std::string prefix = normalizeName(typeName) + ".";
    std::vector<std::string> names;
    for (const auto& [key, signature] : functions_) {
        // Hidden methods (`__invariant`, a contract's `__Name_body`) belong
        // to their own struct and are never reused (CANON-24).
        if (key.size() > prefix.size() && key.compare(0, prefix.size(), prefix) == 0 &&
            normalizeName(signature.name).compare(0, prefix.size(), prefix) == 0 &&
            key.compare(prefix.size(), 2, "__") != 0) {
            names.push_back(signature.name.substr(signature.name.find('.') + 1));
        }
    }
    return names;
}

// The fields and methods a struct offers through its parts, each with the
// path to it. A part's own Override method hides the members it replaces.
std::vector<ReusedMember> SemanticAnalyzer::reusedMembers(TypeRef structType) const
{
    std::vector<ReusedMember> members;
    if (structType == nullptr || !structType->isStruct()) {
        return members;
    }
    for (const types::StructField& part : structType->fields()) {
        if (!part.isPart) {
            continue;
        }
        const TypeRef partType = part.type;
        if (partType == nullptr || !partType->isStruct()) {
            continue;
        }
        for (const types::StructField& field : partType->fields()) {
            members.push_back(ReusedMember{{part.name}, field.name, partType->name(), false});
        }
        const std::vector<std::string> partMethods = ownMethodNames(partType->name());
        for (const std::string& method : partMethods) {
            members.push_back(ReusedMember{{part.name}, method, partType->name(), true});
        }
        for (ReusedMember inner : reusedMembers(partType)) {
            bool replaced = false;
            for (const std::string& method : partMethods) {
                replaced = replaced || equalsIgnoreCase(method, inner.name);
            }
            if (replaced) {
                continue;
            }
            inner.path.insert(inner.path.begin(), part.name);
            members.push_back(std::move(inner));
        }
    }
    return members;
}

// The reused member `name` of a struct, when the struct has no field or
// method of its own with that name. Ambiguities were rejected at the
// struct's declaration (validateReuse).
std::optional<ReusedMember> SemanticAnalyzer::findReusedMember(TypeRef structType, std::string_view name) const
{
    for (ReusedMember& member : reusedMembers(structType)) {
        if (equalsIgnoreCase(member.name, name)) {
            return std::move(member);
        }
    }
    return std::nullopt;
}

void SemanticAnalyzer::validateReuse(const ast::TypeDeclaration& declaration) const
{
    const TypeRef structType = resolveStruct(declaration.name);
    const std::string& outer = structType->name();
    for (const types::StructField& part : structType->fields()) {
        if (part.isPart && (part.type == nullptr || !part.type->isStruct())) {
            throw SemanticError("a struct reuses only structs (CANON-9): " + outer + " Struct (" + part.name +
                                "), and " + part.name + " is not a struct");
        }
    }

    const std::vector<ReusedMember> members = reusedMembers(structType);
    const std::vector<std::string> ownMethods = ownMethodNames(outer);
    const auto ownMethod = [&](const std::string& name) -> const FunctionSignature* {
        for (const std::string& method : ownMethods) {
            if (equalsIgnoreCase(method, name)) {
                return resolveFunctionSignature(outer + "." + method);
            }
        }
        return nullptr;
    };

    // Group the reused members by name, keeping the declaration order.
    std::map<std::string, std::vector<const ReusedMember*>> byName;
    std::vector<std::string> order;
    for (const ReusedMember& member : members) {
        const std::string key = normalizeName(member.name);
        if (!byName.contains(key)) {
            order.push_back(key);
        }
        byName[key].push_back(&member);
    }

    for (const std::string& key : order) {
        const std::vector<const ReusedMember*>& reached = byName[key];
        const ReusedMember& first = *reached.front();

        for (const types::StructField& field : structType->fields()) {
            if (equalsIgnoreCase(field.name, first.name)) {
                throw SemanticError(outer + "." + field.name + " has the name of " + pathText(outer, first) +
                                    ", which " + outer + " reuses (CANON-9): rename the field, or make " +
                                    first.path.front() + " a named field instead of reusing it");
            }
        }

        const FunctionSignature* replacement = ownMethod(first.name);
        bool allMethods = true;
        for (const ReusedMember* member : reached) {
            allMethods = allMethods && member->isMethod;
        }
        if (replacement != nullptr) {
            if (!allMethods) {
                throw SemanticError(outer + "." + first.name + " is a method, but " + outer +
                                    " reuses a field with that name (CANON-9): rename the method");
            }
            if (!replacement->isOverride) {
                throw SemanticError(outer + "." + first.name + " has the name of " + pathText(outer, first) +
                                    ", which " + outer + " reuses (CANON-9): mark it Override to replace "
                                    "it, or rename it");
            }
            for (const ReusedMember* member : reached) {
                const FunctionSignature* replaced = resolveFunctionSignature(member->owner + "." + member->name);
                if (replaced != nullptr && !sameSignature(*replacement, *replaced)) {
                    throw SemanticError(outer + "." + first.name + " Override must have the signature of " +
                                        signatureText(*replaced) + " (CANON-9)");
                }
            }
            continue;
        }

        if (reached.size() > 1) {
            std::string paths;
            for (const ReusedMember* member : reached) {
                paths += (paths.empty() ? "" : " and ") + pathText(outer, *member);
            }
            throw SemanticError(outer + " reaches " + first.name + " by two paths, " + paths +
                                " (CANON-9): " +
                                (allMethods ? "declare " + outer + "." + first.name +
                                                  " Override, or make one of the parts a named field"
                                            : "make one of the parts a named field"));
        }
    }

    for (const std::string& method : ownMethods) {
        const FunctionSignature* signature = resolveFunctionSignature(outer + "." + method);
        if (signature != nullptr && signature->isOverride && !byName.contains(normalizeName(method))) {
            throw SemanticError(outer + "." + method + " is marked Override, but " + outer +
                                " reuses no method " + method + " (CANON-9)");
        }
    }
}

// Rewrites the receiver of `member` (`X.Name`) into the explicit path
// through the parts (`X.TPerson.Name`); the caller analyzes it again.
void SemanticAnalyzer::routeThroughParts(const ast::CallExpression& member, const std::vector<std::string>& path)
{
    // The analyzer otherwise only reads the tree; this is its one rewrite,
    // made so codegen sees the reused member as ordinary field access.
    auto& rewritten = const_cast<ast::CallExpression&>(member);
    ast::ExpressionPtr receiver = rewritten.takeArgument(0);
    for (const std::string& part : path) {
        std::vector<ast::ExpressionPtr> arguments;
        arguments.push_back(std::move(receiver));
        arguments.push_back(std::make_unique<ast::IdentifierExpression>(part));
        receiver = std::make_unique<ast::CallExpression>(
            std::make_unique<ast::IdentifierExpression>("__member"), std::move(arguments));
    }
    rewritten.setArgument(0, std::move(receiver));
}

// A `Self mut` method changes its receiver, so the receiver must be a
// writable place: a mutable variable, a field or element of one, or a
// `ref mut` parameter or `Self mut` (CANON-9, ADR-0016).
void SemanticAnalyzer::requireWritableReceiver(const FunctionSignature& signature,
                                               const ast::Expression& receiver) const
{
    if (signature.parameters.empty() || signature.parameters.front().mode != PassMode::RefMut) {
        return;
    }
    // The object of a pointer is changed through it (safe-pointers).
    if (const TypeRef type = result_.typeOf(receiver); type != nullptr && (type->isOwn() || type->isLink())) {
        return;
    }
    if (writableRoot(receiver) == nullptr) {
        throw SemanticError("method " + signature.name +
                            " changes its receiver (Self mut): call it on a mutable variable, a field or "
                            "an element of one (CANON-9)");
    }
}

} // namespace inox::compiler::semantic
