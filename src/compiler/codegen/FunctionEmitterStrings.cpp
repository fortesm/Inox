// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: String ownership: retain/release, owned slots, statement temporaries (CANON-14).

#include "FunctionEmitter.h"

#include "AstQueries.h"
#include "ExceptionSupport.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "MathBuiltins.h"
#include "Operators.h"
#include "../exceptions/ExceptionTypes.h"
#include "../support/ArrayType.h"
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

bool FunctionEmitter::isStringType(std::string_view inoxType)
{
    return equalsIgnoreCase(inoxType, "String");
}

bool FunctionEmitter::typeOwnsStrings(std::string_view inoxType) const
{
    if (isStringType(inoxType)) {
        return true;
    }
    if (const auto layout = arrayLayout(inoxType)) {
        return typeOwnsStrings(layout->elementType);
    }
    if (const StructDefinition* structType = findStruct(structs_, inoxType)) {
        for (const StructFieldInfo& field : structType->fields) {
            if (typeOwnsStrings(field.inoxType)) {
                return true;
            }
        }
    }
    return false;
}

void FunctionEmitter::requireModuleLine(const std::string& line)
{
    if (std::find(stringGlobals_.begin(), stringGlobals_.end(), line) == stringGlobals_.end()) {
        stringGlobals_.push_back(line);
    }
}

void FunctionEmitter::requireStringRuntime()
{
    requireModuleLine("declare void @__inox_str_retain(ptr) nounwind");
    requireModuleLine("declare void @__inox_str_release(ptr) nounwind");
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
    const std::string name = "@.inox.strv." + std::to_string(nextStringLiteral_++);
    std::ostringstream global;
    global << name << " = private unnamed_addr constant { i64, i64, [" << constant.size
           << " x i8] } { i64 -1, i64 " << constant.size - 1 << ", [" << constant.size
           << " x i8] c\"" << constant.bytes << "\" }";
    stringGlobals_.push_back(global.str());
    return name;
}

// Calls `action` with the address of every String inside a value of
// `inoxType` stored at `pointer`: the value itself, struct fields and
// array elements, recursively.
void FunctionEmitter::forEachStringAt(const std::string& pointer, std::string_view inoxType,
                                      const std::function<void(const std::string&)>& action)
{
    if (isStringType(inoxType)) {
        action(pointer);
        return;
    }
    if (const StructDefinition* structType = findStruct(structs_, inoxType)) {
        for (const StructFieldInfo& field : structType->fields) {
            if (!typeOwnsStrings(field.inoxType)) {
                continue;
            }
            const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << fieldPointer << " = getelementptr " << structType->llvmName << ", ptr "
                    << pointer << ", i32 0, i32 " << field.index << '\n';
            forEachStringAt(fieldPointer, field.inoxType, action);
        }
        return;
    }
    std::optional<ArrayLayout> layout = arrayLayout(inoxType);
    if (!layout) {
        return;
    }
    std::int64_t count = 1;
    std::string element;
    while (layout) {
        count *= layout->high - layout->low + 1;
        element = layout->elementType;
        layout = arrayLayout(element);
    }
    if (!typeOwnsStrings(element)) {
        return;
    }
    const std::string storage = llvmTypeForInoxType(element, structs_);
    const std::string label = std::to_string(nextLabel_++);
    const std::string counter = "%strwalk.i" + label;
    output_ << "  " << counter << " = alloca i64\n";
    output_ << "  store i64 0, ptr " << counter << '\n';
    output_ << "  br label %strwalk.cond" << label << "\n\n";
    output_ << "strwalk.cond" << label << ":\n";
    const std::string index = "%tmp" + std::to_string(nextTemporary_++);
    const std::string more = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << index << " = load i64, ptr " << counter << '\n';
    output_ << "  " << more << " = icmp slt i64 " << index << ", " << count << '\n';
    output_ << "  br i1 " << more << ", label %strwalk.body" << label << ", label %strwalk.end" << label
            << "\n\n";
    output_ << "strwalk.body" << label << ":\n";
    const std::string elementPointer = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << elementPointer << " = getelementptr inbounds " << storage << ", ptr " << pointer
            << ", i64 " << index << '\n';
    forEachStringAt(elementPointer, element, action);
    const std::string next = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << next << " = add i64 " << index << ", 1\n";
    output_ << "  store i64 " << next << ", ptr " << counter << '\n';
    output_ << "  br label %strwalk.cond" << label << "\n\n";
    output_ << "strwalk.end" << label << ":\n";
}

void FunctionEmitter::emitStringCallAt(const std::string& pointer, std::string_view inoxType, std::string_view function)
{
    requireStringRuntime();
    forEachStringAt(pointer, inoxType, [&](const std::string& at) {
        const std::string value = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << value << " = load ptr, ptr " << at << '\n';
        output_ << "  call void @" << function << "(ptr " << value << ")\n";
    });
}

void FunctionEmitter::emitRetainAt(const std::string& pointer, std::string_view inoxType)
{
    emitStringCallAt(pointer, inoxType, "__inox_str_retain");
}

