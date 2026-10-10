// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "SemanticAnalyzer.h"
#include "../exceptions/ExceptionTypes.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>

namespace inox::compiler::semantic {

namespace {

bool equalsIgnoreCase(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t index = 0; index < left.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(left[index])) !=
            std::tolower(static_cast<unsigned char>(right[index]))) {
            return false;
        }
    }

    return true;
}

std::string_view binaryOperatorName(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::Assign:
        return ":=";
    case ast::BinaryOperator::Add:
        return "+";
    case ast::BinaryOperator::Subtract:
        return "-";
    case ast::BinaryOperator::Multiply:
        return "*";
    case ast::BinaryOperator::Divide:
        return "/";
    case ast::BinaryOperator::IntegerDivide:
        return "div";
    case ast::BinaryOperator::Modulo:
        return "mod";
    case ast::BinaryOperator::ShiftLeft:
        return "shl";
    case ast::BinaryOperator::ShiftRight:
        return "shr";
    case ast::BinaryOperator::BitAnd:
        return "bitand";
    case ast::BinaryOperator::BitXor:
        return "bitxor";
    case ast::BinaryOperator::BitOr:
        return "bitor";
    case ast::BinaryOperator::Power:
        return "^";
    case ast::BinaryOperator::Range:
        return "..";
    case ast::BinaryOperator::In:
        return "in";
    case ast::BinaryOperator::Equal:
        return "=";
    case ast::BinaryOperator::NotEqual:
        return "#";
    case ast::BinaryOperator::Less:
        return "<";
    case ast::BinaryOperator::Greater:
        return ">";
    case ast::BinaryOperator::LessEqual:
        return "<=";
    case ast::BinaryOperator::GreaterEqual:
        return ">=";
    case ast::BinaryOperator::And:
        return "and";
    case ast::BinaryOperator::Xor:
        return "xor";
    case ast::BinaryOperator::Or:
        return "or";
    }
    return "?";
}

struct PreludeSignature {
    std::string_view name;
    std::vector<std::string_view> parameterTypes;
    std::string_view returnType;
};

const std::vector<PreludeSignature>& preludeSignatures()
{
    static const std::vector<PreludeSignature> signatures = {
        {"Put", {"*"}, "Void"},
        {"PutLn", {"*"}, "Void"},
        {"Get", {}, "Void"},
        {"GetLn", {}, "Void"},
        {"ReadLn", {}, "String"},
        {"Length", {"String"}, "Int64"},
        {"Ord", {"Char"}, "Int64"},

        {"Abs", {"Int64"}, "Int64"},
        {"Abs", {"Float64"}, "Float64"},
        {"Sqrt", {"Float64"}, "Float64"},
        {"Cbrt", {"Float64"}, "Float64"},
        {"Sin", {"Float64"}, "Float64"},
        {"Cos", {"Float64"}, "Float64"},
        {"Tan", {"Float64"}, "Float64"},
        {"ArcSin", {"Float64"}, "Float64"},
        {"ArcCos", {"Float64"}, "Float64"},
        {"ArcTan", {"Float64"}, "Float64"},
        {"ArcTan2", {"Float64", "Float64"}, "Float64"},
        {"Sinh", {"Float64"}, "Float64"},
        {"Cosh", {"Float64"}, "Float64"},
        {"Tanh", {"Float64"}, "Float64"},
        {"Exp", {"Float64"}, "Float64"},
        {"Ln", {"Float64"}, "Float64"},
        {"LnXP1", {"Float64"}, "Float64"},
        {"Log2", {"Float64"}, "Float64"},
        {"Log10", {"Float64"}, "Float64"},
        {"LogN", {"Float64", "Float64"}, "Float64"},
        {"Power", {"Float64", "Float64"}, "Float64"},
        {"Floor", {"Float64"}, "Float64"},
        {"Ceil", {"Float64"}, "Float64"},
        {"FMod", {"Float64", "Float64"}, "Float64"},
        {"Hypot", {"Float64", "Float64"}, "Float64"},
        {"Hypot3", {"Float64", "Float64", "Float64"}, "Float64"},
        {"RadToDeg", {"Float64"}, "Float64"},
        {"DegToRad", {"Float64"}, "Float64"},
        {"RadToGrad", {"Float64"}, "Float64"},
        {"GradToRad", {"Float64"}, "Float64"},
        {"RadToCycle", {"Float64"}, "Float64"},
        {"CycleToRad", {"Float64"}, "Float64"}
    };
    return signatures;
}

} // namespace

SemanticError::SemanticError(std::string message)
    : std::runtime_error(std::move(message))
{
}

SemanticAnalyzer::SemanticAnalyzer()
{
    declareBuiltins();
    declareBuiltinTypes();
}

const SemanticResult& SemanticAnalyzer::analyze(const ast::ModuleNode& module)
{
    result_.clear();

    if (module.name().empty()) {
        throw SemanticError("module must have a name");
    }

    for (const auto& item : module.items()) {
        declareModuleItem(*item);
    }

    registerFunctionSignatures(module);

    if (!hasMain_) {
        throw SemanticError("module must declare Main");
    }

    for (const auto& item : module.items()) {
        analyzeModuleItem(*item);
    }

    return result_;
}

const SemanticResult& SemanticAnalyzer::result() const
{
    return result_;
}

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
                if (index >= tokens.size() || !looksLikeIdentifier(tokens[index])) {
                    throw SemanticError("expected type for parameter: " + parameterName);
                }
                resolveTypeOrThrow(tokens[index]);
                parameterType = canonicalTypeName(tokens[index++]);
            }

            for (const FunctionParameter& parameter : signature.parameters) {
                if (equalsIgnoreCase(parameter.name, parameterName)) {
                    throw SemanticError("duplicate parameter: " + parameterName);
                }
            }
            signature.parameters.push_back(FunctionParameter{parameterName, parameterType});

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
        if (index + 1 != tokens.size() || !looksLikeIdentifier(tokens[index])) {
            throw SemanticError("invalid return type in function: " + function.name());
        }
        resolveTypeOrThrow(tokens[index]);
        signature.returnType = canonicalTypeName(tokens[index]);
    }

    functions_.emplace(normalizeName(function.name()), std::move(signature));
}

const FunctionSignature* SemanticAnalyzer::resolveFunctionSignature(std::string_view name) const
{
    const auto iterator = functions_.find(normalizeName(name));
    return iterator != functions_.end() ? &iterator->second : nullptr;
}

void SemanticAnalyzer::declareBuiltins()
{
    const std::vector<std::string_view> valueBuiltins = {
        "Put", "PutLn", "Get", "GetLn", "ReadLn",
        "Abs", "Sqrt", "Cbrt", "Sin", "Cos", "Tan", "ArcSin", "ArcCos", "ArcTan", "ArcTan2",
        "Sinh", "Cosh", "Tanh", "Exp", "Ln", "LnXP1", "Log2", "Log10", "LogN", "Power",
        "Floor", "Ceil", "FMod", "Hypot", "Hypot3",
        "RadToDeg", "DegToRad", "RadToGrad", "GradToRad", "RadToCycle", "CycleToRad",
        "Length", "Ord",
        "True", "False",
        "__index", "__member",
        "Sys", "IO", "Math", "Std"
    };

    for (std::string_view name : valueBuiltins) {
        symbols_.globalScope().declare(std::string(name), SymbolKind::Builtin);
    }
}

