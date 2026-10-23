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
    // Numbers and text (r3.90, change numbers-text). The four StringTo*
    // raise ConversionError, so they have no `nounwind`.
    StringFromInteger,
    StringFromUnsigned,
    StringFromFloat,
    StringFromFloat32,
    StringFormatFloat,
    StringToInteger,
    StringToUnsigned,
    StringToFloat,
    StringToFloat32,
    StringFieldCount,
    StringField,
    // Currency and Crypto (r3.91, change decimals): two 64-bit halves in, a
    // 16-byte slot out; StringToDecimal raises ConversionError.
    DecimalMultiply,
    DecimalDivide,
    DecimalRescale,
    DecimalToInteger,
    DecimalFromFloat,
    DecimalToFloat,
    DecimalToFloat32,
    StringFromDecimal,
    StringToDecimal,
    // The classes and the case of a Char (r3.89, change input-chars-exponents).
    CharIs,
    CharCase,
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
    FileWriteFloat32,
    FileWriteDecimal,
    FileWriteBool,
    FileWriteNewline,
    FileExists,
    FileDelete,
    FileRename,
    // Console (runtime/ConsoleRuntime.cpp, change console-abi-terminal): every
    // module declares them; Main calls RuntimeInit first and the fault
    // function RuntimeShutdown before it stops.
    RuntimeInit,
    RuntimeShutdown,
    ConsoleWrite,
    ConsolePutString,
    ConsolePutInteger,
    ConsolePutUnsigned,
    ConsolePutFloat,
    ConsolePutBool,
    ConsolePutNewline,
    ConsoleGetChar,
    ConsoleGetLn,
    // Get of a Char or a real, EndOfInput (change input-chars-exponents),
    // declared on first use.
    ConsoleGetCharacter,
    ConsoleAtEnd,
    ConsoleGetFloat,
    ConsoleGetFloat32,
    ConsolePutFloat32,
    ConsolePutDecimal,
    ConsoleGetDecimal,
    // Std.Terminal (runtime/ConsoleRuntime.cpp), declared on first use.
    TerminalRawMode,
    TerminalReadKey,
    TerminalScreenSize,
    TerminalMoveTo,
    TerminalClearScreen,
    TerminalClearLine,
    TerminalSetColor,
    TerminalResetColor,
    TerminalShowCursor,
    TerminalFlush,
    // Owned blocks of safe pointers (runtime/OwnRuntime.cpp).
    OwnAlloc,
    OwnFree,
};

// The exception functions, in the order the module declares them.
inline constexpr std::array<RuntimeFunction, 5> kExceptionRuntimeFunctions = {
    RuntimeFunction::Raise,          RuntimeFunction::ExceptionCapture, RuntimeFunction::ExceptionType,
    RuntimeFunction::ExceptionRelease, RuntimeFunction::ExceptionRethrow,
};

// The console functions, declared by every module.
inline constexpr std::array<RuntimeFunction, 11> kConsoleRuntimeFunctions = {
    RuntimeFunction::RuntimeInit,       RuntimeFunction::RuntimeShutdown,   RuntimeFunction::ConsoleWrite,
    RuntimeFunction::ConsolePutString,  RuntimeFunction::ConsolePutInteger, RuntimeFunction::ConsolePutUnsigned,
    RuntimeFunction::ConsolePutFloat,   RuntimeFunction::ConsolePutBool,    RuntimeFunction::ConsolePutNewline,
    RuntimeFunction::ConsoleGetChar,    RuntimeFunction::ConsoleGetLn,
};

// The LLVM declaration of a runtime function, exactly as emitted.
std::string_view runtimeDeclaration(RuntimeFunction function);

} // namespace inox::compiler::codegen::detail
