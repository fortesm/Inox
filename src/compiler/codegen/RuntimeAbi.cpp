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
    case RuntimeFunction::StringGetLn:
        return "declare ptr @__inox_str_getln(ptr, ptr) nounwind";
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
    case RuntimeFunction::OwnAlloc:
        return "declare ptr @__inox_own_alloc(i64) nounwind";
    case RuntimeFunction::OwnFree:
        return "declare void @__inox_own_free(ptr, ptr) nounwind";
    }
    return {};
}

} // namespace inox::compiler::codegen::detail