void SemanticAnalyzer::declareBuiltinTypes()
{
    constexpr std::array<std::string_view, 16> builtinTypes = {
        "Bool",
        "Int8", "Int16", "Int32", "Int64",
        "UInt8", "UInt16", "UInt32", "UInt64",
        "Natural",
        "Float32", "Float64",
        "Currency", "Crypto", "Char", "String"
    };

    for (std::string_view name : builtinTypes) {
        declareTypeOrThrow(name, true);
    }

    declareTypeOrThrow("Integer", true, "Int64");
    declareTypeOrThrow("UInteger", true, "UInt64");
    declareTypeOrThrow("Float", true, "Float64");

    // Exception identifiers are genuine nominal TYPES, not enum constants or
    // ordinary values. Their taxonomy is centralized in ExceptionTypes.h; the
    // stdlib documents the public surface while the compiler owns bootstrap
    // type identity until user-defined Exception declarations are specified.
    for (const auto& info : exceptions::kStandardExceptionTypes) {
        declareTypeOrThrow(info.name, true);
    }
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
        for (const ast::SectionInitializer& initializer : section.initializers()) {
            const std::string valueType =
                canonicalTypeName(analyzeExpression(*initializer.value));
            if (!initializer.typeName.empty()) {
                const std::string declared = canonicalTypeName(initializer.typeName);
                if (!valueType.empty() && !canAssign(declared, valueType)) {
                    throw SemanticError("initializer of " + initializer.name + " has type " +
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

    SymbolKind kind = SymbolKind::Variable;
    if (section.sectionKind() == ast::SectionKind::Const) {
        kind = SymbolKind::Constant;
    } else if (section.sectionKind() == ast::SectionKind::State) {
        kind = SymbolKind::State;
    }

    const auto& tokens = section.tokens();
    const auto& lines = section.tokenLines();
    const bool isMutable = kind == SymbolKind::Variable || kind == SymbolKind::State;
    // An initializer runs to the end of its line (one declaration per line).
    // Skipping it as a unit keeps tokens inside it, such as the field names of
    // `TPoint(X := 1, Y := 2)`, from being read as new declarations.
    const auto endOfInitializer = [&](std::size_t valueIndex, std::size_t fallback) {
        if (lines.size() != tokens.size() || valueIndex >= tokens.size()) {
            return fallback;
        }
        std::size_t end = valueIndex;
        while (end < tokens.size() && lines[end] == lines[valueIndex]) {
            ++end;
        }
        return end;
    };
    // A Const value is recorded only when it is a single token; other
    // initializers stay unresolved (the backend reports them as a GAP).
    const auto singleTokenValue = [&](std::size_t valueIndex) {
        return valueIndex < tokens.size() &&
               endOfInitializer(valueIndex, valueIndex + 1) == valueIndex + 1;
    };
    for (std::size_t index = 0; index + 1 < tokens.size();) {
        if (!looksLikeIdentifier(tokens[index])) {
            ++index;
            continue;
        }

        // Canonical forms recognized here:
        //   Name := Expr            (inferred type)            tokens: Name := value ...
        //   Name Type := Expr       (explicit type + value)    tokens: Name Type := value ...
        //   Name Type               (struct/aggregate default) tokens: Name Type
        // The OLD `Name : Type` form (with a ':') is also tolerated.
        const std::string& next = tokens[index + 1];

        if (next == ":=") {
            // Name := Expr  -> inferred-type declaration
            declareOrThrow(tokens[index], kind,
                           inferSectionDeclarationType(tokens, index), isMutable);
            if (kind == SymbolKind::Constant && singleTokenValue(index + 2)) {
                recordConstantValue(tokens[index], tokens[index + 2]);
            }
            index = endOfInitializer(index + 2, index + 3);  // Name := value
            continue;
        }

        if (next == ":") {
            // legacy Name : Type (tolerated); CANON-5 still applies to State.
            declareOrThrow(tokens[index], kind,
                           inferSectionDeclarationType(tokens, index), isMutable);
            const bool hasInitializer = index + 3 < tokens.size() && tokens[index + 3] == ":=";
            if (kind == SymbolKind::State && !hasInitializer && index + 2 < tokens.size()) {
                stateDeclarationsWithoutInitializer_.emplace_back(tokens[index], tokens[index + 2]);
            }
            index = hasInitializer ? endOfInitializer(index + 4, index + 3)
                                   : index + 3;  // Name : Type [:= value]
            continue;
        }

        if (looksLikeIdentifier(next)) {
            // Name Type [:= Expr]  -> explicit-typed declaration.
            // The TYPE token must NOT be declared as a symbol (it is a type name).
            declareOrThrow(tokens[index], kind, canonicalTypeName(next), isMutable);
            if (index + 2 < tokens.size() && tokens[index + 2] == ":=") {
                if (kind == SymbolKind::Constant && singleTokenValue(index + 3)) {
                    recordConstantValue(tokens[index], tokens[index + 3]);
                }
                index = endOfInitializer(index + 3, index + 4);  // Name Type := value
            } else {
                if (kind == SymbolKind::State) {
                    stateDeclarationsWithoutInitializer_.emplace_back(tokens[index], next);
                }
                index += 2;  // Name Type
            }
            continue;
        }

        ++index;
    }
}

std::string SemanticAnalyzer::inferSectionDeclarationType(
    const std::vector<std::string>& tokens,
    std::size_t nameIndex) const
{
    if (nameIndex + 2 < tokens.size() && tokens[nameIndex + 1] == ":") {
        return canonicalTypeName(tokens[nameIndex + 2]);
    }

    if (nameIndex + 2 >= tokens.size() || tokens[nameIndex + 1] != ":=") {
        return {};
    }

    const std::string_view value = tokens[nameIndex + 2];
    if (value == "True" || value == "False" || value == "true" || value == "false") {
        return "Bool";
    }
    if (!value.empty() && value.front() == '"') {
        return "String";
    }
    if (!value.empty() && value.front() == '\'') {
        return "Char";
    }
    if (!value.empty() && (std::isdigit(static_cast<unsigned char>(value.front())) ||
                           value.front() == '$')) {
        return value.find('.') == std::string_view::npos ? "Int64" : "Float64";
    }

    const Symbol* symbol = symbols_.currentScope().resolve(value);
    return symbol != nullptr ? symbol->typeName : std::string{};
}

void SemanticAnalyzer::registerTypeSectionSymbols(const ast::SectionDeclaration& section)
{
    const auto& tokens = section.tokens();
    for (std::size_t index = 0; index < tokens.size();) {
        if (index + 1 < tokens.size() && looksLikeIdentifier(tokens[index]) &&
            equalsIgnoreCase(tokens[index + 1], "Struct")) {
            registerStructDeclaration(tokens, index);
            continue;
        }

        if (index + 1 < tokens.size() && looksLikeIdentifier(tokens[index]) &&
            tokens[index + 1] == ":=") {
            declareTypeOrThrow(tokens[index], false);
            index += 2;
            continue;
        }

        ++index;
    }
}

void SemanticAnalyzer::registerStructDeclaration(const std::vector<std::string>& tokens, std::size_t& index)
{
    const std::string typeName = tokens[index];
    declareTypeOrThrow(typeName, false);

    StructType structType;
    structType.name = typeName;
    index += 2;

    // Duplicate detection by normalized (case-insensitive) name keeps a large
    // struct linear; the former scan over all earlier fields was quadratic.
    std::unordered_set<std::string> seenFieldNames;

    while (index < tokens.size() && tokens[index] != ";") {
        if (index + 1 >= tokens.size() || !looksLikeIdentifier(tokens[index]) ||
            !looksLikeIdentifier(tokens[index + 1])) {
            throw SemanticError("invalid struct field declaration in: " + typeName);
        }

        const std::string fieldName = tokens[index++];
        const std::string fieldType = tokens[index++];
        StructField field;
        field.name = fieldName;
        field.typeName = canonicalTypeName(fieldType);

        if (index < tokens.size() && tokens[index] == ":=") {
            ++index;
            if (index >= tokens.size() || tokens[index] == ";") {
                throw SemanticError("expected default value for struct field: " + fieldName);
            }
            field.hasDefault = true;
            field.defaultValue = tokens[index++];
        }

        if (!seenFieldNames.insert(normalizeName(fieldName)).second) {
            throw SemanticError("duplicate struct field: " + fieldName);
        }
        structType.fields.push_back(std::move(field));
    }

    if (index >= tokens.size() || tokens[index] != ";") {
        throw SemanticError("expected ';' to close Struct: " + typeName);
    }
    ++index;

    structs_.emplace(normalizeName(typeName), std::move(structType));
}


void SemanticAnalyzer::validateStructDeclaration(const std::vector<std::string>& tokens, std::size_t& index) const
{
    const std::string typeName = tokens[index];
    index += 2;

    while (index < tokens.size() && tokens[index] != ";") {
        if (index + 1 >= tokens.size() || !looksLikeIdentifier(tokens[index]) ||
            !looksLikeIdentifier(tokens[index + 1])) {
            throw SemanticError("invalid struct field declaration in: " + typeName);
        }
        const std::string fieldName = tokens[index];
        index += 1;
        const std::string fieldType = canonicalTypeName(tokens[index]);
        resolveTypeOrThrow(tokens[index]);
        index += 1;

        if (index < tokens.size() && tokens[index] == ":=") {
            ++index;
            if (index >= tokens.size() || tokens[index] == ";") {
                throw SemanticError("expected default value for struct field: " + fieldName);
            }
            const std::string defaultValue = tokens[index++];
            if (equalsIgnoreCase(fieldType, "Integer") || equalsIgnoreCase(fieldType, "Int64")) {
                const bool isDecimal = !defaultValue.empty() &&
                    std::all_of(defaultValue.begin(), defaultValue.end(), [](unsigned char ch) {
                        return std::isdigit(ch) != 0;
                    });
                const bool isHex = defaultValue.size() > 1 && defaultValue.front() == '$';
                if (!isDecimal && !isHex) {
                    throw SemanticError("Integer struct field default must be an integer literal: " + fieldName);
                }
            } else if (equalsIgnoreCase(fieldType, "Bool")) {
                if (!equalsIgnoreCase(defaultValue, "true") && !equalsIgnoreCase(defaultValue, "false")) {
                    throw SemanticError("Bool struct field default must be true or false: " + fieldName);
                }
            } else {
                throw SemanticError("struct field defaults are currently supported only for Integer and Bool: " + fieldName);
            }
        }
    }

    if (index >= tokens.size() || tokens[index] != ";") {
        throw SemanticError("expected ';' to close Struct: " + typeName);
    }
    ++index;
}

void SemanticAnalyzer::validateSectionTypes(const ast::SectionDeclaration& section) const
{
    const auto& tokens = section.tokens();

    if (section.sectionKind() == ast::SectionKind::Type) {
        for (std::size_t index = 0; index < tokens.size();) {
            if (index + 1 < tokens.size() && looksLikeIdentifier(tokens[index]) &&
                equalsIgnoreCase(tokens[index + 1], "Struct")) {
                validateStructDeclaration(tokens, index);
                continue;
            }

            if (index + 2 < tokens.size() && looksLikeIdentifier(tokens[index]) &&
                tokens[index + 1] == ":=") {
                const std::string_view head = tokens[index + 2];
                if (equalsIgnoreCase(head, "Struct") || equalsIgnoreCase(head, "Enum")) {
                    index += 3;
                    continue;
                }
                if (equalsIgnoreCase(head, "Set") || equalsIgnoreCase(head, "Vector")) {
                    if (index + 4 < tokens.size() && tokens[index + 3] == "[") {
                        resolveTypeOrThrow(tokens[index + 4]);
                    }
                    index += 3;
                    continue;
                }
                if (equalsIgnoreCase(head, "Array")) {
                    for (std::size_t typeIndex = index + 3; typeIndex < tokens.size(); ++typeIndex) {
                        if (tokens[typeIndex] == "]" && typeIndex + 1 < tokens.size() &&
                            looksLikeIdentifier(tokens[typeIndex + 1])) {
                            resolveTypeOrThrow(tokens[typeIndex + 1]);
                            break;
                        }
                    }
                    index += 3;
                    continue;
                }
                if (looksLikeIdentifier(head)) {
                    resolveTypeOrThrow(head);
                }
                index += 3;
                continue;
            }

            ++index;
        }
        return;
    }

    if (section.sectionKind() != ast::SectionKind::Const &&
        section.sectionKind() != ast::SectionKind::State) {
        return;
    }

    // CANON-5: "State scalars still require initializers"; like locals, only
    // structs may use the type-default form `Name TStruct`.
    if (section.sectionKind() == ast::SectionKind::State) {
        for (const auto& [name, typeName] : stateDeclarationsWithoutInitializer_) {
            if (resolveStruct(typeName) == nullptr) {
                throw SemanticError("scalar declaration requires initializer: " + name);
            }
        }
    }

    for (std::size_t index = 0; index + 2 < tokens.size(); ++index) {
        if (looksLikeIdentifier(tokens[index]) && tokens[index + 1] == ":" &&
            looksLikeIdentifier(tokens[index + 2])) {
            resolveTypeOrThrow(tokens[index + 2]);
        }
    }
}

void SemanticAnalyzer::analyzeFunction(const ast::FunctionDeclaration& function)
{
    const std::string previousReturnType = std::move(currentFunctionReturnType_);
    const bool previousSawReturn = currentFunctionSawReturn_;
    const FunctionSignature* signature = resolveFunctionSignature(function.name());
    currentFunctionReturnType_ = signature != nullptr ? signature->returnType : std::string{};
    currentFunctionSawReturn_ = false;
    symbols_.pushScope();
    if (signature != nullptr) {
        for (const FunctionParameter& parameter : signature->parameters) {
            declareOrThrow(parameter.name, SymbolKind::Variable, parameter.typeName, false);
        }
    }
    analyzeStatements(function.body(), false);
    symbols_.popScope();
    if (!currentFunctionReturnType_.empty() && !currentFunctionSawReturn_) {
        throw SemanticError("function " + function.name() + " must return a value");
    }
    if (!currentFunctionReturnType_.empty() && !cannotFallThrough(function.body())) {
        throw SemanticError("function " + function.name() +
                            " may reach its end without returning a value; every path must end "
                            "in Return or Raise (CANON-12)");
    }
    currentFunctionReturnType_ = previousReturnType;
    currentFunctionSawReturn_ = previousSawReturn;
}

bool SemanticAnalyzer::cannotFallThrough(const std::vector<ast::StatementPtr>& statements)
{
    for (const auto& statement : statements) {
        if (statement && cannotFallThrough(*statement)) {
            return true;
        }
    }
    return false;
}

// Loops are treated as "may fall through" (a loop may run zero times or exit
// through leave). This never accepts a function that can fall through; it may
// reject one whose only exit is inside a loop, which then needs a final Return.
bool SemanticAnalyzer::cannotFallThrough(const ast::Statement& statement)
{
    switch (statement.kind()) {
    case ast::AstNodeKind::ReturnStatement:
    case ast::AstNodeKind::RaiseStatement:
    case ast::AstNodeKind::RetryStatement:
        return true;
    case ast::AstNodeKind::BlockStatement:
        return cannotFallThrough(static_cast<const ast::BlockStatement&>(statement).statements());
    case ast::AstNodeKind::WithStatement:
        return cannotFallThrough(static_cast<const ast::WithStatement&>(statement).body());
    case ast::AstNodeKind::IfStatement: {
        const auto& ifStatement = static_cast<const ast::IfStatement&>(statement);
        if (ifStatement.elseBody().empty() || !cannotFallThrough(ifStatement.thenBody()) ||
            !cannotFallThrough(ifStatement.elseBody())) {
            return false;
        }
        for (const ast::ElseIfClause& clause : ifStatement.elseIfClauses()) {
            if (!cannotFallThrough(clause.body)) {
                return false;
            }
        }
        return true;
    }
    case ast::AstNodeKind::CaseStatement: {
        const auto& caseStatement = static_cast<const ast::CaseStatement&>(statement);
        if (caseStatement.otherwiseBody().empty() ||
            !cannotFallThrough(caseStatement.otherwiseBody())) {
            return false;
        }
        for (const ast::CaseArm& arm : caseStatement.arms()) {
            if (!cannotFallThrough(arm.body)) {
                return false;
            }
        }
        return true;
    }
    case ast::AstNodeKind::TryStatement: {
        const auto& tryStatement = static_cast<const ast::TryStatement&>(statement);
        if (tryStatement.hasEnsure() && cannotFallThrough(tryStatement.ensureBody())) {
            return true;
        }
        if (!cannotFallThrough(tryStatement.body())) {
            return false;
        }
        if (tryStatement.hasPlainExcept() && !cannotFallThrough(tryStatement.exceptBody())) {
            return false;
        }
        for (const ast::ExceptionHandler& handler : tryStatement.handlers()) {
            if (!cannotFallThrough(handler.body)) {
                return false;
            }
        }
        if (!tryStatement.elseBody().empty() && !cannotFallThrough(tryStatement.elseBody())) {
            return false;
        }
        return true;
    }
    default:
        return false;
    }
}

void SemanticAnalyzer::analyzeStatements(const std::vector<ast::StatementPtr>& statements, bool createScope)
{
    if (createScope) {
        symbols_.pushScope();
    }

    for (const auto& statement : statements) {
        analyzeStatement(*statement);
    }

    if (createScope) {
        symbols_.popScope();
    }
}

void SemanticAnalyzer::analyzeStatement(const ast::Statement& statement)
{
    switch (statement.kind()) {
    case ast::AstNodeKind::ExitStatement:
        // CANON-12: `Exit` is valid only in subroutines without a return value
        // and in Main; it is FORBIDDEN in functions. Found by mutation fuzzing:
        // previously only the LLVM backend noticed.
        if (!currentFunctionReturnType_.empty()) {
            throw SemanticError("Exit is not valid in a function that returns a value; use Return (CANON-12)");
        }
        break;
    case ast::AstNodeKind::BlockStatement: {
        const auto& block = static_cast<const ast::BlockStatement&>(statement);
        analyzeStatements(block.statements(), true);
        break;
    }
    case ast::AstNodeKind::ExpressionStatement: {
        const ast::Expression& statementExpression =
            static_cast<const ast::ExpressionStatement&>(statement).expression();
        requireStatementExpression(statementExpression);
        analyzeExpression(statementExpression);
        if (statementExpression.kind() == ast::AstNodeKind::CallExpression) {
            const auto& call = static_cast<const ast::CallExpression&>(statementExpression);
            if (isMemberCall(call) && result_.symbolOf(call) == nullptr) {
                throw SemanticError("field access is not a statement: only assignments and calls may be used as statements");
            }
        }
        break;
    }
    case ast::AstNodeKind::VarStatement: {
        const auto& var = static_cast<const ast::VarStatement&>(statement);
        std::string typeName = var.typeName().empty() ? std::string{} : canonicalTypeName(var.typeName());
        if (!var.typeName().empty()) {
            resolveTypeOrThrow(var.typeName());
        }
        if (var.initializer() != nullptr) {
            const std::string initializerType = analyzeExpression(*var.initializer());
            if (typeName.empty()) {
                typeName = initializerType;
            } else if (!canAssign(typeName, initializerType)) {
                throw SemanticError("cannot initialize " + typeName + " with " + initializerType);
            }
        }
        if (typeName.empty()) {
            throw SemanticError("variable requires a type or initializer: " + var.name());
        }
        // CANON-5 rule 3/4: every variable is born with a value. Only structs and
        // aggregates may omit ":=" (type-default initialization); scalars must be
        // initialized explicitly.
        if (var.initializer() == nullptr && resolveStruct(typeName) == nullptr) {
            throw SemanticError("scalar declaration requires initializer: " + var.name());
        }
        declareOrThrow(var.name(), SymbolKind::Variable, std::move(typeName), true);
        break;
    }
    case ast::AstNodeKind::VarBlockStatement:
        analyzeVarBlock(static_cast<const ast::VarBlockStatement&>(statement));
        break;
    case ast::AstNodeKind::IfStatement: {
        const auto& ifStatement = static_cast<const ast::IfStatement&>(statement);
        requireBoolCondition(ifStatement.condition());
        analyzeStatements(ifStatement.thenBody(), true);
        for (const auto& clause : ifStatement.elseIfClauses()) {
            requireBoolCondition(*clause.condition);
            analyzeStatements(clause.body, true);
        }
        analyzeStatements(ifStatement.elseBody(), true);
        break;
    }
    case ast::AstNodeKind::UnlessStatement: {
        const auto& unlessStatement = static_cast<const ast::UnlessStatement&>(statement);
        requireBoolCondition(unlessStatement.condition());
        analyzeStatements(unlessStatement.body(), true);
        break;
    }
    case ast::AstNodeKind::WhileStatement: {
        const auto& whileStatement = static_cast<const ast::WhileStatement&>(statement);
        requireBoolCondition(whileStatement.condition());
        ++loopDepth_;
        analyzeStatements(whileStatement.body(), true);
        --loopDepth_;
        break;
    }
    case ast::AstNodeKind::RepeatStatement: {
        const auto& repeatStatement = static_cast<const ast::RepeatStatement&>(statement);
        ++loopDepth_;
        ++repeatDepth_;
        analyzeStatements(repeatStatement.body(), true);
        --repeatDepth_;
        --loopDepth_;
        break;
    }
    case ast::AstNodeKind::UntilStatement: {
        const auto& untilStatement = static_cast<const ast::UntilStatement&>(statement);
        if (repeatDepth_ == 0) {
            throw SemanticError("until outside repeat");
        }
        requireBoolCondition(untilStatement.condition());
        break;
    }
    case ast::AstNodeKind::ForInStatement: {
        const auto& forStatement = static_cast<const ast::ForInStatement&>(statement);
        // ADR-0010: `for I in A..N(2)` was the old step form. Now `N(2)` is a
        // call; when N is a value, say how the step is written.
        if (forStatement.iterable().kind() == ast::AstNodeKind::BinaryExpression) {
            const auto& range = static_cast<const ast::BinaryExpression&>(forStatement.iterable());
            if (range.op() == ast::BinaryOperator::Range &&
                range.right().kind() == ast::AstNodeKind::CallExpression) {
                const auto& call = static_cast<const ast::CallExpression&>(range.right());
                if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
                    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
                    const Symbol* symbol = symbols_.currentScope().resolve(callee.name());
                    if (symbol != nullptr && symbol->kind != SymbolKind::Function &&
                        symbol->kind != SymbolKind::Builtin && symbol->kind != SymbolKind::Type) {
                        throw SemanticError(
                            "'" + callee.name() + "(...)' calls a value: the for-loop step "
                            "is written 'step S' (ADR-0010), e.g. 'for I in A.." +
                            callee.name() + " step S'");
                    }
                }
            }
        }
        analyzeExpression(forStatement.iterable());
        if (forStatement.step() != nullptr) {
            const std::string stepType = canonicalTypeName(analyzeExpression(*forStatement.step()));
            if (!isIntegerType(stepType)) {
                throw SemanticError("for-loop step must be an Integer expression, got " +
                                    (stepType.empty() ? std::string("<unknown>") : stepType));
            }
            std::int64_t constantStep = 0;
            if (constantIntegerValue(*forStatement.step(), constantStep) && constantStep <= 0) {
                throw SemanticError("for-loop step must be a positive integer");
            }
        }
        symbols_.pushScope();
        if (symbols_.currentScope().containsLocal(forStatement.iterator()) ||
            symbols_.currentScope().containsInAncestors(forStatement.iterator())) {
            symbols_.popScope();
            throw SemanticError(
                "loop iterator conflicts with existing symbol: " + forStatement.iterator());
        }
        declareOrThrow(forStatement.iterator(), SymbolKind::LoopIterator, "Int64", false);
        ++loopDepth_;
        analyzeStatements(forStatement.body(), false);
        --loopDepth_;
        symbols_.popScope();
        break;
    }
    case ast::AstNodeKind::CaseStatement: {
        const auto& caseStatement = static_cast<const ast::CaseStatement&>(statement);
        analyzeExpression(caseStatement.expression());
        for (const auto& arm : caseStatement.arms()) {
            for (const auto& choice : arm.choices) {
                analyzeExpression(*choice);
            }
            analyzeStatements(arm.body, true);
        }
        analyzeStatements(caseStatement.otherwiseBody(), true);
        break;
    }
    case ast::AstNodeKind::TryStatement: {
        const auto& tryStatement = static_cast<const ast::TryStatement&>(statement);

        analyzeStatements(tryStatement.body(), true);

        if (tryStatement.hasPlainExcept()) {
            // Bare Raise is meaningful here, but Retry is intentionally not:
            // Retry belongs to an explicit On/Else handler for this try.
            ++exceptionHandlerDepth_;
            analyzeStatements(tryStatement.exceptBody(), true);
            --exceptionHandlerDepth_;
        }

        bool catchesAll = false;
        std::unordered_map<std::string, bool> seenHandlers;
        std::vector<std::string> precedingHandlerTypes;
        for (const auto& handler : tryStatement.handlers()) {
            if (!isExceptionType(handler.typeName)) {
                throw SemanticError("unknown exception type in On handler: " + handler.typeName);
            }
            const std::string normalizedType = normalizeName(handler.typeName);
            if (seenHandlers.contains(normalizedType)) {
                throw SemanticError("duplicate exception handler: " + handler.typeName);
            }
            if (catchesAll) {
                throw SemanticError("unreachable exception handler after On Exception: " + handler.typeName);
            }
            for (const std::string& precedingType : precedingHandlerTypes) {
                if (exceptions::isSubtypeOf(handler.typeName, precedingType)) {
                    throw SemanticError(
                        "unreachable exception handler: " + handler.typeName +
                        " is already matched by earlier On " + precedingType);
                }
            }
            seenHandlers.emplace(normalizedType, true);
            precedingHandlerTypes.push_back(handler.typeName);
            catchesAll = exceptions::isSubtypeOf("Exception", handler.typeName);

            symbols_.pushScope();
            if (!handler.bindingName.empty()) {
                declareOrThrow(handler.bindingName, SymbolKind::Variable, handler.typeName, false);
            }
            ++exceptionHandlerDepth_;
            ++retryHandlerDepth_;
            analyzeStatements(handler.body, false);
            --retryHandlerDepth_;
            --exceptionHandlerDepth_;
            symbols_.popScope();
        }

        if (!tryStatement.elseBody().empty()) {
            if (catchesAll) {
                throw SemanticError("unreachable Else after On Exception");
            }
            ++exceptionHandlerDepth_;
            ++retryHandlerDepth_;
            analyzeStatements(tryStatement.elseBody(), true);
            --retryHandlerDepth_;
            --exceptionHandlerDepth_;
        }

        if (tryStatement.hasEnsure()) {
            ++ensureDepth_;
            analyzeStatements(tryStatement.ensureBody(), true);
            --ensureDepth_;
        }
        break;
    }
    case ast::AstNodeKind::RaiseStatement: {
        const auto& raiseStatement = static_cast<const ast::RaiseStatement&>(statement);
        if (raiseStatement.expression() == nullptr) {
            if (exceptionHandlerDepth_ == 0) {
                throw SemanticError("bare Raise is only allowed inside an exception handler");
            }
            break;
        }

        const ast::Expression& expression = *raiseStatement.expression();
        if (expression.kind() != ast::AstNodeKind::IdentifierExpression) {
            throw SemanticError("Raise currently requires an exception type name");
        }
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
        if (!isExceptionType(identifier.name())) {
            throw SemanticError("Raise requires an exception type: " + identifier.name());
        }
        break;
    }
    case ast::AstNodeKind::RetryStatement: {
        const auto& retryStatement = static_cast<const ast::RetryStatement&>(statement);
        if (retryHandlerDepth_ == 0 || ensureDepth_ != 0) {
            throw SemanticError("Retry is only allowed inside an active On/Else exception handler");
        }
        const std::string countType = analyzeExpression(retryStatement.count());
        if (!isIntegerType(countType)) {
            throw SemanticError("Retry count must be an Integer expression");
        }
        if (retryStatement.count().kind() == ast::AstNodeKind::UnaryExpression) {
            const auto& unary = static_cast<const ast::UnaryExpression&>(retryStatement.count());
            if (unary.op() == ast::UnaryOperator::Minus &&
                unary.operand().kind() == ast::AstNodeKind::LiteralExpression) {
                const auto& literal = static_cast<const ast::LiteralExpression&>(unary.operand());
                if (literal.literalKind() == ast::LiteralKind::Integer) {
                    throw SemanticError("Retry count cannot be negative");
                }
            }
        }
        break;
    }
    case ast::AstNodeKind::ReturnStatement: {
        currentFunctionSawReturn_ = true;
        const std::string typeName =
            analyzeExpression(static_cast<const ast::ReturnStatement&>(statement).expression());
        if (currentFunctionReturnType_.empty()) {
            throw SemanticError("subroutine without return type cannot return a value");
        }
        if (!canAssign(currentFunctionReturnType_, typeName)) {
            throw SemanticError(
                "cannot return " + typeName + " from function returning " +
                currentFunctionReturnType_);
        }
        break;
    }
    case ast::AstNodeKind::LeaveStatement:
        if (loopDepth_ == 0) {
            throw SemanticError("leave outside loop");
        }
        break;
    case ast::AstNodeKind::ContinueStatement:
        if (loopDepth_ == 0) {
            throw SemanticError("continue outside loop");
        }
        break;
    case ast::AstNodeKind::WithStatement: {
        const auto& withStatement = static_cast<const ast::WithStatement&>(statement);
        const std::string targetType = analyzeExpression(withStatement.target());
        symbols_.pushScope();
        // Declare the synthetic binding directly — no shadowing check needed
        // because the name (__with_N) is compiler-generated and never user-visible.
        symbols_.currentScope().declare(
            withStatement.bindingName(), SymbolKind::Variable, targetType, true);
        analyzeStatements(withStatement.body(), false);
        symbols_.popScope();
        break;
    }
    default:
        break;
    }
}

void SemanticAnalyzer::analyzeVarBlock(const ast::VarBlockStatement& statement)
{
    for (const auto& declaration : statement.declarations()) {
        if (declaration->kind() == ast::AstNodeKind::VarStatement) {
            analyzeStatement(*declaration);
            continue;
        }

        if (declaration->kind() == ast::AstNodeKind::ExpressionStatement) {
            const auto& expressionStatement = static_cast<const ast::ExpressionStatement&>(*declaration);
            const auto& expression = expressionStatement.expression();
            if (expression.kind() == ast::AstNodeKind::BinaryExpression) {
                const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
                if (binary.op() == ast::BinaryOperator::Assign &&
                    binary.left().kind() == ast::AstNodeKind::IdentifierExpression) {
                    const auto& identifier = static_cast<const ast::IdentifierExpression&>(binary.left());
                    const std::string typeName = analyzeExpression(binary.right());
                    declareOrThrow(identifier.name(), SymbolKind::Variable, typeName, true);
                    const Symbol& symbol = resolveOrThrow(identifier.name());
                    result_.bind(identifier, symbol);
                    result_.setExpressionType(identifier, resolvedType(typeName));
                    result_.setExpressionType(binary, resolvedType(typeName));
                    continue;
                }
            }
        }

        analyzeStatement(*declaration);
    }
}

namespace {

constexpr std::int64_t kInt64Max = std::numeric_limits<std::int64_t>::max();
constexpr std::int64_t kInt64Min = std::numeric_limits<std::int64_t>::min();
constexpr std::uint64_t kInt64MagnitudeOfMin = static_cast<std::uint64_t>(1) << 63;

// Parses an Inox integer literal (decimal, `0x...` or `$...`). Returns false if
// the magnitude does not fit in 64 bits or the text is malformed. Portable:
// no compiler-specific wide integer types are used.
bool parseIntegerLiteralMagnitude(std::string_view text, std::uint64_t& value)
{
    std::uint64_t base = 10;
    if (!text.empty() && text.front() == '$') {
        base = 16;
        text.remove_prefix(1);
    } else if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        base = 16;
        text.remove_prefix(2);
    }
    if (text.empty()) {
        return false;
    }
    value = 0;
    for (const char ch : text) {
        std::uint64_t digit = 0;
        if (ch >= '0' && ch <= '9') {
            digit = static_cast<std::uint64_t>(ch - '0');
        } else if (base == 16 && ch >= 'a' && ch <= 'f') {
            digit = static_cast<std::uint64_t>(ch - 'a') + 10U;
        } else if (base == 16 && ch >= 'A' && ch <= 'F') {
            digit = static_cast<std::uint64_t>(ch - 'A') + 10U;
        } else {
            return false;
        }
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / base) {
            return false;
        }
        value = value * base + digit;
    }
    return true;
}

bool checkedAdd(std::int64_t a, std::int64_t b, std::int64_t& out)
{
    if ((b > 0 && a > kInt64Max - b) || (b < 0 && a < kInt64Min - b)) {
        return false;
    }
    out = a + b;
    return true;
}

bool checkedSub(std::int64_t a, std::int64_t b, std::int64_t& out)
{
    if ((b < 0 && a > kInt64Max + b) || (b > 0 && a < kInt64Min + b)) {
        return false;
    }
    out = a - b;
    return true;
}

bool checkedMul(std::int64_t a, std::int64_t b, std::int64_t& out)
{
    if (a == 0 || b == 0) {
        out = 0;
        return true;
    }
    if ((a == -1 && b == kInt64Min) || (b == -1 && a == kInt64Min)) {
        return false;
    }
    if (a > 0) {
        if (b > 0 ? a > kInt64Max / b : b < kInt64Min / a) {
            return false;
        }
    } else {
        if (b > 0 ? a < kInt64Min / b : a < kInt64Max / b) {
            return false;
        }
    }
    out = a * b;
    return true;
}

[[noreturn]] void throwConstantOverflow(const char* what)
{
    throw SemanticError(std::string("constant integer overflow in ") + what +
                        ": result does not fit in Int64 (integer overflow is never wraparound)");
}

} // namespace

bool SemanticAnalyzer::constantIntegerValue(const ast::Expression& expression, std::int64_t& value) const
{
    const auto found = constants_.find(&expression);
    if (found == constants_.end()) {
        return false;
    }
    value = found->second;
    return true;
}

void SemanticAnalyzer::rejectInvalidConstantRightOperand(ast::BinaryOperator op, std::int64_t value)
{
    switch (op) {
    case ast::BinaryOperator::IntegerDivide:
        if (value == 0) {
            throw SemanticError("constant division by zero in 'div'");
        }
        return;
    case ast::BinaryOperator::Modulo:
        if (value == 0) {
            throw SemanticError("constant division by zero in 'mod'");
        }
        return;
    case ast::BinaryOperator::ShiftLeft:
    case ast::BinaryOperator::ShiftRight:
        if (value < 0 || value > 63) {
            throw SemanticError("constant shift count out of range: must be between 0 and 63");
        }
        return;
    case ast::BinaryOperator::Power:
        if (value < 0) {
            throw SemanticError("constant exponent must not be negative");
        }
        return;
    default:
        return;
    }
}

// A module Const whose value is a single Integer or Bool literal is resolved
// here, once. Other forms stay unresolved; the backend then reports them as
// "not yet implemented" instead of guessing (docs/BACKEND_GAPS.md).
void SemanticAnalyzer::recordConstantValue(std::string_view name, std::string_view valueToken)
{
    const Symbol* symbol = symbols_.currentScope().resolve(name);
    if (symbol == nullptr || symbol->kind != SymbolKind::Constant) {
        return;
    }
    ConstantValue value;
    if (equalsIgnoreCase(valueToken, "true") || equalsIgnoreCase(valueToken, "false")) {
        value.kind = ConstantValue::Kind::Boolean;
        value.boolean = equalsIgnoreCase(valueToken, "true");
        result_.setConstantValue(*symbol, value);
        return;
    }
    const bool looksInteger = !valueToken.empty() &&
        (valueToken.front() == '$' || std::isdigit(static_cast<unsigned char>(valueToken.front())) != 0) &&
        valueToken.find('.') == std::string_view::npos;
    if (!looksInteger) {
        return;
    }
    std::uint64_t magnitude = 0;
    if (!parseIntegerLiteralMagnitude(valueToken, magnitude) || magnitude >= kInt64MagnitudeOfMin) {
        throw SemanticError("integer literal does not fit in Int64 in Const " + std::string(name) +
                            ": " + std::string(valueToken));
    }
    value.kind = ConstantValue::Kind::Integer;
    value.integer = static_cast<std::int64_t>(magnitude);
    result_.setConstantValue(*symbol, value);
}

void SemanticAnalyzer::foldConstantExpression(const ast::Expression& expression)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::IdentifierExpression: {
        // A named Integer constant is a constant expression (CANON-19): overflow
        // or a zero divisor involving it is a compile-time error.
        const Symbol* symbol = result_.symbolOf(static_cast<const ast::IdentifierExpression&>(expression));
        if (symbol == nullptr || symbol->kind != SymbolKind::Constant) {
            return;
        }
        const ConstantValue* value = result_.constantValueOf(*symbol);
        if (value != nullptr && value->kind == ConstantValue::Kind::Integer) {
            constants_[&expression] = value->integer;
        }
        return;
    }
    case ast::AstNodeKind::LiteralExpression: {
        const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
        if (literal.literalKind() != ast::LiteralKind::Integer) {
            return;
        }
        std::uint64_t magnitude = 0;
        if (parseIntegerLiteralMagnitude(literal.value(), magnitude) && magnitude < kInt64MagnitudeOfMin) {
            constants_[&expression] = static_cast<std::int64_t>(magnitude);
        }
        return;
    }
    case ast::AstNodeKind::UnaryExpression: {
        const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
        std::int64_t operand = 0;
        if (!constantIntegerValue(unary.operand(), operand)) {
            return;
        }
        switch (unary.op()) {
        case ast::UnaryOperator::Plus:
            constants_[&expression] = operand;
            return;
        case ast::UnaryOperator::Minus:
            if (operand == kInt64Min) {
                throwConstantOverflow("negation");
            }
            constants_[&expression] = -operand;
            return;
        case ast::UnaryOperator::BitNot:
            constants_[&expression] = ~operand;
            return;
        default:
            return;
        }
    }
    case ast::AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        std::int64_t a = 0;
        std::int64_t b = 0;
        // CANON-8: a CONSTANT divisor of zero is a compile-time error even when the
        // dividend is only known at run time (`A div 0`). The same holds for a
        // constant shift count outside 0..63 and a constant negative exponent:
        // such an operation can never succeed.
        if (constantIntegerValue(binary.right(), b)) {
            rejectInvalidConstantRightOperand(binary.op(), b);
        }
        if (!constantIntegerValue(binary.left(), a) || !constantIntegerValue(binary.right(), b)) {
            return;
        }
        std::int64_t out = 0;
        switch (binary.op()) {
        case ast::BinaryOperator::Add:
            if (!checkedAdd(a, b, out)) throwConstantOverflow("addition");
            constants_[&expression] = out;
            return;
        case ast::BinaryOperator::Subtract:
            if (!checkedSub(a, b, out)) throwConstantOverflow("subtraction");
            constants_[&expression] = out;
            return;
        case ast::BinaryOperator::Multiply:
            if (!checkedMul(a, b, out)) throwConstantOverflow("multiplication");
            constants_[&expression] = out;
            return;
        case ast::BinaryOperator::IntegerDivide:
            if (b == 0) {
                throw SemanticError("constant division by zero in 'div'");
            }
            if (a == kInt64Min && b == -1) {
                throwConstantOverflow("division");
            }
            constants_[&expression] = a / b;
            return;
        case ast::BinaryOperator::Modulo:
            if (b == 0) {
                throw SemanticError("constant division by zero in 'mod'");
            }
            constants_[&expression] = (b == -1) ? 0 : a % b;
            return;
        case ast::BinaryOperator::ShiftLeft:
        case ast::BinaryOperator::ShiftRight:
            if (b < 0 || b > 63) {
                throw SemanticError("constant shift count out of range: must be between 0 and 63");
            }
            if (binary.op() == ast::BinaryOperator::ShiftLeft) {
                constants_[&expression] =
                    static_cast<std::int64_t>(static_cast<std::uint64_t>(a) << static_cast<unsigned>(b));
            } else {
                constants_[&expression] = a >> static_cast<unsigned>(b);
            }
            return;
        case ast::BinaryOperator::BitAnd:
            constants_[&expression] = a & b;
            return;
        case ast::BinaryOperator::BitOr:
            constants_[&expression] = a | b;
            return;
        case ast::BinaryOperator::BitXor:
            constants_[&expression] = a ^ b;
            return;
        case ast::BinaryOperator::Power: {
            if (b < 0) {
                throw SemanticError("constant exponent must not be negative");
            }
            std::int64_t result = 1;
            std::int64_t base = a;
            std::int64_t exponent = b;
            while (exponent > 0) {
                if ((exponent & 1) != 0 && !checkedMul(result, base, result)) {
                    throwConstantOverflow("exponentiation");
                }
                exponent >>= 1;
                if (exponent > 0 && !checkedMul(base, base, base)) {
                    throwConstantOverflow("exponentiation");
                }
            }
            constants_[&expression] = result;
            return;
        }
        default:
            return;
        }
    }
    default:
        return;
    }
}

