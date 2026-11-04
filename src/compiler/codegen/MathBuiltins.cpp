// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "MathBuiltins.h"


namespace inox::compiler::codegen::detail {

using semantic::BuiltinOp;

std::string_view mathIntrinsicName(BuiltinOp op)
{
    switch (op) {
    case BuiltinOp::Sqrt: return "llvm.sqrt.f64";
    case BuiltinOp::Sin: return "llvm.sin.f64";
    case BuiltinOp::Cos: return "llvm.cos.f64";
    case BuiltinOp::Exp: return "llvm.exp.f64";
    case BuiltinOp::Ln: return "llvm.log.f64";
    case BuiltinOp::Log2: return "llvm.log2.f64";
    case BuiltinOp::Log10: return "llvm.log10.f64";
    case BuiltinOp::Power: return "llvm.pow.f64";
    case BuiltinOp::Floor: return "llvm.floor.f64";
    case BuiltinOp::Ceil: return "llvm.ceil.f64";
    case BuiltinOp::Abs: return "llvm.fabs.f64";
    default: return {};
    }
}

std::string_view mathLibmName(BuiltinOp op)
{
    switch (op) {
    case BuiltinOp::Cbrt: return "cbrt";
    case BuiltinOp::Tan: return "tan";
    case BuiltinOp::ArcSin: return "asin";
    case BuiltinOp::ArcCos: return "acos";
    case BuiltinOp::ArcTan: return "atan";
    case BuiltinOp::ArcTan2: return "atan2";
    case BuiltinOp::Sinh: return "sinh";
    case BuiltinOp::Cosh: return "cosh";
    case BuiltinOp::Tanh: return "tanh";
    case BuiltinOp::LnXP1: return "log1p";
    case BuiltinOp::FMod: return "fmod";
    case BuiltinOp::Hypot: return "hypot";
    default: return {};
    }
}

bool isMathBuiltin(BuiltinOp op)
{
    switch (op) {
    case BuiltinOp::LogN:
    case BuiltinOp::Hypot3:
    case BuiltinOp::RadToDeg:
    case BuiltinOp::DegToRad:
    case BuiltinOp::RadToGrad:
    case BuiltinOp::GradToRad:
    case BuiltinOp::RadToCycle:
    case BuiltinOp::CycleToRad:
        return true;
    default:
        return !mathIntrinsicName(op).empty() || !mathLibmName(op).empty();
    }
}

} // namespace inox::compiler::codegen::detail
