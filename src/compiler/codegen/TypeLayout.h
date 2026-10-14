// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// LLVM layout of Inox types: scalars, fixed arrays, slices, structs, routine
// signatures and State globals (OpenSpec change compiler-modularization,
// step 1).

#pragma once

#include "../ast/Ast.h"
#include "../semantic/SemanticResult.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace inox::compiler::codegen::detail {

struct FunctionParameter {
    std::string inoxName;
    std::string llvmName;
    std::string inoxType;
    std::string llvmType;
    // ADR-0016: a `ref`/`ref mut` parameter is a pointer to the caller's
    // storage; valueType is the LLVM type of the value it points to.
    bool byReference = false;
    std::string valueType;
};

struct FunctionSignature {
    std::string llvmName;
    std::string llvmReturnType;
    std::vector<FunctionParameter> parameters;
    std::string returnInoxType;  // canonical Inox type of the result, or empty
};

using FunctionSignatures =
    std::unordered_map<std::string, FunctionSignature>;

struct StructFieldInfo {
    std::string inoxName;
    std::string inoxType;
    std::string llvmType;
    std::size_t index = 0;
    bool isStruct = false;  // a nested struct value, embedded by value
    bool isPart = false;    // a reused struct (CANON-9): its invariant is the struct's too
    bool hasDefault = false;
    std::string defaultValue;
};

struct StructDefinition {
    std::string inoxName;
    std::string llvmName;
    std::vector<StructFieldInfo> fields;
};

using StructDefinitions = std::unordered_map<std::string, StructDefinition>;

struct IntegerRange {
    std::int64_t low;
    std::int64_t high;
};

// CANON-13: a canonical fixed array type (numeric bounds, see
// support/ArrayType.h) and the LLVM storage of its elements. Narrow integers
// are stored in their own width and widened to i64 when read.
struct ArrayLayout {
    std::int64_t low = 0;
    std::int64_t high = 0;
    std::string elementType;      // Inox element type (may be an array)
    std::string elementStorage;   // LLVM storage type of one element
    std::string llvmType;         // [N x elementStorage]
};

// ADR-0017: a slice is { data, low, high }: data points at element `low`.
inline constexpr std::string_view kSliceLlvmType = "{ ptr, i64, i64 }";

// A module `State` variable (CANON-5): a module-level LLVM global, zero at
// load time and initialized in source order at the start of `main`.
struct StateGlobal {
    std::string name;
    std::string inoxType;
    std::string llvmType;
    const ast::Expression* initializer = nullptr;
};
using StateGlobals = std::vector<StateGlobal>;

std::string llvmStructName(std::string_view inoxName);
// The range of a fixed-width integer type narrower than 64 bits (Byte is
// UInt8), or nullopt for any other type.
std::optional<IntegerRange> narrowIntegerRange(std::string_view type);
std::string llvmTypeForScalar(std::string_view inoxType);
bool isFloatLlvmType(std::string_view llvmType);
std::string llvmTypeForInoxType(std::string_view inoxType, const StructDefinitions& structs);
std::string scalarStorageType(std::string_view inoxType);
std::optional<ArrayLayout> arrayLayout(std::string_view canonicalType);
std::string elementStorageType(std::string_view elementType);
const StructDefinition* findStruct(const StructDefinitions& structs, std::string_view inoxName);
const StructFieldInfo* findStructField(const StructDefinition& structType, std::string_view fieldName);
void collectStructDefinitions(const ast::SectionDeclaration& section, StructDefinitions& structs,
                              const semantic::SemanticResult& semantics);
FunctionSignature parseFunctionSignature(const ast::FunctionDeclaration& function,
                                         const StructDefinitions& structs,
                                         const semantic::SemanticResult& semantics);
std::string stateGlobalSymbol(std::string_view name);

} // namespace inox::compiler::codegen::detail
