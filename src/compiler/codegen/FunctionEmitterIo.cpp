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
// end, through the program's getchar (StringRuntime.cpp). Invalid UTF-8 and
// allocation failure are runtime faults 13 and 12.
void FunctionEmitter::emitInputReadLine(const ast::Expression& argument)
{
    const auto& identifier = static_cast<const ast::IdentifierExpression&>(argument);
    const auto local = locals_.find(normalize(identifier.name()));
    if (local == locals_.end()) {
        throw CodegenUnsupported("GetLn LLVM emission currently supports only local or State variables");
    }
    requireStringRuntime();
    requireRuntime(RuntimeFunction::StringGetLn);
    const std::string status = "%getln.status" + std::to_string(nextTemporary_++);
    const std::string line = "%tmp" + std::to_string(nextTemporary_++);
    const std::string code = "%tmp" + std::to_string(nextTemporary_++);
    const std::string label = std::to_string(nextLabel_++);
    output_ << "  " << status << " = alloca i32\n";
    output_ << "  " << line << " = call ptr @__inox_str_getln(ptr @getchar, ptr " << status << ")\n";
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
    if (const auto range = narrowIntegerRange(local->second.type)) {
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

void FunctionEmitter::emitOutputCall(const ast::Expression& argument, bool newline)
{
    if (argument.kind() == ast::AstNodeKind::LiteralExpression) {
        const auto& literal = static_cast<const ast::LiteralExpression&>(argument);
        if (literal.literalKind() == ast::LiteralKind::String) {
            const std::string value = emitStringLiteral(literal.value());
            output_ << "  call i32 (ptr, ...) @printf(ptr "
                    << (newline ? "@.inox.fmt.str.nl" : "@.inox.fmt.str")
                    << ", ptr " << value << ")\n";
            return;
        }
    }

    if (isStringType(typeOf(argument))) {
        // The bytes of the block, printed by the program's printf so the
        // output stays in order with every other Put (StringRuntime.cpp).
        requireModuleLine("@.inox.fmt.strv = private unnamed_addr constant [5 x i8] c\"%.*s\\00\"");
        requireModuleLine("@.inox.fmt.strv.nl = private unnamed_addr constant [6 x i8] c\"%.*s\\0A\\00\"");
        const std::string value = emitExpression(argument);
        const std::string lengthPointer = "%tmp" + std::to_string(nextTemporary_++);
        const std::string length = "%tmp" + std::to_string(nextTemporary_++);
        const std::string length32 = "%tmp" + std::to_string(nextTemporary_++);
        const std::string bytes = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << lengthPointer << " = getelementptr { i64, i64 }, ptr " << value << ", i32 0, i32 1\n";
        output_ << "  " << length << " = load i64, ptr " << lengthPointer << '\n';
        output_ << "  " << length32 << " = trunc i64 " << length << " to i32\n";
        output_ << "  " << bytes << " = getelementptr i8, ptr " << value << ", i64 16\n";
        output_ << "  call i32 (ptr, ...) @printf(ptr " << (newline ? "@.inox.fmt.strv.nl" : "@.inox.fmt.strv")
                << ", i32 " << length32 << ", ptr " << bytes << ")\n";
        return;
    }

    if (isBoolExpression(argument)) {
        const std::string value = emitExpression(argument);
        const std::string selected = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << selected
                << " = select i1 " << value
                << ", ptr @.inox.true, ptr @.inox.false\n";
        output_ << "  call i32 (ptr, ...) @printf(ptr "
                << (newline ? "@.inox.fmt.str.nl" : "@.inox.fmt.str")
                << ", ptr " << selected << ")\n";
        return;
    }

    const std::string argumentType = expressionLlvmType(argument);
    const std::string value = emitExpression(argument);
    if (isFloatLlvmType(argumentType)) {
        output_ << "  call i32 (ptr, ...) @printf(ptr "
                << (newline ? "@.inox.fmt.f64.nl" : "@.inox.fmt.f64")
                << ", double " << value << ")\n";
        return;
    }

    // Change uint64: a UInt64 is printed unsigned.
    if (isUInt64Type(typeOf(argument))) {
        requireModuleLine("@.inox.fmt.u64 = private unnamed_addr constant [5 x i8] c\"%llu\\00\"");
        requireModuleLine("@.inox.fmt.u64.nl = private unnamed_addr constant [6 x i8] c\"%llu\\0A\\00\"");
        output_ << "  call i32 (ptr, ...) @printf(ptr " << (newline ? "@.inox.fmt.u64.nl" : "@.inox.fmt.u64")
                << ", i64 " << value << ")\n";
        return;
    }
    output_ << "  call i32 (ptr, ...) @printf(ptr "
            << (newline ? "@.inox.fmt.i64.nl" : "@.inox.fmt.i64")
            << ", i64 " << value << ")\n";
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
               !llvmBooleanOperation(binary.op()).empty();
    }

    return expressionLlvmType(expression) == "i1";
}

} // namespace inox::compiler::codegen::detail