void SemanticAnalyzer::requireStatementExpression(const ast::Expression& expression)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::IdentifierExpression: {
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
        if (equalsIgnoreCase(identifier.name(), "Get") || equalsIgnoreCase(identifier.name(), "GetLn") ||
            resolveFunctionSignature(identifier.name()) != nullptr) {
            return;
        }
        throw SemanticError("'" + identifier.name() +
                            "' is not a subroutine and cannot be used as a statement");
    }
    case ast::AstNodeKind::LiteralExpression:
    case ast::AstNodeKind::UnaryExpression:
        throw SemanticError("expression is not a statement: only assignments and calls may be used as statements");
    case ast::AstNodeKind::BinaryExpression:
        if (static_cast<const ast::BinaryExpression&>(expression).op() != ast::BinaryOperator::Assign) {
            throw SemanticError("expression is not a statement: only assignments and calls may be used as statements");
        }
        return;
    default:
        return;
    }
}

std::string SemanticAnalyzer::analyzeExpression(const ast::Expression& expression)
{
    std::string typeName = inferExpressionType(expression);
    result_.setExpressionType(expression, resolvedType(typeName));
    foldConstantExpression(expression);
    return typeName;
}

std::string SemanticAnalyzer::inferExpressionType(const ast::Expression& expression)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression: {
        const auto literalKind = static_cast<const ast::LiteralExpression&>(expression).literalKind();
        switch (literalKind) {
        case ast::LiteralKind::Integer: {
            std::uint64_t magnitude = 0;
            const auto& integerLiteral = static_cast<const ast::LiteralExpression&>(expression);
            if (!parseIntegerLiteralMagnitude(integerLiteral.value(), magnitude) ||
                magnitude >= kInt64MagnitudeOfMin) {
                throw SemanticError("integer literal out of range for Int64: " +
                                    std::string(integerLiteral.value()));
            }
            return "Int64";
        }
        case ast::LiteralKind::Float:
            return "Float64";
        case ast::LiteralKind::String:
            return "String";
        case ast::LiteralKind::Char:
            return "Char";
        case ast::LiteralKind::Boolean:
            return "Bool";
        }
        break;
    }
    case ast::AstNodeKind::IdentifierExpression: {
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
        if (equalsIgnoreCase(identifier.name(), "Get") || equalsIgnoreCase(identifier.name(), "GetLn")) {
            const Symbol& symbol = resolveOrThrow(identifier.name());
            result_.bind(identifier, symbol);
            return "Void";
        }
        if (const FunctionSignature* signature = resolveFunctionSignature(identifier.name())) {
            if (signature->parameters.empty()) {
                if (const Symbol* symbol = symbols_.currentScope().resolve(identifier.name())) {
                    result_.bind(identifier, *symbol);
                }
                return signature->returnType;
            }
            throw SemanticError("function " + identifier.name() + " requires arguments; use parentheses only when passing arguments");
        }
        const Symbol& symbol = resolveOrThrow(identifier.name());
        result_.bind(identifier, symbol);
        return symbol.typeName;
    }
    case ast::AstNodeKind::BinaryExpression:
        return analyzeBinaryExpression(static_cast<const ast::BinaryExpression&>(expression));
    case ast::AstNodeKind::UnaryExpression:
        return analyzeUnaryExpression(static_cast<const ast::UnaryExpression&>(expression));
    case ast::AstNodeKind::CallExpression:
        return analyzeCallExpression(static_cast<const ast::CallExpression&>(expression));
    default:
        break;
    }

    return {};
}

