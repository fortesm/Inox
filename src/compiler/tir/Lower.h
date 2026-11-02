// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// Phase 5b (F6, change typed-ir): the typed IR of an analyzed program. It
// copies what the analysis decided (types, symbols and their origins, call
// meanings, field indices, constants, checks) and resolves no name; what
// it cannot express is "valid Inox, not yet lowered" (LoweringError),
// never a crash.

#include "Tir.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace inox::compiler::tir {

class LoweringError : public std::runtime_error {
public:
    explicit LoweringError(const std::string& message) : std::runtime_error(message) {}
};

std::unique_ptr<Program> lowerProgram(const ast::ModuleNode& module, const semantic::SemanticResult& semantics);

} // namespace inox::compiler::tir
