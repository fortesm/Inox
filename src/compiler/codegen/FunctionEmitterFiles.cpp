// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: the file functions of Std.Files and Std.IOUtils (OpenSpec
// changes file-io and module-pairs).
// Each is one call into runtime/FileRuntime.cpp. A File value is an owned
// reference like a String: a function that opens a file returns it owned
// (+1), held for the statement and moved into its variable; the routine's
// owned slots release (close) it at every exit and in the cleanup pad.

#include "FunctionEmitter.h"

#include "LlvmIrEmitter.h"
#include "LlvmText.h"

#include <string>
#include <string_view>

namespace inox::compiler::codegen::detail {

namespace {

enum class FileCall {
    None,
    OpenRead,
    Create,
    OpenAppend,
    ReadLn,
    Eof,
    Close,
    Write,
    WriteLn,
    Exists,
    DeleteFile,
    RenameFile,
};

FileCall fileCallKind(const ast::CallExpression& call)
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
        return FileCall::None;
    }
    const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
    struct Entry {
        std::string_view name;
        FileCall kind;
    };
    static constexpr Entry entries[] = {
        {"OpenRead", FileCall::OpenRead}, {"Create", FileCall::Create},
        {"OpenAppend", FileCall::OpenAppend}, {"ReadLn", FileCall::ReadLn},
        {"Eof", FileCall::Eof}, {"Close", FileCall::Close},
        {"Write", FileCall::Write}, {"WriteLn", FileCall::WriteLn},
        {"Exists", FileCall::Exists}, {"DeleteFile", FileCall::DeleteFile},
        {"RenameFile", FileCall::RenameFile},
    };
    for (const Entry& entry : entries) {
        if (equalsIgnoreCase(name, entry.name)) {
            // ReadLn without an argument is the console prelude function.
            if (entry.kind == FileCall::ReadLn && call.arguments().size() != 1) {
                return FileCall::None;
            }
            return entry.kind;
        }
    }
    return FileCall::None;
}

} // namespace

bool FunctionEmitter::isFileCall(const ast::CallExpression& call) const
{
    if (fileCallKind(call) == FileCall::None) {
        return false;
    }
    // A routine of the program with the same name is called instead; the
    // analyzer rejects that name next to `Use Std.Files`.
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    return signatures_.find(normalize(callee.name())) == signatures_.end();
}

std::string FunctionEmitter::fileCallLlvmType(const ast::CallExpression& call)
{
    switch (fileCallKind(call)) {
    case FileCall::OpenRead:
    case FileCall::Create:
    case FileCall::OpenAppend:
    case FileCall::ReadLn:
        return "ptr";
    case FileCall::Eof:
    case FileCall::Exists:
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
    switch (fileCallKind(call)) {
    case FileCall::OpenRead:
    case FileCall::Create:
    case FileCall::OpenAppend: {
        const FileCall kind = fileCallKind(call);
        const int mode = kind == FileCall::OpenRead ? 0 : kind == FileCall::Create ? 1 : 2;
        const std::string path = value(0);
        const std::string file = emitRuntimeCall(RuntimeFunction::FileOpen, "ptr", "__inox_file_open",
                                                 "ptr " + path + ", i32 " + std::to_string(mode));
        holdOwnedResult(file, semantics_.types().fileType(), "ptr");
        return file;
    }
    case FileCall::ReadLn: {
        const std::string file = value(0);
        const std::string line = emitRuntimeCall(RuntimeFunction::FileReadLn, "ptr", "__inox_file_readln", "ptr " + file);
        holdOwnedResult(line, semantics_.types().stringType(), "ptr");
        return line;
    }
    case FileCall::Eof:
        return emitRuntimeCall(RuntimeFunction::FileEof, "i1", "__inox_file_eof", "ptr " + value(0));
    case FileCall::Close:
        emitRuntimeCall(RuntimeFunction::FileClose, "void", "__inox_file_close", "ptr " + value(0));
        return {};
    case FileCall::Write:
    case FileCall::WriteLn: {
        // The file is evaluated once; the values are written in order, as
        // PutLn prints them (CANON-17).
        const std::string file = value(0);
        for (std::size_t index = 1; index < arguments.size(); ++index) {
            const TypeRef type = typeOf(*arguments[index]);
            const std::string written = value(index);
            if (isStringType(type)) {
                emitRuntimeCall(RuntimeFunction::FileWriteString, "void", "__inox_file_write_str",
                                "ptr " + file + ", ptr " + written);
            } else if (type->isBool()) {
                emitRuntimeCall(RuntimeFunction::FileWriteBool, "void", "__inox_file_write_bool",
                                "ptr " + file + ", i1 zeroext " + written);
            } else if (type->isFloat() && type->bits() == 64) {
                emitRuntimeCall(RuntimeFunction::FileWriteFloat, "void", "__inox_file_write_f64",
                                "ptr " + file + ", double " + written);
            } else {
                emitRuntimeCall(RuntimeFunction::FileWriteInteger, "void", "__inox_file_write_i64",
                                "ptr " + file + ", i64 " + written);
            }
        }
        if (fileCallKind(call) == FileCall::WriteLn) {
            emitRuntimeCall(RuntimeFunction::FileWriteNewline, "void", "__inox_file_write_newline", "ptr " + file);
        }
        return {};
    }
    case FileCall::Exists:
        return emitRuntimeCall(RuntimeFunction::FileExists, "i1", "__inox_file_exists", "ptr " + value(0));
    case FileCall::DeleteFile:
        emitRuntimeCall(RuntimeFunction::FileDelete, "void", "__inox_file_delete", "ptr " + value(0));
        return {};
    case FileCall::RenameFile: {
        const std::string from = value(0);
        const std::string to = value(1);
        emitRuntimeCall(RuntimeFunction::FileRename, "void", "__inox_file_rename", "ptr " + from + ", ptr " + to);
        return {};
    }
    case FileCall::None:
        break;
    }
    throw CodegenError("internal error: not a file call");
}

} // namespace inox::compiler::codegen::detail
