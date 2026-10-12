// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: routine signatures, scopes and symbols, module items, Type/Const/State sections, structs.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"
#include "../exceptions/ExceptionTypes.h"
#include "../support/ArrayType.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

namespace inox::compiler::semantic {

using namespace detail;

void SemanticAnalyzer::registerFunctionSignatures(const ast::ModuleNode& module)
{
    for (const auto& item : module.items()) {
        if (item->kind() == ast::AstNodeKind::FunctionDeclaration) {
            registerFunctionSignature(static_cast<const ast::FunctionDeclaration&>(*item));
        }
    }
}

void SemanticAnalyzer::registerFunctionSignature(const ast::FunctionDeclaration& function)
{
    const auto& tokens = function.signatureTokens();

    FunctionSignature signature;
    signature.name = function.name();

    const std::size_t dot = function.name().find('.');
    const bool isAssociatedMethod = dot != std::string::npos;
    const std::string receiverType = isAssociatedMethod ? function.name().substr(0, dot) : std::string{};

    std::size_t index = 0;
    if (!tokens.empty() && tokens.front() == "(") {
        index = 1;
        if (index < tokens.size() && tokens[index] == ")") {
            throw SemanticError("empty parentheses are not allowed in declarations: " + function.name());
        }

        while (index < tokens.size() && tokens[index] != ")") {
            // ADR-0016: `ref X T` and `ref mut X T` pass by reference.
            PassMode mode = PassMode::Value;
            if (equalsIgnoreCase(tokens[index], "ref")) {
                ++index;
                mode = PassMode::Ref;
                if (index < tokens.size() && equalsIgnoreCase(tokens[index], "mut")) {
                    ++index;
                    mode = PassMode::RefMut;
                }
                if (isAssociatedMethod && signature.parameters.empty()) {
                    throw SemanticError("the receiver of a method is Self or Self mut, never ref (ADR-0016)");
                }
            }
            if (equalsIgnoreCase(tokens[index], "mut")) {
                throw SemanticError("mutable parameters are reserved for a future Inox version: use a local variable or return a new value");
            }
            if (!looksLikeIdentifier(tokens[index])) {
                throw SemanticError("expected parameter name in function: " + function.name());
            }
            const std::string parameterName = tokens[index++];
            std::string parameterType;

            if (isAssociatedMethod && signature.parameters.empty() && equalsIgnoreCase(parameterName, "Self")) {
                if (index < tokens.size() && equalsIgnoreCase(tokens[index], "mut")) {
                    ++index;
                }
                resolveTypeOrThrow(receiverType);
                parameterType = canonicalTypeName(receiverType);
            } else {
                if (index < tokens.size() && equalsIgnoreCase(tokens[index], "mut")) {
                    throw SemanticError("mutable parameters are reserved for a future Inox version: use a local variable or return a new value");
                }
                if (index >= tokens.size() ||
                    (!looksLikeIdentifier(tokens[index]) && !support::isArrayTypeName(tokens[index]) &&
                     !support::isSliceTypeName(tokens[index]))) {
                    throw SemanticError("expected type for parameter: " + parameterName);
                }
                resolveTypeOrThrow(tokens[index]);
                parameterType = canonicalTypeName(tokens[index]);
                if (support::isArrayTypeName(tokens[index]) || support::isSliceTypeName(tokens[index])) {
                    result_.setCanonicalTypeName(tokens[index], parameterType);
                }
                if (mode != PassMode::Value && support::isSliceTypeName(parameterType)) {
                    throw SemanticError("a slice is already a view: pass it without ref (ADR-0017)");
                }
                ++index;
            }

            for (const FunctionParameter& parameter : signature.parameters) {
                if (equalsIgnoreCase(parameter.name, parameterName)) {
                    throw SemanticError("duplicate parameter: " + parameterName);
                }
            }
            signature.parameters.push_back(FunctionParameter{parameterName, parameterType, mode});

            if (index < tokens.size() && tokens[index] == ",") {
                ++index;
            } else if (index >= tokens.size() || tokens[index] != ")") {
                throw SemanticError("expected ',' or ')' in function: " + function.name());
            }
        }

        if (index >= tokens.size() || tokens[index] != ")") {
            throw SemanticError("expected ')' in function: " + function.name());
        }
        ++index;
    }

    if (index < tokens.size()) {
        if (index + 1 != tokens.size() ||
            (!looksLikeIdentifier(tokens[index]) && !support::isArrayTypeName(tokens[index]) &&
             !support::isSliceTypeName(tokens[index]))) {
            throw SemanticError("invalid return type in function: " + function.name());
        }
        resolveTypeOrThrow(tokens[index]);
        signature.returnType = canonicalTypeName(tokens[index]);
        if (support::isSliceTypeName(signature.returnType)) {
            throw SemanticError("a slice cannot be returned: it would outlive its array (ADR-0017)");
        }
        if (support::isArrayTypeName(tokens[index])) {
            result_.setCanonicalTypeName(tokens[index], signature.returnType);
        }
    }

    functions_.emplace(normalizeName(function.name()), std::move(signature));
}

const FunctionSignature* SemanticAnalyzer::resolveFunctionSignature(std::string_view name) const
{
    const auto iterator = functions_.find(normalizeName(name));
    return iterator != functions_.end() ? &iterator->second : nullptr;
}

void SemanticAnalyzer::declareOrThrow(
    std::string_view name,
    SymbolKind kind,
    std::string typeName,
    bool isMutable)
{
    Scope& scope = symbols_.currentScope();
    if (scope.containsLocal(name)) {
        throw SemanticError("duplicate symbol in same scope: " + std::string(name));
    }
    if (scope.containsInAncestors(name)) {
        throw SemanticError("shadowing is forbidden: " + std::string(name));
    }
    scope.declare(std::string(name), kind, std::move(typeName), isMutable);
}

// OPEN-4 (decided by Marcelo Fortes, 2026-10-10): a local variable that is
// never read is a compile error, as in Go. A misspelled name in FORM 1
// (`Coutner := Counter + 1`) declares a new variable that nothing reads, so
// the typo is caught. Parameters, `for` iterators, exception bindings, State
// and Const are not tracked; assigning to a variable is not reading it.
void SemanticAnalyzer::trackLocal(std::string_view name)
{
    if (const Symbol* symbol = symbols_.currentScope().resolve(name)) {
        localsToRead_.push_back({symbol, &symbols_.currentScope()});
    }
}

void SemanticAnalyzer::leaveScope()
{
    const Scope* scope = &symbols_.currentScope();
    std::string unread;
    for (auto it = localsToRead_.begin(); it != localsToRead_.end();) {
        if (it->scope != scope) {
            ++it;
            continue;
        }
        if (!readSymbols_.contains(it->symbol)) {
            unread += (unread.empty() ? "" : ", ") + it->symbol->name;
        }
        it = localsToRead_.erase(it);
    }
    symbols_.popScope();
    if (!unread.empty()) {
        throw SemanticError(
            std::string(unread.find(',') == std::string::npos ? "local variable never read: "
                                                               : "local variables never read: ") +
            unread + " (OPEN-4: read it or remove it; a misspelled name declares a new variable)");
    }
}

void SemanticAnalyzer::declareTypeOrThrow(std::string_view name, bool isBuiltin, std::string aliasOf)
{
    if (types_.contains(name)) {
        throw SemanticError("type already declared: " + std::string(name));
    }
    declareOrThrow(name, SymbolKind::Type);
    types_.declare(std::string(name), isBuiltin, std::move(aliasOf));
}

void SemanticAnalyzer::resolveTypeOrThrow(std::string_view name) const
{
    if (support::isArrayTypeName(name) || support::isSliceTypeName(name)) {
        (void)canonicalTypeName(name);
        return;
    }
    if (types_.resolve(name) == nullptr) {
        throw SemanticError("unknown type: " + std::string(name));
    }
}

const Symbol& SemanticAnalyzer::resolveOrThrow(std::string_view name) const
{
    if (isInternalSyntheticName(name)) {
        const Symbol* symbol = symbols_.currentScope().resolve(name);
        return *symbol;
    }
    const Symbol* symbol = symbols_.currentScope().resolve(name);
    if (symbol == nullptr) {
        throw SemanticError("unknown symbol: " + std::string(name));
    }
    return *symbol;
}

std::string SemanticAnalyzer::canonicalTypeName(std::string_view name) const
{
    if (support::isArrayTypeName(name)) {
        return canonicalArrayTypeName(name);
    }
    if (support::isSliceTypeName(name)) {
        const std::string element = support::sliceElementTypeName(name);
        resolveTypeOrThrow(element);
        const std::string canonical = canonicalTypeName(element);
        if (!support::isArrayTypeName(canonical) && !isNumericType(canonical) && canonical != "Bool") {
            throw SemanticError("slices of " + canonical +
                                " are not supported yet; use an integer, Float, Bool or array element");
        }
        return support::makeSliceTypeName(canonical);
    }
    const TypeSymbol* type = types_.resolve(name);
    if (type == nullptr) {
        return std::string(name);
    }
    if (!type->aliasOf.empty()) {
        return canonicalTypeName(type->aliasOf);
    }
    return type->name;
}

void SemanticAnalyzer::declareModuleItem(const ast::AstNode& item)
{
    switch (item.kind()) {
    case ast::AstNodeKind::FunctionDeclaration: {
        const auto& function = static_cast<const ast::FunctionDeclaration&>(item);
        declareOrThrow(function.name(), SymbolKind::Function);
        if (equalsIgnoreCase(function.name(), "Main")) {
            hasMain_ = true;
        }
        break;
    }
    case ast::AstNodeKind::SectionDeclaration:
        declareSectionSymbols(static_cast<const ast::SectionDeclaration&>(item));
        break;
    default:
        break;
    }
}

void SemanticAnalyzer::analyzeModuleItem(const ast::AstNode& item)
{
    if (item.kind() == ast::AstNodeKind::FunctionDeclaration) {
        analyzeFunction(static_cast<const ast::FunctionDeclaration&>(item));
    } else if (item.kind() == ast::AstNodeKind::SectionDeclaration) {
        const auto& section = static_cast<const ast::SectionDeclaration&>(item);
        validateSectionTypes(section);
        // Const and State initializers are expressions: they are analyzed like
        // any other (CANON-9 named construction, types), not skipped.
        for (const ast::VariableDeclaration& declaration : section.variables()) {
            if (declaration.value == nullptr) {
                continue;
            }
            const std::string valueType = canonicalTypeName(analyzeExpression(*declaration.value));
            if (declaration.typeName.empty() && !valueType.empty()) {
                symbols_.currentScope().inferTypeName(declaration.name, valueType);
            }
            if (section.sectionKind() == ast::SectionKind::Const) {
                recordConstantExpression(declaration.name, *declaration.value);
            }
            if (!declaration.typeName.empty()) {
                const std::string declared = canonicalTypeName(declaration.typeName);
                if (!valueType.empty() && !canAssignValue(declared, valueType, *declaration.value)) {
                    throw SemanticError("initializer of " + declaration.name + " has type " +
                                        valueType + ", expected " + declared);
                }
            }
        }
    }
}

void SemanticAnalyzer::declareSectionSymbols(const ast::SectionDeclaration& section)
{
    if (section.sectionKind() == ast::SectionKind::Type) {
        registerTypeSectionSymbols(section);
        return;
    }

    const SymbolKind kind = section.sectionKind() == ast::SectionKind::Const ? SymbolKind::Constant
                                                                              : SymbolKind::State;
    const bool isMutable = kind == SymbolKind::State;
    for (const ast::VariableDeclaration& declaration : section.variables()) {
        // An array type is made canonical when the section is analyzed: its
        // bounds may name a Const declared later in the module.
        const std::string typeName =
            declaration.typeName.empty() ? inferSectionDeclarationType(declaration.value.get())
            : support::isArrayTypeName(declaration.typeName) ? std::string{}
                                                              : canonicalTypeName(declaration.typeName);
        declareOrThrow(declaration.name, kind, typeName, isMutable);
        if (declaration.value == nullptr) {
            if (kind == SymbolKind::State) {
                stateDeclarationsWithoutInitializer_.emplace_back(declaration.name, declaration.typeName);
            }
            continue;
        }
        // A Const value is recorded here only when it is a single literal or
        // name; other initializers are folded when the section is analyzed.
        if (kind == SymbolKind::Constant) {
            if (const std::string token = singleTokenText(*declaration.value); !token.empty()) {
                recordConstantValue(declaration.name, token);
            }
        }
    }
}

// The text of a one-token expression (a literal or a name), or empty.
std::string SemanticAnalyzer::singleTokenText(const ast::Expression& expression)
{
    if (expression.kind() == ast::AstNodeKind::LiteralExpression) {
        return static_cast<const ast::LiteralExpression&>(expression).value();
    }
    if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
        return static_cast<const ast::IdentifierExpression&>(expression).name();
    }
    return {};
}

