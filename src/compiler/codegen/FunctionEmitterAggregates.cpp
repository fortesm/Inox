// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: struct defaults and construction, places, fields, methods, assignment.

#include "FunctionEmitter.h"
#include "../support/CharLiteral.h"

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

// CANON-9: whether a struct type, or a struct nested in it, has a field
// default other than zero (a String field starts as "", not null).
bool FunctionEmitter::structHasDefaults(TypeRef structType)
{
    for (const types::StructField& field : structType->fields()) {
        if (field.hasDefault || isStringType(field.type)) {
            return true;
        }
        if (defaultedStruct(field.type) != nullptr || stringArray(field.type)) {
            return true;
        }
    }
    return false;
}

// Whether an array type's innermost element is a String.
bool FunctionEmitter::stringArray(TypeRef type)
{
    return isArrayType(type) && isStringType(flattenArray(type).element);
}

// The struct with non-zero defaults that a value of `type` holds: the type
// itself, or the innermost element of an array type; else null.
TypeRef FunctionEmitter::defaultedStruct(TypeRef type)
{
    const TypeRef element = flattenArray(type).element;
    return isStructType(element) && structHasDefaults(element) ? element : nullptr;
}

// Writes the type defaults of a struct at `pointer` (CANON-5 rule 4):
// zero, then each field default, recursively into nested structs.
void FunctionEmitter::emitStructDefaults(const std::string& pointer, TypeRef structType)
{
    output_ << "  store " << llvmStructName(structType) << " zeroinitializer, ptr " << pointer << '\n';
    emitStructFieldDefaults(pointer, structType);
}

void FunctionEmitter::emitStructFieldDefaults(const std::string& pointer, TypeRef structType)
{
    for (std::size_t index = 0; index < structType->fields().size(); ++index) {
        const FieldLayout layout = fieldLayout(structType, index);
        const types::StructField& field = *layout.field;
        const TypeRef defaulted = field.hasDefault ? nullptr : defaultedStruct(field.type);
        if (!field.hasDefault && defaulted == nullptr && !isStringType(field.type) && !stringArray(field.type)) {
            continue;
        }
        const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << fieldPointer << " = getelementptr " << llvmStructName(structType) << ", ptr "
                << pointer << ", i32 0, i32 " << index << '\n';
        if (field.hasDefault && field.fillDefault) {
            emitArrayFillDefault(fieldPointer, field.type, field.defaultValue);
            continue;
        }
        if (field.type->isStruct()) {
            emitStructFieldDefaults(fieldPointer, defaulted);
            continue;
        }
        if (defaulted != nullptr || stringArray(field.type)) {
            // An array field of structs or Strings: every element gets
            // the defaults.
            emitArrayElementDefaults(fieldPointer, field.type);
            continue;
        }
        if (isStringType(field.type)) {
            // A default is an immortal literal (kept quoted by the parser),
            // so storing it needs no retain.
            const std::string& text = field.defaultValue;
            const std::string value = field.hasDefault && text.size() >= 2
                                          ? emitStringValue(std::string_view(text).substr(1, text.size() - 2))
                                          : emptyStringValue();
            output_ << "  store ptr " << value << ", ptr " << fieldPointer << '\n';
            continue;
        }
        if (field.type->isChar()) {
            // The code point of the default literal (char-values).
            output_ << "  store i64 " << support::decodeCharLiteral(field.defaultValue).code << ", ptr "
                    << fieldPointer << '\n';
            continue;
        }
        if (field.type->isEnum()) {
            // The ordinal of the default literal (CANON-13).
            std::size_t ordinal = 0;
            while (ordinal < field.type->literals().size() &&
                   !equalsIgnoreCase(field.type->literals()[ordinal], field.defaultValue)) {
                ++ordinal;
            }
            output_ << "  store i64 " << ordinal << ", ptr " << fieldPointer << '\n';
            continue;
        }
        output_ << "  store " << layout.llvmType << ' ' << llvmDefaultLiteral(field.defaultValue, layout.llvmType)
                << ", ptr " << fieldPointer << '\n';
    }
}

