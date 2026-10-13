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
    }
    return {};
}

} // namespace inox::compiler::codegen::detail
