// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: Std.Terminal (change console-abi-terminal, r3.88). Every
// routine is one call into libinoxrt (runtime/ConsoleRuntime.cpp); ReadKey
// and ScreenSize build their struct from the Int64 the library returns.

#include "FunctionEmitter.h"

#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "TypeLayout.h"
#include "../semantic/StdTerminal.h"

#include <string>
#include <utility>
#include <vector>

namespace inox::compiler::codegen::detail {

using semantic::TerminalFunction;
using semantic::TerminalSignature;

// The Std.Terminal routine an identifier or a call names, or null.
const TerminalSignature* FunctionEmitter::terminalCallOf(const ast::Expression& expression) const
{
    const ast::IdentifierExpression* identifier = nullptr;
    if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
        identifier = static_cast<const ast::IdentifierExpression*>(&expression);
    } else if (expression.kind() == ast::AstNodeKind::CallExpression) {
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
            identifier = static_cast<const ast::IdentifierExpression*>(&call.callee());
        }
    }
    if (identifier == nullptr) {
        return nullptr;
    }
    const TerminalSignature* function = semantic::findTerminalFunction(identifier->name());
    if (function == nullptr) {
        return nullptr;
    }
    const semantic::Symbol* symbol = semantics_.symbolOf(*identifier);
    if (symbol == nullptr || symbol->kind != semantic::SymbolKind::Builtin) {
        return nullptr;
    }
    return function;
}

// An Int64 stored into a field of another LLVM integer type.
std::string FunctionEmitter::narrowFromInt64(const std::string& value, const std::string& llvmType)
{
    if (llvmType == "i64") {
        return value;
    }
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    if (llvmType == "i1") {
        output_ << "  " << result << " = icmp ne i64 " << value << ", 0\n";
    } else {
        output_ << "  " << result << " = trunc i64 " << value << " to " << llvmType << '\n';
    }
    return result;
}

// A struct value of `type` whose named fields hold the given Int64 values.
std::string FunctionEmitter::emitTerminalStruct(TypeRef type,
                                                const std::vector<std::pair<std::string, std::string>>& fields)
{
    const std::string structName = llvmStructName(type);
    const std::string slot = "%terminal" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca " << structName << '\n';
    for (const auto& [name, value] : fields) {
        const auto field = fieldLayout(type, name);
        if (!field) {
            throw CodegenError("Std.Terminal: " + type->name() + " has no field " + name);
        }
        const std::string stored = narrowFromInt64(value, field->llvmType);
        const std::string pointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << pointer << " = getelementptr " << structName << ", ptr " << slot << ", i32 0, i32 "
                << field->index << '\n';
        output_ << "  store " << field->llvmType << ' ' << stored << ", ptr " << pointer << '\n';
    }
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << result << " = load " << structName << ", ptr " << slot << '\n';
    return result;
}

// An argument widened to i64 (an enum may be narrower).
std::string FunctionEmitter::emitInt64Argument(const ast::Expression& argument)
{
    const std::string type = expressionLlvmType(argument);
    const std::string value = emitExpression(argument);
    if (type == "i64") {
        return value;
    }
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << result << " = zext " << type << ' ' << value << " to i64\n";
    return result;
}