namespace {

const ast::BinaryExpression* namedArgument(const ast::Expression& argument)
{
    if (argument.kind() != ast::AstNodeKind::BinaryExpression) {
        return nullptr;
    }
    const auto& binary = static_cast<const ast::BinaryExpression&>(argument);
    if (binary.op() != ast::BinaryOperator::Assign ||
        binary.left().kind() != ast::AstNodeKind::IdentifierExpression) {
        return nullptr;
    }
    return &binary;
}

} // namespace

std::string SemanticAnalyzer::analyzeCallExpression(const ast::CallExpression& call)
{
    // `Name := Value` inside an argument list is named struct construction
    // (CANON-9). It is not an assignment, and only a struct constructor takes it.
    const bool calleeIsType =
        call.callee().kind() == ast::AstNodeKind::IdentifierExpression &&
        types_.resolve(static_cast<const ast::IdentifierExpression&>(call.callee()).name()) != nullptr;
    if (!calleeIsType) {
        for (const auto& argument : call.arguments()) {
            if (const ast::BinaryExpression* named = namedArgument(*argument)) {
                throw SemanticError(
                    "named argument '" +
                    static_cast<const ast::IdentifierExpression&>(named->left()).name() +
                    " := ...' is only allowed in struct construction (CANON-9); "
                    "':=' is a statement, not an expression");
            }
        }
    }

    if (call.callee().kind() == ast::AstNodeKind::CallExpression &&
        isMemberCall(static_cast<const ast::CallExpression&>(call.callee()))) {
        return analyzeMethodCallExpression(call);
    }

    if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
        if (equalsIgnoreCase(callee.name(), "__member")) {
            return analyzeMemberExpression(call);
        }
        if (const TypeSymbol* type = types_.resolve(callee.name())) {
            const Symbol& symbol = resolveOrThrow(callee.name());
            result_.bind(callee, symbol);
            result_.bind(call, symbol);
            result_.setExpressionType(callee, resolvedType(canonicalTypeName(type->name)));
            const StructType* structType = resolveStruct(type->name);
            if (structType == nullptr) {
                // A conversion such as `Float32(0.0)` or `Currency(19.99)`.
                for (const auto& argument : call.arguments()) {
                    if (const ast::BinaryExpression* named = namedArgument(*argument)) {
                        throw SemanticError(
                            "named argument '" +
                            static_cast<const ast::IdentifierExpression&>(named->left()).name() +
                            " := ...' is only allowed in struct construction (CANON-9)");
                    }
                    analyzeExpression(*argument);
                }
                return canonicalTypeName(type->name);
            }

            // Named struct construction (CANON-9): every argument is
            // `Field := Value`. The left side names a field, never a variable;
            // nothing is declared or assigned.
            std::vector<std::string> namedFields;
            for (const auto& argument : call.arguments()) {
                const ast::BinaryExpression* named = namedArgument(*argument);
                if (named == nullptr) {
                    throw SemanticError(
                        "positional construction is not canonical (CANON-9): write " +
                        type->name + "(Field := Value, ...)");
                }
                const std::string& fieldName =
                    static_cast<const ast::IdentifierExpression&>(named->left()).name();
                const StructField* field = resolveStructField(type->name, fieldName);
                if (field == nullptr) {
                    throw SemanticError("unknown field in construction: " +
                                        type->name + "." + fieldName);
                }
                for (const std::string& seen : namedFields) {
                    if (equalsIgnoreCase(seen, fieldName)) {
                        throw SemanticError("duplicate field in construction: " +
                                            type->name + "." + fieldName);
                    }
                }
                namedFields.push_back(fieldName);
                const std::string valueType = analyzeExpression(named->right());
                if (!canAssign(field->typeName, valueType)) {
                    throw SemanticError("field " + type->name + "." + fieldName +
                                        " expects " + field->typeName);
                }
                result_.setExpressionType(*named, resolvedType(field->typeName));
            }
            for (const StructField& field : structType->fields) {
                bool named = false;
                for (const std::string& seen : namedFields) {
                    named = named || equalsIgnoreCase(seen, field.name);
                }
                if (!named && !field.hasDefault && resolveStruct(field.typeName) == nullptr) {
                    throw SemanticError(
                        "construction omits scalar field without default (CANON-9): " +
                        type->name + "." + field.name);
                }
            }
            return canonicalTypeName(type->name);
        }

        if (isPreludeCall(callee.name())) {
            const Symbol& symbol = resolveOrThrow(callee.name());
            result_.bind(callee, symbol);
            result_.bind(call, symbol);
            if (equalsIgnoreCase(callee.name(), "Get") || equalsIgnoreCase(callee.name(), "GetLn")) {
                return analyzeInputCall(callee.name(), call.arguments());
            }
            std::vector<std::string> argumentTypes;
            argumentTypes.reserve(call.arguments().size());
            for (const auto& argument : call.arguments()) {
                argumentTypes.push_back(analyzeExpression(*argument));
            }
            return analyzePreludeCall(callee.name(), argumentTypes);
        }

        if (const FunctionSignature* signature = resolveFunctionSignature(callee.name())) {
            const Symbol& symbol = resolveOrThrow(callee.name());
            result_.bind(callee, symbol);
            result_.bind(call, symbol);
            return analyzeUserFunctionCall(*signature, call.arguments());
        }
    }

    analyzeExpression(call.callee());
    if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
        const Symbol& symbol = resolveOrThrow(callee.name());
        if (symbol.kind == SymbolKind::Variable || symbol.kind == SymbolKind::LoopIterator ||
            symbol.kind == SymbolKind::Constant || symbol.kind == SymbolKind::State) {
            throw SemanticError("'" + callee.name() + "' is a value, not a function; it cannot be called");
        }
        result_.bind(call, symbol);
    }
    for (const auto& argument : call.arguments()) {
        analyzeExpression(*argument);
    }
    return {};
}

