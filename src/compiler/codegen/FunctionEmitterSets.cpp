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

#include <array>
#include <string>
#include <string_view>

namespace inox::compiler::codegen::detail {

namespace {

constexpr std::array<std::string_view, 6> kSetFunctions{
    "Union", "Intersection", "Difference", "SymmetricDifference", "Include", "Exclude"};

unsigned setBits(TypeRef setType)
{
    return static_cast<unsigned>((setType->setSize() + 7) / 8 * 8);
}

} // namespace

bool FunctionEmitter::isSetCall(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
        return false;
    }
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    if (callee.name() == "__set") {
        return true;
    }
    const TypeRef type = typeOf(call);
    if (type == nullptr || !type->isSet() || call.arguments().size() != 2 || calledRoutine(callee) != nullptr) {
        return false;
    }
    for (const std::string_view function : kSetFunctions) {
        if (equalsIgnoreCase(function, callee.name())) {
            return true;
        }
    }
    return false;
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

std::string FunctionEmitter::emitSetCall(const ast::CallExpression& call)
{
    const TypeRef setType = typeOf(call);
    const std::string type = llvmSetType(setType);
    const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
    const auto operation = [&](const char* instruction, const std::string& left, const std::string& right) {
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = " << instruction << ' ' << type << ' ' << left << ", " << right << '\n';
        return result;
    };
    if (name == "__set") {
        std::string value = "0";
        for (const auto& element : call.arguments()) {
            value = operation("or", value, emitSetMask(emitExpression(*element), setType));
        }
        return value;
    }
    const std::string set = emitExpression(*call.arguments()[0]);
    if (equalsIgnoreCase(name, "Include") || equalsIgnoreCase(name, "Exclude")) {
        const std::string mask = emitSetMask(emitExpression(*call.arguments()[1]), setType);
        if (equalsIgnoreCase(name, "Include")) {
            return operation("or", set, mask);
        }
        return operation("and", set, operation("xor", mask, "-1"));
    }
    const std::string other = emitExpression(*call.arguments()[1]);
    if (equalsIgnoreCase(name, "Union")) {
        return operation("or", set, other);
    }
    if (equalsIgnoreCase(name, "Intersection")) {
        return operation("and", set, other);
    }
    if (equalsIgnoreCase(name, "Difference")) {
        return operation("and", set, operation("xor", other, "-1"));
    }
    return operation("xor", set, other);  // SymmetricDifference
}

// `=`, `#`, `<=` (subset) and `>=` (superset) on two sets.
std::string FunctionEmitter::emitSetComparison(const ast::BinaryExpression& expression)
{
    const TypeRef setType = typeOf(expression.left());
    const std::string type = llvmSetType(setType);
    const std::string left = emitExpression(expression.left());
    const std::string right = emitExpression(expression.right());
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    if (expression.op() == ast::BinaryOperator::Equal || expression.op() == ast::BinaryOperator::NotEqual) {
        output_ << "  " << result << " = icmp " << (expression.op() == ast::BinaryOperator::Equal ? "eq" : "ne")
                << ' ' << type << ' ' << left << ", " << right << '\n';
        return result;
    }
    // A <= B: nothing of A is outside B; A >= B: B <= A.
    const bool subset = expression.op() == ast::BinaryOperator::LessEqual;
    const std::string inverted = "%tmp" + std::to_string(nextTemporary_++);
    const std::string outside = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << inverted << " = xor " << type << ' ' << (subset ? right : left) << ", -1\n";
    output_ << "  " << outside << " = and " << type << ' ' << (subset ? left : right) << ", " << inverted << '\n';
    output_ << "  " << result << " = icmp eq " << type << ' ' << outside << ", 0\n";
    return result;
}

// `X in S` and `X in A..B`.
std::string FunctionEmitter::emitMembership(const ast::BinaryExpression& expression)
{
    const ast::Expression& right = expression.right();
    const std::string value = emitExpression(expression.left());
    if (right.kind() == ast::AstNodeKind::BinaryExpression &&
        static_cast<const ast::BinaryExpression&>(right).op() == ast::BinaryOperator::Range) {
        const auto& range = static_cast<const ast::BinaryExpression&>(right);
        const std::string low = emitExpression(range.left());
        const std::string high = emitExpression(range.right());
        const std::string above = "%tmp" + std::to_string(nextTemporary_++);
        const std::string below = "%tmp" + std::to_string(nextTemporary_++);
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << above << " = icmp sge i64 " << value << ", " << low << '\n';
        output_ << "  " << below << " = icmp sle i64 " << value << ", " << high << '\n';
        output_ << "  " << result << " = and i1 " << above << ", " << below << '\n';
        return result;
    }
    const TypeRef setType = typeOf(right);
    const std::string type = llvmSetType(setType);
    const unsigned bits = setBits(setType);
    const std::string set = emitExpression(right);
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
