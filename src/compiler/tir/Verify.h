// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Tir.h"

#include <ostream>
#include <string>

namespace inox::compiler::tir {

// Checks that every node of the program has a type and a meaning: an
// expression its type, a call its routine and an argument per parameter
// (a reference argument a place), a builtin its operation, a place the
// symbol of a variable declared before it in its routine, a field its
// index. Returns the first problem, or an empty string.
std::string verifyProgram(const Program& program);

// The program as text, for tests and debugging (`inox --dump-tir`):
// deterministic, no addresses, every expression with its type.
void printProgram(const Program& program, std::ostream& output);

} // namespace inox::compiler::tir