void FunctionEmitter::emitReleaseAt(const std::string& pointer, std::string_view inoxType)
{
    emitStringCallAt(pointer, inoxType, "__inox_str_release");
}

// A store into owned storage (variable, field, element, State): the new
// value is retained before the old one is released, so `S := S` is safe.
void FunctionEmitter::emitOwningStore(const std::string& pointer, std::string_view inoxType, const std::string& llvmType,
                                      const std::string& value)
{
    if (!typeOwnsStrings(inoxType)) {
        output_ << "  store " << llvmType << ' ' << value << ", ptr " << pointer << '\n';
        return;
    }
    requireStringRuntime();
    if (isStringType(inoxType)) {
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
    emitRetainAt(copy, inoxType);
    emitReleaseAt(pointer, inoxType);
    output_ << "  store " << llvmType << ' ' << value << ", ptr " << pointer << '\n';
}

// An owned slot starts empty (null) in the entry block, so the exit and
// cleanup releases are valid on every path, also before its declaration.
void FunctionEmitter::registerOwnedSlot(const std::string& slot, std::string_view inoxType, const std::string& llvmType,
                                        bool initialize)
{
    ownedSlots_.push_back(OwnedSlot{slot, std::string(inoxType), llvmType, statementDepth_});
    if (initialize) {
        output_ << "  store " << llvmType << ' ' << (llvmType == "ptr" ? "null" : "zeroinitializer")
                << ", ptr " << slot << " ; inox.entry-init\n";
    }
}

// A call result that owns Strings (+1) is held in a statement temporary
// and released at the end of the statement that produced it.
void FunctionEmitter::holdOwnedResult(const std::string& value, std::string_view inoxType, const std::string& llvmType)
{
    const std::string slot = "%strtmp" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca " << llvmType << '\n';
    registerOwnedSlot(slot, inoxType, llvmType);
    emitReleaseAt(slot, inoxType);
    output_ << "  store " << llvmType << ' ' << value << ", ptr " << slot << '\n';
    pendingTemps_.push_back(ownedSlots_.back());
}

void FunctionEmitter::emitReleaseOwnedSlots()
{
    for (auto it = ownedSlots_.rbegin(); it != ownedSlots_.rend(); ++it) {
        emitReleaseAt(it->pointer, it->inoxType);
    }
}

void FunctionEmitter::flushStatementTemporaries()
{
    while (!pendingTemps_.empty() && pendingTemps_.back().depth >= statementDepth_) {
        const OwnedSlot temp = pendingTemps_.back();
        pendingTemps_.pop_back();
        emitReleaseAt(temp.pointer, temp.inoxType);
        output_ << "  store " << temp.llvmType << ' ' << (temp.llvmType == "ptr" ? "null" : "zeroinitializer")
                << ", ptr " << temp.pointer << '\n';
    }
}

// Return: the result leaves the routine owned (+1).
void FunctionEmitter::emitRetainResult(const std::string& value)
{
    if (!typeOwnsStrings(signature_.returnInoxType)) {
        return;
    }
    requireStringRuntime();
    if (isStringType(signature_.returnInoxType)) {
        output_ << "  call void @__inox_str_retain(ptr " << value << ")\n";
        return;
    }
    const std::string copy = "%strret" + std::to_string(nextTemporary_++);
    output_ << "  " << copy << " = alloca " << signature_.llvmReturnType << '\n';
    output_ << "  store " << signature_.llvmReturnType << ' ' << value << ", ptr " << copy << '\n';
    emitRetainAt(copy, signature_.returnInoxType);
}

// Whether this routine holds owned Strings: a declaration or assignment of
// a String-holding type, or a call whose result holds Strings.
bool FunctionEmitter::scanOwnsStrings() const
{
    bool owns = false;
    forEachStatement(function_.body(), [&](const ast::Statement& statement) {
        if (owns) {
            return;
        }
        if (statement.kind() == ast::AstNodeKind::VarStatement) {
            const auto& variable = static_cast<const ast::VarStatement&>(statement);
            const std::string type = !variable.typeName().empty()
                                         ? semantics_.canonicalTypeName(variable.typeName())
                                         : (variable.initializer() != nullptr ? semanticTypeName(*variable.initializer())
                                                                              : std::string{});
            owns = typeOwnsStrings(type);
        }
        forEachExpressionOf(statement, [&](const ast::Expression& expression) {
            if (owns || expression.kind() != ast::AstNodeKind::CallExpression) {
                if (!owns && expression.kind() == ast::AstNodeKind::BinaryExpression &&
                    static_cast<const ast::BinaryExpression&>(expression).op() == ast::BinaryOperator::Assign) {
                    owns = typeOwnsStrings(semanticTypeName(static_cast<const ast::BinaryExpression&>(expression).right()));
                }
                return;
            }
            owns = typeOwnsStrings(semanticTypeName(expression)) && !isMemberAccessCall(expression) &&
                   !isIndexCall(expression);
        });
    });
    if (signature_.llvmName == "main") {
        for (const StateGlobal& state : stateGlobals_) {
            owns = owns || typeOwnsStrings(state.inoxType);
        }
    }
    return owns;
}

} // namespace inox::compiler::codegen::detail
