// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: Put/PutLn and Get/GetLn.

#include "FunctionEmitter.h"

#include "AstQueries.h"
#include "ExceptionAbi.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "MathBuiltins.h"
#include "Operators.h"
#include "RuntimeHelpersIr.h"
#include "../exceptions/ExceptionTypes.h"
#include "../support/Platform.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace inox::compiler::codegen::detail {

void FunctionEmitter::emitInputCallSequence(const std::vector<ast::ExpressionPtr>& arguments, bool consumeRestOfLine)
{
    if (arguments.empty()) {
        throw CodegenError("Get/GetLn calls with parentheses require at least one argument");
    }

    bool lastReadLine = false;
    for (const auto& argument : arguments) {
        lastReadLine = isStringType(typeOf(*argument));
        if (lastReadLine) {
            emitInputReadLine(*argument);
        } else {
            emitInputReadInteger(*argument);
        }
    }

    // A String argument already consumed the rest of its line.
    if (consumeRestOfLine && !lastReadLine) {
        emitInputDiscardLine();
    }
}

// CANON-17 (v3.50): GetLn(S) reads the rest of the line, without its line
// end, through libinoxrt's console (change console-abi-terminal). Invalid
// UTF-8 and allocation failure are runtime faults 13 and 12.
void FunctionEmitter::emitInputReadLine(const ast::Expression& argument)
{
    const auto& identifier = static_cast<const ast::IdentifierExpression&>(argument);
    const auto local = locals_.find(normalize(identifier.name()));
    if (local == locals_.end()) {
        throw CodegenUnsupported("GetLn LLVM emission currently supports only local or State variables");
    }
    requireStringRuntime();
    const std::string status = "%getln.status" + std::to_string(nextTemporary_++);
    const std::string line = "%tmp" + std::to_string(nextTemporary_++);
    const std::string code = "%tmp" + std::to_string(nextTemporary_++);
    const std::string label = std::to_string(nextLabel_++);
    output_ << "  " << status << " = alloca i32\n";
    output_ << "  " << line << " = call ptr @__inox_con_getln(ptr " << status << ")\n";
    output_ << "  " << code << " = load i32, ptr " << status << '\n';
    output_ << "  switch i32 " << code << ", label %getln.ok" << label << " [\n"
            << "    i32 1, label %getln.utf8" << label << "\n"
            << "    i32 2, label %getln.memory" << label << "\n  ]\n\n";
    output_ << "getln.utf8" << label << ":\n  call void @__inox_arith_fault(i32 13)\n  unreachable\n\n";
    output_ << "getln.memory" << label << ":\n  call void @__inox_arith_fault(i32 12)\n  unreachable\n\n";
    output_ << "getln.ok" << label << ":\n";
    // The line is owned (+1): it replaces the old value without a retain.
    const std::string old = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << old << " = load ptr, ptr " << local->second.slot << '\n';
    output_ << "  store ptr " << line << ", ptr " << local->second.slot << '\n';
    output_ << "  call void @__inox_str_release(ptr " << old << ")\n";
}

void FunctionEmitter::emitInputReadInteger(const ast::Expression& argument)
{
    if (argument.kind() != ast::AstNodeKind::IdentifierExpression) {
        throw CodegenUnsupported("Get/GetLn LLVM emission requires assignable local variables");
    }

    const auto& identifier = static_cast<const ast::IdentifierExpression&>(argument);
    const auto local = locals_.find(normalize(identifier.name()));
    if (local == locals_.end()) {
        throw CodegenUnsupported("Get/GetLn LLVM emission currently supports only local variables");
    }
    if (local->second.llvmType != "i64") {
        throw CodegenUnsupported("Get/GetLn LLVM emission currently supports only Integer/Int64 variables");
    }

    // Change uint64: a UInt64 reads 0..18446744073709551615.
    if (isUInt64Type(local->second.type)) {
        requireUnsignedHelpers();
        requireModuleLine(unsignedInputHelper());
        output_ << "  call void @__inox_read_u64(ptr " << local->second.slot << ")\n";
        return;
    }
    output_ << "  call void @__inox_read_i64(ptr " << local->second.slot << ")\n";
    std::optional<IntegerRange> range = narrowIntegerRange(local->second.type);
    if (local->second.type != nullptr && local->second.type->isRange()) {
        range = IntegerRange{local->second.type->minValue(), local->second.type->maxValue()};
    }
    if (range) {
        const std::string read = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << read << " = load i64, ptr " << local->second.slot << "\n";
        const std::string checked = emitRangeCheck(read, *range, 6);
        output_ << "  store i64 " << checked << ", ptr " << local->second.slot << "\n";
    }
}

