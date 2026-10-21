// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Symbol.h"
#include "../ast/Ast.h"
#include "StdStrings.h"
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
    enum class Kind { Integer, Boolean, String, Real };
    Kind kind = Kind::Integer;
    std::int64_t integer = 0;
    bool boolean = false;
    std::string text;  // a String Const: the literal's text
    double real = 0.0;  // a Float Const (change float-conversions), already
                        // rounded to Float32 when its type is Float32
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
        rangeChecks_.clear();
        fillUnits_.clear();
        invariantBefore_.clear();
        invariantAfter_.clear();
        newValueInvariants_.clear();
        signatures_.clear();
        declaredTypes_.clear();
        instances_.clear();
        structInstances_.clear();
        constructions_.clear();
        stringCalls_.clear();
    }

    // Std.Strings / Std.StrUtils (change std-strings): the function a call
    // of the module runs, or null when the call is not one.
    void setStringCall(const ast::CallExpression& call, StringFunction function)
    {
        stringCalls_.insert_or_assign(&call, function);
    }
    const StringFunction* stringCall(const ast::CallExpression& call) const
    {
        const auto iterator = stringCalls_.find(&call);
        return iterator != stringCalls_.end() ? &iterator->second : nullptr;
    }

    // CANON-8 (r3.77): the copy of a generic routine for one list of types
    // (`Max<Int64>`), made and checked by the analyzer; codegen compiles
    // each one like a routine of the program. In the order they were made.
    const ast::FunctionDeclaration& addInstance(std::unique_ptr<ast::FunctionDeclaration> instance)
    {
        instances_.push_back(std::move(instance));
        return *instances_.back();
    }
    const std::vector<std::unique_ptr<ast::FunctionDeclaration>>& instances() const { return instances_; }

    // The struct types made from generic structs (`TPair<String, Int64>`).
    void addStructInstance(const types::Type* type) { structInstances_.push_back(type); }
    const std::vector<const types::Type*>& structInstances() const { return structInstances_; }

    // The struct a named construction `T(Field := Value, ...)` builds.
    void setConstructedStruct(const ast::CallExpression& call, const types::Type* type)
    {
        constructions_.insert_or_assign(&call, type);
    }
    const types::Type* constructedStruct(const ast::CallExpression& call) const
    {
        const auto iterator = constructions_.find(&call);
        return iterator != constructions_.end() ? iterator->second : nullptr;
    }

    // What the code generator reads instead of the written text (OpenSpec
    // change type-representation): each routine's signature and the type of
    // each declared variable (a local, a State or a Const).
    void setSignature(const ast::FunctionDeclaration& function, FunctionSignature signature)
    {
        signatures_.insert_or_assign(&function, std::move(signature));
    }

    const FunctionSignature* signatureOf(const ast::FunctionDeclaration& function) const
    {
        const auto iterator = signatures_.find(&function);
        return iterator != signatures_.end() ? &iterator->second : nullptr;
    }

    void setDeclaredType(const ast::VarStatement& declaration, const types::Type* type)
    {
        declaredTypes_.insert_or_assign(&declaration, type);
    }

    void setDeclaredType(const ast::VariableDeclaration& declaration, const types::Type* type)
    {
        declaredTypes_.insert_or_assign(&declaration, type);
    }

    const types::Type* declaredType(const ast::VarStatement& declaration) const
    {
        return findDeclaredType(&declaration);
    }

    const types::Type* declaredType(const ast::VariableDeclaration& declaration) const
    {
        return findDeclaredType(&declaration);
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

    // CANON-13 Range (Q1): an Integer value stored into a Range is checked
    // against its bounds at run time, implicitly, as for Natural.
    void requireRangeCheck(const ast::Expression& expression, const types::Type* rangeType)
    {
        rangeChecks_[&expression] = rangeType;
    }

    const types::Type* rangeCheck(const ast::Expression& expression) const
    {
        const auto found = rangeChecks_.find(&expression);
        return found != rangeChecks_.end() ? found->second : nullptr;
    }

    // `Fill(Value)` (r3.81): the call fills its target array with copies of
    // Value; `unit` is the element (or row) type each copy has.
    void setFillUnit(const ast::CallExpression& call, const types::Type* unit) { fillUnits_[&call] = unit; }

    const types::Type* fillUnit(const ast::CallExpression& call) const
    {
        const auto found = fillUnits_.find(&call);
        return found != fillUnits_.end() ? found->second : nullptr;
    }

    // CANON-24: where a struct invariant must hold. At a site (a qualified
    // method call, a field write, a call with ref mut arguments) the backend
    // checks the invariant of each listed place before or after the site;
    // a construction or a declaration with defaults checks its new value.
    struct InvariantCheck {
        const ast::Expression* place = nullptr;
        const types::Type* type = nullptr;  // a struct with an invariant
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
    void requireNewValueInvariant(const ast::AstNode& site, const types::Type* type)
    {
        newValueInvariants_[&site] = type;
    }
    // The struct whose invariant the new value at `site` must satisfy, or null.
    const types::Type* newValueInvariant(const ast::AstNode& site) const
    {
        const auto found = newValueInvariants_.find(&site);
        return found != newValueInvariants_.end() ? found->second : nullptr;
    }

private:
    const types::Type* findDeclaredType(const void* declaration) const
    {
        const auto iterator = declaredTypes_.find(declaration);
        return iterator != declaredTypes_.end() ? iterator->second : nullptr;
    }

    std::unordered_map<const ast::FunctionDeclaration*, FunctionSignature> signatures_;
    std::vector<std::unique_ptr<ast::FunctionDeclaration>> instances_;
    std::vector<const types::Type*> structInstances_;
    std::unordered_map<const ast::CallExpression*, const types::Type*> constructions_;
    std::unordered_map<const ast::CallExpression*, StringFunction> stringCalls_;
    // Keyed by a VarStatement or a VariableDeclaration.
    std::unordered_map<const void*, const types::Type*> declaredTypes_;
    std::unordered_map<const ast::AstNode*, std::vector<InvariantCheck>> invariantBefore_;
    std::unordered_map<const ast::AstNode*, std::vector<InvariantCheck>> invariantAfter_;
    std::unordered_map<const ast::AstNode*, const types::Type*> newValueInvariants_;
    std::unordered_set<const ast::Expression*> naturalChecks_;
    std::unordered_map<const ast::Expression*, const types::Type*> rangeChecks_;
    std::unordered_map<const ast::CallExpression*, const types::Type*> fillUnits_;
    std::unique_ptr<types::TypeContext> types_;
    std::unordered_map<const ast::Expression*, const types::Type*> expressionTypes_;
    std::unordered_map<const ast::IdentifierExpression*, const Symbol*> identifierSymbols_;
    std::unordered_map<const ast::CallExpression*, const Symbol*> callSymbols_;
    std::unordered_map<const Symbol*, ConstantValue> constantValues_;
};

} // namespace inox::compiler::semantic
