// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Std.Math functions lowered to LLVM intrinsics or libm (OpenSpec change
// compiler-modularization, step 1).

#pragma once

#include <string>
#include <string_view>

namespace inox::compiler::codegen::detail {

std::string mathIntrinsicName(std::string_view inoxName);
std::string mathLibmName(std::string_view inoxName);
bool isMathBuiltin(std::string_view inoxName);

} // namespace inox::compiler::codegen::detail