std::string SemanticAnalyzer::analyzeMemberExpression(const ast::CallExpression& call)
{
    if (call.arguments().size() != 2 ||
        call.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
        throw SemanticError("member access requires object and field name");
    }

    const std::string baseType = analyzeExpression(*call.arguments()[0]);
    const auto& fieldIdentifier =
        static_cast<const ast::IdentifierExpression&>(*call.arguments()[1]);
    const StructField* field = resolveStructField(baseType, fieldIdentifier.name());
    if (field != nullptr) {
        result_.setExpressionType(fieldIdentifier, resolvedType(field->typeName));
        return field->typeName;
    }

    const std::string qualifiedName = baseType + "." + fieldIdentifier.name();
    const FunctionSignature* signature = resolveFunctionSignature(qualifiedName);
    if (signature != nullptr) {
        if (signature->parameters.empty()) {
            throw SemanticError("method " + signature->name + " must declare an explicit receiver parameter");
        }
        if (signature->parameters.size() != 1) {
            throw SemanticError("method " + signature->name + " requires arguments; use parentheses only when passing arguments");
        }
        if (!canAssign(signature->parameters.front().typeName, baseType)) {
            throw SemanticError(
                "method " + signature->name + " receiver expects " +
                signature->parameters.front().typeName + ", got " + baseType);
        }
        const Symbol& symbol = resolveOrThrow(signature->name);
        result_.bind(fieldIdentifier, symbol);
        result_.bind(call, symbol);
        return signature->returnType;
    }

    throw SemanticError("unknown field or zero-argument method " + fieldIdentifier.name() + " in " + baseType);
}

