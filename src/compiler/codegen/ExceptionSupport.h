// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Exception lowering support shared by the emitter (OpenSpec change
// compiler-modularization; step 3 turns it into the ExceptionAbi strategy).

#pragma once

#include <cstdint>
#include <string_view>

namespace inox::compiler::codegen::detail {

// The runtime type id of a standard exception type (CANON-15).
std::uint64_t exceptionTypeId(std::string_view name);

// Windows uses the MSVC exception ABI (funclets), every other host Itanium.
bool usesMsvcExceptionAbi();

} // namespace inox::compiler::codegen::detail
