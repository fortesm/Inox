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

const Symbol* Scope::declare(std::string name, SymbolKind kind, const types::Type* type, bool isMutable)
{
    const std::string normalized = normalize(name);
    auto [iterator, inserted] = symbols_.emplace(
        normalized,
        Symbol{std::move(name), normalized, kind, type, isMutable});

    return inserted ? &iterator->second : nullptr;
}

void Scope::markDeepReadOnly(std::string_view name)
{
    if (auto iterator = symbols_.find(normalize(name)); iterator != symbols_.end()) {
        iterator->second.deepReadOnly = true;
    }
}

void Scope::inferType(std::string_view name, const types::Type* type)
{
    auto iterator = symbols_.find(normalize(name));
    if (iterator != symbols_.end() && iterator->second.type == nullptr) {
        iterator->second.type = type;
    }
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
