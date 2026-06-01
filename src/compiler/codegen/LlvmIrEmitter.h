// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "../ast/Ast.h"

#include <stdexcept>
#include <string>

namespace inox::compiler::codegen {

class CodegenError final : public std::runtime_error {
public:
    explicit CodegenError(std::string message);
};

class LlvmIrEmitter {
public:
    std::string emit(const ast::ModuleNode& module) const;
};

} // namespace inox::compiler::codegen
