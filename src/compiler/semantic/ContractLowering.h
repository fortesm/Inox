// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "../ast/Ast.h"

namespace inox::compiler::semantic {

// Design by contract (CANON-24; OpenSpec change design-by-contract). Runs
// once on the whole program, after every module is loaded and before the
// analyzer. A routine with `pre`/`pos` clauses (its own or, for an Override,
// those of the methods it replaces) becomes two routines: its body moves to
// a hidden `__Name_body`, and the routine itself checks the precondition,
// saves every `old` value, calls the body, checks the postcondition with
// `Result`, and returns. A Struct with `invariant` lines gets a hidden
// method `TName.__invariant(Self)`, which the backend calls where the
// invariant must hold. Every check is a `__check(Kind, Condition, Text)`
// call that stops the program with a contract trap (CANON-19).
void lowerContracts(ast::ModuleNode& program);

} // namespace inox::compiler::semantic
