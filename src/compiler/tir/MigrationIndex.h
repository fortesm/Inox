// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// TRANSITIONAL (F6, change typed-ir, step 2; removed at step 5).
//
// While code generation still walks the AST, it takes the types, symbols
// and constants of the AST nodes it meets from the typed IR through this
// index: the lowering records, by node identity, which TIR node each AST
// node became. It is not part of the typed IR's API (tir/Tir.h), resolves
// no name, infers nothing, and never falls back to the SemanticResult: a
// node without its TIR node is an internal error (design of typed-ir,
// "Step 2: the migration index", approved 2026-11-04).

#include "Tir.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace inox::compiler::tir {

class MigrationError : public std::logic_error {
public:
    explicit MigrationError(const std::string& message) : std::logic_error(message) {}
};

class MigrationIndex {
public:
    // What an AST expression became. `value` is the expression as the
    // analysis typed it, before the checks the analysis asked for on the
    // way into a place (RangeCheck, NaturalCheck); `place` is set when the
    // node was lowered as a place (a target, a `with`, a reference).
    // `call` is set on the callee of a call (the routine's or the method's
    // name, the identifier of `__member` of a method): the TIR call it
    // names, whose symbol it carries.
    // `statement` is set on an assignment `L := V` and on the `L` it
    // declares (`declared`); `member` on the field name of a field access
    // (`__member(P, FName)`): the Field place it names.
    struct Entry {
        const Expr* value = nullptr;
        const Place* place = nullptr;
        const Expr* call = nullptr;
        const Stmt* statement = nullptr;
        bool declared = false;
        const Place* member = nullptr;
        // The written name of an encoding (`__member`, `__index`): no value,
        // no symbol.
        bool name = false;
        // `F := V` of a construction: field `field` of `construct`.
        const Expr* construct = nullptr;
        std::uint32_t field = 0;
    };

    // Recording, by the lowering.
    void recordValue(const ast::Expression& node, const Expr* value) { entries_[&node].value = value; }
    void recordPlace(const ast::Expression& node, const Place* place) { entries_[&node].place = place; }
    void recordCallee(const ast::Expression& node, const Expr* call) { entries_[&node].call = call; }
    void recordStatement(const ast::Expression& node, const Stmt* statement) { entries_[&node].statement = statement; }
    void recordDeclared(const ast::Expression& node, const Stmt* declaration)
    {
        entries_[&node].statement = declaration;
        entries_[&node].declared = true;
    }
    void recordMember(const ast::Expression& node, const Place* field) { entries_[&node].member = field; }
    void recordName(const ast::Expression& node) { entries_[&node].name = true; }
    void recordFieldValue(const ast::Expression& node, const Expr* construct, std::uint32_t field)
    {
        entries_[&node].construct = construct;
        entries_[&node].field = field;
    }
    void recordDeclaration(const void* declaration, const Stmt* statement) { declarations_[declaration] = statement; }
    void recordState(const ast::VariableDeclaration& declaration, const StateVariable* state)
    {
        states_[&declaration] = state;
    }

    // Queries, by code generation. Each throws MigrationError when the
    // node has no TIR node to answer from.
    TypeRef typeOf(const ast::Expression& node) const;
    const semantic::Symbol* symbolOf(const ast::Expression& node) const;
    // The value of a Const the identifier reads; null when it reads none.
    const semantic::ConstantValue* constantOf(const ast::Expression& node) const;
    TypeRef declaredType(const ast::VarStatement& declaration) const;
    TypeRef declaredType(const ast::VariableDeclaration& declaration) const;

    // Debug agreement with the analysis's record: the same type, or one of
    // the two differences the TIR makes on purpose. An operation without a
    // result (a call, a builtin, a string function, an intrinsic) is Void
    // where the analysis recorded none; a written field or element the
    // analysis did not type (`P.F := V`) has its place's type. A node the
    // analysis typed agrees only with that type.
    bool typeAgrees(const ast::Expression& node, TypeRef recorded) const;

    std::size_t size() const { return entries_.size() + declarations_.size() + states_.size(); }

private:
    const Entry& entryOf(const ast::Expression& node, const char* question) const;

    std::unordered_map<const ast::Expression*, Entry> entries_;
    std::unordered_map<const void*, const Stmt*> declarations_;
    std::unordered_map<const ast::VariableDeclaration*, const StateVariable*> states_;
};

} // namespace inox::compiler::tir
