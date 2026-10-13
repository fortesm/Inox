// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: struct defaults and construction, places, fields, methods, assignment.

#include "FunctionEmitter.h"

#include "AstQueries.h"
#include "ExceptionAbi.h"
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

// CANON-9: whether a struct type, or a struct nested in it, has a field
// default other than zero (a String field starts as "", not null).
bool FunctionEmitter::structHasDefaults(const StructDefinition& structType) const
{
    for (const StructFieldInfo& field : structType.fields) {
        if (field.hasDefault || isStringType(field.inoxType)) {
            return true;
        }
        if (defaultedStruct(field.inoxType) != nullptr || stringArray(field.inoxType)) {
            return true;
        }
    }
    return false;
}

// Whether an array type's innermost element is a String.
bool FunctionEmitter::stringArray(std::string_view inoxType)
{
    std::string type(inoxType);
    bool isArray = false;
    while (const auto layout = arrayLayout(type)) {
        type = layout->elementType;
        isArray = true;
    }
    return isArray && isStringType(type);
}

// The struct with non-zero defaults that a value of `inoxType` holds: the
// type itself, or the innermost element of an array type; else null.
const StructDefinition* FunctionEmitter::defaultedStruct(std::string_view inoxType) const
{
    std::string type(inoxType);
    while (const auto layout = arrayLayout(type)) {
        type = layout->elementType;
    }
    const StructDefinition* structType = findStruct(structs_, type);
    return structType != nullptr && structHasDefaults(*structType) ? structType : nullptr;
}

// Writes the type defaults of a struct at `pointer` (CANON-5 rule 4):
// zero, then each field default, recursively into nested structs.
void FunctionEmitter::emitStructDefaults(const std::string& pointer, const StructDefinition& structType)
{
    output_ << "  store " << structType.llvmName << " zeroinitializer, ptr " << pointer << '\n';
    emitStructFieldDefaults(pointer, structType);
}

void FunctionEmitter::emitStructFieldDefaults(const std::string& pointer, const StructDefinition& structType)
{
    for (const StructFieldInfo& field : structType.fields) {
        const StructDefinition* defaulted = field.hasDefault ? nullptr : defaultedStruct(field.inoxType);
        if (!field.hasDefault && defaulted == nullptr && !isStringType(field.inoxType) &&
            !stringArray(field.inoxType)) {
            continue;
        }
        const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << fieldPointer << " = getelementptr " << structType.llvmName << ", ptr "
                << pointer << ", i32 0, i32 " << field.index << '\n';
        if (field.isStruct) {
            emitStructFieldDefaults(fieldPointer, *defaulted);
            continue;
        }
        if (defaulted != nullptr || stringArray(field.inoxType)) {
            // An array field of structs or Strings: every element gets
            // the defaults.
            emitArrayElementDefaults(fieldPointer, field.inoxType);
            continue;
        }
        if (isStringType(field.inoxType)) {
            // A default is an immortal literal (kept quoted by the parser),
            // so storing it needs no retain.
            const std::string& text = field.defaultValue;
            const std::string value = field.hasDefault && text.size() >= 2
                                          ? emitStringValue(std::string_view(text).substr(1, text.size() - 2))
                                          : emptyStringValue();
            output_ << "  store ptr " << value << ", ptr " << fieldPointer << '\n';
            continue;
        }
        output_ << "  store " << field.llvmType << ' '
                << llvmDefaultLiteral(field.defaultValue, field.llvmType) << ", ptr " << fieldPointer
                << '\n';
    }
}

