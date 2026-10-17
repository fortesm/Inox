// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "../types/Type.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace inox::compiler::semantic {

enum class SymbolKind {
    Module,
    Function,
    Variable,
    LoopIterator,
    Constant,
    State,
    Type,
    Builtin
};

struct Symbol {
    std::string name;
    std::string normalizedName;
    SymbolKind kind = SymbolKind::Variable;
    // The symbol's type; null while not known (a routine, a module, a Const
    // whose initializer is analyzed later).
    const types::Type* type = nullptr;
    bool isMutable = false;
};

class Scope {
public:
    explicit Scope(Scope* parent = nullptr);

    Scope* parent() const;
    const Symbol* declare(std::string name, SymbolKind kind, const types::Type* type = nullptr,
                          bool isMutable = false);
    const Symbol* resolve(std::string_view name) const;
    // Gives a symbol of this scope its inferred type once it is known (a
    // module Const whose initializer is an expression). No effect when the
    // symbol already has a type.
    void inferType(std::string_view name, const types::Type* type);
    bool containsLocal(std::string_view name) const;
    bool containsInAncestors(std::string_view name) const;

private:
    static std::string normalize(std::string_view name);

    Scope* parent_ = nullptr;
    std::unordered_map<std::string, Symbol> symbols_;
};

class SymbolTable {
public:
    SymbolTable();

    Scope& globalScope();
    const Scope& globalScope() const;

    Scope& pushScope();
    void popScope();
    Scope& currentScope();
    const Scope& currentScope() const;

private:
    std::vector<std::unique_ptr<Scope>> scopes_;
    Scope* current_ = nullptr;
};

} // namespace inox::compiler::semantic
