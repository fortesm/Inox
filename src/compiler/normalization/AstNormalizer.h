// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Phase 2b, normalization of the AST: the AST of what a module wrote
// becomes the normalized AST the later
// phases understand. Chained assignments become one store per target,
// compound assignments `L := L op R`, `with` targets named bindings
// (`__with_N`), and loop `invariant`/`variant` lines checks in and around
// the loop. Nothing else: it is
// not a general transformation or optimization pass.

#pragma once

#include "../ast/Ast.h"

namespace inox::compiler::normalization {

// Runs once on one parsed module (the driver calls it right after the
// parser, before module resolution). Afterwards the module holds no
// ChainedAssignmentStatement, CompoundAssignmentStatement,
// WithTargetExpression or loop contract.
void normalizeModule(ast::ModuleNode& module);

// Throws std::logic_error when a form of phase 2b is still in `module`: the
// later phases never see one (checked by the driver in Debug builds).
void requireNormalized(const ast::ModuleNode& module);

} // namespace inox::compiler::normalization
