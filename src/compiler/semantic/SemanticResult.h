// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Symbol.h"
#include "../ast/Ast.h"
#include "../types/TypeContext.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace inox::compiler::semantic {

// ADR-0016: how an argument is passed.
enum class PassMode {
    Value,   // a copy; the parameter is immutable
    Ref,     // `ref X T`: a read-only reference to the caller's storage
    RefMut   // `ref mut X T`: a writable reference to the caller's variable
};

struct FunctionParameter {
    std::string name;
    const types::Type* type = nullptr;
    PassMode mode = PassMode::Value;
};

struct FunctionSignature {
    std::string name;
    std::vector<FunctionParameter> parameters;
    // The result's type; null for a subroutine.
    const types::Type* returnType = nullptr;
    // A method `T.M(...) Override :` replaces a method T reuses (CANON-9).
    bool isOverride = false;
};

// Compile-time value of a module `Const`, resolved by semantic analysis.
// Later phases read it from here instead of re-parsing the Const section
// (CANON E11, decision P-C stage 1).
struct ConstantValue {
    enum class Kind { Integer, Boolean, String };
    Kind kind = Kind::Integer;
    std::int64_t integer = 0;
    bool boolean = false;
    std::string text;  // a String Const: the literal's text
};

class SemanticResult {
public:
    SemanticResult() : types_(std::make_unique<types::TypeContext>()) {}

    // Every type of the program (OpenSpec change type-representation).
    types::TypeContext& types() { return *types_; }
    const types::TypeContext& types() const { return *types_; }

    void clear()
    {
        expressionTypes_.clear();
        identifierSymbols_.clear();
        callSymbols_.clear();
        constantValues_.clear();
        naturalChecks_.clear();
        invariantBefore_.clear();
        invariantAfter_.clear();
        newValueInvariants_.clear();
        writtenTypes_.clear();
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

    void setExpressionType(const ast::Expression& expression, const types::Type* type)
    {
        expressionTypes_.insert_or_assign(&expression, type);
    }

    // The type of an analyzed expression; null when it has none.
    const types::Type* typeOf(const ast::Expression& expression) const
    {
        const auto iterator = expressionTypes_.find(&expression);
        return iterator != expressionTypes_.end() ? iterator->second : nullptr;
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

    // The type a written type text denotes (`Array[1..Size] Byte` with Size
    // a Const), for later phases.
    void setWrittenType(const std::string& written, const types::Type* type)
    {
        writtenTypes_[written] = type;
    }

    const types::Type* writtenType(const std::string& written) const
    {
        const auto iterator = writtenTypes_.find(written);
        return iterator != writtenTypes_.end() ? iterator->second : nullptr;
    }

    std::string canonicalTypeName(const std::string& written) const
    {
        const types::Type* type = writtenType(written);
        return type != nullptr ? type->name() : written;
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

    // CANON-24: where a struct invariant must hold. At a site (a qualified
    // method call, a field write, a call with ref mut arguments) the backend
    // checks the invariant of each listed place before or after the site;
    // a construction or a declaration with defaults checks its new value.
    struct InvariantCheck {
        const ast::Expression* place = nullptr;
        std::string typeName;
    };
    void addInvariantCheck(const ast::AstNode& site, bool after, InvariantCheck check)
    {
        (after ? invariantAfter_ : invariantBefore_)[&site].push_back(std::move(check));
    }
    const std::vector<InvariantCheck>* invariantChecks(const ast::AstNode& site, bool after) const
    {
        const auto& checks = after ? invariantAfter_ : invariantBefore_;
        const auto found = checks.find(&site);
        return found != checks.end() ? &found->second : nullptr;
    }
    void requireNewValueInvariant(const ast::AstNode& site, std::string typeName)
    {
        newValueInvariants_[&site] = std::move(typeName);
    }
    const std::string* newValueInvariant(const ast::AstNode& site) const
    {
        const auto found = newValueInvariants_.find(&site);
        return found != newValueInvariants_.end() ? &found->second : nullptr;
    }

private:
    std::unordered_map<const ast::AstNode*, std::vector<InvariantCheck>> invariantBefore_;
    std::unordered_map<const ast::AstNode*, std::vector<InvariantCheck>> invariantAfter_;
    std::unordered_map<const ast::AstNode*, std::string> newValueInvariants_;
    std::unordered_set<const ast::Expression*> naturalChecks_;
    std::unique_ptr<types::TypeContext> types_;
    std::unordered_map<std::string, const types::Type*> writtenTypes_;
    std::unordered_map<const ast::Expression*, const types::Type*> expressionTypes_;
    std::unordered_map<const ast::IdentifierExpression*, const Symbol*> identifierSymbols_;
    std::unordered_map<const ast::CallExpression*, const Symbol*> callSymbols_;
    std::unordered_map<const Symbol*, ConstantValue> constantValues_;
};

} // namespace inox::compiler::semantic
