// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The owner of every Type of a compilation (OpenSpec change
// type-representation). It creates each type once (interning): asking twice
// for `Array[1..5] Int64` gives the same object. It also holds the names a
// program may use for types (built-in names, aliases such as Integer and
// Byte, struct names); which names are visible is the analyzer's decision.

#pragma once

#include "Type.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace inox::compiler::types {

class TypeContext {
public:
    TypeContext();
    TypeContext(const TypeContext&) = delete;
    TypeContext& operator=(const TypeContext&) = delete;

    // The built-in types, whatever their names in the program.
    const Type* voidType() const noexcept { return void_; }
    const Type* noneType() const noexcept { return none_; }
    const Type* boolType() const noexcept { return bool_; }
    const Type* int64Type() const noexcept { return int64_; }
    const Type* naturalType() const noexcept { return natural_; }
    const Type* float64Type() const noexcept { return float64_; }
    const Type* charType() const noexcept { return char_; }
    const Type* stringType() const noexcept { return string_; }
    const Type* fileType() const noexcept { return file_; }
    // A built-in type by its canonical name (`Int8`, `Float32`, `File`),
    // declared or not; null for any other name.
    const Type* builtin(std::string_view canonicalName) const;
    // The canonical names of the built-in types the language always offers
    // (not File, which comes with Std.Files, nor the exception types).
    static const std::vector<std::string_view>& coreBuiltinNames();

    // Names: case-insensitive; an alias is a second name of the same object.
    void declareName(std::string_view name, const Type* type);
    const Type* lookup(std::string_view name) const;
    bool isDeclared(std::string_view name) const { return lookup(name) != nullptr; }

    // A struct is nominal: declared by name, its fields given once every
    // type it may use is declared (a field may name a later struct, or the
    // struct itself through a pointer).
    const Type* declareStruct(std::string_view name);
    void setStructFields(const Type* structType, std::vector<StructField> fields);
    void setStructFieldType(const Type* structType, std::size_t index, const Type* fieldType);

    // Constructed types, interned.
    const Type* array(std::int64_t low, std::int64_t high, const Type* element);
    const Type* slice(const Type* element);
    const Type* own(const Type* target);
    const Type* link(const Type* target);
    // An Option holds an owner (CANON-25): `owner` is an Own.
    const Type* optionOwn(const Type* owner);
    // An exception type, nominal; its taxonomy is the exceptions module's.
    const Type* exception(std::string_view name);

private:
    Type* create(TypeKind kind, std::string name);
    Type* addInteger(std::string name, unsigned bits, bool isSigned, std::int64_t low, std::int64_t high,
                     bool exactRange = true);
    const Type* unary(TypeKind kind, const Type* element, std::string name);
    Type* mutableStruct(const Type* type);
    static std::string normalize(std::string_view name);

    std::vector<std::unique_ptr<Type>> storage_;
    std::unordered_map<std::string, const Type*> builtins_;  // by normalized canonical name
    std::unordered_map<std::string, const Type*> names_;     // declared names, normalized
    std::map<std::tuple<std::int64_t, std::int64_t, const Type*>, const Type*> arrays_;
    std::map<std::pair<TypeKind, const Type*>, const Type*> unaries_;
    std::unordered_map<std::string, const Type*> exceptions_;  // by normalized name
    std::unordered_map<const Type*, Type*> structs_;            // the only types completed later

    const Type* void_ = nullptr;
    const Type* none_ = nullptr;
    const Type* bool_ = nullptr;
    const Type* int64_ = nullptr;
    const Type* natural_ = nullptr;
    const Type* float64_ = nullptr;
    const Type* char_ = nullptr;
    const Type* string_ = nullptr;
    const Type* file_ = nullptr;
};

} // namespace inox::compiler::types
