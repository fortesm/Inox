// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: the file functions of Std.Files and Std.IOUtils (OpenSpec
// changes file-io and module-pairs).
// Each is one call into src/runtime/FileRuntime.cpp. A File value is an owned
// reference like a String: a function that opens a file returns it owned
// (+1), held for the statement and moved into its variable; the routine's
// owned slots release (close) it at every exit and in the cleanup pad.

#include "FunctionEmitter.h"

#include "LlvmIrEmitter.h"
#include "LlvmText.h"

#include <string>
#include <string_view>

namespace inox::compiler::codegen::detail {

// A function of Std.Files / Std.IOUtils.
bool FunctionEmitter::isFileCall(const ast::CallExpression& call) const
{
    switch (builtinOf(call)) {
    case semantic::BuiltinOp::OpenRead:
    case semantic::BuiltinOp::Create:
    case semantic::BuiltinOp::OpenAppend:
    case semantic::BuiltinOp::FileReadLn:
    case semantic::BuiltinOp::Eof:
    case semantic::BuiltinOp::Close:
    case semantic::BuiltinOp::Write:
    case semantic::BuiltinOp::WriteLn:
    case semantic::BuiltinOp::Exists:
    case semantic::BuiltinOp::DeleteFile:
    case semantic::BuiltinOp::RenameFile:
        return true;
    default:
        return false;
    }
}

std::string FunctionEmitter::fileCallLlvmType(const ast::CallExpression& call) const
{
    switch (builtinOf(call)) {
    case semantic::BuiltinOp::OpenRead:
    case semantic::BuiltinOp::Create:
    case semantic::BuiltinOp::OpenAppend:
    case semantic::BuiltinOp::FileReadLn:
        return "ptr";
    case semantic::BuiltinOp::Eof:
    case semantic::BuiltinOp::Exists:
        return "i1";
    default:
        return "void";
    }
}

std::string FunctionEmitter::emitRuntimeCall(RuntimeFunction function, std::string_view returnType,
                                             std::string_view symbol, const std::string& arguments)
{
    requireRuntime(function);
    std::string result;
    output_ << "  ";
    if (returnType != "void") {
        result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << result << " = ";
    }
    const bool protectedRegion = !unwindTargets_.empty();
    const std::string attributes = returnType == "i1" ? "zeroext " : "";
    output_ << (protectedRegion ? "invoke " : "call ") << attributes << returnType << " @" << symbol << '('
            << arguments << ')';
    if (protectedRegion) {
        const std::string continuation = "eh.invoke.cont" + std::to_string(nextLabel_++);
        output_ << " to label %" << continuation << " unwind label %" << currentUnwindTarget() << "\n\n";
        output_ << continuation << ":\n";
    } else {
        output_ << '\n';
    }
    return result;
}

std::string FunctionEmitter::emitFileCall(const ast::CallExpression& call)
{
    const auto& arguments = call.arguments();
    const auto value = [&](std::size_t index) { return emitExpression(*arguments[index]); };
    const semantic::BuiltinOp op = builtinOf(call);
    switch (op) {
    case semantic::BuiltinOp::OpenRead:
    case semantic::BuiltinOp::Create:
    case semantic::BuiltinOp::OpenAppend: {
        const int mode = op == semantic::BuiltinOp::OpenRead ? 0 : op == semantic::BuiltinOp::Create ? 1 : 2;
        const std::string path = value(0);
        const std::string file = emitRuntimeCall(RuntimeFunction::FileOpen, "ptr", "__inox_file_open",
                                                 "ptr " + path + ", i32 " + std::to_string(mode));
        holdOwnedResult(file, semantics_.types().fileType(), "ptr");
        return file;
    }
    case semantic::BuiltinOp::FileReadLn: {
        const std::string file = value(0);
        const std::string line = emitRuntimeCall(RuntimeFunction::FileReadLn, "ptr", "__inox_file_readln", "ptr " + file);
        holdOwnedResult(line, semantics_.types().stringType(), "ptr");
        return line;
    }
    case semantic::BuiltinOp::Eof:
        return emitRuntimeCall(RuntimeFunction::FileEof, "i1", "__inox_file_eof", "ptr " + value(0));
    case semantic::BuiltinOp::Close:
        emitRuntimeCall(RuntimeFunction::FileClose, "void", "__inox_file_close", "ptr " + value(0));
        return {};
    case semantic::BuiltinOp::Write:
    case semantic::BuiltinOp::WriteLn: {
        // The file is evaluated once; the values are written in order, as
        // PutLn prints them (CANON-17).
        const std::string file = value(0);
        for (std::size_t index = 1; index < arguments.size(); ++index) {
            const TypeRef type = typeOf(*arguments[index]);
            const std::string written = value(index);
            if (isStringType(type) || type->isEnum() || type->isChar()) {
                // An enum is written as its literal's name (CANON-13), a
                // Char as its character (char-values).
                const std::string text = type->isEnum()   ? emitEnumName(written, type)
                                         : type->isChar() ? emitCharString(written)
                                                          : written;
                emitRuntimeCall(RuntimeFunction::FileWriteString, "void", "__inox_file_write_str",
                                "ptr " + file + ", ptr " + text);
            } else if (type->isBool()) {
                emitRuntimeCall(RuntimeFunction::FileWriteBool, "void", "__inox_file_write_bool",
                                "ptr " + file + ", i1 zeroext " + written);
            } else if (type->isDecimal()) {
                const auto [low, high] = decimalHalves(written, type);
                emitRuntimeCall(RuntimeFunction::FileWriteDecimal, "void", "__inox_file_write_dec",
                                "ptr " + file + ", i64 " + low + ", i64 " + high + ", i32 " +
                                    std::to_string(type->decimalPlaces()));
            } else if (type->isFloat()) {
                // B3 (r3.90): the shortest text of the value's own type.
                if (type->bits() == 32) {
                    emitRuntimeCall(RuntimeFunction::FileWriteFloat32, "void", "__inox_file_write_f32",
                                    "ptr " + file + ", float " + written);
                } else {
                    emitRuntimeCall(RuntimeFunction::FileWriteFloat, "void", "__inox_file_write_f64",
                                    "ptr " + file + ", double " + written);
                }
            } else if (isUInt64Type(type)) {
                emitRuntimeCall(RuntimeFunction::FileWriteUnsigned, "void", "__inox_file_write_u64",
                                "ptr " + file + ", i64 " + written);
            } else {
                emitRuntimeCall(RuntimeFunction::FileWriteInteger, "void", "__inox_file_write_i64",
                                "ptr " + file + ", i64 " + written);
            }
        }
        if (op == semantic::BuiltinOp::WriteLn) {
            emitRuntimeCall(RuntimeFunction::FileWriteNewline, "void", "__inox_file_write_newline", "ptr " + file);
        }
        return {};
    }
    case semantic::BuiltinOp::Exists:
        return emitRuntimeCall(RuntimeFunction::FileExists, "i1", "__inox_file_exists", "ptr " + value(0));
    case semantic::BuiltinOp::DeleteFile:
        emitRuntimeCall(RuntimeFunction::FileDelete, "void", "__inox_file_delete", "ptr " + value(0));
        return {};
    case semantic::BuiltinOp::RenameFile: {
        const std::string from = value(0);
        const std::string to = value(1);
        emitRuntimeCall(RuntimeFunction::FileRename, "void", "__inox_file_rename", "ptr " + from + ", ptr " + to);
        return {};
    }
    default:
        break;
    }
    throw CodegenError("internal error: not a file call");
}

} // namespace inox::compiler::codegen::detail
