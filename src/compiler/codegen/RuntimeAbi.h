// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The functions of libinoxrt that generated code calls (OpenSpec change
// compiler-modularization, step 3). This table is the one place that names
// them and their exact LLVM declarations; runtime/*.cpp implements them.
// A module that declares any of them must be linked with libinoxrt.

#pragma once

#include <array>
#include <string_view>

namespace inox::compiler::codegen::detail {

enum class RuntimeFunction {
    // Exception transport (runtime/ExceptionRuntime.cpp), declared together.
    Raise,
    ExceptionCapture,
    ExceptionType,
    ExceptionRelease,
    ExceptionRethrow,
    // String runtime (runtime/StringRuntime.cpp), declared on first use.
    StringRetain,
    StringRelease,
    StringEqual,
    StringCompare,
    StringGetLn,
    // Std.Strings / Std.StrUtils and `+` (runtime/StringFunctions.cpp). A
    // result is owned; the last parameter of a function that makes a String
    // receives its status (1 out of range, 2 out of memory).
    StringConcat,
    StringAppend,
    StringLength,
    StringSubstr,
    StringEdge,
    StringPosition,
    StringCase,
    StringTrim,
    StringPad,
    StringReplicate,
    StringReplace,
    StringFind,
    StringCompareText,
    // Char values (change char-values): String(C) and CharAt(S, I).
    StringFromChar,
    StringCharAt,
    // File runtime (runtime/FileRuntime.cpp), declared on first use. The
    // functions without `nounwind` raise Inox exceptions (F3).
    FileOpen,
    FileRetain,
    FileRelease,
    FileClose,
    FileReadLn,
    FileEof,
    FileWriteString,
    FileWriteInteger,
    FileWriteUnsigned,
    FileWriteFloat,
    FileWriteBool,
    FileWriteNewline,
    FileExists,
    FileDelete,
    FileRename,
    // Owned blocks of safe pointers (runtime/OwnRuntime.cpp).
    OwnAlloc,
    OwnFree,
};

// The exception functions, in the order the module declares them.
inline constexpr std::array<RuntimeFunction, 5> kExceptionRuntimeFunctions = {
    RuntimeFunction::Raise,          RuntimeFunction::ExceptionCapture, RuntimeFunction::ExceptionType,
    RuntimeFunction::ExceptionRelease, RuntimeFunction::ExceptionRethrow,
};

// The LLVM declaration of a runtime function, exactly as emitted.
std::string_view runtimeDeclaration(RuntimeFunction function);

} // namespace inox::compiler::codegen::detail
