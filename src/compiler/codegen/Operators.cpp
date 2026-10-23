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

std::string fcmpPredicate(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::Equal:
        return "oeq";
    case ast::BinaryOperator::NotEqual:
        return "one";
    case ast::BinaryOperator::Less:
        return "olt";
    case ast::BinaryOperator::Greater:
        return "ogt";
    case ast::BinaryOperator::LessEqual:
        return "ole";
    case ast::BinaryOperator::GreaterEqual:
        return "oge";
    default:
        return {};
    }
}

std::string llvmFloatOperation(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::Add:
        return "fadd";
    case ast::BinaryOperator::Subtract:
        return "fsub";
    case ast::BinaryOperator::Multiply:
        return "fmul";
    case ast::BinaryOperator::Divide:
        return "fdiv";
    default:
        throw CodegenUnsupported("unsupported Float operator for LLVM emission");
    }
}

// Integer arithmetic is checked (CANON-19, checked arithmetic and limits): overflow, division by zero and an
// out-of-range shift count trap instead of wrapping or being undefined.
// These operators lower to always-inlined runtime helpers.
std::string checkedIntegerHelper(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::Add:
        return "__inox_add_i64";
    case ast::BinaryOperator::Subtract:
        return "__inox_sub_i64";
    case ast::BinaryOperator::Multiply:
        return "__inox_mul_i64";
    case ast::BinaryOperator::Divide:
        return "__inox_div_i64";
    case ast::BinaryOperator::Modulo:
        return "__inox_mod_i64";
    case ast::BinaryOperator::ShiftLeft:
        return "__inox_shl_i64";
    case ast::BinaryOperator::ShiftRight:
        return "__inox_shr_i64";
    default:
        return {};
    }
}

// Change uint64: the UInt64 helpers of the same operators.
std::string llvmUnsignedHelper(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::Add:
        return "__inox_addu_i64";
    case ast::BinaryOperator::Subtract:
        return "__inox_subu_i64";
    case ast::BinaryOperator::Multiply:
        return "__inox_mulu_i64";
    case ast::BinaryOperator::Divide:
        return "__inox_divu_i64";
    case ast::BinaryOperator::Modulo:
        return "__inox_modu_i64";
    case ast::BinaryOperator::ShiftLeft:
        return "__inox_shlu_i64";
    case ast::BinaryOperator::ShiftRight:
        return "__inox_shru_i64";
    case ast::BinaryOperator::Power:
        return "__inox_ipowu_i64";
    default:
        return {};
    }
}

std::string llvmOperation(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::BitAnd:
        return "and";
    case ast::BinaryOperator::BitOr:
        return "or";
    case ast::BinaryOperator::BitXor:
        return "xor";
    default:
        throw CodegenUnsupported(
            "unsupported Integer operator for LLVM emission");
    }
}

std::string llvmComparisonPredicate(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::Equal:
        return "eq";
    case ast::BinaryOperator::NotEqual:
        return "ne";
    case ast::BinaryOperator::Less:
        return "slt";
    case ast::BinaryOperator::Greater:
        return "sgt";
    case ast::BinaryOperator::LessEqual:
        return "sle";
    case ast::BinaryOperator::GreaterEqual:
        return "sge";
    default:
        return {};
    }
}

std::string llvmBooleanOperation(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::And:
        return "and";
    case ast::BinaryOperator::Xor:
        return "xor";
    case ast::BinaryOperator::Or:
        return "or";
    default:
        return {};
    }
}

} // namespace inox::compiler::codegen::detail