// The type of `Name := Expr` known before the section is analyzed: the type of
// a literal, or of the name or routine the expression starts with. Other
// initializers get their type when the section is analyzed.
std::string SemanticAnalyzer::inferSectionDeclarationType(const ast::Expression* value) const
{
    while (value != nullptr && value->kind() == ast::AstNodeKind::BinaryExpression) {
        value = &static_cast<const ast::BinaryExpression*>(value)->left();
    }
    if (value != nullptr && value->kind() == ast::AstNodeKind::CallExpression) {
        value = &static_cast<const ast::CallExpression*>(value)->callee();
    }
    if (value == nullptr) {
        return {};
    }
    if (value->kind() == ast::AstNodeKind::LiteralExpression) {
        switch (static_cast<const ast::LiteralExpression*>(value)->literalKind()) {
        case ast::LiteralKind::Boolean:
            return "Bool";
        case ast::LiteralKind::String:
            return "String";
        case ast::LiteralKind::Char:
            return "Char";
        case ast::LiteralKind::Integer:
            return "Int64";
        case ast::LiteralKind::Float:
            return "Float64";
        default:
            return {};
        }
    }
    if (value->kind() == ast::AstNodeKind::IdentifierExpression) {
        const Symbol* symbol =
            symbols_.currentScope().resolve(static_cast<const ast::IdentifierExpression*>(value)->name());
        return symbol != nullptr ? symbol->typeName : std::string{};
    }
    return {};
}

