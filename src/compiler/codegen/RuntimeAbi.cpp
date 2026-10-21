// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "RuntimeAbi.h"

namespace inox::compiler::codegen::detail {

std::string_view runtimeDeclaration(RuntimeFunction function)
{
    switch (function) {
    case RuntimeFunction::Raise:
        return "declare void @__inox_raise(i64) noreturn";
    case RuntimeFunction::ExceptionCapture:
        return "declare ptr @__inox_exception_capture(ptr) nounwind";
    case RuntimeFunction::ExceptionType:
        return "declare i64 @__inox_exception_type(ptr) nounwind";
    case RuntimeFunction::ExceptionRelease:
        return "declare void @__inox_exception_release(ptr) nounwind";
    case RuntimeFunction::ExceptionRethrow:
        return "declare void @__inox_exception_rethrow(ptr) noreturn";
    case RuntimeFunction::StringRetain:
        return "declare void @__inox_str_retain(ptr) nounwind";
    case RuntimeFunction::StringRelease:
        return "declare void @__inox_str_release(ptr) nounwind";
    case RuntimeFunction::StringEqual:
        return "declare zeroext i1 @__inox_str_equal(ptr, ptr) nounwind";
    case RuntimeFunction::StringCompare:
        return "declare i32 @__inox_str_compare(ptr, ptr) nounwind";
    case RuntimeFunction::StringConcat:
        return "declare ptr @__inox_str_concat(i64, ptr, ptr) nounwind";
    case RuntimeFunction::StringAppend:
        return "declare void @__inox_str_append(ptr, i64, ptr, ptr) nounwind";
    case RuntimeFunction::StringLength:
        return "declare i64 @__inox_str_length(ptr) nounwind";
    case RuntimeFunction::StringSubstr:
        return "declare ptr @__inox_str_substr(ptr, i64, i64, ptr) nounwind";
    case RuntimeFunction::StringEdge:
        return "declare ptr @__inox_str_edge(ptr, i64, i32, ptr) nounwind";
    case RuntimeFunction::StringPosition:
        return "declare i64 @__inox_str_pos(ptr, ptr) nounwind";
    case RuntimeFunction::StringCase:
        return "declare ptr @__inox_str_case(ptr, i32, ptr) nounwind";
    case RuntimeFunction::StringTrim:
        return "declare ptr @__inox_str_trim(ptr, i32, ptr) nounwind";
    case RuntimeFunction::StringPad:
        return "declare ptr @__inox_str_pad(ptr, i64, i32, ptr) nounwind";
    case RuntimeFunction::StringReplicate:
        return "declare ptr @__inox_str_replicate(ptr, i64, ptr) nounwind";
    case RuntimeFunction::StringReplace:
        return "declare ptr @__inox_str_replace(ptr, ptr, ptr, ptr) nounwind";
    case RuntimeFunction::StringFind:
        return "declare zeroext i1 @__inox_str_find(ptr, ptr, i32) nounwind";
    case RuntimeFunction::StringCompareText:
        return "declare i32 @__inox_str_compare_text(ptr, ptr) nounwind";
    case RuntimeFunction::StringFromChar:
        return "declare ptr @__inox_str_from_char(i64, ptr) nounwind";
    case RuntimeFunction::StringCharAt:
        return "declare i64 @__inox_str_char_at(ptr, i64, ptr) nounwind";
    case RuntimeFunction::StringFromInteger:
        return "declare ptr @__inox_str_from_i64(i64, ptr) nounwind";
    case RuntimeFunction::StringFromUnsigned:
        return "declare ptr @__inox_str_from_u64(i64, ptr) nounwind";
    case RuntimeFunction::StringFromFloat:
        return "declare ptr @__inox_str_from_f64(double, ptr) nounwind";
    case RuntimeFunction::StringFromFloat32:
        return "declare ptr @__inox_str_from_f32(float, ptr) nounwind";
    case RuntimeFunction::StringFormatFloat:
        return "declare ptr @__inox_str_format_float(double, i64, ptr) nounwind";
    case RuntimeFunction::StringToInteger:
        return "declare i64 @__inox_str_to_i64(ptr, i64, i64)";
    case RuntimeFunction::StringToUnsigned:
        return "declare i64 @__inox_str_to_u64(ptr)";
    case RuntimeFunction::StringToFloat:
        return "declare double @__inox_str_to_f64(ptr)";
    case RuntimeFunction::StringToFloat32:
        return "declare float @__inox_str_to_f32(ptr)";
    case RuntimeFunction::StringFieldCount:
        return "declare i64 @__inox_str_field_count(ptr, ptr) nounwind";
    case RuntimeFunction::StringField:
        return "declare ptr @__inox_str_field(ptr, ptr, i64, ptr) nounwind";
    case RuntimeFunction::CharIs:
        return "declare zeroext i1 @__inox_char_is(i64, i32) nounwind readnone";
    case RuntimeFunction::CharCase:
        return "declare i64 @__inox_char_case(i64, i32) nounwind readnone";
    case RuntimeFunction::FileOpen:
        return "declare ptr @__inox_file_open(ptr, i32)";
    case RuntimeFunction::FileRetain:
        return "declare void @__inox_file_retain(ptr) nounwind";
    case RuntimeFunction::FileRelease:
        return "declare void @__inox_file_release(ptr) nounwind";
    case RuntimeFunction::FileClose:
        return "declare void @__inox_file_close(ptr)";
    case RuntimeFunction::FileReadLn:
        return "declare ptr @__inox_file_readln(ptr)";
    case RuntimeFunction::FileEof:
        return "declare zeroext i1 @__inox_file_eof(ptr)";
    case RuntimeFunction::FileWriteString:
        return "declare void @__inox_file_write_str(ptr, ptr)";
    case RuntimeFunction::FileWriteInteger:
        return "declare void @__inox_file_write_i64(ptr, i64)";
    case RuntimeFunction::FileWriteUnsigned:
        return "declare void @__inox_file_write_u64(ptr, i64)";
    case RuntimeFunction::FileWriteFloat:
        return "declare void @__inox_file_write_f64(ptr, double)";
    case RuntimeFunction::FileWriteFloat32:
        return "declare void @__inox_file_write_f32(ptr, float)";
    case RuntimeFunction::FileWriteBool:
        return "declare void @__inox_file_write_bool(ptr, i1 zeroext)";
    case RuntimeFunction::FileWriteNewline:
        return "declare void @__inox_file_write_newline(ptr)";
    case RuntimeFunction::FileExists:
        return "declare zeroext i1 @__inox_file_exists(ptr) nounwind";
    case RuntimeFunction::FileDelete:
        return "declare void @__inox_file_delete(ptr)";
    case RuntimeFunction::FileRename:
        return "declare void @__inox_file_rename(ptr, ptr)";
    case RuntimeFunction::RuntimeInit:
        return "declare void @__inox_runtime_init() nounwind";
    case RuntimeFunction::RuntimeShutdown:
        return "declare void @__inox_runtime_shutdown() nounwind";
    case RuntimeFunction::ConsoleWrite:
        return "declare void @__inox_con_write(ptr, i64) nounwind";
    case RuntimeFunction::ConsolePutString:
        return "declare void @__inox_con_put_str(ptr) nounwind";
    case RuntimeFunction::ConsolePutInteger:
        return "declare void @__inox_con_put_i64(i64) nounwind";
    case RuntimeFunction::ConsolePutUnsigned:
        return "declare void @__inox_con_put_u64(i64) nounwind";
    case RuntimeFunction::ConsolePutFloat:
        return "declare void @__inox_con_put_f64(double) nounwind";
    case RuntimeFunction::ConsolePutBool:
        return "declare void @__inox_con_put_bool(i1 zeroext) nounwind";
    case RuntimeFunction::ConsolePutNewline:
        return "declare void @__inox_con_put_newline() nounwind";
    case RuntimeFunction::ConsoleGetChar:
        return "declare i32 @__inox_con_getc() nounwind";
    case RuntimeFunction::ConsoleGetLn:
        return "declare ptr @__inox_con_getln(ptr) nounwind";
    case RuntimeFunction::ConsoleGetCharacter:
        return "declare i64 @__inox_con_get_char() nounwind";
    case RuntimeFunction::ConsoleAtEnd:
        return "declare zeroext i1 @__inox_con_at_end() nounwind";
    case RuntimeFunction::ConsoleGetFloat:
        return "declare i32 @__inox_con_get_f64(ptr) nounwind";
    case RuntimeFunction::ConsoleGetFloat32:
        return "declare i32 @__inox_con_get_f32(ptr) nounwind";
    case RuntimeFunction::ConsolePutFloat32:
        return "declare void @__inox_con_put_f32(float) nounwind";
    case RuntimeFunction::TerminalRawMode:
        return "declare void @__inox_term_raw_mode(i1 zeroext) nounwind";
    case RuntimeFunction::TerminalReadKey:
        return "declare i64 @__inox_term_read_key() nounwind";
    case RuntimeFunction::TerminalScreenSize:
        return "declare i64 @__inox_term_screen_size() nounwind";
    case RuntimeFunction::TerminalMoveTo:
        return "declare void @__inox_term_move_to(i64, i64) nounwind";
    case RuntimeFunction::TerminalClearScreen:
        return "declare void @__inox_term_clear_screen() nounwind";
    case RuntimeFunction::TerminalClearLine:
        return "declare void @__inox_term_clear_line() nounwind";
    case RuntimeFunction::TerminalSetColor:
        return "declare void @__inox_term_set_color(i64, i64) nounwind";
    case RuntimeFunction::TerminalResetColor:
        return "declare void @__inox_term_reset_color() nounwind";
    case RuntimeFunction::TerminalShowCursor:
        return "declare void @__inox_term_show_cursor(i1 zeroext) nounwind";
    case RuntimeFunction::TerminalFlush:
        return "declare void @__inox_term_flush() nounwind";
    case RuntimeFunction::OwnAlloc:
        return "declare ptr @__inox_own_alloc(i64) nounwind";
    case RuntimeFunction::OwnFree:
        return "declare void @__inox_own_free(ptr, ptr) nounwind";
    }
    return {};
}

} // namespace inox::compiler::codegen::detail
