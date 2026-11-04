// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// The standard exception types: name, the id the runtime transports, and
// parent. The analysis declares them as types (types::Type, F7), where
// the hierarchy lives for the compiler; libinoxrt finds an id by name.

#pragma once

#include <array>
#include <cctype>
#include <cstdint>
#include <string_view>

namespace inox::compiler::exceptions {

struct ExceptionTypeInfo {
    std::string_view name;
    std::uint64_t typeId;
    std::string_view parent;
};

// Keep existing v3.16 ids stable. New ids are appended deliberately.
inline constexpr std::array<ExceptionTypeInfo, 13> kStandardExceptionTypes{{
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
    // r3.90 (B2): a String that is not a number of the type, Integer("4x").
    {"ConversionError", 0x10C, "Exception"},
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

} // namespace inox::compiler::exceptions