void SemanticAnalyzer::registerTypeSectionSymbols(const ast::SectionDeclaration& section)
{
    // Range and Enum declarations are parsed and shape-checked; their types
    // come with the OpenSpec change enum-range-set.
    for (const ast::TypeDeclaration& declaration : section.types()) {
        if (declaration.kind == ast::TypeDeclarationKind::Struct) {
            registerStructDeclaration(declaration);
        }
    }
}

void SemanticAnalyzer::registerStructDeclaration(const ast::TypeDeclaration& declaration)
{
    declareTypeOrThrow(declaration.name, false);

    StructType structType;
    structType.name = declaration.name;
    // Duplicate detection by normalized (case-insensitive) name keeps a large
    // struct linear.
    std::unordered_set<std::string> seenFieldNames;
    for (const ast::StructFieldDeclaration& fieldDeclaration : declaration.fields) {
        if (!seenFieldNames.insert(normalizeName(fieldDeclaration.name)).second) {
            throw SemanticError("duplicate struct field: " + fieldDeclaration.name);
        }
        StructField field;
        field.name = fieldDeclaration.name;
        // An array field type is made canonical once every Type and Const is
        // declared (completeStructFields): its bounds may name a Const and
        // its element may be a struct declared later.
        field.typeName = support::isArrayTypeName(fieldDeclaration.typeName)
                             ? fieldDeclaration.typeName
                             : canonicalTypeName(fieldDeclaration.typeName);
        field.hasDefault = fieldDeclaration.hasDefault;
        field.defaultValue = fieldDeclaration.defaultValue;
        structType.fields.push_back(std::move(field));
    }
    structs_.emplace(normalizeName(declaration.name), std::move(structType));
}

