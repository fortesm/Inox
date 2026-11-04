// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Operators.h"

#include "LlvmIrEmitter.h"

#include <string>

namespace inox::compiler::codegen::detail {

std::string fcmpPredicate(tir::BinaryOp op)
{
    switch (op) {
    case tir::BinaryOp::Equal:
        return "oeq";
    case tir::BinaryOp::NotEqual:
        return "one";
    case tir::BinaryOp::Less:
        return "olt";
    case tir::BinaryOp::Greater:
        return "ogt";
    case tir::BinaryOp::LessEqual:
        return "ole";
    case tir::BinaryOp::GreaterEqual:
        return "oge";
    default:
        return {};
    }
}

std::string llvmFloatOperation(tir::BinaryOp op)
{
    switch (op) {
    case tir::BinaryOp::Add:
        return "fadd";
    case tir::BinaryOp::Subtract:
        return "fsub";
    case tir::BinaryOp::Multiply:
        return "fmul";
    case tir::BinaryOp::Divide:
        return "fdiv";
    default:
        throw CodegenUnsupported("unsupported Float operator for LLVM emission");
    }
}

// Integer arithmetic is checked (CANON-19, checked arithmetic and limits): overflow, division by zero and an
// out-of-range shift count trap instead of wrapping or being undefined.
// These operators lower to always-inlined runtime helpers.
std::string checkedIntegerHelper(tir::BinaryOp op)
{
    switch (op) {
    case tir::BinaryOp::Add:
        return "__inox_add_i64";
    case tir::BinaryOp::Subtract:
        return "__inox_sub_i64";
    case tir::BinaryOp::Multiply:
        return "__inox_mul_i64";
    case tir::BinaryOp::Divide:
        return "__inox_div_i64";
    case tir::BinaryOp::Modulo:
        return "__inox_mod_i64";
    case tir::BinaryOp::Remainder:
        return "__inox_rem_i64";
    case tir::BinaryOp::ShiftLeft:
        return "__inox_shl_i64";
    case tir::BinaryOp::ShiftRight:
        return "__inox_shr_i64";
    default:
        return {};
    }
}

// Change uint64: the UInt64 helpers of the same operators.
std::string llvmUnsignedHelper(tir::BinaryOp op)
{
    switch (op) {
    case tir::BinaryOp::Add:
        return "__inox_addu_i64";
    case tir::BinaryOp::Subtract:
        return "__inox_subu_i64";
    case tir::BinaryOp::Multiply:
        return "__inox_mulu_i64";
    case tir::BinaryOp::Divide:
        return "__inox_divu_i64";
    case tir::BinaryOp::Modulo:
    case tir::BinaryOp::Remainder:
        return "__inox_modu_i64";
    case tir::BinaryOp::ShiftLeft:
        return "__inox_shlu_i64";
    case tir::BinaryOp::ShiftRight:
        return "__inox_shru_i64";
    case tir::BinaryOp::Power:
        return "__inox_ipowu_i64";
    default:
        return {};
    }
}

std::string llvmOperation(tir::BinaryOp op)
{
    switch (op) {
    case tir::BinaryOp::BitAnd:
        return "and";
    case tir::BinaryOp::BitOr:
        return "or";
    case tir::BinaryOp::BitXor:
        return "xor";
    default:
        throw CodegenUnsupported(
            "unsupported Integer operator for LLVM emission");
    }
}

std::string llvmComparisonPredicate(tir::BinaryOp op)
{
    switch (op) {
    case tir::BinaryOp::Equal:
        return "eq";
    case tir::BinaryOp::NotEqual:
        return "ne";
    case tir::BinaryOp::Less:
        return "slt";
    case tir::BinaryOp::Greater:
        return "sgt";
    case tir::BinaryOp::LessEqual:
        return "sle";
    case tir::BinaryOp::GreaterEqual:
        return "sge";
    default:
        return {};
    }
}

std::string llvmBooleanOperation(tir::BinaryOp op)
{
    switch (op) {
    case tir::BinaryOp::And:
        return "and";
    case tir::BinaryOp::Xor:
        return "xor";
    case tir::BinaryOp::Or:
        return "or";
    default:
        return {};
    }
}

} // namespace inox::compiler::codegen::detail