// An array of structs with defaults, or of Strings, starts with every
// element at its defaults. The array was zeroed; nested arrays are
// contiguous, so the elements are walked as one flat run. Any other type
// keeps its zero.
void FunctionEmitter::emitArrayElementDefaults(const std::string& pointer, TypeRef arrayType)
{
    if (!isArrayType(arrayType)) {
        return;
    }
    const FlatArray flat = flattenArray(arrayType);
    const TypeRef structType = isStructType(flat.element) ? flat.element : nullptr;
    if (!isStringType(flat.element) && (structType == nullptr || !structHasDefaults(structType))) {
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
    output_ << "  " << more << " = icmp slt i64 " << index << ", " << flat.count << '\n';
    output_ << "  br i1 " << more << ", label %elemdefault.body" << label << ", label %elemdefault.end" << label
            << "\n\n";
    output_ << "elemdefault.body" << label << ":\n";
    const std::string elementPointer = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << elementPointer << " = getelementptr inbounds "
            << (structType != nullptr ? llvmStructName(structType) : std::string("ptr")) << ", ptr " << pointer
            << ", i64 " << index << '\n';
    if (structType != nullptr) {
        emitStructFieldDefaults(elementPointer, structType);
    } else {
        output_ << "  store ptr " << emptyStringValue() << ", ptr " << elementPointer << '\n';
    }
    const std::string next = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << next << " = add i64 " << index << ", 1\n";
    output_ << "  store i64 " << next << ", ptr " << counter << '\n';
    output_ << "  br label %elemdefault.cond" << label << "\n\n";
    output_ << "elemdefault.end" << label << ":\n";
}

// r3.82: an array field with the default `Fill(Literal)` gets the literal in
// every element (an immortal String literal needs no retain; None and the
// empty set are the zero the field already holds).
void FunctionEmitter::emitArrayFillDefault(const std::string& pointer, TypeRef arrayType, const std::string& text)
{
    const FlatArray flat = flattenArray(arrayType);
    const TypeRef element = flat.element;
    if (element->isOptionOwn() || element->isSet()) {
        return;
    }
    const std::string storage = llvmStorageType(element);
    std::string value;
    if (isStringType(element)) {
        value = emitStringValue(std::string_view(text).substr(1, text.size() - 2));
    } else if (element->isChar()) {
        value = std::to_string(support::decodeCharLiteral(text).code);
    } else if (element->isEnum()) {
        std::size_t ordinal = 0;
        while (ordinal < element->literals().size() && !equalsIgnoreCase(element->literals()[ordinal], text)) {
            ++ordinal;
        }
        value = std::to_string(ordinal);
    } else {
        value = llvmDefaultLiteral(text, storage);
    }
    const std::string label = std::to_string(nextLabel_++);
    const std::string counter = "%filldefault.i" + label;
    output_ << "  " << counter << " = alloca i64\n";
    output_ << "  store i64 0, ptr " << counter << '\n';
    output_ << "  br label %filldefault.cond" << label << "\n\n";
    output_ << "filldefault.cond" << label << ":\n";
    const std::string index = "%tmp" + std::to_string(nextTemporary_++);
    const std::string more = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << index << " = load i64, ptr " << counter << '\n';
    output_ << "  " << more << " = icmp slt i64 " << index << ", " << flat.count << '\n';
    output_ << "  br i1 " << more << ", label %filldefault.body" << label << ", label %filldefault.end" << label
            << "\n\n";
    output_ << "filldefault.body" << label << ":\n";
    const std::string elementPointer = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << elementPointer << " = getelementptr inbounds " << storage << ", ptr " << pointer << ", i64 "
            << index << '\n';
    output_ << "  store " << storage << ' ' << value << ", ptr " << elementPointer << '\n';
    const std::string next = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << next << " = add i64 " << index << ", 1\n";
    output_ << "  store i64 " << next << ", ptr " << counter << '\n';
    output_ << "  br label %filldefault.cond" << label << "\n\n";
    output_ << "filldefault.end" << label << ":\n";
}

// The struct a construction `T(Field := Value, ...)` builds, as the analysis
// recorded it (`TPair(...)` builds an instance, `TPair<String, Int64>`), or
// null when the call is not a construction.
TypeRef FunctionEmitter::constructedStruct(const ast::CallExpression& call) const
{
    return semantics_.constructedStruct(call);
}

// CANON-9 named construction `T(Field := Value, ...)`: the type defaults,
// then each named value in source order.
std::string FunctionEmitter::emitConstruction(const ast::CallExpression& call, TypeRef structType)
{
    const std::string structName = llvmStructName(structType);
    const std::string slot = "%construct" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca " << structName << '\n';
    emitStructDefaults(slot, structType);
    for (const auto& argument : call.arguments()) {
        const auto& named = static_cast<const ast::BinaryExpression&>(*argument);
        const auto& fieldName = static_cast<const ast::IdentifierExpression&>(named.left());
        const auto field = fieldLayout(structType, fieldName.name());
        if (!field) {
            throw CodegenError("unknown field in construction: " + fieldName.name());
        }
        const std::string value = emitExpression(named.right());
        const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << fieldPointer << " = getelementptr " << structName << ", ptr " << slot << ", i32 0, i32 "
                << field->index << '\n';
        output_ << "  store " << field->llvmType << ' ' << value << ", ptr " << fieldPointer << '\n';
        // Safe pointers: an owner given to a field moves into the value.
        if (isMoveOnly(field->field->type)) {
            disarmFresh(value);
        }
    }
    // CANON-24: the new value satisfies its invariant.
    if (const TypeRef invariantType = semantics_.newValueInvariant(call)) {
        emitInvariantOf(invariantType, slot);
    }
    // Safe pointers: a value that holds owners is fresh: it owns its
    // Strings (+1) and its owners, held until a store takes it.
    const bool moveOnly = isMoveOnly(structType);
    if (moveOnly) {
        emitRetainAt(slot, structType);
    }
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << result << " = load " << structName << ", ptr " << slot << '\n';
    if (moveOnly) {
        holdOwnedResult(result, structType, structName);
    }
    return result;
}

FunctionEmitter::Place FunctionEmitter::emitPlace(const ast::Expression& expression)
{
    if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
        const std::string name = normalize(static_cast<const ast::IdentifierExpression&>(expression).name());
        const auto local = locals_.find(name);
        if (local != locals_.end()) {
            emitWithGuard(name);
            return Place{local->second.slot, local->second.type};
        }
    }
    if (isIndexCall(expression)) {
        const ElementAddress element = emitElementAddress(static_cast<const ast::CallExpression&>(expression));
        return Place{element.pointer, element.elementType};
    }
    // Safe pointers: `Value(P)` is the object P reaches.
    if (expression.kind() == ast::AstNodeKind::CallExpression &&
        isPointerBuiltin(static_cast<const ast::CallExpression&>(expression)) &&
        equalsIgnoreCase(static_cast<const ast::IdentifierExpression&>(
                             static_cast<const ast::CallExpression&>(expression).callee())
                             .name(),
                         "Value")) {
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        return Place{emitPointerTarget(*call.arguments()[0]), typeOf(expression)};
    }
    if (isMemberAccessCall(expression)) {
        const auto& member = static_cast<const ast::CallExpression&>(expression);
        if (const auto field = memberField(member)) {
            // Through a pointer, the field is in the object it reaches.
            const ast::Expression& baseExpression = *member.arguments()[0];
            const Place base = isPointerType(typeOf(baseExpression))
                                   ? Place{emitPointerTarget(baseExpression), nullptr}
                                   : emitPlace(baseExpression);
            const std::string pointer = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << pointer << " = getelementptr " << llvmStructName(field->owner) << ", ptr "
                    << base.pointer << ", i32 0, i32 " << field->index << '\n';
            return Place{pointer, field->field->type};
        }
    }
    const TypeRef type = typeOf(expression);
    const std::string llvmType = llvmValueType(type);
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
// struct value (or a pointer to one) and Name one of its fields (otherwise
// it is a method).
std::optional<FieldLayout> FunctionEmitter::memberField(const ast::CallExpression& member) const
{
    if (member.arguments().size() != 2 ||
        member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
        return std::nullopt;
    }
    TypeRef baseType = typeOf(*member.arguments()[0]);
    if (isPointerType(baseType)) {
        baseType = baseType->pointerTarget();
    }
    return fieldLayout(baseType, static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]).name());
}

