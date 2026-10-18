// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "TypeContext.h"

#include <cctype>
#include <limits>
#include <stdexcept>

namespace inox::compiler::types {

namespace {

constexpr std::int64_t kInt64Min = std::numeric_limits<std::int64_t>::min();
constexpr std::int64_t kInt64Max = std::numeric_limits<std::int64_t>::max();

} // namespace

std::uint64_t Type::length() const noexcept
{
    return kind_ == TypeKind::Array ? static_cast<std::uint64_t>(high_ - low_) + 1U : 0U;
}

const StructField* Type::field(std::string_view name) const noexcept
{
    for (const StructField& candidate : fields_) {
        if (candidate.name.size() != name.size()) {
            continue;
        }
        bool same = true;
        for (std::size_t index = 0; index < name.size() && same; ++index) {
            same = std::tolower(static_cast<unsigned char>(candidate.name[index])) ==
                   std::tolower(static_cast<unsigned char>(name[index]));
        }
        if (same) {
            return &candidate;
        }
    }
    return nullptr;
}

std::size_t Type::fieldIndex(const StructField& field) const noexcept
{
    return static_cast<std::size_t>(&field - fields_.data());
}

const Type* Type::pointerTarget() const noexcept
{
    switch (kind_) {
    case TypeKind::Own:
    case TypeKind::Link:
        return element_;
    case TypeKind::Option:
        return element_ != nullptr ? element_->element_ : nullptr;
    default:
        return nullptr;
    }
}

TypeContext::TypeContext()
{
    void_ = create(TypeKind::Void, "Void");
    none_ = create(TypeKind::None, "None");
    bool_ = create(TypeKind::Bool, "Bool");
    addInteger("Int8", 8, true, -128, 127);
    addInteger("Int16", 16, true, -32768, 32767);
    addInteger("Int32", 32, true, -2147483648LL, 2147483647LL);
    int64_ = addInteger("Int64", 64, true, kInt64Min, kInt64Max);
    addInteger("UInt8", 8, false, 0, 255);
    addInteger("UInt16", 16, false, 0, 65535);
    addInteger("UInt32", 32, false, 0, 4294967295LL);
    // UInt64 values above Int64's maximum are not folded or lowered yet
    // (change uint64): its range here stops there, and is not exact.
    addInteger("UInt64", 64, false, 0, kInt64Max, false);
    // CANON-8: Natural is Int64's non-negative subtype, a distinct type.
    natural_ = addInteger("Natural", 64, true, 0, kInt64Max);
    for (const auto& [name, bits] : {std::pair<const char*, unsigned>{"Float32", 32}, {"Float64", 64}}) {
        Type* type = create(TypeKind::Float, name);
        type->bits_ = bits;
        builtins_.emplace(normalize(name), type);
    }
    float64_ = builtins_.at("float64");
    char_ = create(TypeKind::Char, "Char");
    string_ = create(TypeKind::String, "String");
    file_ = create(TypeKind::File, "File");
    for (const Type* type : {bool_, char_, string_, file_}) {
        builtins_.emplace(normalize(type->name()), type);
    }
    for (const char* name : {"Currency", "Crypto"}) {
        builtins_.emplace(normalize(name), create(TypeKind::Decimal, name));
    }
}

const std::vector<std::string_view>& TypeContext::coreBuiltinNames()
{
    static const std::vector<std::string_view> names = {
        "Bool",
        "Int8", "Int16", "Int32", "Int64",
        "UInt8", "UInt16", "UInt32", "UInt64",
        "Natural",
        "Float32", "Float64",
        "Currency", "Crypto", "Char", "String",
    };
    return names;
}

const Type* TypeContext::builtin(std::string_view canonicalName) const
{
    const auto found = builtins_.find(normalize(canonicalName));
    return found != builtins_.end() ? found->second : nullptr;
}

void TypeContext::declareName(std::string_view name, const Type* type)
{
    names_.insert_or_assign(normalize(name), type);
}

const Type* TypeContext::lookup(std::string_view name) const
{
    const auto found = names_.find(normalize(name));
    return found != names_.end() ? found->second : nullptr;
}

const Type* TypeContext::declareStruct(std::string_view name)
{
    Type* type = create(TypeKind::Struct, std::string(name));
    structs_.emplace(type, type);
    declareName(name, type);
    return type;
}

const Type* TypeContext::declareEnum(std::string_view name, std::vector<std::string> literals)
{
    Type* type = create(TypeKind::Enum, std::string(name));
    type->low_ = 0;
    type->high_ = static_cast<std::int64_t>(literals.size()) - 1;
    type->literals_ = std::move(literals);
    declareName(name, type);
    return type;
}

const Type* TypeContext::declareRange(std::string_view name, bool ofChar)
{
    Type* type = create(ofChar ? TypeKind::Char : TypeKind::Integer, std::string(name));
    type->bits_ = 64;
    type->signed_ = true;
    type->element_ = ofChar ? char_ : int64_;
    ranges_.emplace(type, type);
    declareName(name, type);
    return type;
}

void TypeContext::setRangeBounds(const Type* rangeType, std::int64_t low, std::int64_t high)
{
    Type* type = ranges_.at(rangeType);
    type->low_ = low;
    type->high_ = high;
}

void TypeContext::setStructFields(const Type* structType, std::vector<StructField> fields)
{
    mutableStruct(structType)->fields_ = std::move(fields);
}

void TypeContext::setStructFieldType(const Type* structType, std::size_t index, const Type* fieldType)
{
    mutableStruct(structType)->fields_.at(index).type = fieldType;
}

const Type* TypeContext::array(std::int64_t low, std::int64_t high, const Type* element)
{
    const auto key = std::make_tuple(low, high, element);
    if (const auto found = arrays_.find(key); found != arrays_.end()) {
        return found->second;
    }
    Type* type = create(TypeKind::Array,
                        "Array[" + std::to_string(low) + ".." + std::to_string(high) + "] " + element->name());
    type->low_ = low;
    type->high_ = high;
    type->element_ = element;
    arrays_.emplace(key, type);
    return type;
}

const Type* TypeContext::slice(const Type* element)
{
    return unary(TypeKind::Slice, element, "Slice<" + element->name() + ">");
}

const Type* TypeContext::set(const Type* base)
{
    return unary(TypeKind::Set, base, "Set<" + base->name() + ">");
}

const Type* TypeContext::own(const Type* target)
{
    return unary(TypeKind::Own, target, "Own<" + target->name() + ">");
}

const Type* TypeContext::link(const Type* target)
{
    return unary(TypeKind::Link, target, "Link<" + target->name() + ">");
}

const Type* TypeContext::optionOwn(const Type* owner)
{
    if (owner == nullptr || !owner->isOwn()) {
        throw std::logic_error("an Option holds an Own");
    }
    return unary(TypeKind::Option, owner, "Option<" + owner->name() + ">");
}

const Type* TypeContext::exception(std::string_view name)
{
    const std::string key = normalize(name);
    if (const auto found = exceptions_.find(key); found != exceptions_.end()) {
        return found->second;
    }
    const Type* type = create(TypeKind::Exception, std::string(name));
    exceptions_.emplace(key, type);
    return type;
}

Type* TypeContext::create(TypeKind kind, std::string name)
{
    storage_.push_back(std::unique_ptr<Type>(new Type(kind, std::move(name))));
    return storage_.back().get();
}

Type* TypeContext::addInteger(std::string name, unsigned bits, bool isSigned, std::int64_t low, std::int64_t high,
                              bool exactRange)
{
    Type* type = create(TypeKind::Integer, name);
    type->bits_ = bits;
    type->signed_ = isSigned;
    type->low_ = low;
    type->high_ = high;
    type->exactRange_ = exactRange;
    builtins_.emplace(normalize(name), type);
    return type;
}

const Type* TypeContext::unary(TypeKind kind, const Type* element, std::string name)
{
    const auto key = std::make_pair(kind, element);
    if (const auto found = unaries_.find(key); found != unaries_.end()) {
        return found->second;
    }
    Type* type = create(kind, std::move(name));
    type->element_ = element;
    unaries_.emplace(key, type);
    return type;
}

Type* TypeContext::mutableStruct(const Type* type)
{
    const auto found = structs_.find(type);
    if (found == structs_.end()) {
        throw std::logic_error("only a struct of this TypeContext gets its fields later");
    }
    return found->second;
}

std::string TypeContext::normalize(std::string_view name)
{
    std::string normalized(name);
    for (char& ch : normalized) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return normalized;
}

} // namespace inox::compiler::types
