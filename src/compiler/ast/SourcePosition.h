// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstddef>

namespace inox::compiler::ast {

// Where a node starts in its source file (E14; change source-positions).
// Line 0 means unknown: a node the compiler made that no source text wrote.
struct SourcePosition {
    std::size_t line = 0;
    std::size_t column = 0;

    bool known() const { return line != 0; }
};

} // namespace inox::compiler::ast