void SemanticAnalyzer::completeStructFields(const ast::TypeDeclaration& declaration)
{
    const auto structType = structs_.find(normalizeName(declaration.name));
    for (StructField& field : structType->second.fields) {
        if (support::isArrayTypeName(field.typeName)) {
            const std::string written = field.typeName;
            field.typeName = canonicalTypeName(written);
            result_.setCanonicalTypeName(written, field.typeName);
        }
    }
}

void SemanticAnalyzer::validateStructDeclaration(const ast::TypeDeclaration& declaration) const
{
    for (const ast::StructFieldDeclaration& field : declaration.fields) {
        if (support::isSliceTypeName(field.typeName)) {
            throw SemanticError("a slice cannot be a struct field (ADR-0017): " + field.name);
        }
        resolveTypeOrThrow(field.typeName);
        if (field.hasDefault) {
            if (support::isArrayTypeName(field.typeName) || resolveStruct(field.typeName) != nullptr) {
                throw SemanticError("only a scalar struct field has a default (CANON-9): " + field.name);
            }
            validateStructFieldDefault(field.name, canonicalTypeName(field.typeName), field.defaultValue);
        }
    }
    rejectRecursiveStruct(declaration.name);
}

void SemanticAnalyzer::validateStructFieldDefault(const std::string& fieldName,
                                                  const std::string& fieldType,
                                                  const std::string& defaultValue) const
{
    const bool quoted = defaultValue.size() >= 2 && defaultValue.front() == '"' && defaultValue.back() == '"';
    if (equalsIgnoreCase(fieldType, "String") || quoted) {
        if (!equalsIgnoreCase(fieldType, "String")) {
            throw SemanticError("a String literal cannot be the default of struct field " + fieldName +
                                " of type " + fieldType);
        }
        if (!quoted) {
            throw SemanticError("String struct field default must be a String literal: " + fieldName);
        }
        return;
    }
    const bool negative = !defaultValue.empty() && defaultValue.front() == '-';
    const std::string_view magnitudeText = negative ? std::string_view(defaultValue).substr(1)
                                                    : std::string_view(defaultValue);
    if (equalsIgnoreCase(fieldType, "Bool")) {
        if (!equalsIgnoreCase(defaultValue, "true") && !equalsIgnoreCase(defaultValue, "false")) {
            throw SemanticError("Bool struct field default must be true or false: " + fieldName);
        }
        return;
    }
    if (equalsIgnoreCase(fieldType, "Float64") || equalsIgnoreCase(fieldType, "Float32")) {
        const bool isReal = !magnitudeText.empty() &&
                            std::isdigit(static_cast<unsigned char>(magnitudeText.front())) != 0 &&
                            magnitudeText.find_first_of(".eE") != std::string_view::npos &&
                            magnitudeText.find_first_not_of("0123456789.eE+-") == std::string_view::npos;
        if (!isReal) {
            throw SemanticError("Float struct field default must be a real literal: " + fieldName);
        }
        return;
    }
    std::int64_t low = 0;
    std::int64_t high = 0;
    if (!integerTypeRange(fieldType, low, high)) {
        throw SemanticError("struct field defaults of type " + fieldType +
                            " are not supported yet: " + fieldName);
    }
    std::uint64_t magnitude = 0;
    if (!parseIntegerLiteralMagnitude(magnitudeText, magnitude)) {
        throw SemanticError("integer struct field default must be an integer literal: " + fieldName);
    }
    const bool fits = negative ? (low < 0 && magnitude <= static_cast<std::uint64_t>(-(low + 1)) + 1U)
                               : magnitude <= static_cast<std::uint64_t>(high);
    if (!fits) {
        throw SemanticError("struct field default out of range for " + fieldType + ": " + fieldName);
    }
}

