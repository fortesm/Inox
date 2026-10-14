// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "TypeLayout.h"

#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "../support/ArrayType.h"

#include <cctype>

namespace inox::compiler::codegen::detail {

std::string llvmStructName(std::string_view inoxName)
{
    return "%" + normalize(inoxName);
}

// The range of a fixed-width integer type narrower than 64 bits (Byte is
// UInt8), or nullopt for any other type.
std::optional<IntegerRange> narrowIntegerRange(std::string_view type)
{
    struct Entry {
        std::string_view name;
        IntegerRange range;
    };
    static constexpr Entry entries[] = {
        {"Int8", {-128, 127}},
        {"Int16", {-32768, 32767}},
        {"Int32", {-2147483648LL, 2147483647LL}},
        {"UInt8", {0, 255}},
        {"Byte", {0, 255}},
        {"UInt16", {0, 65535}},
        {"UInt32", {0, 4294967295LL}},
    };
    for (const Entry& entry : entries) {
        if (equalsIgnoreCase(type, entry.name)) {
            return entry.range;
        }
    }
    return std::nullopt;
}

std::string llvmTypeForScalar(std::string_view inoxType)
{
    // CANON-8: Natural is the non-negative subtype of Integer and shares its
    // i64 representation; the range is checked where a value becomes Natural.
    if (equalsIgnoreCase(inoxType, "Integer") || equalsIgnoreCase(inoxType, "Int64") ||
        equalsIgnoreCase(inoxType, "Natural")) {
        return "i64";
    }
    // CANON-8 fixed widths narrower than 64 bits are computed as i64 holding
    // the mathematical value and range-checked where a value takes the type.
    if (narrowIntegerRange(inoxType)) {
        return "i64";
    }
    if (equalsIgnoreCase(inoxType, "Bool")) {
        return "i1";
    }
    if (equalsIgnoreCase(inoxType, "Float") || equalsIgnoreCase(inoxType, "Float64")) {
        return "double";
    }
    if (equalsIgnoreCase(inoxType, "Float32")) {
        return "float";
    }
    // CANON-14: a String is a pointer to an immutable, reference-counted
    // block of the runtime (StringRuntime.cpp).
    if (equalsIgnoreCase(inoxType, "String")) {
        return "ptr";
    }
    // Std.Files (file-io): a File is a pointer to a handle of the runtime
    // (FileRuntime.cpp), owned like a String.
    if (equalsIgnoreCase(inoxType, "File")) {
        return "ptr";
    }
    return {};
}

bool isFloatLlvmType(std::string_view llvmType)
{
    return llvmType == "double" || llvmType == "float";
}

std::string llvmTypeForInoxType(std::string_view inoxType, const StructDefinitions& structs)
{
    if (const std::string scalar = llvmTypeForScalar(inoxType); !scalar.empty()) {
        return scalar;
    }
    if (const auto layout = arrayLayout(inoxType)) {
        return layout->llvmType;
    }
    if (support::isSliceTypeName(inoxType)) {
        return std::string(kSliceLlvmType);
    }
    const auto structType = structs.find(normalize(inoxType));
    if (structType != structs.end()) {
        return structType->second.llvmName;
    }
    return {};
}


std::string scalarStorageType(std::string_view inoxType)
{
    if (const auto range = narrowIntegerRange(inoxType)) {
        if (range->high <= 255) {
            return "i8";
        }
        if (range->high <= 65535) {
            return "i16";
        }
        return "i32";
    }
    return llvmTypeForScalar(inoxType);
}

std::optional<ArrayLayout> arrayLayout(std::string_view canonicalType)
{
    const auto parts = support::splitArrayTypeName(canonicalType);
    if (!parts) {
        return std::nullopt;
    }
    ArrayLayout layout;
    layout.low = std::stoll(parts->low);
    layout.high = std::stoll(parts->high);
    layout.elementType = parts->element;
    if (const auto inner = arrayLayout(parts->element)) {
        layout.elementStorage = inner->llvmType;
    } else {
        layout.elementStorage = scalarStorageType(parts->element);
    }
    if (layout.elementStorage.empty() && !parts->element.empty() &&
        std::isalpha(static_cast<unsigned char>(parts->element.front())) != 0) {
        // The semantic analyzer accepts only scalar or struct elements.
        layout.elementStorage = llvmStructName(parts->element);
    }
    if (layout.elementStorage.empty()) {
        return std::nullopt;
    }
    layout.llvmType = "[" + std::to_string(layout.high - layout.low + 1) + " x " +
                      layout.elementStorage + "]";
    return layout;
}

std::string elementStorageType(std::string_view elementType)
{
    if (const auto inner = arrayLayout(elementType)) {
        return inner->llvmType;
    }
    return scalarStorageType(elementType);
}

const StructDefinition* findStruct(const StructDefinitions& structs, std::string_view inoxName)
{
    const auto iterator = structs.find(normalize(inoxName));
    return iterator != structs.end() ? &iterator->second : nullptr;
}

const StructFieldInfo* findStructField(const StructDefinition& structType, std::string_view fieldName)
{
    for (const StructFieldInfo& field : structType.fields) {
        if (equalsIgnoreCase(field.inoxName, fieldName)) {
            return &field;
        }
    }
    return nullptr;
}

void collectStructDefinitions(const ast::SectionDeclaration& section, StructDefinitions& structs,
                              const semantic::SemanticResult& semantics)
{
    for (const ast::TypeDeclaration& declaration : section.types()) {
        if (declaration.kind != ast::TypeDeclarationKind::Struct) {
            continue;
        }
        StructDefinition definition;
        definition.inoxName = declaration.name;
        definition.llvmName = llvmStructName(declaration.name);
        for (const ast::StructFieldDeclaration& fieldDeclaration : declaration.fields) {
            StructFieldInfo field;
            field.inoxName = fieldDeclaration.name;
            field.inoxType = fieldDeclaration.typeName;
            field.index = definition.fields.size();
            field.llvmType = llvmTypeForScalar(fieldDeclaration.typeName);
            if (support::isArrayTypeName(fieldDeclaration.typeName)) {
                // CANON-13: an array field is embedded by value, [N x T].
                field.inoxType = semantics.canonicalTypeName(fieldDeclaration.typeName);
                const auto layout = arrayLayout(field.inoxType);
                if (!layout) {
                    throw CodegenError("array field without a layout: " + fieldDeclaration.name);
                }
                field.llvmType = layout->llvmType;
            } else if (field.llvmType.empty()) {
                // The semantic analyzer accepts only scalar, array and struct
                // field types; a struct may be declared later in the module,
                // and LLVM allows forward references to named struct types.
                field.isStruct = true;
                field.llvmType = llvmStructName(fieldDeclaration.typeName);
            }
            field.hasDefault = fieldDeclaration.hasDefault;
            field.defaultValue = fieldDeclaration.defaultValue;
            field.isPart = fieldDeclaration.isPart;
            definition.fields.push_back(std::move(field));
        }
        structs.emplace(normalize(definition.inoxName), std::move(definition));
    }
}

FunctionSignature parseFunctionSignature(const ast::FunctionDeclaration& function,
                                         const StructDefinitions& structs,
                                         const semantic::SemanticResult& semantics)
{
    const auto& tokens = function.signatureTokens();

    const std::size_t dot = function.name().find('.');
    const bool isAssociatedMethod = dot != std::string::npos;
    const std::string receiverType = isAssociatedMethod ? function.name().substr(0, dot) : std::string{};

    std::vector<FunctionParameter> parameters;
    std::size_t index = 0;
    if (!tokens.empty() && tokens.front() == "(") {
        index = 1;
        if (index < tokens.size() && tokens[index] == ")") {
            throw CodegenError("empty parentheses are not allowed in declarations: " + function.name());
        }

        while (index < tokens.size() && tokens[index] != ")") {
            bool byReference = false;
            if (equalsIgnoreCase(tokens[index], "ref")) {
                byReference = true;
                ++index;
                if (index < tokens.size() && equalsIgnoreCase(tokens[index], "mut")) {
                    ++index;
                }
            }
            const std::string parameterName = tokens[index++];
            std::string parameterType;
            bool isReceiver = false;
            if (isAssociatedMethod && parameters.empty() && equalsIgnoreCase(parameterName, "Self")) {
                isReceiver = true;
                bool receiverMut = false;
                if (index < tokens.size() && equalsIgnoreCase(tokens[index], "mut")) {
                    ++index;
                    receiverMut = true;
                }
                parameterType = receiverType;
                // CANON-9: `Self mut` of a built-in type is the caller's
                // variable, passed like `ref mut`; `Self` is a copy. A
                // struct receiver is always passed by address.
                if (receiverMut && findStruct(structs, receiverType) == nullptr) {
                    byReference = true;
                }
            } else {
                if (index >= tokens.size() || tokens[index] == "," || tokens[index] == ")") {
                    throw CodegenUnsupported("unsupported function signature: " + function.name());
                }
                parameterType = tokens[index++];
            }
            if (support::isArrayTypeName(parameterType) || support::isSliceTypeName(parameterType)) {
                parameterType = semantics.canonicalTypeName(parameterType);
            }

            std::string llvmParameterType = llvmTypeForScalar(parameterType);
            if (const auto layout = arrayLayout(parameterType)) {
                // CANON-13: an array is a value; a by-value parameter is a copy.
                llvmParameterType = layout->llvmType;
            } else if (support::isSliceTypeName(parameterType)) {
                llvmParameterType = std::string(kSliceLlvmType);
            } else if (llvmParameterType.empty()) {
                const StructDefinition* structType = findStruct(structs, parameterType);
                if (structType == nullptr) {
                    throw CodegenUnsupported(
                        "LLVM emission currently supports only scalar and struct parameters");
                }
                llvmParameterType = isReceiver ? "ptr" : structType->llvmName;
            }

            std::string valueType;
            if (byReference) {
                valueType = llvmParameterType;
                llvmParameterType = "ptr";
            }
            parameters.push_back(FunctionParameter{
                parameterName,
                safeLlvmName(normalize(parameterName)),
                parameterType,
                llvmParameterType,
                byReference,
                valueType});
            if (index < tokens.size() && tokens[index] == ",") {
                ++index;
            } else if (index >= tokens.size() || tokens[index] != ")") {
                throw CodegenUnsupported("unsupported function signature: " + function.name());
            }
        }

        if (index >= tokens.size() || tokens[index] != ")") {
            throw CodegenUnsupported("unsupported function signature: " + function.name());
        }
        ++index;
    }

    std::string llvmReturnType;
    std::string returnInoxType;
    if (index + 1 == tokens.size()) {
        returnInoxType = support::isArrayTypeName(tokens[index]) ? semantics.canonicalTypeName(tokens[index])
                                                                  : tokens[index];
        if (const StructDefinition* structType = findStruct(structs, returnInoxType)) {
            returnInoxType = structType->inoxName;
        }
    }
    if (index == tokens.size()) {
        llvmReturnType = "void";
    } else if (index + 1 != tokens.size()) {
        throw CodegenUnsupported(
            "LLVM emission currently supports scalar, struct return types, or subroutines without return type");
    } else if (const std::string scalarReturnType = llvmTypeForScalar(tokens[index]); !scalarReturnType.empty()) {
        llvmReturnType = scalarReturnType;
    } else if (const auto layout = arrayLayout(semantics.canonicalTypeName(tokens[index]))) {
        llvmReturnType = layout->llvmType;
    } else if (const StructDefinition* structType = findStruct(structs, tokens[index])) {
        llvmReturnType = structType->llvmName;
    } else {
        throw CodegenUnsupported(
            "LLVM emission currently supports scalar, struct return types, or subroutines without return type");
    }

    return FunctionSignature{
        "inox_" + normalize(function.name()),
        std::move(llvmReturnType),
        std::move(parameters),
        std::move(returnInoxType)};
}

std::string stateGlobalSymbol(std::string_view name)
{
    return "@inox.state." + normalize(name);
}

} // namespace inox::compiler::codegen::detail
