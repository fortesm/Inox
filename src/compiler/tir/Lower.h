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
// it cannot express is "valid Inox, not yet lowered" (LoweringError, E15),
// never a crash: since step 2 the driver reports it as a gap of the
// backend, as code generation reports its own.

#include "MigrationIndex.h"
#include "Tir.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace inox::compiler::tir {

class LoweringError : public std::runtime_error {
public:
    explicit LoweringError(const std::string& message) : std::runtime_error(message) {}
};

// `index`, when given, records what each AST node became (F6 step 2,
// TRANSITIONAL: code generation reads it until step 5).
std::unique_ptr<Program> lowerProgram(const ast::ModuleNode& module, const semantic::SemanticResult& semantics,
                                      MigrationIndex* index = nullptr);

} // namespace inox::compiler::tir
