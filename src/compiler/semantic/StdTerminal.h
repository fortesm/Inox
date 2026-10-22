// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Std.Terminal (change console-abi-terminal, r3.88, decision F1): the
// routines, shared by the analyzer, which checks a call, and the code
// generator, which lowers it to libinoxrt. Their types (TKey, TKeyKind,
// TScreenSize, TColor) are declared in stdlib/Std.Terminal.inox.

#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace inox::compiler::semantic {

enum class TerminalFunction {
    RawMode,
    ReadKey,
    ScreenSize,
    MoveTo,
    ClearScreen,
    ClearLine,
    SetColor,
    ResetColor,
    ShowCursor,
    Flush,
};

struct TerminalSignature {
    std::string_view name;
    TerminalFunction function;
    std::array<std::string_view, 2> parameters;  // type names; "" when absent
    std::size_t parameterCount;
    std::string_view result;  // a type name, or "Void"
};

inline constexpr std::array<TerminalSignature, 10> kTerminalFunctions{{
    {"RawMode", TerminalFunction::RawMode, {"Bool", ""}, 1, "Void"},
    {"ReadKey", TerminalFunction::ReadKey, {"", ""}, 0, "TKey"},
    {"ScreenSize", TerminalFunction::ScreenSize, {"", ""}, 0, "TScreenSize"},
    {"MoveTo", TerminalFunction::MoveTo, {"Integer", "Integer"}, 2, "Void"},
    {"ClearScreen", TerminalFunction::ClearScreen, {"", ""}, 0, "Void"},
    {"ClearLine", TerminalFunction::ClearLine, {"", ""}, 0, "Void"},
    {"SetColor", TerminalFunction::SetColor, {"TColor", "TColor"}, 2, "Void"},
    {"ResetColor", TerminalFunction::ResetColor, {"", ""}, 0, "Void"},
    {"ShowCursor", TerminalFunction::ShowCursor, {"Bool", ""}, 1, "Void"},
    {"Flush", TerminalFunction::Flush, {"", ""}, 0, "Void"},
}};

inline const TerminalSignature* findTerminalFunction(std::string_view name) noexcept
{
    const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; };
    for (const TerminalSignature& signature : kTerminalFunctions) {
        bool same = signature.name.size() == name.size();
        for (std::size_t index = 0; same && index < name.size(); ++index) {
            same = lower(signature.name[index]) == lower(name[index]);
        }
        if (same) {
            return &signature;
        }
    }
    return nullptr;
}

// ReadKey's Int64 from libinoxrt: the TKeyKind ordinal in bits 0..7, Ctrl in
// bit 8, Alt in bit 9, the Char's code point from bit 16; -1 at the end of
// input. ScreenSize's: rows in bits 32..63, columns in bits 0..31.
inline constexpr int kKeyCtrlBit = 8;
inline constexpr int kKeyAltBit = 9;
inline constexpr int kKeyCharShift = 16;

} // namespace inox::compiler::semantic