// An array of structs with defaults starts with every element at its
// struct's defaults. The array was zeroed; nested arrays are contiguous,
// so the elements are walked as one flat run.
void FunctionEmitter::emitArrayElementDefaults(const std::string& pointer, std::string_view arrayType)
{
    std::optional<ArrayLayout> layout = arrayLayout(arrayType);
    std::int64_t count = 1;
    while (layout) {
        count *= layout->high - layout->low + 1;
        const std::string element = layout->elementType;
        if (const auto inner = arrayLayout(element)) {
            layout = inner;
            continue;
        }
        const StructDefinition* structType = findStruct(structs_, element);
        if (!isStringType(element) && (structType == nullptr || !structHasDefaults(*structType))) {
            return;
        }
        const std::string label = std::to_string(nextLabel_++);
        const std::string counter = "%elemdefault.i" + label;
        output_ << "  " << counter << " = alloca i64\n";
        output_ << "  store i64 0, ptr " << counter << '\n';
        output_ << "  br label %elemdefault.cond" << label << "\n\n";
        output_ << "elemdefault.cond" << label << ":\n";
        const std::string index = "%tmp" + std::to_string(nextTemporary_++);
        const std::string more = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << index << " = load i64, ptr " << counter << '\n';
        output_ << "  " << more << " = icmp slt i64 " << index << ", " << count << '\n';
        output_ << "  br i1 " << more << ", label %elemdefault.body" << label
                << ", label %elemdefault.end" << label << "\n\n";
        output_ << "elemdefault.body" << label << ":\n";
        const std::string elementPointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << elementPointer << " = getelementptr inbounds "
                << (structType != nullptr ? structType->llvmName : std::string("ptr"))
                << ", ptr " << pointer << ", i64 " << index << '\n';
        if (structType != nullptr) {
            emitStructFieldDefaults(elementPointer, *structType);
        } else {
            output_ << "  store ptr " << emptyStringValue() << ", ptr " << elementPointer << '\n';
        }
        const std::string next = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << next << " = add i64 " << index << ", 1\n";
        output_ << "  store i64 " << next << ", ptr " << counter << '\n';
        output_ << "  br label %elemdefault.cond" << label << "\n\n";
        output_ << "elemdefault.end" << label << ":\n";
        return;
    }
}

// CANON-9 named construction `T(Field := Value, ...)`: the type defaults,
// then each named value in source order.
std::string FunctionEmitter::emitConstruction(const ast::CallExpression& call, const StructDefinition& structType)
{
    const std::string slot = "%construct" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca " << structType.llvmName << '\n';
    emitStructDefaults(slot, structType);
    for (const auto& argument : call.arguments()) {
        const auto& named = static_cast<const ast::BinaryExpression&>(*argument);
        const auto& fieldName = static_cast<const ast::IdentifierExpression&>(named.left());
        const StructFieldInfo* field = findStructField(structType, fieldName.name());
        if (field == nullptr) {
            throw CodegenError("unknown field in construction: " + fieldName.name());
        }
        const std::string value = emitExpression(named.right());
        const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << fieldPointer << " = getelementptr " << structType.llvmName << ", ptr " << slot
                << ", i32 0, i32 " << field->index << '\n';
        output_ << "  store " << field->llvmType << ' ' << value << ", ptr " << fieldPointer << '\n';
    }
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << result << " = load " << structType.llvmName << ", ptr " << slot << '\n';
    return result;
}

FunctionEmitter::Place FunctionEmitter::emitPlace(const ast::Expression& expression)
{
    if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto local = locals_.find(normalize(static_cast<const ast::IdentifierExpression&>(expression).name()));
        if (local != locals_.end()) {
            return Place{local->second.slot, local->second.inoxType};
        }
    }
    if (isIndexCall(expression)) {
        const ElementAddress element = emitElementAddress(static_cast<const ast::CallExpression&>(expression));
        return Place{element.pointer, element.elementType};
    }
    if (isMemberAccessCall(expression)) {
        const auto& member = static_cast<const ast::CallExpression&>(expression);
        if (const auto field = memberField(member)) {
            const Place base = emitPlace(*member.arguments()[0]);
            const std::string pointer = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << pointer << " = getelementptr " << field->first->llvmName << ", ptr "
                    << base.pointer << ", i32 0, i32 " << field->second->index << '\n';
            return Place{pointer, field->second->inoxType};
        }
    }
    const std::string type = semanticTypeName(expression);
    const std::string llvmType = llvmTypeForInoxType(type, structs_);
    if (llvmType.empty()) {
        throw CodegenUnsupported("this value has no storage in the LLVM backend");
    }
    const std::string value = emitExpression(expression);
    const std::string slot = "%placetmp" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca " << llvmType << '\n';
    output_ << "  store " << llvmType << ' ' << value << ", ptr " << slot << '\n';
    return Place{slot, type};
}

