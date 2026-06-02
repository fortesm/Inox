// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Symbol.h"

#include <cctype>
#include <utility>

namespace inox::compiler::semantic {

Scope::Scope(Scope* parent) : parent_(parent) {}

Scope* Scope::parent() const
{
    return parent_;
}

const Symbol* Scope::declare(std::string name, SymbolKind kind, std::string typeName, bool isMutable)
{
    const std::string normalized = normalize(name);
    auto [iterator, inserted] = symbols_.emplace(
        normalized,
        Symbol{std::move(name), normalized, kind, std::move(typeName), isMutable});

    return inserted ? &iterator->second : nullptr;
}

const Symbol* Scope::resolve(std::string_view name) const
{
    const std::string normalized = normalize(name);
    auto iterator = symbols_.find(normalized);
    if (iterator != symbols_.end()) {
        return &iterator->second;
    }

    return parent_ != nullptr ? parent_->resolve(name) : nullptr;
}

bool Scope::containsLocal(std::string_view name) const
{
    return symbols_.find(normalize(name)) != symbols_.end();
}

bool Scope::containsInAncestors(std::string_view name) const
{
    for (const Scope* scope = parent_; scope != nullptr; scope = scope->parent_) {
        if (scope->containsLocal(name)) {
            return true;
        }
    }
    return false;
}

std::string Scope::normalize(std::string_view name)
{
    std::string normalized;
    normalized.reserve(name.size());
    for (const char ch : name) {
        normalized.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

const TypeSymbol* TypeTable::declare(std::string name, bool isBuiltin, std::string aliasOf)
{
    const std::string normalized = normalize(name);
    auto [iterator, inserted] = types_.emplace(
        normalized,
        TypeSymbol{std::move(name), normalized, std::move(aliasOf), isBuiltin});

    return inserted ? &iterator->second : nullptr;
}

const TypeSymbol* TypeTable::resolve(std::string_view name) const
{
    auto iterator = types_.find(normalize(name));
    return iterator != types_.end() ? &iterator->second : nullptr;
}

bool TypeTable::contains(std::string_view name) const
{
    return resolve(name) != nullptr;
}

std::string TypeTable::normalize(std::string_view name)
{
    std::string normalized;
    normalized.reserve(name.size());
    for (const char ch : name) {
        normalized.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

SymbolTable::SymbolTable()
{
    scopes_.push_back(std::make_unique<Scope>());
    current_ = scopes_.back().get();
}

Scope& SymbolTable::globalScope()
{
    return *scopes_.front();
}

const Scope& SymbolTable::globalScope() const
{
    return *scopes_.front();
}

Scope& SymbolTable::pushScope()
{
    scopes_.push_back(std::make_unique<Scope>(current_));
    current_ = scopes_.back().get();
    return *current_;
}

void SymbolTable::popScope()
{
    if (current_->parent() != nullptr) {
        current_ = current_->parent();
    }
}

Scope& SymbolTable::currentScope()
{
    return *current_;
}

const Scope& SymbolTable::currentScope() const
{
    return *current_;
}

} // namespace inox::compiler::semantic
