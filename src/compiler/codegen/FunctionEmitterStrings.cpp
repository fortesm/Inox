// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: String ownership: retain/release, owned slots, statement temporaries (CANON-14).

#include "FunctionEmitter.h"

#include "AstQueries.h"
#include "ExceptionAbi.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "MathBuiltins.h"
#include "Operators.h"
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

// The types whose values are references owned by the routine that holds
// them: String (CANON-14), File (file-io), owners (safe-pointers: an owner
// frees its object), and aggregates holding any of them (a File is never
// inside an aggregate).
bool FunctionEmitter::typeOwnsReferences(TypeRef type) const
{
    if (isStringType(type) || isFileType(type) || isOwnerType(type)) {
        return true;
    }
    if (isArrayType(type)) {
        return typeOwnsReferences(type->element());
    }
    if (isStructType(type)) {
        for (const types::StructField& field : type->fields()) {
            if (typeOwnsReferences(field.type)) {
                return true;
            }
        }
    }
    return false;
}

void FunctionEmitter::requireModuleLine(const std::string& line)
{
    if (std::find(module_.moduleLines.begin(), module_.moduleLines.end(), line) == module_.moduleLines.end()) {
        module_.moduleLines.push_back(line);
    }
}

void FunctionEmitter::requireRuntime(RuntimeFunction function)
{
    requireModuleLine(std::string(runtimeDeclaration(function)));
    module_.usesInoxRuntime = true;
}

void FunctionEmitter::requireStringRuntime()
{
    requireRuntime(RuntimeFunction::StringRetain);
    requireRuntime(RuntimeFunction::StringRelease);
}

// The shared immortal "" of this module.
std::string FunctionEmitter::emptyStringValue()
{
    requireModuleLine("@.inox.strv.empty = private unnamed_addr constant { i64, i64, [1 x i8] } "
                      "{ i64 -1, i64 0, [1 x i8] zeroinitializer }");
    return "@.inox.strv.empty";
}

// A String literal as a value: an immortal block in the module.
std::string FunctionEmitter::emitStringValue(std::string_view text)
{
    const LlvmStringConstant constant = llvmStringConstant(text);
    if (constant.size == 1) {
        return emptyStringValue();
    }
    const std::string name = "@.inox.strv." + std::to_string(module_.nextStringLiteral++);
    std::ostringstream global;
    global << name << " = private unnamed_addr constant { i64, i64, [" << constant.size
           << " x i8] } { i64 -1, i64 " << constant.size - 1 << ", [" << constant.size
           << " x i8] c\"" << constant.bytes << "\" }";
    module_.moduleLines.push_back(global.str());
    return name;
}

// Calls `action` with the address of every String inside a value of `type`
// stored at `pointer`: the value itself, struct fields and array elements,
// recursively.
void FunctionEmitter::forEachStringAt(const std::string& pointer, TypeRef type,
                                      const std::function<void(const std::string&)>& action)
{
    if (isStringType(type)) {
        action(pointer);
        return;
    }
    if (isStructType(type)) {
        for (std::size_t index = 0; index < type->fields().size(); ++index) {
            if (!typeOwnsReferences(type->fields()[index].type)) {
                continue;
            }
            const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << fieldPointer << " = getelementptr " << llvmStructName(type) << ", ptr " << pointer
                    << ", i32 0, i32 " << index << '\n';
            forEachStringAt(fieldPointer, type->fields()[index].type, action);
        }
        return;
    }
    if (!isArrayType(type)) {
        return;
    }
    const FlatArray flat = flattenArray(type);
    if (!typeOwnsReferences(flat.element)) {
        return;
    }
    const std::string storage = llvmValueType(flat.element);
    const std::string label = std::to_string(nextLabel_++);
    const std::string counter = "%strwalk.i" + label;
    output_ << "  " << counter << " = alloca i64\n";
    output_ << "  store i64 0, ptr " << counter << '\n';
    output_ << "  br label %strwalk.cond" << label << "\n\n";
    output_ << "strwalk.cond" << label << ":\n";
    const std::string index = "%tmp" + std::to_string(nextTemporary_++);
    const std::string more = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << index << " = load i64, ptr " << counter << '\n';
    output_ << "  " << more << " = icmp slt i64 " << index << ", " << flat.count << '\n';
    output_ << "  br i1 " << more << ", label %strwalk.body" << label << ", label %strwalk.end" << label
            << "\n\n";
    output_ << "strwalk.body" << label << ":\n";
    const std::string elementPointer = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << elementPointer << " = getelementptr inbounds " << storage << ", ptr " << pointer
            << ", i64 " << index << '\n';
    forEachStringAt(elementPointer, flat.element, action);
    const std::string next = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << next << " = add i64 " << index << ", 1\n";
    output_ << "  store i64 " << next << ", ptr " << counter << '\n';
    output_ << "  br label %strwalk.cond" << label << "\n\n";
    output_ << "strwalk.end" << label << ":\n";
}

