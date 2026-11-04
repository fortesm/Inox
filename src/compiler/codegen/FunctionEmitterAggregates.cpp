// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: struct defaults and construction, places, fields, methods, assignment.

#include "FunctionEmitter.h"
#include "../support/DecimalLiteral.h"
#include "../support/CharLiteral.h"

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
            emitArrayFillDefault(fieldPointer, field.type, field.resolvedDefault);
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
            // A default is an immortal literal, so storing it needs no
            // retain.
            const types::FieldDefault& text = field.resolvedDefault;
            const std::string value = field.hasDefault && text.kind == types::FieldDefault::Kind::Text
                                          ? emitStringValue(text.text)
                                          : emptyStringValue();
            output_ << "  store ptr " << value << ", ptr " << fieldPointer << '\n';
            continue;
        }
        if (field.type->isDecimal()) {
            // Change decimals (r3.91): the exact scaled value of the literal.
            const std::string scaled = field.hasDefault ? field.resolvedDefault.text : std::string("0");
            output_ << "  store " << layout.llvmType << ' ' << scaled << ", ptr " << fieldPointer << '\n';
            continue;
        }
        output_ << "  store " << layout.llvmType << ' ' << defaultConstant(field.resolvedDefault, field.type, layout.llvmType)
                << ", ptr " << fieldPointer << '\n';
    }
}

