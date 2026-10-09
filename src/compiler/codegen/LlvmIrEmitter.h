// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "../semantic/SemanticResult.h"
#include "../ast/Ast.h"

#include <stdexcept>
#include <string>

namespace inox::compiler::codegen {

class CodegenError : public std::runtime_error {
public:
    explicit CodegenError(std::string message);
};

// A construct the semantic analyzer accepts but the LLVM backend does not
// implement yet. It is a separate type so the driver can tell the user that
// the program is legal Inox and the gap is in the compiler, not in the code.
// The known gaps are listed in docs/BACKEND_GAPS.md.
class CodegenUnsupported final : public CodegenError {
public:
    explicit CodegenUnsupported(std::string message);
};

// Exit status of a program stopped by a runtime fault (CANON-19): overflow,
// division by zero, invalid shift count, invalid for step, negative exponent,
// invalid integer input. Status 70 is Inox-defined; its numeric value
// intentionally coincides with BSD EX_SOFTWARE where that convention exists.
inline constexpr int kRuntimeFaultExitStatus = 70;

class LlvmIrEmitter {
public:
    // The backend consumes the semantic result instead of re-deriving what
    // semantic analysis already resolved (CANON E11, decision P-C).
    std::string emit(const ast::ModuleNode& module, const semantic::SemanticResult& semantics) const;
};

} // namespace inox::compiler::codegen
