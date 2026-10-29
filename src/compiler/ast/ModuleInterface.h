// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Ast.h"

#include <string>

namespace inox::compiler::ast {

// `inox interface File.inox` (r3.99, B4): what other modules see of one
// module, written as Inox: what it re-exports (`Export Use`), its public
// types with their public fields and invariants, public Consts, and the
// headers of its public routines and methods with their contracts. Private
// declarations and every body are left out. Reads only the parsed file.
std::string moduleInterface(const ModuleNode& module);

} // namespace inox::compiler::ast
