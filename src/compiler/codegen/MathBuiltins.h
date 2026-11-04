// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Std.Math functions lowered to LLVM intrinsics or libm (OpenSpec change
// compiler-modularization).

#pragma once

#include "../semantic/CallMeaning.h"

#include <string_view>

namespace inox::compiler::codegen::detail {

// The LLVM intrinsic, or the libm function, that computes a math builtin;
// empty when it has none.
std::string_view mathIntrinsicName(semantic::BuiltinOp op);
std::string_view mathLibmName(semantic::BuiltinOp op);
// One of the math builtins of the prelude and Std.Math that work on Floats.
bool isMathBuiltin(semantic::BuiltinOp op);

} // namespace inox::compiler::codegen::detail