std::string SemanticAnalyzer::analyzeMethodCallExpression(const ast::CallExpression& call)
{
    const auto& member = static_cast<const ast::CallExpression&>(call.callee());
    if (member.arguments().size() != 2 ||
        member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
        throw SemanticError("method call requires object and method name");
    }

    const std::string receiverType = analyzeExpression(*member.arguments()[0]);
    const auto& methodIdentifier =
        static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
    const std::string qualifiedName = receiverType + "." + methodIdentifier.name();
    const FunctionSignature* signature = resolveFunctionSignature(qualifiedName);
    if (signature == nullptr) {
        throw SemanticError("unknown method " + methodIdentifier.name() + " for " + receiverType);
    }

    if (signature->parameters.empty()) {
        throw SemanticError("method " + signature->name + " must declare an explicit receiver parameter");
    }
    if (!canAssign(signature->parameters.front().typeName, receiverType)) {
        throw SemanticError(
            "method " + signature->name + " receiver expects " +
            signature->parameters.front().typeName + ", got " + receiverType);
    }
    if (call.arguments().size() + 1 != signature->parameters.size()) {
        throw SemanticError(
            "method " + signature->name + " expects " +
            std::to_string(signature->parameters.size() - 1) + " explicit arguments, got " +
            std::to_string(call.arguments().size()));
    }

    const Symbol& symbol = resolveOrThrow(signature->name);
    result_.bind(methodIdentifier, symbol);
    result_.bind(call, symbol);

    for (std::size_t index = 0; index < call.arguments().size(); ++index) {
        const std::string argumentType = analyzeExpression(*call.arguments()[index]);
        const FunctionParameter& parameter = signature->parameters[index + 1];
        if (!canAssign(parameter.typeName, argumentType)) {
            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + signature->name +
                " expects " + parameter.typeName + ", got " + argumentType);
        }
    }

    return signature->returnType;
}

