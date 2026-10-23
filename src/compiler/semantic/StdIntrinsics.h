// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The routines of the standard library that are compiler intrinsics carried
// out by libinoxrt: Std.Terminal (r3.88), Std.SystemUtils and the clock of
// Std.DateTime (r3.92). One table, shared by the analyzer, which checks a
// call and makes the names visible with the module's `Use`, and the code
// generator, which lowers it. Their types (TKey, TColor, TDateTime, ...) are
// declared in the module's .inox file.

#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace inox::compiler::semantic {

enum class Intrinsic {
    // Std.Terminal (change console-abi-terminal, r3.88)
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
    // Std.SystemUtils (change datetime-systemutils, r3.92, D4)
    ParamCount,
    ParamStr,
    GetEnv,
    EnvExists,
    Halt,
    Sleep,
    CurrentDir,
    PlatformName,
    // Std.DateTime: the clock and the local zone (r3.92, D1-D2); the rest of
    // the module is written in Inox (stdlib/Std.DateTime.inox).
    NowUTC,
    UTCOffset,
};

struct ModuleIntrinsic {
    std::string_view module;  // visible in a program that uses it
    std::string_view name;
    Intrinsic function;
    std::array<std::string_view, 2> parameters;  // type names; "" when absent
    std::size_t parameterCount;
    std::string_view result;  // a type name, or "Void"
};

inline constexpr std::array<ModuleIntrinsic, 20> kModuleIntrinsics{{
    {"Std.Terminal", "RawMode", Intrinsic::RawMode, {"Bool", ""}, 1, "Void"},
    {"Std.Terminal", "ReadKey", Intrinsic::ReadKey, {"", ""}, 0, "TKey"},
    {"Std.Terminal", "ScreenSize", Intrinsic::ScreenSize, {"", ""}, 0, "TScreenSize"},
    {"Std.Terminal", "MoveTo", Intrinsic::MoveTo, {"Integer", "Integer"}, 2, "Void"},
    {"Std.Terminal", "ClearScreen", Intrinsic::ClearScreen, {"", ""}, 0, "Void"},
    {"Std.Terminal", "ClearLine", Intrinsic::ClearLine, {"", ""}, 0, "Void"},
    {"Std.Terminal", "SetColor", Intrinsic::SetColor, {"TColor", "TColor"}, 2, "Void"},
    {"Std.Terminal", "ResetColor", Intrinsic::ResetColor, {"", ""}, 0, "Void"},
    {"Std.Terminal", "ShowCursor", Intrinsic::ShowCursor, {"Bool", ""}, 1, "Void"},
    {"Std.Terminal", "Flush", Intrinsic::Flush, {"", ""}, 0, "Void"},
    {"Std.SystemUtils", "ParamCount", Intrinsic::ParamCount, {"", ""}, 0, "Integer"},
    {"Std.SystemUtils", "ParamStr", Intrinsic::ParamStr, {"Integer", ""}, 1, "String"},
    {"Std.SystemUtils", "GetEnv", Intrinsic::GetEnv, {"String", ""}, 1, "String"},
    {"Std.SystemUtils", "EnvExists", Intrinsic::EnvExists, {"String", ""}, 1, "Bool"},
    {"Std.SystemUtils", "Halt", Intrinsic::Halt, {"Integer", ""}, 1, "Void"},
    {"Std.SystemUtils", "Sleep", Intrinsic::Sleep, {"Integer", ""}, 1, "Void"},
    {"Std.SystemUtils", "CurrentDir", Intrinsic::CurrentDir, {"", ""}, 0, "String"},
    {"Std.SystemUtils", "PlatformName", Intrinsic::PlatformName, {"", ""}, 0, "String"},
    {"Std.DateTime", "NowUTC", Intrinsic::NowUTC, {"", ""}, 0, "TDateTime"},
    {"Std.DateTime", "UTCOffset", Intrinsic::UTCOffset, {"TDateTime", ""}, 1, "TDuration"},
}};

inline const ModuleIntrinsic* findModuleIntrinsic(std::string_view name) noexcept
{
    const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; };
    for (const ModuleIntrinsic& signature : kModuleIntrinsics) {
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