void FunctionEmitter::emitStringCallAt(const std::string& pointer, TypeRef type, std::string_view function)
{
    requireStringRuntime();
    forEachStringAt(pointer, type, [&](const std::string& at) {
        const std::string value = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << value << " = load ptr, ptr " << at << '\n';
        output_ << "  call void @" << function << "(ptr " << value << ")\n";
    });
}

void FunctionEmitter::emitFileCallAt(const std::string& pointer, RuntimeFunction function)
{
    requireRuntime(function);
    const std::string value = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << value << " = load ptr, ptr " << pointer << '\n';
    output_ << "  call void @"
            << (function == RuntimeFunction::FileRetain ? "__inox_file_retain" : "__inox_file_release") << "(ptr "
            << value << ")\n";
}

void FunctionEmitter::emitRetainAt(const std::string& pointer, TypeRef type)
{
    // An owner is never copied, only moved: nothing to retain.
    if (isOwnerType(type)) {
        return;
    }
    if (isFileType(type)) {
        emitFileCallAt(pointer, RuntimeFunction::FileRetain);
        return;
    }
    emitStringCallAt(pointer, type, "__inox_str_retain");
}

// Releasing the last reference of a File closes it (FileRuntime.cpp).
void FunctionEmitter::emitReleaseAt(const std::string& pointer, TypeRef type)
{
    if (isFileType(type)) {
        emitFileCallAt(pointer, RuntimeFunction::FileRelease);
        return;
    }
    if (isMoveOnly(type)) {
        // The Strings it holds, then its owners (their objects are freed).
        if (!isOwnerType(type)) {
            emitStringCallAt(pointer, type, "__inox_str_release");
        }
        emitOwnerFreesAt(pointer, type);
        return;
    }
    emitStringCallAt(pointer, type, "__inox_str_release");
}

void FunctionEmitter::emitReleaseSlot(const OwnedSlot& slot)
{
    if (slot.tombstone) {
        emitTombstoneFree(slot.pointer);
        return;
    }
    emitReleaseAt(slot.pointer, slot.type);
}

// A store into owned storage (variable, field, element, State): the new
// value is retained before the old one is released, so `S := S` is safe.
void FunctionEmitter::emitOwningStore(const std::string& pointer, TypeRef type, const std::string& llvmType,
                                      const std::string& value)
{
    // Safe pointers: a move-only value is moved in. The old value is
    // released (its owners free their objects), and the temporary that held
    // the fresh value is disarmed: nothing is copied or retained.
    if (isMoveOnly(type)) {
        emitReleaseAt(pointer, type);
        output_ << "  store " << llvmType << ' ' << value << ", ptr " << pointer << '\n';
        disarmFresh(value);
        return;
    }
    if (!typeOwnsReferences(type)) {
        output_ << "  store " << llvmType << ' ' << value << ", ptr " << pointer << '\n';
        return;
    }
    // A File is only ever moved in from the call that opened it (F6): the
    // store keeps the new handle and releases (closes) the previous one.
    if (isFileType(type)) {
        requireRuntime(RuntimeFunction::FileRetain);
        requireRuntime(RuntimeFunction::FileRelease);
        const std::string old = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  call void @__inox_file_retain(ptr " << value << ")\n";
        output_ << "  " << old << " = load ptr, ptr " << pointer << '\n';
        output_ << "  store ptr " << value << ", ptr " << pointer << '\n';
        output_ << "  call void @__inox_file_release(ptr " << old << ")\n";
        return;
    }
    requireStringRuntime();
    if (isStringType(type)) {
        const std::string old = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  call void @__inox_str_retain(ptr " << value << ")\n";
        output_ << "  " << old << " = load ptr, ptr " << pointer << '\n';
        output_ << "  store ptr " << value << ", ptr " << pointer << '\n';
        output_ << "  call void @__inox_str_release(ptr " << old << ")\n";
        return;
    }
    const std::string copy = "%strcopy" + std::to_string(nextTemporary_++);
    output_ << "  " << copy << " = alloca " << llvmType << '\n';
    output_ << "  store " << llvmType << ' ' << value << ", ptr " << copy << '\n';
    emitRetainAt(copy, type);
    emitReleaseAt(pointer, type);
    output_ << "  store " << llvmType << ' ' << value << ", ptr " << pointer << '\n';
}

