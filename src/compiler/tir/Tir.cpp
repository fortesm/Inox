// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Tir.h"

#include <cstring>

namespace inox::compiler::tir {

void* Arena::allocate(std::size_t size, std::size_t alignment)
{
    std::size_t start = (offset_ + alignment - 1) & ~(alignment - 1);
    if (blocks_.empty() || start + size > kBlockSize) {
        const std::size_t blockSize = size > kBlockSize ? size : kBlockSize;
        blocks_.push_back(std::make_unique<std::byte[]>(blockSize));
        reserved_ += blockSize;
        start = 0;
        // A block bigger than the others is full at once.
        offset_ = size > kBlockSize ? kBlockSize : 0;
        if (size > kBlockSize) {
            used_ += size;
            return blocks_.back().get();
        }
    }
    offset_ = start + size;
    used_ += size;
    return blocks_.back().get() + start;
}

std::string_view Arena::text(std::string_view value)
{
    if (value.empty()) {
        return {};
    }
    char* copy = static_cast<char*>(allocate(value.size(), 1));
    std::memcpy(copy, value.data(), value.size());
    return std::string_view(copy, value.size());
}

std::string_view placeKindName(PlaceKind kind)
{
    switch (kind) {
    case PlaceKind::Local: return "local";
    case PlaceKind::Parameter: return "param";
    case PlaceKind::Receiver: return "self";
    case PlaceKind::LoopIterator: return "iterator";
    case PlaceKind::WithBinding: return "with";
    case PlaceKind::HandlerBinding: return "handler";
    case PlaceKind::State: return "state";
    case PlaceKind::Field: return "field";
    case PlaceKind::Element: return "element";
    case PlaceKind::Through: return "through";
    case PlaceKind::Value: return "value";
    }
    return "?";
}

std::string_view exprKindName(ExprKind kind)
{
    switch (kind) {
    case ExprKind::Literal: return "lit";
    case ExprKind::Read: return "read";
    case ExprKind::Call: return "call";
    case ExprKind::Builtin: return "builtin";
    case ExprKind::String: return "string";
    case ExprKind::Intrinsic: return "intrinsic";
    case ExprKind::Unary: return "unary";
    case ExprKind::Binary: return "binary";
    case ExprKind::Convert: return "convert";
    case ExprKind::Construct: return "construct";
    case ExprKind::Conditional: return "iif";
    case ExprKind::SetLiteral: return "set";
    case ExprKind::Slice: return "slice";
    case ExprKind::Check: return "check";
    case ExprKind::Pure: return "pure";
    case ExprKind::RangeCheck: return "range-check";
    case ExprKind::NaturalCheck: return "natural-check";
    case ExprKind::TypeValue: return "type";
    }
    return "?";
}

std::string_view stmtKindName(StmtKind kind)
{
    switch (kind) {
    case StmtKind::Evaluate: return "evaluate";
    case StmtKind::Assign: return "assign";
    case StmtKind::Declare: return "declare";
    case StmtKind::If: return "if";
    case StmtKind::Unless: return "unless";
    case StmtKind::While: return "while";
    case StmtKind::Repeat: return "repeat";
    case StmtKind::Until: return "until";
    case StmtKind::ForIn: return "for";
    case StmtKind::Case: return "case";
    case StmtKind::Try: return "try";
    case StmtKind::Raise: return "raise";
    case StmtKind::Retry: return "retry";
    case StmtKind::Return: return "return";
    case StmtKind::Exit: return "exit";
    case StmtKind::Leave: return "leave";
    case StmtKind::Continue: return "continue";
    case StmtKind::With: return "with";
    case StmtKind::Block: return "block";
    }
    return "?";
}

std::string_view binaryOpText(BinaryOp op)
{
    switch (op) {
    case BinaryOp::Add: return "add";
    case BinaryOp::Subtract: return "sub";
    case BinaryOp::Multiply: return "mul";
    case BinaryOp::Divide: return "div";
    case BinaryOp::Modulo: return "mod";
    case BinaryOp::Remainder: return "rem";
    case BinaryOp::ShiftLeft: return "shl";
    case BinaryOp::ShiftRight: return "shr";
    case BinaryOp::BitAnd: return "bitand";
    case BinaryOp::BitXor: return "bitxor";
    case BinaryOp::BitOr: return "bitor";
    case BinaryOp::Power: return "pow";
    case BinaryOp::Range: return "range";
    case BinaryOp::In: return "in";
    case BinaryOp::Equal: return "eq";
    case BinaryOp::NotEqual: return "ne";
    case BinaryOp::Less: return "lt";
    case BinaryOp::Greater: return "gt";
    case BinaryOp::LessEqual: return "le";
    case BinaryOp::GreaterEqual: return "ge";
    case BinaryOp::And: return "and";
    case BinaryOp::Xor: return "xor";
    case BinaryOp::Or: return "or";
    }
    return "?";
}

std::string_view unaryOpText(UnaryOp op)
{
    switch (op) {
    case UnaryOp::Plus: return "plus";
    case UnaryOp::Minus: return "neg";
    case UnaryOp::Not: return "not";
    case UnaryOp::BitNot: return "bitnot";
    }
    return "?";
}

} // namespace inox::compiler::tir
