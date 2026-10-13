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
    }
    return {};
}

} // namespace inox::compiler::codegen::detail