// An owned slot starts empty (null) in the entry block, so the exit and
// cleanup releases are valid on every path, also before its declaration.
void FunctionEmitter::registerOwnedSlot(const std::string& slot, TypeRef type, const std::string& llvmType,
                                        bool initialize)
{
    ownedSlots_.push_back(OwnedSlot{slot, type, llvmType, statementDepth_});
    if (initialize) {
        output_ << "  store " << llvmType << ' ' << (llvmType == "ptr" ? "null" : "zeroinitializer")
                << ", ptr " << slot << " ; inox.entry-init\n";
    }
}

// A call result that owns Strings (+1) is held in a statement temporary
// and released at the end of the statement that produced it.
void FunctionEmitter::holdOwnedResult(const std::string& value, TypeRef type, const std::string& llvmType)
{
    const std::string slot = "%strtmp" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca " << llvmType << '\n';
    registerOwnedSlot(slot, type, llvmType);
    emitReleaseAt(slot, type);
    output_ << "  store " << llvmType << ' ' << value << ", ptr " << slot << '\n';
    pendingTemps_.push_back(ownedSlots_.back());
    if (isMoveOnly(type)) {
        freshTemps_[value] = FreshTemp{slot, llvmType};
    }
}

void FunctionEmitter::emitReleaseOwnedSlots()
{
    for (auto it = ownedSlots_.rbegin(); it != ownedSlots_.rend(); ++it) {
        emitReleaseSlot(*it);
    }
}

void FunctionEmitter::flushStatementTemporaries()
{
    while (!pendingTemps_.empty() && pendingTemps_.back().depth >= statementDepth_) {
        const OwnedSlot temp = pendingTemps_.back();
        pendingTemps_.pop_back();
        emitReleaseSlot(temp);
        output_ << "  store " << temp.llvmType << ' ' << (temp.llvmType == "ptr" ? "null" : "zeroinitializer")
                << ", ptr " << temp.pointer << '\n';
    }
}

// Return: the result leaves the routine owned (+1).
void FunctionEmitter::emitRetainResult(const std::string& value)
{
    if (!typeOwnsReferences(signature_.returnType)) {
        return;
    }
    // A move-only result leaves the routine: its temporary lets it go.
    if (isMoveOnly(signature_.returnType)) {
        disarmFresh(value);
        return;
    }
    requireStringRuntime();
    if (isStringType(signature_.returnType)) {
        output_ << "  call void @__inox_str_retain(ptr " << value << ")\n";
        return;
    }
    const std::string copy = "%strret" + std::to_string(nextTemporary_++);
    output_ << "  " << copy << " = alloca " << signature_.llvmReturnType << '\n';
    output_ << "  store " << signature_.llvmReturnType << ' ' << value << ", ptr " << copy << '\n';
    emitRetainAt(copy, signature_.returnType);
}

// Whether this routine holds owned Strings: a declaration or assignment of
// a String-holding type, or a call whose result holds Strings.
bool FunctionEmitter::scanOwnsReferences() const
{
    // Safe pointers: owned parameters and tombstones are released too.
    bool owns = !tombstoneLocals_.empty() || !ownedSlots_.empty();
    forEachStatement(function_.body(), [&](const ast::Statement& statement) {
        if (owns) {
            return;
        }
        if (statement.kind() == ast::AstNodeKind::VarStatement) {
            const auto& variable = static_cast<const ast::VarStatement&>(statement);
            owns = typeOwnsReferences(semantics_.declaredType(variable));
        }
        forEachExpressionOf(statement, [&](const ast::Expression& expression) {
            if (owns || expression.kind() != ast::AstNodeKind::CallExpression) {
                if (!owns && expression.kind() == ast::AstNodeKind::BinaryExpression &&
                    static_cast<const ast::BinaryExpression&>(expression).op() == ast::BinaryOperator::Assign) {
                    owns = typeOwnsReferences(typeOf(static_cast<const ast::BinaryExpression&>(expression).right()));
                }
                return;
            }
            owns = typeOwnsReferences(typeOf(expression)) && !isMemberAccessCall(expression) &&
                   !isIndexCall(expression);
        });
    });
    if (signature_.llvmName == "main") {
        for (const StateGlobal& state : stateGlobals_) {
            owns = owns || typeOwnsReferences(state.type);
        }
    }
    return owns;
}

} // namespace inox::compiler::codegen::detail
