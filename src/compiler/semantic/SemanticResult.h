// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Symbol.h"
#include "../ast/Ast.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace inox::compiler::semantic {

struct ResolvedType {
    std::string name;
    const TypeSymbol* symbol = nullptr;
};

// Compile-time value of a module `Const`, resolved by semantic analysis.
// Later phases read it from here instead of re-parsing the Const section
// (CANON E11, decision P-C stage 1).
struct ConstantValue {
    enum class Kind { Integer, Boolean };
    Kind kind = Kind::Integer;
    std::int64_t integer = 0;
    bool boolean = false;
};

class SemanticResult {
public:
    void clear()
    {
        expressionTypes_.clear();
        identifierSymbols_.clear();
        callSymbols_.clear();
        constantValues_.clear();
        naturalChecks_.clear();
        canonicalTypes_.clear();
    }

    void setConstantValue(const Symbol& symbol, ConstantValue value)
    {
        constantValues_.insert_or_assign(&symbol, value);
    }

    const ConstantValue* constantValueOf(const Symbol& symbol) const
    {
        const auto iterator = constantValues_.find(&symbol);
        return iterator != constantValues_.end() ? &iterator->second : nullptr;
    }

    void setExpressionType(const ast::Expression& expression, ResolvedType type)
    {
        expressionTypes_.insert_or_assign(&expression, std::move(type));
    }

    const ResolvedType* typeOf(const ast::Expression& expression) const
    {
        const auto iterator = expressionTypes_.find(&expression);
        return iterator != expressionTypes_.end() ? &iterator->second : nullptr;
    }

    void bind(const ast::IdentifierExpression& expression, const Symbol& symbol)
    {
        identifierSymbols_.insert_or_assign(&expression, &symbol);
    }

    const Symbol* symbolOf(const ast::IdentifierExpression& expression) const
    {
        const auto iterator = identifierSymbols_.find(&expression);
        return iterator != identifierSymbols_.end() ? iterator->second : nullptr;
    }

    void bind(const ast::CallExpression& expression, const Symbol& symbol)
    {
        callSymbols_.insert_or_assign(&expression, &symbol);
    }

    const Symbol* symbolOf(const ast::CallExpression& expression) const
    {
        const auto iterator = callSymbols_.find(&expression);
        return iterator != callSymbols_.end() ? iterator->second : nullptr;
    }

    // CANON-13: the canonical spelling of an array type written in source
    // (Const bounds replaced by their values), for later phases.
    void setCanonicalTypeName(const std::string& written, const std::string& canonical)
    {
        canonicalTypes_[written] = canonical;
    }

    std::string canonicalTypeName(const std::string& written) const
    {
        const auto iterator = canonicalTypes_.find(written);
        return iterator != canonicalTypes_.end() ? iterator->second : written;
    }

    // ADR-0015: an Integer value stored into a Natural is range-checked at run
    // time, implicitly (Ada subtype semantics). The emitter wraps every marked
    // expression in the check.
    void requireNaturalCheck(const ast::Expression& expression)
    {
        naturalChecks_.insert(&expression);
    }

    bool needsNaturalCheck(const ast::Expression& expression) const
    {
        return naturalChecks_.count(&expression) != 0;
    }

private:
    std::unordered_set<const ast::Expression*> naturalChecks_;
    std::unordered_map<std::string, std::string> canonicalTypes_;
    std::unordered_map<const ast::Expression*, ResolvedType> expressionTypes_;
    std::unordered_map<const ast::IdentifierExpression*, const Symbol*> identifierSymbols_;
    std::unordered_map<const ast::CallExpression*, const Symbol*> callSymbols_;
    std::unordered_map<const Symbol*, ConstantValue> constantValues_;
};

} // namespace inox::compiler::semantic
