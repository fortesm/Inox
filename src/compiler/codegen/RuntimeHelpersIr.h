// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// IR text of the internal helpers every module may define: the runtime-fault
// function of CANON-19, checked Integer arithmetic and console input
// (OpenSpec change compiler-modularization, step 1).

#pragma once

#include <string>

namespace inox::compiler::codegen::detail {

// `__inox_discard_line`, `__inox_discard_token`, `__inox_read_i64`.
std::string inputRuntimeHelpers();
// The fault messages and `__inox_arith_fault` (CANON-19).
std::string runtimeFaultSeam();
// `@__inox_assert_message(ptr)`: a failed Assert with a message (Std.Debug);
// emitted only by a module that needs it.
std::string contractMessageHelper();
std::string assertMessageHelper();
// Checked Integer arithmetic helpers (`__inox_add_i64`, ...).
std::string mathRuntimeHelpers();

} // namespace inox::compiler::codegen::detail