// The LLVM constant of a resolved default: a code point, an ordinal, a
// number in the field's width, a Bool.
std::string FunctionEmitter::defaultConstant(const types::FieldDefault& value, TypeRef type,
                                             const std::string& llvmType)
{
    switch (value.kind) {
    case types::FieldDefault::Kind::Bool:
        return value.boolean ? "1" : "0";
    case types::FieldDefault::Kind::Real:
        return llvmRealConstant(value.real, llvmType == "float");
    case types::FieldDefault::Kind::Integer:
        // A UInt64 above Int64.Max is written unsigned (change uint64).
        return isUInt64Type(type) ? std::to_string(static_cast<std::uint64_t>(value.integer))
                                  : std::to_string(value.integer);
    default:
        throw CodegenError("internal error: a field default without its value");
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
void FunctionEmitter::emitArrayFillDefault(const std::string& pointer, TypeRef arrayType,
                                           const types::FieldDefault& fill)
{
    const FlatArray flat = flattenArray(arrayType);
    const TypeRef element = flat.element;
    if (element->isOptionOwn() || element->isSet()) {
        return;
    }
    const std::string storage = llvmStorageType(element);
    const std::string value = isStringType(element) ? emitStringValue(fill.text)
                                                    : defaultConstant(fill, element, storage);
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

// CANON-9 named construction `T(Field := Value, ...)`: the type defaults,
// then each named value in source order.
std::string FunctionEmitter::emitConstruction(const tir::Expr& construction)
{
    const TypeRef structType = construction.type;
    const std::string structName = llvmStructName(structType);
    const std::string slot = "%construct" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca " << structName << '\n';
    emitStructDefaults(slot, structType);
    for (const tir::FieldValue* named : constructionOrder(construction)) {
        const FieldLayout field = fieldLayout(structType, named->field);
        const std::string value = emitValue(*named->value);
        const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << fieldPointer << " = getelementptr " << structName << ", ptr " << slot << ", i32 0, i32 "
                << field.index << '\n';
        output_ << "  store " << field.llvmType << ' ' << value << ", ptr " << fieldPointer << '\n';
        // Safe pointers: an owner given to a field moves into the value.
        if (isMoveOnly(field.field->type)) {
            disarmFresh(value);
        }
    }
    // CANON-24: the new value satisfies its invariant.
    if (construction.invariants != nullptr && construction.invariants->newValue != nullptr) {
        emitInvariantOf(construction.invariants->newValue, slot);
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

// The written field values of a construction, in the order they are written
// (CANON-9: evaluated in source order).
std::vector<const tir::FieldValue*> FunctionEmitter::constructionOrder(const tir::Expr& construction)
{
    std::vector<const tir::FieldValue*> written;
    for (const tir::FieldValue& field : construction.fields) {
        if (field.value != nullptr) {
            written.push_back(&field);
        }
    }
    std::sort(written.begin(), written.end(), [](const tir::FieldValue* first, const tir::FieldValue* second) {
        return first->written < second->written;
    });
    return written;
}

// The storage of a place: a variable, a field of a place, an element, the
// object a pointer reaches. Any other value (`value`, when the place is
// read as one) is copied into a temporary so its fields can be read.
FunctionEmitter::Place FunctionEmitter::emitPlace(const tir::Place& place, const tir::Expr* value)
{
    switch (place.kind) {
    case tir::PlaceKind::Element: {
        const ElementAddress element = emitElementAddress(place);
        return Place{element.pointer, element.elementType};
    }
    // Safe pointers: `Value(P)` is the object P reaches.
    case tir::PlaceKind::Through:
        return Place{emitPointerTarget(*place.base), place.type};
    case tir::PlaceKind::Field: {
        // Through a pointer, the field is in the object it reaches.
        const tir::Place& base = *place.base;
        const Place reached = base.kind == tir::PlaceKind::Through ? Place{emitPointerTarget(*base.base), nullptr}
                                                                    : emitPlace(base);
        const FieldLayout field = fieldOf(place);
        const std::string pointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << pointer << " = getelementptr " << llvmStructName(field.owner) << ", ptr "
                << reached.pointer << ", i32 0, i32 " << field.index << '\n';
        return Place{pointer, field.field->type};
    }
    case tir::PlaceKind::Value:
        return emitPlaceCopy(*place.value);
    default: {
        const std::string name = normalize(place.symbol->name);
        const auto local = locals_.find(name);
        if (local != locals_.end()) {
            emitStorageGuard(name);
            return Place{local->second.slot, local->second.type};
        }
        break;
    }
    }
    if (value != nullptr) {
        return emitPlaceCopy(*value);
    }
    const TypeRef type = place.type;
    const std::string llvmType = llvmValueType(type);
    if (llvmType.empty()) {
        throw CodegenUnsupported("this value has no storage in the LLVM backend");
    }
    return storePlaceTemporary(emitPlaceValue(place), type, llvmType);
}

FunctionEmitter::Place FunctionEmitter::emitPlace(const tir::Expr& expression)
{
    const tir::Expr& value = bare(expression);
    if (value.kind == tir::ExprKind::Read) {
        return emitPlace(*value.place, &expression);
    }
    return emitPlaceCopy(expression);
}

FunctionEmitter::Place FunctionEmitter::emitPlaceCopy(const tir::Expr& expression)
{
    const TypeRef type = bare(expression).type;
    const std::string llvmType = llvmValueType(type);
    if (llvmType.empty()) {
        throw CodegenUnsupported("this value has no storage in the LLVM backend");
    }
    return storePlaceTemporary(emitValue(expression), type, llvmType);
}

FunctionEmitter::Place FunctionEmitter::storePlaceTemporary(const std::string& value, TypeRef type,
                                                           const std::string& llvmType)
{
    const std::string slot = "%placetmp" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca " << llvmType << '\n';
    output_ << "  store " << llvmType << ' ' << value << ", ptr " << slot << '\n';
    return Place{slot, type};
}

// The value a place holds, read: a variable, an element, a field, the
// object a pointer reaches.
std::string FunctionEmitter::emitPlaceRead(const tir::Place& place)
{
    switch (place.kind) {
    case tir::PlaceKind::Element:
        return loadElement(emitElementAddress(place));
    case tir::PlaceKind::Field: {
        const std::string pointer = emitPlace(place).pointer;
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = load " << fieldOf(place).llvmType << ", ptr " << pointer << '\n';
        return result;
    }
    case tir::PlaceKind::Through: {
        const std::string target = emitPointerTarget(*place.base);
        const std::string value = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << value << " = load " << valueLlvmType(place.type) << ", ptr " << target << '\n';
        return value;
    }
    case tir::PlaceKind::Value:
        return emitValue(*place.value);
    default:
        return emitVariableRead(place);
    }
}

// The value of a place that is not written as an expression of its own (the
// base of a field or an element): read, and held when borrowed.
std::string FunctionEmitter::emitPlaceValue(const tir::Place& place)
{
    if (place.kind == tir::PlaceKind::Value) {
        return emitValue(*place.value);
    }
    std::string value = emitPlaceRead(place);
    if (statementRunsRoutine_ && isBorrowedPlace(place.type, place)) {
        value = protectBorrowed(place.type, value);
    }
    return value;
}

// The struct and field a field place names.
FieldLayout FunctionEmitter::fieldOf(const tir::Place& field)
{
    return fieldLayout(field.base->type, field.field);
}

// The lowered signature of a routine of the program (LlvmIrEmitter).
const FunctionSignature* FunctionEmitter::routineSignature(const tir::Routine* routine) const
{
    if (routine == nullptr) {
        return nullptr;
    }
    const auto signature = signatures_.find(routine);
    return signature != signatures_.end() ? &signature->second : nullptr;
}

// A struct receiver is passed by address; a built-in one as its parameter
// says: the caller's variable for `Self mut`, a copy for `Self` (CANON-9).
std::string FunctionEmitter::emitReceiver(const FunctionSignature& signature, const tir::Expr& receiver)
{
    const FunctionParameter& self = signature.parameters.front();
    // Safe pointers: the receiver is the object the pointer reaches.
    if (isPointerType(bare(receiver).type)) {
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
    return self.byReference ? emitReferenceArgument(receiver, self) : emitValue(receiver);
}

// A call of a routine of the program: its arguments, the storage of each
// parameter that is the caller's (CANON-25), and the call; a method's
// receiver first, its invariants checked before and after (CANON-24).
std::string FunctionEmitter::emitCall(const tir::Expr& call, bool requireValue)
{
    const FunctionSignature* routine = routineSignature(call.routine);
    if (routine == nullptr) {
        throw CodegenUnsupported("unknown routine for LLVM emission: " + std::string(call.routine->name));
    }
    if (requireValue && routine->llvmReturnType == "void") {
        throw CodegenError("void function call cannot be used as an expression");
    }
    if (call.operands.size() != routine->parameters.size()) {
        throw CodegenUnsupported("unsupported call argument count in function: " + std::string(routine_.name));
    }
    if (isMethodCall(call)) {
        return emitMethodCall(call, *routine, requireValue);
    }
    std::vector<std::string> storage;
    const std::vector<std::string> arguments = emitArguments(*routine, call.operands, 0, &storage);
    const std::string result = emitUserCall(*routine, arguments, requireValue, storage);
    emitInvariantChecks(call.invariants, true);
    return result;
}

bool FunctionEmitter::isMethodCall(const tir::Expr& call)
{
    return call.kind == tir::ExprKind::Call && !call.routine->parameters.empty() &&
           call.routine->parameters[0].isReceiver;
}

std::string FunctionEmitter::emitMethodCall(const tir::Expr& call, const FunctionSignature& signature,
                                            bool requireValue)
{
    emitInvariantChecks(call.invariants, false);
    const bool guarded = beginReceiverGuard(call, signature);
    const tir::Expr& receiver = *call.operands[0];
    beginCallerStorage(receiver, true);
    const std::string receiverPointer = emitReceiver(signature, receiver);
    const std::string result = emitMethodCallWith(signature, call, receiverPointer, guarded, requireValue);
    emitInvariantChecks(call.invariants, true);
    return result;
}

// The arguments of a method call after its receiver, the storage of each
// parameter that is the caller's (CANON-25), and the call. Out of line: the
// recursive callers keep their frames.
std::string FunctionEmitter::emitMethodCallWith(const FunctionSignature& signature, const tir::Expr& call,
                                                const std::string& receiverPointer, bool guarded,
                                                bool requireValue)
{
    std::vector<std::string> storage{endCallerStorage(*call.operands[0])};
    std::vector<std::string> arguments;
    arguments.reserve(call.operands.size());
    arguments.push_back(receiverPointer);
    std::vector<std::string> argumentStorage;
    const tir::Span<const tir::Expr*> rest{call.operands.items + 1, call.operands.size() - 1};
    for (std::string& value : emitArguments(signature, rest, 1, &argumentStorage)) {
        arguments.push_back(std::move(value));
    }
    storage.insert(storage.end(), argumentStorage.begin(), argumentStorage.end());
    if (guarded) {
        endPlaceGuard();
    }
    return emitUserCall(signature, arguments, requireValue, storage);
}

// CANON-5: `L := R` evaluates R, then L once as a place, then stores R's
// value there; a call in R that replaces what L reaches never leaves the
// store a stale place.
void FunctionEmitter::emitAssignment(const tir::Stmt& assignment)
{
    // CANON-14: `S := S + X` and `S += X` append to S in place.
    if (emitStringAppend(assignment)) {
        return;
    }
    if (assignment.compound) {
        emitCompoundAssignment(assignment);
        return;
    }
    const tir::Place& target = *assignment.target;
    switch (target.kind) {
    case tir::PlaceKind::Element: {
        const std::string value = emitValue(*assignment.value);
        const ElementAddress element = emitElementAddress(target);
        const std::string stored = narrowForStorage(value, element);
        emitOwningStore(element.pointer, element.elementType, element.storage, stored);
        emitInvariantChecks(assignment.invariants, true);
        return;
    }
    // Safe pointers: `Value(P) := X` replaces the object P reaches.
    case tir::PlaceKind::Through: {
        const std::string value = emitValue(*assignment.value);
        const Place place = emitPlace(target);
        emitOwningStore(place.pointer, place.type, valueLlvmType(place.type), value);
        return;
    }
    case tir::PlaceKind::Field: {
        const std::string value = emitValue(*assignment.value);
        const std::string pointer = emitPlace(target).pointer;
        const FieldLayout field = fieldOf(target);
        emitOwningStore(pointer, field.field->type, field.llvmType, value);
        emitInvariantChecks(assignment.invariants, true);
        return;
    }
    case tir::PlaceKind::Value:
        break;
    default: {
        const auto local = locals_.find(normalize(target.symbol->name));
        if (local == locals_.end()) {
            throw CodegenUnsupported("LLVM emission found an assignment to a variable without storage");
        }
        emitStorageGuard(local->first);
        const std::string value = emitValue(*assignment.value);
        emitOwningStore(local->second.slot, local->second.type, local->second.llvmType, value);
        return;
    }
    }
    throw CodegenUnsupported("LLVM emission currently supports only local variable or field assignments");
}

// CANON-5: `L op= R`, normalized as `L := L op R` marked compound,
// evaluates R once, then the place of L once, reads it, applies op and
// stores into that same place. The written `L` on the right is never
// evaluated as a place of its own.
void FunctionEmitter::emitCompoundAssignment(const tir::Stmt& assignment)
{
    const tir::Expr& operation = bare(*assignment.value);
    const tir::Place& target = *assignment.target;
    const std::string value = emitValue(*operation.operands[1]);
    if (isStringType(target.type)) {
        // CANON-14: appended in place, R already evaluated.
        const Place place = emitPlace(target);
        const std::string part = "%strparts" + std::to_string(nextTemporary_++);
        output_ << "  " << part << " = alloca [1 x ptr]\n";
        output_ << "  store ptr " << value << ", ptr " << part << '\n';
        requireRuntime(RuntimeFunction::StringAppend);
        requireStringRuntime();
        const std::string status = newStringStatus();
        output_ << "  call void @__inox_str_append(ptr " << place.pointer << ", i64 1, ptr " << part << ", ptr "
                << status << ")\n";
        emitStringStatusCheck(status);
        emitInvariantChecks(assignment.invariants, true);
        return;
    }
    if (target.kind == tir::PlaceKind::Element) {
        const ElementAddress element = emitElementAddress(target);
        const std::string current = loadElement(element);
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        const std::string combined = finishValue(*assignment.value, emitBinaryValues(operation, result, current, value));
        emitOwningStore(element.pointer, element.elementType, element.storage, narrowForStorage(combined, element));
        emitInvariantChecks(assignment.invariants, true);
        return;
    }
    const Place place = emitPlace(target);
    const TypeRef type = place.type != nullptr ? place.type : target.type;
    const std::string llvmType = target.kind == tir::PlaceKind::Field ? fieldOf(target).llvmType : valueLlvmType(type);
    const std::string current = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << current << " = load " << llvmType << ", ptr " << place.pointer << '\n';
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    const std::string combined = finishValue(*assignment.value, emitBinaryValues(operation, result, current, value));
    emitOwningStore(place.pointer, type, llvmType, combined);
    emitInvariantChecks(assignment.invariants, true);
}

} // namespace inox::compiler::codegen::detail
