// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "TypeLayout.h"

#include "LlvmIrEmitter.h"
#include "LlvmText.h"

namespace inox::compiler::codegen::detail {

std::string llvmStructName(TypeRef structType)
{
    return "%" + llvmGlobalName(normalize(structType->name()));
}

std::optional<IntegerRange> narrowIntegerRange(TypeRef type)
{
    if (type == nullptr || !type->isNarrowInteger()) {
        return std::nullopt;
    }
    return IntegerRange{type->minValue(), type->maxValue()};
}

std::string llvmSetType(TypeRef setType)
{
    const std::int64_t bytes = (setType->setSize() + 7) / 8;
    return "i" + std::to_string(bytes * 8);
}

std::string llvmValueType(TypeRef type)
{
    if (type == nullptr) {
        return {};
    }
    switch (type->kind()) {
    case types::TypeKind::Bool:
        return "i1";
    case types::TypeKind::Enum:  // CANON-13: the ordinal, an i64 like every integer
    case types::TypeKind::Integer:
        // CANON-8: every integer is computed as i64 holding the mathematical
        // value; Natural and the narrow widths are range-checked where a
        // value takes the type.
        return "i64";
    case types::TypeKind::Float:
        return type->bits() == 32 ? "float" : "double";
    case types::TypeKind::Set:
        return llvmSetType(type);
    case types::TypeKind::String:  // CANON-14: an immutable, counted block of the runtime
    case types::TypeKind::File:    // file-io: a handle of the runtime, owned like a String
    case types::TypeKind::Own:     // safe-pointers: an owner is its object's address
    case types::TypeKind::Option:
        return "ptr";
    case types::TypeKind::Link:
        return std::string(kLinkLlvmType);
    case types::TypeKind::Slice:
        return std::string(kSliceLlvmType);
    case types::TypeKind::Array:
        return arrayLayout(type)->llvmType;
    case types::TypeKind::Struct:
        return llvmStructName(type);
    case types::TypeKind::Void:
    case types::TypeKind::None:
    case types::TypeKind::Char:
    case types::TypeKind::Decimal:
    case types::TypeKind::Exception:
        break;
    }
    return {};
}

std::string llvmStorageType(TypeRef type)
{
    if (type != nullptr && type->isNarrowInteger()) {
        return "i" + std::to_string(type->bits());
    }
    return llvmValueType(type);
}

bool isFloatLlvmType(std::string_view llvmType)
{
    return llvmType == "double" || llvmType == "float";
}

std::optional<ArrayLayout> arrayLayout(TypeRef type)
{
    if (!isArrayType(type)) {
        return std::nullopt;
    }
    ArrayLayout layout;
    layout.low = type->low();
    layout.high = type->high();
    layout.element = type->element();
    layout.elementStorage = llvmStorageType(layout.element);
    if (layout.elementStorage.empty()) {
        throw CodegenError("an array element without an LLVM layout: " + type->name());
    }
    layout.llvmType = "[" + std::to_string(type->length()) + " x " + layout.elementStorage + "]";
    return layout;
}

FlatArray flattenArray(TypeRef arrayType)
{
    FlatArray flat{1, arrayType};
    while (isArrayType(flat.element)) {
        flat.count *= flat.element->length();
        flat.element = flat.element->element();
    }
    return flat;
}

std::optional<FieldLayout> fieldLayout(TypeRef structType, std::string_view fieldName)
{
    if (!isStructType(structType)) {
        return std::nullopt;
    }
    const types::StructField* field = structType->field(fieldName);
    if (field == nullptr) {
        return std::nullopt;
    }
    return fieldLayout(structType, structType->fieldIndex(*field));
}

FieldLayout fieldLayout(TypeRef structType, std::size_t index)
{
    const types::StructField& field = structType->fields().at(index);
    // CANON-13: an array field is embedded by value, [N x T]; a nested
    // struct too (LLVM allows forward references to named struct types).
    return FieldLayout{structType, &field, index, llvmValueType(field.type)};
}

FunctionSignature lowerSignature(const ast::FunctionDeclaration& function,
                                 const semantic::FunctionSignature& signature)
{
    const bool isMethod = ast::methodDot(function.name()) != std::string::npos;
    std::vector<FunctionParameter> parameters;
    for (std::size_t index = 0; index < signature.parameters.size(); ++index) {
        const semantic::FunctionParameter& parameter = signature.parameters[index];
        std::string llvmType = llvmValueType(parameter.type);
        if (llvmType.empty()) {
            throw CodegenUnsupported("LLVM emission currently supports only scalar and struct parameters");
        }
        const bool isReceiver = isMethod && index == 0 && equalsIgnoreCase(parameter.name, "Self");
        bool byReference = parameter.mode != semantic::PassMode::Value;
        if (isReceiver) {
            // CANON-9: a struct receiver is always passed by address; `Self
            // mut` of a built-in type is the caller's variable, passed like
            // `ref mut`; `Self` is a copy.
            byReference = parameter.mode == semantic::PassMode::RefMut && !parameter.type->isStruct();
            if (parameter.type->isStruct()) {
                llvmType = "ptr";
            }
        }
        std::string valueType;
        if (byReference) {
            valueType = llvmType;
            llvmType = "ptr";
        }
        parameters.push_back(FunctionParameter{parameter.name, safeLlvmName(normalize(parameter.name)),
                                               parameter.type, std::move(llvmType), byReference,
                                               std::move(valueType)});
    }

    std::string llvmReturnType = "void";
    if (signature.returnType != nullptr) {
        llvmReturnType = llvmValueType(signature.returnType);
        if (llvmReturnType.empty() || signature.returnType->isSlice()) {
            throw CodegenUnsupported(
                "LLVM emission currently supports scalar, struct return types, or subroutines without return type");
        }
    }
    return FunctionSignature{llvmGlobalName("inox_" + normalize(function.name())), std::move(llvmReturnType),
                             std::move(parameters), signature.returnType};
}

std::string stateGlobalSymbol(std::string_view name)
{
    return "@inox.state." + normalize(name);
}

} // namespace inox::compiler::codegen::detail
