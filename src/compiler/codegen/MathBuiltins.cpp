// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "MathBuiltins.h"

#include "LlvmText.h"

namespace inox::compiler::codegen::detail {

std::string mathIntrinsicName(std::string_view inoxName)
{
    if (equalsIgnoreCase(inoxName, "Sqrt")) return "llvm.sqrt.f64";
    if (equalsIgnoreCase(inoxName, "Sin")) return "llvm.sin.f64";
    if (equalsIgnoreCase(inoxName, "Cos")) return "llvm.cos.f64";
    if (equalsIgnoreCase(inoxName, "Exp")) return "llvm.exp.f64";
    if (equalsIgnoreCase(inoxName, "Ln")) return "llvm.log.f64";
    if (equalsIgnoreCase(inoxName, "Log2")) return "llvm.log2.f64";
    if (equalsIgnoreCase(inoxName, "Log10")) return "llvm.log10.f64";
    if (equalsIgnoreCase(inoxName, "Power")) return "llvm.pow.f64";
    if (equalsIgnoreCase(inoxName, "Floor")) return "llvm.floor.f64";
    if (equalsIgnoreCase(inoxName, "Ceil")) return "llvm.ceil.f64";
    if (equalsIgnoreCase(inoxName, "Abs")) return "llvm.fabs.f64";
    return {};
}

std::string mathLibmName(std::string_view inoxName)
{
    if (equalsIgnoreCase(inoxName, "Cbrt")) return "cbrt";
    if (equalsIgnoreCase(inoxName, "Tan")) return "tan";
    if (equalsIgnoreCase(inoxName, "ArcSin")) return "asin";
    if (equalsIgnoreCase(inoxName, "ArcCos")) return "acos";
    if (equalsIgnoreCase(inoxName, "ArcTan")) return "atan";
    if (equalsIgnoreCase(inoxName, "ArcTan2")) return "atan2";
    if (equalsIgnoreCase(inoxName, "Sinh")) return "sinh";
    if (equalsIgnoreCase(inoxName, "Cosh")) return "cosh";
    if (equalsIgnoreCase(inoxName, "Tanh")) return "tanh";
    if (equalsIgnoreCase(inoxName, "LnXP1")) return "log1p";
    if (equalsIgnoreCase(inoxName, "FMod")) return "fmod";
    if (equalsIgnoreCase(inoxName, "Hypot")) return "hypot";
    return {};
}

bool isMathBuiltin(std::string_view inoxName)
{
    return !mathIntrinsicName(inoxName).empty() ||
           !mathLibmName(inoxName).empty() ||
           equalsIgnoreCase(inoxName, "LogN") ||
           equalsIgnoreCase(inoxName, "Hypot3") ||
           equalsIgnoreCase(inoxName, "RadToDeg") ||
           equalsIgnoreCase(inoxName, "DegToRad") ||
           equalsIgnoreCase(inoxName, "RadToGrad") ||
           equalsIgnoreCase(inoxName, "GradToRad") ||
           equalsIgnoreCase(inoxName, "RadToCycle") ||
           equalsIgnoreCase(inoxName, "CycleToRad");
}

} // namespace inox::compiler::codegen::detail