// Emits a Std.Terminal routine; its value, or "" for one without a result.
std::string FunctionEmitter::emitTerminalCall(const ast::Expression& expression, const TerminalSignature& function)
{
    static const std::vector<ast::ExpressionPtr> kNoArguments;
    const std::vector<ast::ExpressionPtr>& arguments =
        expression.kind() == ast::AstNodeKind::CallExpression
            ? static_cast<const ast::CallExpression&>(expression).arguments()
            : kNoArguments;
    const auto temporary = [this] { return "%tmp" + std::to_string(nextTemporary_++); };
    switch (function.function) {
    case TerminalFunction::RawMode:
    case TerminalFunction::ShowCursor: {
        const bool raw = function.function == TerminalFunction::RawMode;
        requireRuntime(raw ? RuntimeFunction::TerminalRawMode : RuntimeFunction::TerminalShowCursor);
        const std::string flag = emitExpression(*arguments[0]);
        output_ << "  call void @" << (raw ? "__inox_term_raw_mode" : "__inox_term_show_cursor") << "(i1 zeroext "
                << flag << ")\n";
        return {};
    }
    case TerminalFunction::ReadKey: {
        requireRuntime(RuntimeFunction::TerminalReadKey);
        const std::string code = temporary();
        output_ << "  " << code << " = call i64 @__inox_term_read_key()\n";
        // The end of input stops the program (fault kind 18).
        const std::string present = temporary();
        output_ << "  " << present << " = icmp ne i64 " << code << ", -1\n";
        emitTrapUnless(present, 18);
        const std::string kind = temporary();
        const std::string ctrlBit = temporary();
        const std::string ctrl = temporary();
        const std::string altBit = temporary();
        const std::string alt = temporary();
        const std::string character = temporary();
        output_ << "  " << kind << " = and i64 " << code << ", 255\n";
        output_ << "  " << ctrlBit << " = lshr i64 " << code << ", " << semantic::kKeyCtrlBit << '\n';
        output_ << "  " << ctrl << " = and i64 " << ctrlBit << ", 1\n";
        output_ << "  " << altBit << " = lshr i64 " << code << ", " << semantic::kKeyAltBit << '\n';
        output_ << "  " << alt << " = and i64 " << altBit << ", 1\n";
        output_ << "  " << character << " = lshr i64 " << code << ", " << semantic::kKeyCharShift << '\n';
        return emitTerminalStruct(typeOf(expression),
                                  {{"Kind", kind}, {"Ch", character}, {"Ctrl", ctrl}, {"Alt", alt}});
    }
    case TerminalFunction::ScreenSize: {
        requireRuntime(RuntimeFunction::TerminalScreenSize);
        const std::string code = temporary();
        const std::string rows = temporary();
        const std::string columns = temporary();
        output_ << "  " << code << " = call i64 @__inox_term_screen_size()\n";
        output_ << "  " << rows << " = lshr i64 " << code << ", 32\n";
        output_ << "  " << columns << " = and i64 " << code << ", 4294967295\n";
        return emitTerminalStruct(typeOf(expression), {{"Rows", rows}, {"Cols", columns}});
    }
    case TerminalFunction::MoveTo: {
        requireRuntime(RuntimeFunction::TerminalMoveTo);
        const std::string row = emitInt64Argument(*arguments[0]);
        const std::string column = emitInt64Argument(*arguments[1]);
        // Rows and columns count from 1 ("value out of range", kind 11).
        const std::string rowOk = temporary();
        const std::string columnOk = temporary();
        const std::string bothOk = temporary();
        output_ << "  " << rowOk << " = icmp sge i64 " << row << ", 1\n";
        output_ << "  " << columnOk << " = icmp sge i64 " << column << ", 1\n";
        output_ << "  " << bothOk << " = and i1 " << rowOk << ", " << columnOk << '\n';
        emitTrapUnless(bothOk, 11);
        output_ << "  call void @__inox_term_move_to(i64 " << row << ", i64 " << column << ")\n";
        return {};
    }
    case TerminalFunction::SetColor: {
        requireRuntime(RuntimeFunction::TerminalSetColor);
        const std::string foreground = emitInt64Argument(*arguments[0]);
        const std::string background = emitInt64Argument(*arguments[1]);
        output_ << "  call void @__inox_term_set_color(i64 " << foreground << ", i64 " << background << ")\n";
        return {};
    }
    case TerminalFunction::ClearScreen:
        requireRuntime(RuntimeFunction::TerminalClearScreen);
        output_ << "  call void @__inox_term_clear_screen()\n";
        return {};
    case TerminalFunction::ClearLine:
        requireRuntime(RuntimeFunction::TerminalClearLine);
        output_ << "  call void @__inox_term_clear_line()\n";
        return {};
    case TerminalFunction::ResetColor:
        requireRuntime(RuntimeFunction::TerminalResetColor);
        output_ << "  call void @__inox_term_reset_color()\n";
        return {};
    case TerminalFunction::Flush:
        requireRuntime(RuntimeFunction::TerminalFlush);
        output_ << "  call void @__inox_term_flush()\n";
        return {};
    }
    return {};
}

} // namespace inox::compiler::codegen::detail
