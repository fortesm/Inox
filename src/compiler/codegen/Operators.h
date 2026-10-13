// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The LLVM instruction or predicate for an Inox operator (OpenSpec change
// compiler-modularization, step 1).

#pragma once

#include "../ast/Ast.h"

#include <string>

namespace inox::compiler::codegen::detail {

std::string fcmpPredicate(ast::BinaryOperator op);
std::string llvmFloatOperation(ast::BinaryOperator op);
// The internal helper (`__inox_add_i64`, ...) of a checked Integer operator.
std::string checkedIntegerHelper(ast::BinaryOperator op);
std::string llvmOperation(ast::BinaryOperator op);
std::string llvmComparisonPredicate(ast::BinaryOperator op);
std::string llvmBooleanOperation(ast::BinaryOperator op);

} // namespace inox::compiler::codegen::detail
