// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// LLVM layout of Inox types: the LLVM type of each kind of type, fixed
// arrays, struct fields, routine signatures and State globals. Every type
// is a types::Type from the semantic analysis (OpenSpec change
// type-representation); nothing here reads written type text.

#pragma once

#include "../ast/Ast.h"
#include "../semantic/SemanticResult.h"
#include "../types/Type.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace inox::compiler::codegen::detail {

using TypeRef = const types::Type*;

struct FunctionParameter {
    std::string inoxName;
    std::string llvmName;
    TypeRef type = nullptr;
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
    TypeRef returnType = nullptr;  // null for a subroutine
};

using FunctionSignatures = std::unordered_map<std::string, FunctionSignature>;

struct IntegerRange {
    std::int64_t low;
    std::int64_t high;
};

// CANON-13: a fixed array and the LLVM storage of its elements. Narrow
// integers are stored in their own width and widened to i64 when read.
struct ArrayLayout {
    std::int64_t low = 0;
    std::int64_t high = 0;
    TypeRef element = nullptr;    // may be an array
    std::string elementStorage;   // LLVM storage type of one element
    std::string llvmType;         // [N x elementStorage]
};

// Nested fixed arrays are contiguous: `Array[1..3] Array[1..4] T` is a run
// of 12 values of T.
struct FlatArray {
    std::int64_t count = 0;
    TypeRef element = nullptr;  // never an array
};

// A field of a struct and its place in the LLVM struct.
struct FieldLayout {
    TypeRef owner = nullptr;
    const types::StructField* field = nullptr;
    std::size_t index = 0;
    std::string llvmType;
};

// ADR-0017: a slice is { data, low, high }: data points at element `low`.
inline constexpr std::string_view kSliceLlvmType = "{ ptr, i64, i64 }";
// Safe pointers: a Link is its object's address, the header of the block
// that holds the object, and the generation it saw (change safe-pointers).
// An Own or an Option<Own> is the address of its object (null: None).
inline constexpr std::string_view kLinkLlvmType = "{ ptr, ptr, i64 }";

// A module `State` variable (CANON-5): a module-level LLVM global, zero at
// load time and initialized in source order at the start of `main`.
struct StateGlobal {
    std::string name;
    TypeRef type = nullptr;
    std::string llvmType;
    const ast::Expression* initializer = nullptr;
};
using StateGlobals = std::vector<StateGlobal>;

// Kind tests that accept null (a type the analysis did not record).
inline bool isStringType(TypeRef type) { return type != nullptr && type->isString(); }
inline bool isFileType(TypeRef type) { return type != nullptr && type->isFile(); }
inline bool isStructType(TypeRef type) { return type != nullptr && type->isStruct(); }
inline bool isArrayType(TypeRef type) { return type != nullptr && type->isArray(); }
inline bool isSliceType(TypeRef type) { return type != nullptr && type->isSlice(); }
inline bool isLinkType(TypeRef type) { return type != nullptr && type->isLink(); }
inline bool isPointerType(TypeRef type) { return type != nullptr && type->isPointer(); }
// An owner frees its object: an Own or an Option<Own>.
inline bool isOwnerType(TypeRef type) { return type != nullptr && (type->isOwn() || type->isOptionOwn()); }

std::string llvmStructName(TypeRef structType);
// The range of a fixed-width integer type narrower than 64 bits, or nullopt
// for any other type.
std::optional<IntegerRange> narrowIntegerRange(TypeRef type);
// The LLVM type of a value in SSA form: every integer is i64; empty when the
// type has no LLVM representation (Char, Decimal, an exception, unknown).
std::string llvmValueType(TypeRef type);
// The LLVM type of an array element: a narrow integer in its own width.
std::string llvmStorageType(TypeRef type);
bool isFloatLlvmType(std::string_view llvmType);
std::optional<ArrayLayout> arrayLayout(TypeRef type);
FlatArray flattenArray(TypeRef arrayType);
std::optional<FieldLayout> fieldLayout(TypeRef structType, std::string_view fieldName);
FieldLayout fieldLayout(TypeRef structType, std::size_t index);
// The LLVM signature of a routine, from its semantic signature.
FunctionSignature lowerSignature(const ast::FunctionDeclaration& function,
                                 const semantic::FunctionSignature& signature);
std::string stateGlobalSymbol(std::string_view name);

} // namespace inox::compiler::codegen::detail