std::string SemanticAnalyzer::analyzeUserFunctionCall(
    const FunctionSignature& signature,
    const std::vector<ast::ExpressionPtr>& arguments)
{
    if (arguments.size() != signature.parameters.size()) {
        throw SemanticError(
            "function " + signature.name + " expects " +
            std::to_string(signature.parameters.size()) + " arguments, got " +
            std::to_string(arguments.size()));
    }

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string argumentType = analyzeExpression(*arguments[index]);
        const FunctionParameter& parameter = signature.parameters[index];
        if (!canAssign(parameter.typeName, argumentType)) {
            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + signature.name +
                " expects " + parameter.typeName + ", got " + argumentType);
        }
    }

    return signature.returnType;
}

std::string SemanticAnalyzer::analyzeInputCall(
    std::string_view name,
    const std::vector<ast::ExpressionPtr>& arguments)
{
    if (arguments.empty()) {
        return "Void";
    }

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const ast::Expression& argument = *arguments[index];
        if (argument.kind() != ast::AstNodeKind::IdentifierExpression) {
            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + std::string(name) +
                " must be an assignable variable");
        }

        const auto& identifier = static_cast<const ast::IdentifierExpression&>(argument);
        const Symbol& symbol = resolveOrThrow(identifier.name());
        result_.bind(identifier, symbol);
        result_.setExpressionType(identifier, resolvedType(symbol.typeName));

        if (symbol.kind == SymbolKind::LoopIterator) {
            throw SemanticError(std::string(name) + " cannot assign to read-only loop iterator: " + identifier.name());
        }
        if ((symbol.kind != SymbolKind::Variable && symbol.kind != SymbolKind::State) ||
            !symbol.isMutable) {
            throw SemanticError(std::string(name) + " argument must be an assignable variable: " + identifier.name());
        }
        if (!isIntegerType(symbol.typeName)) {
            throw SemanticError(
                std::string(name) + " currently supports only Integer/Int64 input, got " +
                symbol.typeName + " for " + identifier.name());
        }
    }

    return "Void";
}

std::string SemanticAnalyzer::analyzePreludeCall(
    std::string_view name,
    const std::vector<std::string>& argumentTypes) const
{
    if (equalsIgnoreCase(name, "Put") || equalsIgnoreCase(name, "PutLn")) {
        if (argumentTypes.empty()) {
            throw SemanticError(std::string(name) + " expects at least one argument");
        }

        for (std::size_t index = 0; index < argumentTypes.size(); ++index) {
            const std::string& typeName = argumentTypes[index];
            if (typeName == "String" || typeName == "Bool" || isIntegerType(typeName) || typeName == "Float64") {
                continue;
            }

            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + std::string(name) +
                " cannot be printed yet: " + typeName);
        }

        return "Void";
    }

    for (const PreludeSignature& signature : preludeSignatures()) {
        if (!equalsIgnoreCase(signature.name, name) ||
            signature.parameterTypes.size() != argumentTypes.size()) {
            continue;
        }

        bool matches = true;
        for (std::size_t index = 0; index < argumentTypes.size(); ++index) {
            const std::string_view expected = signature.parameterTypes[index];
            if (expected != "*" && argumentTypes[index] != expected) {
                matches = false;
                break;
            }
        }

        if (matches) {
            return std::string(signature.returnType);
        }
    }

    if (equalsIgnoreCase(name, "Length")) {
        throw SemanticError("Length expects String");
    }
    if (equalsIgnoreCase(name, "Ord")) {
        throw SemanticError("Ord expects Char");
    }

    throw SemanticError("no matching prelude signature for call: " + std::string(name));
}

void SemanticAnalyzer::requireBoolCondition(const ast::Expression& expression)
{
    const std::string typeName = analyzeExpression(expression);
    if (!typeName.empty() && typeName != "Bool") {
        throw SemanticError("condition must be Bool");
    }
}

