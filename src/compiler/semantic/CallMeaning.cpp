// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "CallMeaning.h"

#include <array>
#include <cctype>
#include <utility>

namespace inox::compiler::semantic {

namespace {

struct Named {
    BuiltinOp op;
    std::string_view name;
};

// Every builtin, in the order of the enumeration.
constexpr std::array<Named, 71> kBuiltins{{
    {BuiltinOp::Put, "Put"}, {BuiltinOp::PutLn, "PutLn"}, {BuiltinOp::Get, "Get"}, {BuiltinOp::GetLn, "GetLn"},
    {BuiltinOp::ReadLn, "ReadLn"}, {BuiltinOp::EndOfInput, "EndOfInput"},
    {BuiltinOp::Abs, "Abs"}, {BuiltinOp::Sqrt, "Sqrt"}, {BuiltinOp::Cbrt, "Cbrt"}, {BuiltinOp::Sin, "Sin"},
    {BuiltinOp::Cos, "Cos"}, {BuiltinOp::Tan, "Tan"}, {BuiltinOp::ArcSin, "ArcSin"}, {BuiltinOp::ArcCos, "ArcCos"},
    {BuiltinOp::ArcTan, "ArcTan"}, {BuiltinOp::ArcTan2, "ArcTan2"}, {BuiltinOp::Sinh, "Sinh"},
    {BuiltinOp::Cosh, "Cosh"}, {BuiltinOp::Tanh, "Tanh"}, {BuiltinOp::Exp, "Exp"}, {BuiltinOp::Ln, "Ln"},
    {BuiltinOp::LnXP1, "LnXP1"}, {BuiltinOp::Log2, "Log2"}, {BuiltinOp::Log10, "Log10"}, {BuiltinOp::LogN, "LogN"},
    {BuiltinOp::Power, "Power"}, {BuiltinOp::Floor, "Floor"}, {BuiltinOp::Ceil, "Ceil"}, {BuiltinOp::FMod, "FMod"},
    {BuiltinOp::Hypot, "Hypot"}, {BuiltinOp::Hypot3, "Hypot3"}, {BuiltinOp::RadToDeg, "RadToDeg"},
    {BuiltinOp::DegToRad, "DegToRad"}, {BuiltinOp::RadToGrad, "RadToGrad"}, {BuiltinOp::GradToRad, "GradToRad"},
    {BuiltinOp::RadToCycle, "RadToCycle"}, {BuiltinOp::CycleToRad, "CycleToRad"},
    {BuiltinOp::Round, "Round"}, {BuiltinOp::Trunc, "Trunc"},
    {BuiltinOp::Low, "Low"}, {BuiltinOp::High, "High"}, {BuiltinOp::Length, "Length"}, {BuiltinOp::Ord, "Ord"},
    {BuiltinOp::Succ, "Succ"}, {BuiltinOp::Pred, "Pred"}, {BuiltinOp::Fill, "Fill"},
    {BuiltinOp::New, "New"}, {BuiltinOp::Free, "Free"}, {BuiltinOp::Move, "Move"}, {BuiltinOp::Value, "Value"},
    {BuiltinOp::Link, "Link"}, {BuiltinOp::Alive, "Alive"}, {BuiltinOp::Same, "Same"},
    {BuiltinOp::Union, "Union"}, {BuiltinOp::Intersection, "Intersection"}, {BuiltinOp::Difference, "Difference"},
    {BuiltinOp::SymmetricDifference, "SymmetricDifference"}, {BuiltinOp::Include, "Include"},
    {BuiltinOp::Exclude, "Exclude"},
    {BuiltinOp::Assert, "Assert"},
    {BuiltinOp::OpenRead, "OpenRead"}, {BuiltinOp::Create, "Create"}, {BuiltinOp::OpenAppend, "OpenAppend"},
    {BuiltinOp::FileReadLn, "ReadLn"}, {BuiltinOp::Eof, "Eof"}, {BuiltinOp::Close, "Close"},
    {BuiltinOp::Write, "Write"}, {BuiltinOp::WriteLn, "WriteLn"}, {BuiltinOp::Exists, "Exists"},
    {BuiltinOp::DeleteFile, "DeleteFile"}, {BuiltinOp::RenameFile, "RenameFile"},
}};

constexpr std::size_t kFirstFile = 60;  // OpenRead

bool sameName(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(left[index])) !=
            std::tolower(static_cast<unsigned char>(right[index]))) {
            return false;
        }
    }
    return true;
}

static_assert(kBuiltins[kFirstFile].op == BuiltinOp::OpenRead, "the file functions start at OpenRead");
static_assert(static_cast<std::size_t>(BuiltinOp::RenameFile) == kBuiltins.size(), "one name per builtin");

} // namespace

BuiltinOp builtinOpNamed(std::string_view name, bool file)
{
    const std::size_t first = file ? kFirstFile : 0;
    const std::size_t last = file ? kBuiltins.size() : kFirstFile;
    for (std::size_t index = first; index < last; ++index) {
        if (sameName(kBuiltins[index].name, name)) {
            return kBuiltins[index].op;
        }
    }
    return BuiltinOp::None;
}

std::string_view builtinName(BuiltinOp op)
{
    if (op == BuiltinOp::None) {
        return "None";
    }
    return kBuiltins[static_cast<std::size_t>(op) - 1].name;
}

std::string_view callKindName(CallKind kind)
{
    switch (kind) {
    case CallKind::Routine: return "Routine";
    case CallKind::Method: return "Method";
    case CallKind::MethodNoArguments: return "MethodNoArguments";
    case CallKind::Field: return "Field";
    case CallKind::Index: return "Index";
    case CallKind::Slice: return "Slice";
    case CallKind::SetLiteral: return "SetLiteral";
    case CallKind::Check: return "Check";
    case CallKind::Pure: return "Pure";
    case CallKind::Construction: return "Construction";
    case CallKind::Conversion: return "Conversion";
    case CallKind::Builtin: return "Builtin";
    case CallKind::StringFunction: return "StringFunction";
    case CallKind::Intrinsic: return "Intrinsic";
    }
    return "?";
}

} // namespace inox::compiler::semantic
