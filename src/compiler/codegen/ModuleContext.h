// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// What the routines of one module share while they are emitted (OpenSpec
// change compiler-modularization, step 3).

#pragma once

#include "ExceptionAbi.h"

#include <cstddef>
#include <string>
#include <vector>

namespace inox::compiler::codegen::detail {

struct ModuleContext {
    explicit ModuleContext(const ExceptionAbi& abi) : exceptionAbi(abi) {}

    const ExceptionAbi& exceptionAbi;
    // Globals and declarations added while routines are emitted (string
    // constants, runtime declarations), each once, in order of first use.
    std::vector<std::string> moduleLines;
    std::size_t nextStringLiteral = 0;
    // Set when the module calls libinoxrt; the driver then links it.
    bool usesInoxRuntime = false;
};

} // namespace inox::compiler::codegen::detail