void FunctionEmitter::emitInputDiscardToken()
{
    output_ << "  call void @__inox_discard_token()\n";
}

void FunctionEmitter::emitInputDiscardLine()
{
    output_ << "  call void @__inox_discard_line()\n";
}

void FunctionEmitter::emitOutputCallSequence(const std::vector<ast::ExpressionPtr>& arguments, bool newline)
{
    if (arguments.empty()) {
        throw CodegenError("Put/PutLn LLVM emission expects at least one argument");
    }

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        emitOutputCall(*arguments[index], newline && index + 1 == arguments.size());
    }
}

// E1 (change console-abi-terminal): every value is printed by libinoxrt,
// which owns standard output; a line end is one more call.
void FunctionEmitter::emitOutputCall(const ast::Expression& argument, bool newline)
{
    emitOutputValue(argument);
    if (newline) {
        output_ << "  call void @__inox_con_put_newline()\n";
    }
}

void FunctionEmitter::emitOutputValue(const ast::Expression& argument)
{
    if (argument.kind() == ast::AstNodeKind::LiteralExpression) {
        const auto& literal = static_cast<const ast::LiteralExpression&>(argument);
        if (literal.literalKind() == ast::LiteralKind::String) {
            const std::string value = emitStringLiteral(literal.value());
            const std::size_t length = llvmStringConstant(literal.value()).size - 1;
            output_ << "  call void @__inox_con_write(ptr " << value << ", i64 " << length << ")\n";
            return;
        }
    }

    const TypeRef printedType = typeOf(argument);
    if (isStringType(printedType) || (printedType != nullptr && (printedType->isEnum() || printedType->isChar()))) {
        // An enum prints its literal's name (CANON-13); a Char its
        // character in UTF-8 (change char-values).
        const std::string value = printedType->isEnum()   ? emitEnumName(emitExpression(argument), printedType)
                                  : printedType->isChar() ? emitCharString(emitExpression(argument))
                                                          : emitExpression(argument);
        output_ << "  call void @__inox_con_put_str(ptr " << value << ")\n";
        return;
    }

    if (isBoolExpression(argument)) {
        const std::string value = emitExpression(argument);
        output_ << "  call void @__inox_con_put_bool(i1 zeroext " << value << ")\n";
        return;
    }

    const std::string argumentType = expressionLlvmType(argument);
    const std::string value = emitExpression(argument);
    if (isFloatLlvmType(argumentType)) {
        // A Float32 prints as the Float64 it widens to exactly.
        std::string printed = value;
        if (argumentType == "float") {
            printed = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << printed << " = fpext float " << value << " to double\n";
        }
        output_ << "  call void @__inox_con_put_f64(double " << printed << ")\n";
        return;
    }

    // Change uint64: a UInt64 is printed unsigned.
    if (isUInt64Type(typeOf(argument))) {
        output_ << "  call void @__inox_con_put_u64(i64 " << value << ")\n";
        return;
    }
    output_ << "  call void @__inox_con_put_i64(i64 " << value << ")\n";
}

std::string FunctionEmitter::emitStringLiteral(std::string_view value)
{
    const std::string name = ".inox.str." + std::to_string(module_.nextStringLiteral++);
    const LlvmStringConstant constant = llvmStringConstant(value);
    std::ostringstream global;
    global << "@" << name << " = private unnamed_addr constant ["
           << constant.size << " x i8] c\"" << constant.bytes << "\"";
    module_.moduleLines.push_back(global.str());
    return "@" + name;
}

bool FunctionEmitter::isBoolExpression(const ast::Expression& expression) const
{
    if (expression.kind() == ast::AstNodeKind::LiteralExpression) {
        const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
        return literal.literalKind() == ast::LiteralKind::Boolean;
    }

    if (expression.kind() == ast::AstNodeKind::UnaryExpression) {
        const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
        return unary.op() == ast::UnaryOperator::Not;
    }

    if (expression.kind() == ast::AstNodeKind::BinaryExpression) {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        return !llvmComparisonPredicate(binary.op()).empty() ||
               !llvmBooleanOperation(binary.op()).empty() || binary.op() == ast::BinaryOperator::In;
    }

    return expressionLlvmType(expression) == "i1";
}

} // namespace inox::compiler::codegen::detail
