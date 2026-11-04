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
// it cannot express is "valid Inox, not yet supported"
// (UnsupportedConstructError, a gap of the backend, E15), and a broken
// invariant of the analysis's output is an internal error
// (InvariantError); neither is a crash.

#include "MigrationIndex.h"
#include "Tir.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace inox::compiler::tir {

// Valid Inox the typed IR cannot express yet: a gap of the backend (E15),
// reported as code generation reports its own.
class UnsupportedConstructError : public std::runtime_error {
public:
    explicit UnsupportedConstructError(const std::string& message) : std::runtime_error(message) {}
};

// A decision the analysis must have recorded is missing, or a structure
// the earlier phases guarantee is broken: an internal error of the compiler.
class InvariantError : public std::logic_error {
public:
    explicit InvariantError(const std::string& message) : std::logic_error(message) {}
};

// `index`, when given, records what each AST node became (F6 step 2,
// TRANSITIONAL: code generation reads it until step 5).
std::unique_ptr<Program> lowerProgram(const ast::ModuleNode& module, const semantic::SemanticResult& semantics,
                                      MigrationIndex* index = nullptr);

} // namespace inox::compiler::tir
