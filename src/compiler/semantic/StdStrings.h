// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The functions of Std.Strings and Std.StrUtils (OpenSpec change
// std-strings): compiler intrinsics visible in a program that uses the
// module, so their names stay free elsewhere. One table, read by the
// analyzer (types, visibility) and by the code generator (the runtime
// function of each), so the two never disagree.

#pragma once

#include <array>
#include <string_view>

namespace inox::compiler::semantic {

enum class StringFunction {
    // Std.Strings
    Length,
    ByteLength,
    Substr,
    Position,
    UpperCase,
    LowerCase,
    CharAt,  // change char-values
    // r3.89 (C3): the classes and the case of a Char
    IsLetter,
    IsDigit,
    IsSpace,
    IsUpper,
    IsLower,
    IsPunctuation,
    UpperCaseChar,
    LowerCaseChar,
    // r3.90 (B1, B4): numbers and fields
    FormatFloat,
    FieldCount,
    Field,
    // Std.StrUtils
    Trim,
    LTrim,
    RTrim,
    Left,
    Right,
    PadLeft,
    PadRight,
    Replicate,
    Replace,
    Contains,
    StartsWith,
    EndsWith,
    SameText,
    CompareText,
};

struct StringFunctionInfo {
    std::string_view name;
    StringFunction function;
    // One letter per parameter: S a String, I an integer, C a Char, R a
    // real (Float or Float32).
    std::string_view parameters;
    // S a String, I an Integer, B a Bool, C a Char.
    char result;
    bool convenience;  // declared in Std.StrUtils, not Std.Strings
};

inline constexpr std::array<StringFunctionInfo, 32> kStringFunctions{{
    {"Length", StringFunction::Length, "S", 'I', false},
    {"ByteLength", StringFunction::ByteLength, "S", 'I', false},
    {"Substr", StringFunction::Substr, "SII", 'S', false},
    {"Position", StringFunction::Position, "SS", 'I', false},
    {"UpperCase", StringFunction::UpperCase, "S", 'S', false},
    {"LowerCase", StringFunction::LowerCase, "S", 'S', false},
    {"CharAt", StringFunction::CharAt, "SI", 'C', false},
    {"IsLetter", StringFunction::IsLetter, "C", 'B', false},
    {"IsDigit", StringFunction::IsDigit, "C", 'B', false},
    {"IsSpace", StringFunction::IsSpace, "C", 'B', false},
    {"IsUpper", StringFunction::IsUpper, "C", 'B', false},
    {"IsLower", StringFunction::IsLower, "C", 'B', false},
    {"IsPunctuation", StringFunction::IsPunctuation, "C", 'B', false},
    {"FormatFloat", StringFunction::FormatFloat, "RI", 'S', false},
    {"Trim", StringFunction::Trim, "S", 'S', true},
    {"LTrim", StringFunction::LTrim, "S", 'S', true},
    {"RTrim", StringFunction::RTrim, "S", 'S', true},
    {"Left", StringFunction::Left, "SI", 'S', true},
    {"Right", StringFunction::Right, "SI", 'S', true},
    {"PadLeft", StringFunction::PadLeft, "SI", 'S', true},
    {"PadRight", StringFunction::PadRight, "SI", 'S', true},
    {"Replicate", StringFunction::Replicate, "SI", 'S', true},
    {"Replace", StringFunction::Replace, "SSS", 'S', true},
    {"Contains", StringFunction::Contains, "SS", 'B', true},
    {"StartsWith", StringFunction::StartsWith, "SS", 'B', true},
    {"EndsWith", StringFunction::EndsWith, "SS", 'B', true},
    {"SameText", StringFunction::SameText, "SS", 'B', true},
    {"CompareText", StringFunction::CompareText, "SS", 'I', true},
    {"FieldCount", StringFunction::FieldCount, "SS", 'I', true},
    {"Field", StringFunction::Field, "SSI", 'S', true},
    // UpperCase(C) and LowerCase(C) of a Char: found by name as the String
    // functions above, chosen by the analyzer when the argument is a Char.
    {"UpperCase", StringFunction::UpperCaseChar, "C", 'C', false},
    {"LowerCase", StringFunction::LowerCaseChar, "C", 'C', false},
}};

// The function named `name` (case-insensitive), or null.
const StringFunctionInfo* findStringFunction(std::string_view name) noexcept;
const StringFunctionInfo& stringFunctionInfo(StringFunction function) noexcept;

} // namespace inox::compiler::semantic