// CANON-9: a struct is a value, so it cannot contain itself, directly or
// through other struct fields.
void SemanticAnalyzer::rejectRecursiveStruct(const std::string& typeName) const
{
    std::vector<std::string> path;
    std::function<void(const std::string&)> visit = [&](const std::string& name) {
        const auto structType = structs_.find(normalizeName(name));
        if (structType == structs_.end()) {
            return;
        }
        for (const std::string& seen : path) {
            if (seen == normalizeName(name)) {
                throw SemanticError("struct contains itself: " + typeName);
            }
        }
        path.push_back(normalizeName(name));
        for (const StructField& field : structType->second.fields) {
            visit(field.typeName);
        }
        path.pop_back();
    };
    visit(typeName);
}

void SemanticAnalyzer::validateSectionTypes(const ast::SectionDeclaration& section)
{
    if (section.sectionKind() == ast::SectionKind::Type) {
        for (const ast::TypeDeclaration& declaration : section.types()) {
            if (declaration.kind == ast::TypeDeclarationKind::Struct) {
                completeStructFields(declaration);
            }
        }
        for (const ast::TypeDeclaration& declaration : section.types()) {
            if (declaration.kind == ast::TypeDeclarationKind::Struct) {
                validateStructDeclaration(declaration);
            }
        }
        return;
    }

    for (const ast::VariableDeclaration& declaration : section.variables()) {
        if (declaration.typeName.empty()) {
            continue;
        }
        if (support::isSliceTypeName(declaration.typeName)) {
            throw SemanticError("a slice cannot be a " +
                                std::string(section.sectionKind() == ast::SectionKind::State ? "State variable"
                                                                                               : "Const") +
                                " (ADR-0017): " + declaration.name);
        }
        resolveTypeOrThrow(declaration.typeName);
        if (support::isArrayTypeName(declaration.typeName)) {
            const std::string canonical = canonicalTypeName(declaration.typeName);
            result_.setCanonicalTypeName(declaration.typeName, canonical);
            symbols_.currentScope().inferTypeName(declaration.name, canonical);
        }
    }

    // CANON-5: "State scalars still require initializers"; like locals, only
    // structs and arrays may use the type-default form `Name TType`.
    if (section.sectionKind() == ast::SectionKind::State) {
        for (const auto& [name, typeName] : stateDeclarationsWithoutInitializer_) {
            if (resolveStruct(typeName) == nullptr && !support::isArrayTypeName(typeName)) {
                throw SemanticError("scalar declaration requires initializer: " + name);
            }
        }
    }
}

} // namespace inox::compiler::semantic
