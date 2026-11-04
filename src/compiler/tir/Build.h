// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// Construction of the typed IR (phase 5b) from an analyzed program. It
// copies what the analysis decided (types, symbols and their origins, call
// meanings, field indices, constants, checks) and resolves no name. What it
// cannot express is an UnsupportedConstructError (a gap of the backend,
// E15); a broken invariant of the analysis's output is an InvariantError
// (tir/Errors.h).

#include "Errors.h"
#include "Errors.h"
#include "Tir.h"

#include <memory>

namespace inox::compiler::tir {

std::unique_ptr<Program> buildProgram(const ast::ModuleNode& module, const semantic::SemanticResult& semantics);

} // namespace inox::compiler::tir
