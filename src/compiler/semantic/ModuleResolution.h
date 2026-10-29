// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "../ast/Ast.h"

#include <memory>
#include <vector>

namespace inox::compiler::semantic {

// Modules and visibility (OpenSpec change modules-encapsulation, r3.99).
// Runs once on the parsed modules of a program, before contracts are
// lowered and before the analyzer, and gives the analyzer one program in
// which every name already means one declaration:
//
// - each module has its own names: a declaration of a used module gets an
//   internal qualified name (`Bank.Accounts.Fee`), and so does a
//   declaration of the entry file whose name a library also writes;
// - a name written without qualification is the module's own declaration
//   first, then the one public declaration of that name among the modules
//   it uses (with what they re-export through `Export Use`); two of them is
//   an ambiguity the programmer resolves by qualifying;
// - `Bank.Accounts.Fee`, `Accounts.Fee` (an alias) reach a public
//   declaration of a module the file uses;
// - `Use M.(X, Y)` limits the unqualified names taken from M;
// - nothing public exposes something private, and a method is declared in
//   the module of its type.
//
// Field and method visibility, which need the types of expressions, are
// checked by the analyzer with the modules recorded here. `modules` holds
// the entry module last.
std::unique_ptr<ast::ModuleNode> resolveModules(std::vector<std::unique_ptr<ast::ModuleNode>> modules);

} // namespace inox::compiler::semantic