FunctionEmitter::FieldAddress FunctionEmitter::emitMemberAddress(const ast::CallExpression& call)
{
    if (!memberField(call)) {
        // Not a field: the caller treats it as a zero-argument method.
        throw CodegenError("not a struct field");
    }
    const Place place = emitPlace(call);
    return FieldAddress{place.pointer, memberField(call)->llvmType};
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
    const FunctionSignature* signature = boundSignature(call);
    if (signature == nullptr) {
        throw CodegenUnsupported("unknown method for LLVM emission: " + method.name());
    }
    if (call.arguments().size() + 1 != signature->parameters.size()) {
        throw CodegenUnsupported("unsupported method argument count: " + method.name());
    }
    return MethodCallTarget{signature, emitReceiver(*signature, *member.arguments()[0])};
}

// The routine the analysis bound a call site to: a method of the receiver's
// type (a struct method, or a method of a built-in type, CANON-9) or a
// routine of the program.
const FunctionSignature* FunctionEmitter::boundSignature(const ast::CallExpression& site) const
{
    const semantic::Symbol* symbol = semantics_.symbolOf(site);
    if (symbol == nullptr) {
        return nullptr;
    }
    const auto signature = signatures_.find(normalize(symbol->name));
    return signature != signatures_.end() ? &signature->second : nullptr;
}