// The struct and field a `__member(Base, Name)` names, when Base is a
// struct value and Name one of its fields (otherwise it is a method).
std::optional<std::pair<const StructDefinition*, const StructFieldInfo*>>
FunctionEmitter::memberField(const ast::CallExpression& member) const
{
    if (member.arguments().size() != 2 ||
        member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
        return std::nullopt;
    }
    const StructDefinition* structType = findStruct(structs_, placeTypeName(*member.arguments()[0]));
    if (structType == nullptr) {
        return std::nullopt;
    }
    const auto& fieldName = static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
    const StructFieldInfo* field = findStructField(*structType, fieldName.name());
    if (field == nullptr) {
        return std::nullopt;
    }
    return std::make_pair(structType, field);
}

// The Inox type of a place or value: a local's declared type, else the
// semantic type.
std::string FunctionEmitter::placeTypeName(const ast::Expression& expression) const
{
    if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto local = locals_.find(normalize(static_cast<const ast::IdentifierExpression&>(expression).name()));
        if (local != locals_.end()) {
            return local->second.inoxType;
        }
    }
    if (isMemberAccessCall(expression)) {
        if (const auto field = memberField(static_cast<const ast::CallExpression&>(expression))) {
            return field->second->inoxType;
        }
    }
    return semanticTypeName(expression);
}

FunctionEmitter::FieldAddress FunctionEmitter::emitMemberAddress(const ast::CallExpression& call)
{
    if (!memberField(call)) {
        // Not a field: the caller treats it as a zero-argument method.
        throw CodegenError("not a struct field");
    }
    const Place place = emitPlace(call);
    const auto field = memberField(call);
    return FieldAddress{place.pointer, field->second->llvmType};
}

bool FunctionEmitter::isMemberAccessCall(const ast::Expression& expression)
{
    if (expression.kind() != ast::AstNodeKind::CallExpression) {
        return false;
    }
    const auto& call = static_cast<const ast::CallExpression&>(expression);
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
        return false;
    }
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    return equalsIgnoreCase(callee.name(), "__member");
}

FunctionEmitter::MethodCallTarget FunctionEmitter::resolveMethodCall(const ast::CallExpression& call)
{
    if (!isMemberAccessCall(call.callee())) {
        throw CodegenError("LLVM emission expected a method call");
    }

    const auto& member = static_cast<const ast::CallExpression&>(call.callee());
    if (member.arguments().size() != 2 ||
        member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
        throw CodegenUnsupported("LLVM emission currently supports only local method calls");
    }

    const auto& method =
        static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
    const std::string receiverType = placeTypeName(*member.arguments()[0]);
    if (findStruct(structs_, receiverType) == nullptr) {
        throw CodegenUnsupported("LLVM emission method receiver is not a struct");
    }

    const std::string qualifiedName = receiverType + "." + method.name();
    const auto signature = signatures_.find(normalize(qualifiedName));
    if (signature == signatures_.end()) {
        throw CodegenUnsupported("unknown method for LLVM emission: " + qualifiedName);
    }
    if (signature->second.parameters.empty() ||
        signature->second.parameters.front().llvmType != "ptr") {
        throw CodegenUnsupported("LLVM method emission requires an explicit struct receiver parameter");
    }
    if (call.arguments().size() + 1 != signature->second.parameters.size()) {
        throw CodegenUnsupported("unsupported method argument count: " + qualifiedName);
    }

    return MethodCallTarget{&signature->second, emitPlace(*member.arguments()[0]).pointer};
}

