// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// What a call is: the analysis records, for
// every call it accepts (and every identifier that is one, `GetLn`,
// `EndOfInput`, a routine without arguments), the meaning it decided; the
// typed IR (tir/Build) copies it and never looks at a name. The builtin
// enumeration below is the one the analysis and the TIR share.

#include <cstdint>
#include <optional>
#include <string_view>

namespace inox::compiler::semantic {

struct FunctionSignature;
struct ModuleIntrinsic;

enum class CallKind : std::uint8_t {
    Routine,            // a routine of the program, `F(X)` or `F`; its signature is recorded
    Method,             // `P.M(X)`: a method, the receiver first
    MethodNoArguments,  // `P.M`: a method with only its receiver (`__member` of a method)
    Field,              // `P.F` (`__member` of a field)
    Index,              // `A[I]` (`__index`)
    Slice,              // `A[L..H]` (`__slice`)
    SetLiteral,         // `[A, B..C]` (`__set`)
    Check,              // a contract check (`__check`, CANON-24)
    Pure,               // `old X` and a loop variant, evaluated where they stand (`__pure`)
    Construction,       // `T(F := V, ...)`
    Conversion,         // `T(X)`
    Builtin,            // one of BuiltinOp
    StringFunction,     // Std.Strings / Std.StrUtils (the function is recorded apart)
    Intrinsic,          // a module intrinsic (Std.Terminal, Std.SystemUtils, Std.DateTime)
};

// The language's builtins and the functions of Std.Math, Std.Debug and
// Std.Files / Std.IOUtils. No names: a builtin is one of these.
enum class BuiltinOp : std::uint8_t {
    None,
    // Console (CANON-17)
    Put, PutLn, Get, GetLn, ReadLn, EndOfInput,
    // Math (prelude) and Std.Math
    Abs, Sqrt, Cbrt, Sin, Cos, Tan, ArcSin, ArcCos, ArcTan, ArcTan2,
    Sinh, Cosh, Tanh, Exp, Ln, LnXP1, Log2, Log10, LogN, Power,
    Floor, Ceil, FMod, Hypot, Hypot3,
    RadToDeg, DegToRad, RadToGrad, GradToRad, RadToCycle, CycleToRad,
    Round, Trunc,
    // Arrays, slices, enums (CANON-13)
    Low, High, Length, Ord, Succ, Pred, Fill,
    // Safe pointers
    New, Free, Move, Value, Link, Alive, Same,
    // Sets (CANON-13)
    Union, Intersection, Difference, SymmetricDifference, Include, Exclude,
    // Std.Debug
    Assert,
    // Std.Files / Std.IOUtils
    OpenRead, Create, OpenAppend, FileReadLn, Eof, Close, Write, WriteLn, Exists, DeleteFile, RenameFile,
};

// The meaning of one call. `routine` for Routine, Method and
// MethodNoArguments; `intrinsic` for Intrinsic; `op` for Builtin; `constant`
// when the analysis computed the value (Low, High, Length of an array, Low
// and High of an enum).
struct CallMeaning {
    CallKind kind = CallKind::Builtin;
    BuiltinOp op = BuiltinOp::None;
    const FunctionSignature* routine = nullptr;
    const ModuleIntrinsic* intrinsic = nullptr;
    std::optional<std::int64_t> constant;
    // Field: the index of the field in its struct (the struct the access
    // reaches, through a pointer when the base is one). Also recorded on
    // each `F := V` of a construction.
    std::uint32_t field = 0;
    // Slice: the bound that is the base's own, not written (`A[..H]`,
    // `A[L..]`, `A[..]`).
    bool lowOmitted = false;
    bool highOmitted = false;
};

// The builtin a name denotes, ignoring case; `file` selects the functions
// of Std.Files (ReadLn(F) is FileReadLn). BuiltinOp::None for any other name.
BuiltinOp builtinOpNamed(std::string_view name, bool file = false);
// The name a builtin is written with (FileReadLn is ReadLn).
std::string_view builtinName(BuiltinOp op);
std::string_view callKindName(CallKind kind);

} // namespace inox::compiler::semantic
