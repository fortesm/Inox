// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: Set<T> (CANON-13, OpenSpec change enum-range-set). A set
// is an integer of whole bytes with one bit per value of T, bit 0 for
// Low(T); LLVM handles the wide integers, so there is no runtime support.
// Also `X in A..B`, a value against integer bounds.

#include "FunctionEmitter.h"

#include "LlvmIrEmitter.h"
#include "LlvmText.h"

#include <string>

namespace inox::compiler::codegen::detail {

namespace {

unsigned setBits(TypeRef setType)
{
    return static_cast<unsigned>((setType->setSize() + 7) / 8 * 8);
}

} // namespace

bool FunctionEmitter::isSetCall(const tir::Expr& call)
{
    if (call.kind == tir::ExprKind::SetLiteral) {
        return true;
    }
    if (call.kind != tir::ExprKind::Builtin) {
        return false;
    }
    switch (call.op) {
    case semantic::BuiltinOp::Union:
    case semantic::BuiltinOp::Intersection:
    case semantic::BuiltinOp::Difference:
    case semantic::BuiltinOp::SymmetricDifference:
    case semantic::BuiltinOp::Include:
    case semantic::BuiltinOp::Exclude: {
        const TypeRef type = call.type;
        return type != nullptr && type->isSet() && call.operands.size() == 2;
    }
    default:
        return false;
    }
}

// The one-bit mask of the value `value` (an i64 of the base) in `setType`.
std::string FunctionEmitter::emitSetMask(const std::string& value, TypeRef setType)
{
    const std::string type = llvmSetType(setType);
    const unsigned bits = setBits(setType);
    std::string offset = value;
    if (setType->element()->minValue() != 0) {
        offset = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << offset << " = sub i64 " << value << ", " << setType->element()->minValue() << '\n';
    }
    if (bits != 64) {
        const std::string sized = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << sized << " = " << (bits < 64 ? "trunc" : "zext") << " i64 " << offset << " to " << type
                << '\n';
        offset = sized;
    }
    const std::string mask = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << mask << " = shl " << type << " 1, " << offset << '\n';
    return mask;
}

std::string FunctionEmitter::emitSetCall(const tir::Expr& call)
{
    const TypeRef setType = call.type;
    const std::string type = llvmSetType(setType);
    const auto operation = [&](const char* instruction, const std::string& left, const std::string& right) {
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = " << instruction << ' ' << type << ' ' << left << ", " << right << '\n';
        return result;
    };
    if (call.kind == tir::ExprKind::SetLiteral) {
        std::string value = "0";
        for (const tir::Expr* element : call.operands) {
            value = operation("or", value, emitSetMask(emitValue(*element), setType));
        }
        return value;
    }
    const semantic::BuiltinOp op = call.op;
    const std::string set = emitValue(*call.operands[0]);
    if (op == semantic::BuiltinOp::Include || op == semantic::BuiltinOp::Exclude) {
        const std::string mask = emitSetMask(emitValue(*call.operands[1]), setType);
        if (op == semantic::BuiltinOp::Include) {
            return operation("or", set, mask);
        }
        return operation("and", set, operation("xor", mask, "-1"));
    }
    const std::string other = emitValue(*call.operands[1]);
    if (op == semantic::BuiltinOp::Union) {
        return operation("or", set, other);
    }
    if (op == semantic::BuiltinOp::Intersection) {
        return operation("and", set, other);
    }
    if (op == semantic::BuiltinOp::Difference) {
        return operation("and", set, operation("xor", other, "-1"));
    }
    return operation("xor", set, other);  // SymmetricDifference
}

// `=`, `#`, `<=` (subset) and `>=` (superset) on two sets.
std::string FunctionEmitter::emitSetComparison(const tir::Expr& expression)
{
    const TypeRef setType = bare(*expression.operands[0]).type;
    const std::string type = llvmSetType(setType);
    const std::string left = emitValue(*expression.operands[0]);
    const std::string right = emitValue(*expression.operands[1]);
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    if (expression.binary == tir::BinaryOp::Equal || expression.binary == tir::BinaryOp::NotEqual) {
        output_ << "  " << result << " = icmp " << (expression.binary == tir::BinaryOp::Equal ? "eq" : "ne")
                << ' ' << type << ' ' << left << ", " << right << '\n';
        return result;
    }
    // A <= B: nothing of A is outside B; A >= B: B <= A.
    const bool subset = expression.binary == tir::BinaryOp::LessEqual;
    const std::string inverted = "%tmp" + std::to_string(nextTemporary_++);
    const std::string outside = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << inverted << " = xor " << type << ' ' << (subset ? right : left) << ", -1\n";
    output_ << "  " << outside << " = and " << type << ' ' << (subset ? left : right) << ", " << inverted << '\n';
    output_ << "  " << result << " = icmp eq " << type << ' ' << outside << ", 0\n";
    return result;
}

// `X in S` and `X in A..B`.
std::string FunctionEmitter::emitMembership(const tir::Expr& expression)
{
    const tir::Expr& right = *expression.operands[1];
    const std::string value = emitValue(*expression.operands[0]);
    if (const tir::Expr& range = bare(right); range.kind == tir::ExprKind::Binary && range.binary == tir::BinaryOp::Range) {
        const std::string low = emitValue(*range.operands[0]);
        const std::string high = emitValue(*range.operands[1]);
        const std::string above = "%tmp" + std::to_string(nextTemporary_++);
        const std::string below = "%tmp" + std::to_string(nextTemporary_++);
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << above << " = icmp sge i64 " << value << ", " << low << '\n';
        output_ << "  " << below << " = icmp sle i64 " << value << ", " << high << '\n';
        output_ << "  " << result << " = and i1 " << above << ", " << below << '\n';
        return result;
    }
    const TypeRef setType = bare(right).type;
    const std::string type = llvmSetType(setType);
    const unsigned bits = setBits(setType);
    const std::string set = emitValue(right);
    // A value outside the base is in no set of it.
    const std::string offset = "%tmp" + std::to_string(nextTemporary_++);
    const std::string inside = "%tmp" + std::to_string(nextTemporary_++);
    const std::string safe = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << offset << " = sub i64 " << value << ", " << setType->element()->minValue() << '\n';
    output_ << "  " << inside << " = icmp ult i64 " << offset << ", " << setType->setSize() << '\n';
    output_ << "  " << safe << " = select i1 " << inside << ", i64 " << offset << ", i64 0\n";
    std::string amount = safe;
    if (bits != 64) {
        amount = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << amount << " = " << (bits < 64 ? "trunc" : "zext") << " i64 " << safe << " to " << type
                << '\n';
    }
    const std::string shifted = "%tmp" + std::to_string(nextTemporary_++);
    const std::string bit = "%tmp" + std::to_string(nextTemporary_++);
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << shifted << " = lshr " << type << ' ' << set << ", " << amount << '\n';
    output_ << "  " << bit << " = trunc " << type << ' ' << shifted << " to i1\n";
    output_ << "  " << result << " = and i1 " << bit << ", " << inside << '\n';
    return result;
}

} // namespace inox::compiler::codegen::detail