FunctionEmitter::MethodCallTarget FunctionEmitter::resolveNoArgumentMethodAccess(const ast::CallExpression& member)
{
    if (!isMemberAccessCall(member)) {
        throw CodegenError("LLVM emission expected member access");
    }
    if (member.arguments().size() != 2 ||
        member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
        throw CodegenUnsupported("LLVM emission currently supports only local zero-argument method calls");
    }

    const auto& method =
        static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
    const std::string receiverType = placeTypeName(*member.arguments()[0]);
    if (findStruct(structs_, receiverType) == nullptr) {
        throw CodegenUnsupported("LLVM emission method receiver is not a struct");
    }

    const std::string qualifiedName = receiverType + "." + method.name();
    const auto signature = signatures_.find(normalize(qualifiedName));
    if (signature == signatures_.end()) {
        throw CodegenUnsupported("unknown field or zero-argument method for LLVM emission: " + qualifiedName);
    }
    if (signature->second.parameters.size() != 1 ||
        signature->second.parameters.front().llvmType != "ptr") {
        throw CodegenUnsupported("LLVM zero-argument method emission requires only an explicit struct receiver parameter");
    }
    return MethodCallTarget{&signature->second, emitPlace(*member.arguments()[0]).pointer};
}

std::string FunctionEmitter::emitNoArgumentMethodCall(const ast::CallExpression& member, bool requireValue)
{
    const MethodCallTarget target = resolveNoArgumentMethodAccess(member);
    const FunctionSignature& signature = *target.signature;

    return emitUserCall(signature, {target.receiverPointer}, requireValue);
}

std::string FunctionEmitter::emitMethodCall(const ast::CallExpression& call, bool requireValue)
{
    const MethodCallTarget target = resolveMethodCall(call);
    const FunctionSignature& signature = *target.signature;

    std::vector<std::string> arguments;
    arguments.reserve(call.arguments().size() + 1);
    arguments.push_back(target.receiverPointer);
    for (std::string& value : emitArguments(signature, call.arguments(), 1)) {
        arguments.push_back(std::move(value));
    }

    return emitUserCall(signature, arguments, requireValue);
}

void FunctionEmitter::emitLocalAssignment(const ast::Expression& expression)
{
    if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
        throw CodegenUnsupported(
            "LLVM emission currently supports only simple local assignments");
    }

    const auto& assignment = static_cast<const ast::BinaryExpression&>(expression);
    if (assignment.op() != ast::BinaryOperator::Assign) {
        throw CodegenUnsupported(
            "LLVM emission currently supports only simple local assignments");
    }

    if (assignment.left().kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto& identifier =
            static_cast<const ast::IdentifierExpression&>(assignment.left());
        const auto local = locals_.find(normalize(identifier.name()));
        if (local == locals_.end()) {
            // CANON-5 / A6 / A7: first appearance of `Name := Expr` is an
            // inline declaration with an inferred type. The semantic layer
            // already validated this; here we allocate a fresh local and
            // store the initializer, mirroring emitLocalVariable.
            emitLocalVariable(identifier.name(), assignment.right());
            return;
        }

        const std::string value = emitExpression(assignment.right());
        emitOwningStore(local->second.slot, local->second.inoxType, local->second.llvmType, value);
        return;
    }

    if (isIndexCall(assignment.left())) {
        const ElementAddress element = emitElementAddress(
            static_cast<const ast::CallExpression&>(assignment.left()));
        const std::string value = emitExpression(assignment.right());
        const std::string stored = narrowForStorage(value, element);
        emitOwningStore(element.pointer, element.elementType, element.storage, stored);
        return;
    }

    if (isMemberAccessCall(assignment.left())) {
        const auto& member = static_cast<const ast::CallExpression&>(assignment.left());
        const FieldAddress field = emitMemberAddress(member);
        const std::string value = emitExpression(assignment.right());
        const auto info = memberField(member);
        emitOwningStore(field.pointer, info ? info->second->inoxType : std::string{}, field.llvmType, value);
        return;
    }

    throw CodegenUnsupported(
        "LLVM emission currently supports only local variable or field assignments");
}

} // namespace inox::compiler::codegen::detail