std::string SemanticAnalyzer::analyzeBinaryExpression(const ast::BinaryExpression& expression)
{
    const ast::BinaryOperator op = expression.op();

    if (op == ast::BinaryOperator::Assign) {
        std::string targetType;

        if (expression.left().kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression.left());

            // CANON-5 / A6 + A7: first appearance of a name is a DECLARATION.
            // `Name := Expr` where Name is not yet visible in any scope is an
            // inline declaration with the type INFERRED from the initializer
            // (Ada/SPARK safe inference). A subsequent `Name := Expr` is a plain
            // assignment, handled by the existing path below.
            if (symbols_.currentScope().resolve(identifier.name()) == nullptr) {
                const std::string inferredType = analyzeExpression(expression.right());
                if (inferredType.empty()) {
                    throw SemanticError(
                        "cannot infer type for declaration: " + identifier.name());
                }
                declareOrThrow(
                    identifier.name(),
                    SymbolKind::Variable,
                    inferredType,
                    true);
                const Symbol& declared = resolveOrThrow(identifier.name());
                result_.bind(identifier, declared);
                result_.setExpressionType(identifier, resolvedType(declared.typeName));
                return inferredType;
            }

            const Symbol& target = resolveOrThrow(identifier.name());
            result_.bind(identifier, target);
            result_.setExpressionType(identifier, resolvedType(target.typeName));
            if (target.kind == SymbolKind::LoopIterator) {
                throw SemanticError("cannot assign to read-only loop iterator: " + identifier.name());
            }
            if ((target.kind != SymbolKind::Variable && target.kind != SymbolKind::State) ||
                !target.isMutable) {
                throw SemanticError("assignment target is not mutable: " + identifier.name());
            }
            targetType = target.typeName;
        } else if (expression.left().kind() == ast::AstNodeKind::CallExpression &&
                   isMemberCall(static_cast<const ast::CallExpression&>(expression.left()))) {
            targetType = analyzeMemberExpression(
                static_cast<const ast::CallExpression&>(expression.left()));
        } else {
            throw SemanticError("assignment target must be an identifier or field");
        }

        const std::string rightType = analyzeExpression(expression.right());
        if (!canAssign(targetType, rightType)) {
            throw SemanticError("cannot assign " + rightType + " to " + targetType);
        }
        return targetType;
    }

    const std::string leftType = analyzeExpression(expression.left());
    const std::string rightType = analyzeExpression(expression.right());

    const auto requireMatchingTypes = [&] {
        if (!typesMatch(leftType, rightType)) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires matching operand types");
        }
    };
    const auto requireNumericOperands = [&] {
        if ((!leftType.empty() && !isNumericType(leftType)) ||
            (!rightType.empty() && !isNumericType(rightType))) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires numeric operands");
        }
        requireMatchingTypes();
    };
    const auto requireIntegerOperands = [&] {
        if ((!leftType.empty() && !isIntegerType(leftType)) ||
            (!rightType.empty() && !isIntegerType(rightType))) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires integer operands");
        }
        requireMatchingTypes();
    };

    switch (op) {
    case ast::BinaryOperator::Add:
    case ast::BinaryOperator::Subtract:
    case ast::BinaryOperator::Multiply:
    case ast::BinaryOperator::Power:
        requireNumericOperands();
        return leftType;
    case ast::BinaryOperator::Divide:
        if (isIntegerType(leftType) && isIntegerType(rightType)) {
            throw SemanticError("operator '/' is not supported for Integer operands; use 'div' for integer division");
        }
        requireNumericOperands();
        return leftType;
    case ast::BinaryOperator::IntegerDivide:
    case ast::BinaryOperator::Modulo:
    case ast::BinaryOperator::ShiftLeft:
    case ast::BinaryOperator::ShiftRight:
    case ast::BinaryOperator::BitAnd:
    case ast::BinaryOperator::BitXor:
    case ast::BinaryOperator::BitOr:
        requireIntegerOperands();
        return leftType;
    case ast::BinaryOperator::Equal:
    case ast::BinaryOperator::NotEqual:
    case ast::BinaryOperator::Less:
    case ast::BinaryOperator::Greater:
    case ast::BinaryOperator::LessEqual:
    case ast::BinaryOperator::GreaterEqual:
        requireMatchingTypes();
        return "Bool";
    case ast::BinaryOperator::And:
    case ast::BinaryOperator::Xor:
    case ast::BinaryOperator::Or:
        requireMatchingTypes();
        if ((!leftType.empty() && leftType != "Bool") ||
            (!rightType.empty() && rightType != "Bool")) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires Bool operands");
        }
        return "Bool";
    case ast::BinaryOperator::Range:
        requireIntegerOperands();
        return leftType;
    case ast::BinaryOperator::In:
        return "Bool";
    case ast::BinaryOperator::Assign:
        break;
    }

    return {};
}

std::string SemanticAnalyzer::analyzeUnaryExpression(const ast::UnaryExpression& expression)
{
    // `-9223372036854775808` is the one literal whose magnitude only fits when
    // it is directly negated.
    if (expression.op() == ast::UnaryOperator::Minus &&
        expression.operand().kind() == ast::AstNodeKind::LiteralExpression) {
        const auto& literal = static_cast<const ast::LiteralExpression&>(expression.operand());
        std::uint64_t magnitude = 0;
        if (literal.literalKind() == ast::LiteralKind::Integer &&
            parseIntegerLiteralMagnitude(literal.value(), magnitude) &&
            magnitude == kInt64MagnitudeOfMin) {
            result_.setExpressionType(literal, resolvedType("Int64"));
            constants_[&expression] = kInt64Min;
            return "Int64";
        }
    }
    const std::string operandType = analyzeExpression(expression.operand());

    switch (expression.op()) {
    case ast::UnaryOperator::Plus:
    case ast::UnaryOperator::Minus:
        if (!operandType.empty() && !isNumericType(operandType)) {
            throw SemanticError("unary arithmetic operator requires a numeric operand");
        }
        return operandType;
    case ast::UnaryOperator::Not:
        if (!operandType.empty() && operandType != "Bool") {
            throw SemanticError("operator 'not' requires a Bool operand");
        }
        return "Bool";
    case ast::UnaryOperator::BitNot:
        if (!operandType.empty() && !isIntegerType(operandType)) {
            throw SemanticError("operator 'bitnot' requires an integer operand");
        }
        return operandType;
    }

    return {};
}

bool SemanticAnalyzer::looksLikeIdentifier(std::string_view text)
{
    if (text.empty()) {
        return false;
    }
    const auto isIdentifierStart = [](char ch) {
        return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
    };
    const auto isIdentifierPart = [](char ch) {
        return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
    };

    if (!isIdentifierStart(text.front())) {
        return false;
    }
    for (char ch : text.substr(1)) {
        if (!isIdentifierPart(ch)) {
            return false;
        }
    }
    return true;
}

const StructType* SemanticAnalyzer::resolveStruct(std::string_view name) const
{
    const auto iterator = structs_.find(normalizeName(name));
    return iterator != structs_.end() ? &iterator->second : nullptr;
}

const StructField* SemanticAnalyzer::resolveStructField(std::string_view structName, std::string_view fieldName) const
{
    const StructType* structType = resolveStruct(structName);
    if (structType == nullptr) {
        return nullptr;
    }
    for (const StructField& field : structType->fields) {
        if (equalsIgnoreCase(field.name, fieldName)) {
            return &field;
        }
    }
    return nullptr;
}

bool SemanticAnalyzer::isMemberCall(const ast::CallExpression& expression)
{
    if (expression.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
        return false;
    }
    const auto& callee = static_cast<const ast::IdentifierExpression&>(expression.callee());
    return equalsIgnoreCase(callee.name(), "__member");
}

bool SemanticAnalyzer::isInternalSyntheticName(std::string_view name)
{
    return name == "__index" || name == "__member";
}

bool SemanticAnalyzer::isNumericType(std::string_view typeName)
{
    return isIntegerType(typeName) ||
           typeName == "Float32" ||
           typeName == "Float64";
}

bool SemanticAnalyzer::isIntegerType(std::string_view typeName)
{
    return typeName == "Integer" ||
           typeName == "UInteger" ||
           typeName == "Int8" ||
           typeName == "Int16" ||
           typeName == "Int32" ||
           typeName == "Int64" ||
           typeName == "UInt8" ||
           typeName == "UInt16" ||
           typeName == "UInt32" ||
           typeName == "UInt64" ||
           typeName == "Natural";
}

bool SemanticAnalyzer::isPreludeCall(std::string_view name)
{
    for (const PreludeSignature& signature : preludeSignatures()) {
        if (equalsIgnoreCase(signature.name, name)) {
            return true;
        }
    }
    return false;
}

bool SemanticAnalyzer::isExceptionType(std::string_view name)
{
    return exceptions::isExceptionType(name);
}


std::string SemanticAnalyzer::normalizeName(std::string_view name)
{
    std::string normalized;
    normalized.reserve(name.size());
    for (const char ch : name) {
        normalized.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

bool SemanticAnalyzer::canAssign(std::string_view targetType, std::string_view valueType)
{
    return targetType.empty() || (!valueType.empty() && targetType == valueType);
}

bool SemanticAnalyzer::typesMatch(std::string_view left, std::string_view right)
{
    return left.empty() || right.empty() || left == right;
}

ResolvedType SemanticAnalyzer::resolvedType(std::string typeName) const
{
    return ResolvedType{typeName, types_.resolve(typeName)};
}

} // namespace inox::compiler::semantic