const FunctionSignature* FunctionEmitter::calledRoutine(const ast::IdentifierExpression& callee) const
{
    const semantic::Symbol* symbol = semantics_.symbolOf(callee);
    const std::string& name =
        symbol != nullptr && symbol->kind == semantic::SymbolKind::Function ? symbol->name : callee.name();
    const auto signature = signatures_.find(normalize(name));
    return signature != signatures_.end() ? &signature->second : nullptr;
}

// A struct receiver is passed by address; a built-in one as its parameter
// says: the caller's variable for `Self mut`, a copy for `Self` (CANON-9).
std::string FunctionEmitter::emitReceiver(const FunctionSignature& signature, const ast::Expression& receiver)
{
    const FunctionParameter& self = signature.parameters.front();
    // Safe pointers: the receiver is the object the pointer reaches.
    if (isPointerType(typeOf(receiver))) {
        const std::string target = emitPointerTarget(receiver);
        if (self.type->isStruct() || self.byReference) {
            return target;
        }
        const std::string value = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << value << " = load " << self.llvmType << ", ptr " << target << '\n';
        return value;
    }
    if (self.type->isStruct()) {
        return emitPlace(receiver).pointer;
    }
    return self.byReference ? emitReferenceArgument(receiver, self) : emitExpression(receiver);
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
    const FunctionSignature* signature = boundSignature(member);
    if (signature == nullptr) {
        throw CodegenUnsupported("unknown field or zero-argument method for LLVM emission: " + method.name());
    }
    if (signature->parameters.size() != 1) {
        throw CodegenUnsupported("LLVM zero-argument method emission requires only an explicit receiver parameter");
    }
    return MethodCallTarget{signature, emitReceiver(*signature, *member.arguments()[0])};
}

std::string FunctionEmitter::emitNoArgumentMethodCall(const ast::CallExpression& member, bool requireValue)
{
    emitInvariantChecks(member, false);
    const MethodCallTarget target = resolveNoArgumentMethodAccess(member);
    const FunctionSignature& signature = *target.signature;

    const std::string result = emitUserCall(signature, {target.receiverPointer}, requireValue);
    emitInvariantChecks(member, true);
    return result;
}

std::string FunctionEmitter::emitMethodCall(const ast::CallExpression& call, bool requireValue)
{
    emitInvariantChecks(call, false);
    const MethodCallTarget target = resolveMethodCall(call);
    const FunctionSignature& signature = *target.signature;

    std::vector<std::string> arguments;
    arguments.reserve(call.arguments().size() + 1);
    arguments.push_back(target.receiverPointer);
    for (std::string& value : emitArguments(signature, call.arguments(), 1)) {
        arguments.push_back(std::move(value));
    }

    const std::string result = emitUserCall(signature, arguments, requireValue);
    emitInvariantChecks(call, true);
    return result;
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

    // CANON-14: `S := S + X` and `S += X` append to S in place.
    if (emitStringAppend(assignment)) {
        return;
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
        emitOwningStore(local->second.slot, local->second.type, local->second.llvmType, value);
        return;
    }

    if (isIndexCall(assignment.left())) {
        const ElementAddress element = emitElementAddress(
            static_cast<const ast::CallExpression&>(assignment.left()));
        const std::string value = emitExpression(assignment.right());
        const std::string stored = narrowForStorage(value, element);
        emitOwningStore(element.pointer, element.elementType, element.storage, stored);
        emitInvariantChecks(assignment, true);
        return;
    }

    // Safe pointers: `Value(P) := X` replaces the object P reaches.
    if (assignment.left().kind() == ast::AstNodeKind::CallExpression &&
        isPointerBuiltin(static_cast<const ast::CallExpression&>(assignment.left()))) {
        const Place place = emitPlace(assignment.left());
        const std::string value = emitExpression(assignment.right());
        emitOwningStore(place.pointer, place.type, valueLlvmType(place.type), value);
        return;
    }

    if (isMemberAccessCall(assignment.left())) {
        const auto& member = static_cast<const ast::CallExpression&>(assignment.left());
        const FieldAddress field = emitMemberAddress(member);
        const std::string value = emitExpression(assignment.right());
        emitOwningStore(field.pointer, memberField(member)->field->type, field.llvmType, value);
        emitInvariantChecks(assignment, true);
        return;
    }

    throw CodegenUnsupported(
        "LLVM emission currently supports only local variable or field assignments");
}

} // namespace inox::compiler::codegen::detail
