// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// The two ways building or reading the typed IR can fail. Neither is a
// crash, and they are never mixed: one is a limit of the backend, the other
// a defect of the compiler.

#include <stdexcept>
#include <string>

namespace inox::compiler::tir {

// Valid Inox the typed IR cannot express yet: a gap of the backend (E15),
// reported as code generation reports its own.
class UnsupportedConstructError : public std::runtime_error {
public:
    explicit UnsupportedConstructError(const std::string& message) : std::runtime_error(message) {}
};

// A decision the analysis must have recorded is missing, a structure the
// earlier phases guarantee is broken, or an AST node has no TIR node: an
// internal error of the compiler.
class InvariantError : public std::logic_error {
public:
    explicit InvariantError(const std::string& message) : std::logic_error(message) {}
};

} // namespace inox::compiler::tir
