// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// The Link analysis of a routine of the typed IR (CANON-25; rules 7 and 10
// of the safe-pointers design). It decides two things code generation
// lowers without deciding them again:
//
// - where a variable some Link points into lives: plain storage of the
//   frame when every Link to it provably stays inside the variable's block
//   (`Routine::stackLinkedLocals`), else with a generation block that dies
//   with it (`Routine::generationLocals`);
// - which uses of a Link need no generation check (`Place::linkAlive`): a
//   Link that only ever reaches a variable kept on the stack, and a Link
//   already checked by an earlier statement of the same block when nothing
//   between them can free an object.

#include "Tir.h"

namespace inox::compiler::tir {

void analyzeLinks(Routine& routine, Arena& arena);

} // namespace inox::compiler::tir
