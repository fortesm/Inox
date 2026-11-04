// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The LLVM instruction or predicate for an Inox operator (OpenSpec change
// compiler-modularization).

#pragma once

#include "../tir/Tir.h"

#include <string>

namespace inox::compiler::codegen::detail {

std::string fcmpPredicate(tir::BinaryOp op);
std::string llvmFloatOperation(tir::BinaryOp op);
// The internal helper (`__inox_add_i64`, ...) of a checked Integer operator.
std::string checkedIntegerHelper(tir::BinaryOp op);
std::string llvmUnsignedHelper(tir::BinaryOp op);
std::string llvmOperation(tir::BinaryOp op);
std::string llvmComparisonPredicate(tir::BinaryOp op);
std::string llvmBooleanOperation(tir::BinaryOp op);

} // namespace inox::compiler::codegen::detail
