// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// Central nominal exception taxonomy for the bootstrap compiler. The runtime
// transports opaque type ids; names and subtype relationships belong to the
// language/compiler layer, not to the C++ exception ABI used underneath.

#pragma once

#include <array>
#include <cctype>
#include <cstdint>
#include <string_view>
#include <vector>

namespace inox::compiler::exceptions {

struct ExceptionTypeInfo {
    std::string_view name;
    std::uint64_t typeId;
    std::string_view parent;
};

// Keep existing v3.16 ids stable. New ids are appended deliberately.
inline constexpr std::array<ExceptionTypeInfo, 12> kStandardExceptionTypes{{
    {"Exception",       0x100, ""},
    {"RangeError",      0x101, "Exception"},
    {"IndexError",      0x102, "RangeError"},
    {"DivisionByZero",  0x103, "ArithmeticError"},
    {"OverflowError",   0x104, "ArithmeticError"},
    {"IOError",         0x105, "Exception"},
    {"ArithmeticError", 0x106, "Exception"},
    {"DomainError",     0x107, "ArithmeticError"},
    {"FileNotFound",    0x108, "IOError"},
    {"PermissionDenied",0x109, "IOError"},
    {"AlreadyExists",   0x10A, "IOError"},
    {"DiskFull",        0x10B, "IOError"},
}};

inline bool equalsIgnoreCase(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))) {
            return false;
        }
    }
    return true;
}

inline const ExceptionTypeInfo* findExceptionType(std::string_view name)
{
    for (const auto& info : kStandardExceptionTypes) {
        if (equalsIgnoreCase(info.name, name)) {
            return &info;
        }
    }
    return nullptr;
}

inline const ExceptionTypeInfo* findExceptionType(std::uint64_t typeId)
{
    for (const auto& info : kStandardExceptionTypes) {
        if (info.typeId == typeId) {
            return &info;
        }
    }
    return nullptr;
}

inline bool isExceptionType(std::string_view name)
{
    return findExceptionType(name) != nullptr;
}

inline bool isSubtypeOf(std::string_view actual, std::string_view expected)
{
    const ExceptionTypeInfo* current = findExceptionType(actual);
    while (current != nullptr) {
        if (equalsIgnoreCase(current->name, expected)) {
            return true;
        }
        if (current->parent.empty()) {
            break;
        }
        current = findExceptionType(current->parent);
    }
    return false;
}

inline std::vector<std::uint64_t> matchingTypeIds(std::string_view expected)
{
    std::vector<std::uint64_t> ids;
    for (const auto& info : kStandardExceptionTypes) {
        if (isSubtypeOf(info.name, expected)) {
            ids.push_back(info.typeId);
        }
    }
    return ids;
}

} // namespace inox::compiler::exceptions
