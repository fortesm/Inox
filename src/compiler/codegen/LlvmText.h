// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Names, literals and string constants as they are written in LLVM IR text
// (OpenSpec change compiler-modularization, step 1).

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace inox::compiler::codegen::detail {

bool equalsIgnoreCase(std::string_view left, std::string_view right);
std::string normalize(std::string_view name);

// A user identifier as an LLVM local name that cannot collide with the
// emitter's own temporaries and labels.
std::string safeLlvmName(const std::string& name);
// A global or type name as LLVM text: as is when LLVM accepts it bare,
// quoted otherwise (the name of a generic instance, `inox_max<int64>`).
std::string llvmGlobalName(const std::string& name);
std::string llvmIntegerLiteral(std::string_view value);
std::string llvmDefaultLiteral(std::string_view value, std::string_view llvmType);

struct LlvmStringConstant {
    std::size_t size = 0;
    std::string bytes;
};

// An Inox string literal as the bytes of an LLVM `c"..."` constant (escapes
// resolved, NUL terminated).
LlvmStringConstant llvmStringConstant(std::string_view value);
// Raw text as the bytes of an LLVM `c"..."` constant (no Inox escapes).
std::string llvmByteString(const std::string& text);

} // namespace inox::compiler::codegen::detail
