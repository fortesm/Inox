// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The types of Inox (OpenSpec change type-representation). A Type is created
// and interned by a TypeContext: two equal types are the same object, so
// type identity is pointer identity. The kinds are the ones LLVM offers
// (integer, floating point, pointer, array, struct), with the Inox meaning on
// top; the LLVM spelling of each kind lives in codegen (TypeLayout).
//
// This module depends only on the C++ standard library.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace inox::compiler::types {

enum class TypeKind : std::uint8_t {
    Void,       // no value (a subroutine call, Put, Free)
    None,       // the type of `None` before it meets its target
    Bool,
    Integer,    // Int8..Int64, UInt8..UInt64, Natural
    Float,      // Float32, Float64
    Char,
    Decimal,    // Currency, Crypto
    String,
    File,
    Exception,  // the standard exception types
    Struct,
    Array,      // Array[Low..High] Element
    Slice,      // Slice<Element>
    Own,        // Own<Target>
    Option,     // Option<Own<Target>>
    Link,       // Link<Target>
    Enum,       // TSuit (Club, Diamond, Heart, Spade): ordinals 0..N-1
    Set,        // Set<Base>: a finite set over an Enum or a Range
};

class Type;

// A field of a struct type, in declaration order (the LLVM field index).
struct StructField {
    std::string name;
    const Type* type = nullptr;
    bool isPart = false;      // a reused struct (CANON-9)
    bool hasDefault = false;
    std::string defaultValue; // the literal as written
};

class Type {
public:
    Type(const Type&) = delete;
    Type& operator=(const Type&) = delete;

    TypeKind kind() const noexcept { return kind_; }
    // The canonical spelling: `Int64` for Integer, `Array[1..5] TItem`.
    const std::string& name() const noexcept { return name_; }

    // Integer and Float: the width in bits.
    unsigned bits() const noexcept { return bits_; }
    // Integer: the range (UInt64 and Natural stop at Int64's maximum, the
    // values the compiler folds) and whether it is exact.
    bool isSigned() const noexcept { return signed_; }
    std::int64_t minValue() const noexcept { return low_; }
    std::int64_t maxValue() const noexcept { return high_; }
    bool rangeIsExact() const noexcept { return exactRange_; }

    // Array: the bounds and the number of elements.
    std::int64_t low() const noexcept { return low_; }
    std::int64_t high() const noexcept { return high_; }
    std::uint64_t length() const noexcept;

    // Array and Slice: the element; Own and Link: the target; Option: the
    // Own it may hold.
    const Type* element() const noexcept { return element_; }

    // Struct: the fields, in declaration order.
    const std::vector<StructField>& fields() const noexcept { return fields_; }
    const StructField* field(std::string_view name) const noexcept;
    std::size_t fieldIndex(const StructField& field) const noexcept;

    bool isVoid() const noexcept { return kind_ == TypeKind::Void; }
    bool isNone() const noexcept { return kind_ == TypeKind::None; }
    bool isBool() const noexcept { return kind_ == TypeKind::Bool; }
    bool isInteger() const noexcept { return kind_ == TypeKind::Integer; }
    bool isFloat() const noexcept { return kind_ == TypeKind::Float; }
    bool isNumeric() const noexcept { return isInteger() || isFloat(); }
    // CANON-8: Int8..Int32 and UInt8..UInt32, computed in i64 and
    // range-checked where a value takes the type.
    bool isNarrowInteger() const noexcept { return isInteger() && bits_ < 64; }
    bool isChar() const noexcept { return kind_ == TypeKind::Char; }
    bool isString() const noexcept { return kind_ == TypeKind::String; }
    bool isFile() const noexcept { return kind_ == TypeKind::File; }
    bool isException() const noexcept { return kind_ == TypeKind::Exception; }
    bool isStruct() const noexcept { return kind_ == TypeKind::Struct; }
    bool isArray() const noexcept { return kind_ == TypeKind::Array; }
    bool isSlice() const noexcept { return kind_ == TypeKind::Slice; }
    bool isOwn() const noexcept { return kind_ == TypeKind::Own; }
    bool isOptionOwn() const noexcept { return kind_ == TypeKind::Option; }
    bool isLink() const noexcept { return kind_ == TypeKind::Link; }
    // Range (CANON-13): an Integer subtype `TMonth Range 1..12` holding
    // minValue..maxValue; its arithmetic is its base's (Int64).
    bool isRange() const noexcept { return kind_ == TypeKind::Integer && element_ != nullptr; }
    const Type* rangeBase() const noexcept { return isRange() ? element_ : nullptr; }
    // Enum (CANON-13): nominal, ordinals 0..N-1 (minValue..maxValue).
    bool isEnum() const noexcept { return kind_ == TypeKind::Enum; }
    // Set<T> (CANON-13): element() is T, an Enum or a Range of at most 256
    // values; setSize() is how many values T has.
    bool isSet() const noexcept { return kind_ == TypeKind::Set; }
    std::int64_t setSize() const noexcept
    {
        return isSet() ? element_->maxValue() - element_->minValue() + 1 : 0;
    }
    // Enum: the literals in declaration order, spelled as declared.
    const std::vector<std::string>& literals() const noexcept { return literals_; }
    // Own, Option<Own> and Link.
    bool isPointer() const noexcept { return isOwn() || isOptionOwn() || isLink(); }
    // The object a pointer reaches (Option<Own<T>> reaches T); null otherwise.
    const Type* pointerTarget() const noexcept;

private:
    friend class TypeContext;
    Type(TypeKind kind, std::string name) : kind_(kind), name_(std::move(name)) {}

    TypeKind kind_;
    std::string name_;
    unsigned bits_ = 0;
    bool signed_ = false;
    bool exactRange_ = true;
    std::int64_t low_ = 0;
    std::int64_t high_ = 0;
    const Type* element_ = nullptr;
    std::vector<StructField> fields_;
    std::vector<std::string> literals_;
};

// The name of a type, or the empty text for a type not known (null): the
// spelling diagnostics use.
inline std::string nameOf(const Type* type)
{
    return type != nullptr ? type->name() : std::string{};
}

} // namespace inox::compiler::types
