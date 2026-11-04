// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: safe pointers (OpenSpec change safe-pointers).
//
// An owner (`Own<T>`, `Option<Own<T>>`) is the address of its object in a
// block of libinoxrt (OwnRuntime.cpp); null is None, or an owner given away
// by Move or Free. The block's header, 16 bytes before the object, holds a
// 64-bit generation that Free increments. A `Link<T>` is { object, header,
// generation }: every use compares the header's generation with its own, so
// a Link to a freed object (also one whose block was reused) never reaches
// it: `Alive` answers false, or the use traps (kind 16). A Link to a
// variable carries the variable's tombstone, a block of size 0 that the
// routine frees when the variable's declaration runs again or the routine
// returns; a State variable never dies (an immortal header).
//
// The object of a type that holds owners or Strings is released by its drop
// function, emitted once per type; libinoxrt runs the drops in a loop, so a
// long list is freed without recursion.

#include "FunctionEmitter.h"

#include "TirWalk.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>

namespace inox::compiler::codegen::detail {

// A value that holds an owner is moved, never copied: an owner, or an
// aggregate holding one. A Link and a slice only look at storage.
bool FunctionEmitter::isMoveOnly(TypeRef type)
{
    if (isOwnerType(type)) {
        return true;
    }
    if (isArrayType(type)) {
        return isMoveOnly(type->element());
    }
    if (isStructType(type)) {
        for (const types::StructField& field : type->fields()) {
            if (isMoveOnly(field.type)) {
                return true;
            }
        }
    }
    return false;
}

// Walks a value of `type` stored at `pointer` (struct fields and array
// elements, recursively) and calls `action` on every leaf `isLeaf` accepts.
void FunctionEmitter::forEachLeafAt(const std::string& pointer, TypeRef type,
                                    const std::function<bool(TypeRef)>& isLeaf,
                                    const std::function<void(const std::string&, TypeRef)>& action)
{
    if (isLeaf(type)) {
        action(pointer, type);
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
            forEachLeafAt(fieldPointer, type->fields()[index].type, isLeaf, action);
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
    const std::string counter = "%leafwalk.i" + label;
    output_ << "  " << counter << " = alloca i64\n";
    output_ << "  store i64 0, ptr " << counter << '\n';
    output_ << "  br label %leafwalk.cond" << label << "\n\n";
    output_ << "leafwalk.cond" << label << ":\n";
    const std::string index = "%tmp" + std::to_string(nextTemporary_++);
    const std::string more = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << index << " = load i64, ptr " << counter << '\n';
    output_ << "  " << more << " = icmp slt i64 " << index << ", " << flat.count << '\n';
    output_ << "  br i1 " << more << ", label %leafwalk.body" << label << ", label %leafwalk.end" << label
            << "\n\n";
    output_ << "leafwalk.body" << label << ":\n";
    const std::string elementPointer = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << elementPointer << " = getelementptr inbounds " << storage << ", ptr " << pointer << ", i64 "
            << index << '\n';
    forEachLeafAt(elementPointer, flat.element, isLeaf, action);
    const std::string next = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << next << " = add i64 " << index << ", 1\n";
    output_ << "  store i64 " << next << ", ptr " << counter << '\n';
    output_ << "  br label %leafwalk.cond" << label << "\n\n";
    output_ << "leafwalk.end" << label << ":\n";
}

// Frees every owner held by the value at `pointer`.
void FunctionEmitter::emitOwnerFreesAt(const std::string& pointer, TypeRef type)
{
    forEachLeafAt(
        pointer, type, [](TypeRef leaf) { return isOwnerType(leaf); },
        [&](const std::string& at, TypeRef owner) {
            requireRuntime(RuntimeFunction::OwnFree);
            const std::string drop = dropFunctionFor(owner->pointerTarget());
            const std::string object = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << object << " = load ptr, ptr " << at << '\n';
            output_ << "  call void @__inox_own_free(ptr " << object << ", ptr " << drop << ")\n";
        });
}

// A tombstone is a block of the runtime with no object: freeing it makes
// every Link to the variable it stands for dead.
void FunctionEmitter::emitTombstoneFree(const std::string& pointer)
{
    requireRuntime(RuntimeFunction::OwnFree);
    const std::string tombstone = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << tombstone << " = load ptr, ptr " << pointer << '\n';
    output_ << "  call void @__inox_own_free(ptr " << tombstone << ", ptr null)\n";
}

// The function that releases what an object of `targetType` holds (its
// Strings and owners) before its block is freed; "null" when it holds none.
std::string FunctionEmitter::dropFunctionFor(TypeRef targetType)
{
    if (!typeOwnsReferences(targetType)) {
        return "null";
    }
    if (const auto existing = module_.dropFunctions.find(targetType); existing != module_.dropFunctions.end()) {
        return existing->second;
    }
    const std::string name = "@__inox_drop." + std::to_string(module_.dropFunctions.size());
    module_.dropFunctions.emplace(targetType, name);
    // The drop function is emitted with this emitter's helpers into a
    // separate text, then added to the module.
    const std::string saved = output_.str();
    output_.str("");
    output_.clear();
    output_ << "define internal void " << name << "(ptr %object) nounwind {\nentry:\n";
    emitReleaseAt("%object", targetType);
    output_ << "  ret void\n}";
    const std::string definition = output_.str();
    output_.str(saved);
    output_.clear();
    output_.seekp(0, std::ios_base::end);
    requireModuleLine("; the drop function of " + targetType->name() + " (safe pointers)\n" + definition);
    return name;
}

void FunctionEmitter::disarmFresh(const std::string& value)
{
    const auto fresh = freshTemps_.find(value);
    if (fresh == freshTemps_.end()) {
        return;
    }
    output_ << "  store " << fresh->second.llvmType << " zeroinitializer, ptr " << fresh->second.slot << '\n';
    freshTemps_.erase(fresh);
}

std::string FunctionEmitter::emitOwnerNullCheck(const std::string& owner)
{
    requireModuleLine("declare i1 @llvm.expect.i1(i1, i1)");
    const std::string label = std::to_string(nextLabel_++);
    const std::string isNull = "%tmp" + std::to_string(nextTemporary_++);
    const std::string expected = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << isNull << " = icmp eq ptr " << owner << ", null\n";
    output_ << "  " << expected << " = call i1 @llvm.expect.i1(i1 " << isNull << ", i1 false)\n";
    output_ << "  br i1 " << expected << ", label %own.moved" << label << ", label %own.ok" << label << "\n\n";
    output_ << "own.moved" << label << ":\n  call void @__inox_arith_fault(i32 17)\n  unreachable\n\n";
    output_ << "own.ok" << label << ":\n";
    return owner;
}

FunctionEmitter::LinkParts FunctionEmitter::extractLink(const std::string& link)
{
    LinkParts parts;
    parts.target = "%tmp" + std::to_string(nextTemporary_++);
    parts.header = "%tmp" + std::to_string(nextTemporary_++);
    parts.generation = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << parts.target << " = extractvalue " << kLinkLlvmType << ' ' << link << ", 0\n";
    output_ << "  " << parts.header << " = extractvalue " << kLinkLlvmType << ' ' << link << ", 1\n";
    output_ << "  " << parts.generation << " = extractvalue " << kLinkLlvmType << ' ' << link << ", 2\n";
    return parts;
}

std::string FunctionEmitter::buildLink(const LinkParts& parts)
{
    const std::string first = "%tmp" + std::to_string(nextTemporary_++);
    const std::string second = "%tmp" + std::to_string(nextTemporary_++);
    const std::string link = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << first << " = insertvalue " << kLinkLlvmType << " undef, ptr " << parts.target << ", 0\n";
    output_ << "  " << second << " = insertvalue " << kLinkLlvmType << ' ' << first << ", ptr " << parts.header
            << ", 1\n";
    output_ << "  " << link << " = insertvalue " << kLinkLlvmType << ' ' << second << ", i64 " << parts.generation
            << ", 2\n";
    return link;
}

// Whether a Link still reaches its object: a header, and the generation it saw.
std::string FunctionEmitter::emitLinkAlive(const LinkParts& parts)
{
    const std::string label = std::to_string(nextLabel_++);
    const std::string isNull = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << isNull << " = icmp eq ptr " << parts.header << ", null\n";
    output_ << "  br i1 " << isNull << ", label %link.empty" << label << ", label %link.load" << label << "\n\n";
    output_ << "link.load" << label << ":\n";
    const std::string current = "%tmp" + std::to_string(nextTemporary_++);
    const std::string same = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << current << " = load i64, ptr " << parts.header << '\n';
    output_ << "  " << same << " = icmp eq i64 " << current << ", " << parts.generation << '\n';
    output_ << "  br label %link.alive" << label << "\n\n";
    output_ << "link.empty" << label << ":\n  br label %link.alive" << label << "\n\n";
    output_ << "link.alive" << label << ":\n";
    const std::string alive = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << alive << " = phi i1 [ false, %link.empty" << label << " ], [ " << same << ", %link.load"
            << label << " ]\n";
    return alive;
}

void FunctionEmitter::emitLinkCheck(const LinkParts& parts)
{
    requireModuleLine("declare i1 @llvm.expect.i1(i1, i1)");
    const std::string alive = emitLinkAlive(parts);
    const std::string label = std::to_string(nextLabel_++);
    const std::string expected = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << expected << " = call i1 @llvm.expect.i1(i1 " << alive << ", i1 true)\n";
    output_ << "  br i1 " << expected << ", label %link.ok" << label << ", label %link.dead" << label << "\n\n";
    output_ << "link.dead" << label << ":\n  call void @__inox_arith_fault(i32 16)\n  unreachable\n\n";
    output_ << "link.ok" << label << ":\n";
}

// The identity of a pointer on a place's path: its place, or the value
// itself when it is not stored anywhere.
const void* FunctionEmitter::pointerKey(const tir::Expr& pointer)
{
    const tir::Expr& value = bare(pointer);
    return value.kind == tir::ExprKind::Read ? static_cast<const void*>(value.place) : &pointer;
}

std::string FunctionEmitter::emitPointerTarget(const tir::Place& pointer)
{
    return emitPointerTargetOf(&pointer, pointer.type, emitPlaceValue(pointer));
}

std::string FunctionEmitter::emitPointerTarget(const tir::Expr& pointer)
{
    const tir::Expr& value = bare(pointer);
    if (value.kind == tir::ExprKind::Read) {
        return emitPointerTarget(*value.place);
    }
    return emitPointerTargetOf(&pointer, value.type, emitValue(pointer));
}

// The address of the object a pointer value reaches, checked: an owner given
// away traps (17), a dead or empty Link traps (16). `key` names the pointer
// for the guards waiting for it.
std::string FunctionEmitter::emitPointerTargetOf(const void* key, TypeRef type, const std::string& value)
{
    if (isLinkType(type)) {
        const LinkParts parts = extractLink(value);
        emitLinkCheck(parts);
        if (!placeGuards_.empty()) {
            capturePlaceGuards(key, parts.target, &parts);
        }
        if (key == withGuardPointer_) {
            withGuardCaptured_ = parts;
        }
        return parts.target;
    }
    const std::string object = emitOwnerNullCheck(value);
    if (!placeGuards_.empty()) {
        capturePlaceGuards(key, object, nullptr);
    }
    if (key == withGuardPointer_) {
        const std::string header = "%tmp" + std::to_string(nextTemporary_++);
        const std::string generation = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << header << " = getelementptr i8, ptr " << object << ", i64 -16\n";
        output_ << "  " << generation << " = load i64, ptr " << header << '\n';
        withGuardCaptured_ = LinkParts{object, header, generation};
    }
    return object;
}

// CANON-25: when an index or a slice bound that may run a routine follows
// the pointer a place is reached through, that routine may free the
// pointer's object (or replace an owner above it, which frees it too); the
// object's generation is captured when the pointer is reached and checked
// after the index, before the place is used.
void FunctionEmitter::beginPlaceGuard(const tir::Place& base, bool laterRunsRoutine)
{
    if (base.kind == tir::PlaceKind::Value) {
        beginPlaceGuard(*base.value, laterRunsRoutine);
        return;
    }
    placeGuards_.push_back(PlaceGuard{laterRunsRoutine ? lastPointerOnPath(base) : nullptr, std::nullopt});
}

void FunctionEmitter::beginPlaceGuard(const tir::Expr& base, bool laterRunsRoutine)
{
    const void* pointer = nullptr;
    if (laterRunsRoutine) {
        const tir::Expr* path = &base;
        while (bare(*path).kind == tir::ExprKind::Slice) {
            path = bare(*path).operands[0];
        }
        if (isPointerType(bare(*path).type)) {
            pointer = pointerKey(*path);
        } else if (bare(*path).kind == tir::ExprKind::Read) {
            pointer = lastPointerOnPath(*bare(*path).place);
        }
    }
    placeGuards_.push_back(PlaceGuard{pointer, std::nullopt});
}

// `A[I]`: the index follows the base.
void FunctionEmitter::beginElementGuard(const tir::Place& element)
{
    beginPlaceGuard(*element.base, mayRunRoutine(*element.indices[0]));
}

// A reference argument (ADR-0016) is a place: when a later argument may run
// a routine, the object holding it is checked before the call. 1 when a
// guard was opened, for emitArguments to close.
std::size_t FunctionEmitter::beginReferenceGuard(tir::Span<const tir::Expr*> arguments, std::size_t index)
{
    const tir::Expr& argument = bare(*arguments[index]);
    if (argument.kind != tir::ExprKind::Read ||
        (argument.place->kind != tir::PlaceKind::Element && argument.place->kind != tir::PlaceKind::Field)) {
        return 0;
    }
    bool laterRuns = false;
    for (std::size_t later = index + 1; later < arguments.size() && !laterRuns; ++later) {
        laterRuns = mayRunRoutine(*arguments[later]);
    }
    if (!laterRuns) {
        return 0;
    }
    placeGuards_.push_back(PlaceGuard{lastPointerOnPath(*argument.place), std::nullopt});
    return 1;
}

// The receiver of `P.M(X)` is reached before its arguments: when one of
// them may run a routine, the object holding the receiver is checked
// before the call.
bool FunctionEmitter::beginReceiverGuard(const tir::Expr& call, const FunctionSignature& signature)
{
    bool runs = false;
    for (std::size_t index = 1; index < call.operands.size(); ++index) {
        runs = runs || mayRunRoutine(*call.operands[index]);
    }
    if (!runs) {
        return false;
    }
    // A built-in receiver passed as a copy (`Self` of an Integer) is a
    // value, not a place.
    if (signature.parameters.empty() ||
        !(signature.parameters.front().type->isStruct() || signature.parameters.front().byReference)) {
        return false;
    }
    beginPlaceGuard(*call.operands[0], true);
    return true;
}

// CANON-25: a parameter that is the caller's storage receives the header
// and generation of the object holding that storage, captured when the
// pointer nearest to it is reached (the receiver itself when it is one).
void FunctionEmitter::beginCallerStorage(const tir::Expr& place, bool receiver)
{
    const void* pointer = nullptr;
    const tir::Expr& value = bare(place);
    if (receiver && isPointerType(value.type)) {
        pointer = pointerKey(place);
    } else if (value.kind == tir::ExprKind::Read &&
               (value.place->kind == tir::PlaceKind::Element || value.place->kind == tir::PlaceKind::Field)) {
        pointer = lastPointerOnPath(*value.place);
    }
    placeGuards_.push_back(PlaceGuard{pointer, std::nullopt});
}

// `ptr H, i64 G`: the object reached, else the guard of the name the place
// starts from (a parameter that is itself the caller's storage, a `with`
// binding), else storage no owner frees.
std::string FunctionEmitter::endCallerStorage(const tir::Expr& place)
{
    if (placeGuards_.empty()) {
        throw CodegenError("internal error: a caller storage guard closed twice");
    }
    const PlaceGuard guard = placeGuards_.back();
    placeGuards_.pop_back();
    if (guard.reached) {
        return "ptr " + guard.reached->header + ", i64 " + guard.reached->generation;
    }
    if (const LinkParts* root = rootStorageGuard(place)) {
        return "ptr " + root->header + ", i64 " + root->generation;
    }
    return immortalStorage();
}

const FunctionEmitter::LinkParts* FunctionEmitter::rootStorageGuard(const tir::Expr& place) const
{
    const tir::Expr& value = bare(place);
    if (value.kind != tir::ExprKind::Read) {
        return nullptr;
    }
    const tir::Place* path = value.place;
    while ((path->kind == tir::PlaceKind::Element || path->kind == tir::PlaceKind::Field) &&
           path->base->kind != tir::PlaceKind::Through) {
        path = path->base;
    }
    if (!isVariablePlace(*path)) {
        return nullptr;
    }
    const auto guard = storageGuards_.find(normalize(path->symbol->name));
    return guard != storageGuards_.end() ? &guard->second : nullptr;
}

// A `with` binding keeps the guard of its target: the object captured when
// the target was reached through a pointer, else the guard of the name the
// target starts from.
void FunctionEmitter::bindStorageGuard(const std::string& binding, const tir::Expr& target)
{
    if (withGuardCaptured_) {
        storageGuards_.insert_or_assign(binding, *withGuardCaptured_);
        withGuardCaptured_.reset();
    } else if (const LinkParts* root = rootStorageGuard(target)) {
        storageGuards_.insert_or_assign(binding, *root);
    }
}

// The header of storage no owner frees (a local, State, a temporary): its
// generation never changes.
std::string FunctionEmitter::immortalStorage()
{
    requireModuleLine("@__inox.immortal.header = internal global [2 x i64] [i64 1, i64 0], align 16");
    return "ptr @__inox.immortal.header, i64 1";
}

// `B[L..H]`: the bounds follow the base.
void FunctionEmitter::beginSliceGuard(const tir::Expr& slice)
{
    bool laterRuns = false;
    for (std::size_t index = 1; index < slice.operands.size(); ++index) {
        laterRuns = laterRuns || mayRunRoutine(*slice.operands[index]);
    }
    beginPlaceGuard(*slice.operands[0], laterRuns);
}

void FunctionEmitter::capturePlaceGuards(const void* pointer, const std::string& object, const LinkParts* link)
{
    for (PlaceGuard& guard : placeGuards_) {
        if (guard.pointer != pointer || guard.reached) {
            continue;
        }
        if (link != nullptr) {
            guard.reached = *link;
            continue;
        }
        const std::string header = "%tmp" + std::to_string(nextTemporary_++);
        const std::string generation = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << header << " = getelementptr i8, ptr " << object << ", i64 -16\n";
        output_ << "  " << generation << " = load i64, ptr " << header << '\n';
        guard.reached = LinkParts{object, header, generation};
    }
}

void FunctionEmitter::endPlaceGuard()
{
    if (placeGuards_.empty()) {
        throw CodegenError("internal error: a place guard closed twice");
    }
    const PlaceGuard guard = placeGuards_.back();
    placeGuards_.pop_back();
    if (guard.pointer == nullptr) {
        return;
    }
    if (!guard.reached) {
        throw CodegenError("internal error: a guarded place whose pointer was not reached");
    }
    emitLinkCheck(*guard.reached);
}

// The pointer nearest to a place on its path (`L.FNotas` -> L,
// `A[I].FNo.FX` -> A[I].FNo, `Value(P)` -> P), or null when the path
// crosses none. Index expressions are not on the path.
const void* FunctionEmitter::lastPointerOnPath(const tir::Place& place)
{
    switch (place.kind) {
    case tir::PlaceKind::Through:
        return place.base;
    case tir::PlaceKind::Field:
    case tir::PlaceKind::Element:
        if (place.base->kind == tir::PlaceKind::Through) {
            return place.base->base;
        }
        return lastPointerOnPath(*place.base);
    default:
        return nullptr;
    }
}

void FunctionEmitter::emitStorageGuard(const std::string& normalizedName)
{
    if (const auto guard = storageGuards_.find(normalizedName); guard != storageGuards_.end()) {
        emitLinkCheck(guard->second);
    }
}

// The header and generation of the storage that holds a place.
FunctionEmitter::LinkParts FunctionEmitter::emitHeaderOfPlace(const tir::Place& place)
{
    switch (place.kind) {
    case tir::PlaceKind::Through:
        return emitHeaderOfPointer(*place.base);
    case tir::PlaceKind::Field:
    case tir::PlaceKind::Element:
        if (place.base->kind == tir::PlaceKind::Through) {
            return emitHeaderOfPointer(*place.base->base);
        }
        return emitHeaderOfPlace(*place.base);
    case tir::PlaceKind::Value:
        break;
    default: {
        const std::string name = normalize(place.symbol->name);
        if (const auto tombstone = tombstoneSlots_.find(name); tombstone != tombstoneSlots_.end()) {
            const std::string block = "%tmp" + std::to_string(nextTemporary_++);
            const std::string header = "%tmp" + std::to_string(nextTemporary_++);
            const std::string generation = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << block << " = load ptr, ptr " << tombstone->second << '\n';
            output_ << "  " << header << " = getelementptr i8, ptr " << block << ", i64 -16\n";
            output_ << "  " << generation << " = load i64, ptr " << header << '\n';
            return LinkParts{{}, header, generation};
        }
        // A State variable lives as long as the program.
        requireModuleLine("@__inox.immortal.header = internal global [2 x i64] [i64 1, i64 0], align 16");
        return LinkParts{{}, "@__inox.immortal.header", "1"};
    }
    }
    throw CodegenUnsupported("a Link to this place is not lowered yet");
}

FunctionEmitter::LinkParts FunctionEmitter::emitHeaderOfPointer(const tir::Place& pointer)
{
    return emitHeaderOfPointerValue(pointer.type, emitPlaceValue(pointer));
}

FunctionEmitter::LinkParts FunctionEmitter::emitHeaderOfPointerValue(TypeRef type, const std::string& value)
{
    if (isLinkType(type)) {
        return extractLink(value);
    }
    const std::string object = emitOwnerNullCheck(value);
    const std::string header = "%tmp" + std::to_string(nextTemporary_++);
    const std::string generation = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << header << " = getelementptr i8, ptr " << object << ", i64 -16\n";
    output_ << "  " << generation << " = load i64, ptr " << header << '\n';
    return LinkParts{object, header, generation};
}

std::string FunctionEmitter::emitLinkOf(const tir::Expr& argument)
{
    const TypeRef type = bare(argument).type;
    if (isLinkType(type)) {
        return emitValue(argument);
    }
    if (type->isOwn()) {
        return buildLink(emitHeaderOfPointerValue(type, emitValue(argument)));
    }
    if (type->isOptionOwn()) {
        // None gives an empty Link.
        const std::string object = emitValue(argument);
        const std::string label = std::to_string(nextLabel_++);
        const std::string isNull = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << isNull << " = icmp eq ptr " << object << ", null\n";
        output_ << "  br i1 " << isNull << ", label %optlink.none" << label << ", label %optlink.some" << label
                << "\n\n";
        output_ << "optlink.some" << label << ":\n";
        const std::string header = "%tmp" + std::to_string(nextTemporary_++);
        const std::string generation = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << header << " = getelementptr i8, ptr " << object << ", i64 -16\n";
        output_ << "  " << generation << " = load i64, ptr " << header << '\n';
        output_ << "  br label %optlink.end" << label << "\n\n";
        output_ << "optlink.none" << label << ":\n  br label %optlink.end" << label << "\n\n";
        output_ << "optlink.end" << label << ":\n";
        LinkParts parts;
        parts.target = object;
        parts.header = "%tmp" + std::to_string(nextTemporary_++);
        parts.generation = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << parts.header << " = phi ptr [ null, %optlink.none" << label << " ], [ " << header
                << ", %optlink.some" << label << " ]\n";
        output_ << "  " << parts.generation << " = phi i64 [ 0, %optlink.none" << label << " ], [ " << generation
                << ", %optlink.some" << label << " ]\n";
        return buildLink(parts);
    }
    // A variable, a field or an element: the place and the header of the
    // storage that holds it (the analyzer made the place side-effect free).
    const std::string target = emitPlace(argument).pointer;
    const tir::Expr& place = bare(argument);
    if (place.kind != tir::ExprKind::Read) {
        throw CodegenUnsupported("a Link to this place is not lowered yet");
    }
    LinkParts parts = emitHeaderOfPlace(*place.place);
    parts.target = target;
    return buildLink(parts);
}

std::string FunctionEmitter::emitNewTombstone()
{
    requireRuntime(RuntimeFunction::OwnAlloc);
    const std::string block = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << block << " = call ptr @__inox_own_alloc(i64 0)\n";
    const std::string label = std::to_string(nextLabel_++);
    const std::string isNull = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << isNull << " = icmp eq ptr " << block << ", null\n";
    output_ << "  br i1 " << isNull << ", label %tomb.oom" << label << ", label %tomb.ok" << label << "\n\n";
    output_ << "tomb.oom" << label << ":\n  call void @__inox_arith_fault(i32 12)\n  unreachable\n\n";
    output_ << "tomb.ok" << label << ":\n";
    return block;
}

// The variables some Link points into, directly or through fields and
// elements (not through another pointer): each gets a tombstone.
void FunctionEmitter::collectTombstoneLocals()
{
    std::unordered_set<std::string> stateNames;
    for (const StateGlobal& state : stateGlobals_) {
        stateNames.insert(normalize(state.name));
    }
    forEachStatement(routine_.body, [&](const tir::Stmt& statement) {
        forEachExpressionOf(statement, [&](const tir::Expr& expression) {
            if (expression.kind != tir::ExprKind::Builtin || expression.op != semantic::BuiltinOp::Link) {
                return;
            }
            const tir::Expr& argument = bare(*expression.operands[0]);
            if (isPointerType(argument.type) || argument.kind != tir::ExprKind::Read) {
                return;
            }
            const tir::Place* root = argument.place;
            while (root != nullptr && (root->kind == tir::PlaceKind::Field || root->kind == tir::PlaceKind::Element)) {
                root = root->base->kind == tir::PlaceKind::Through ? nullptr : root->base;
            }
            if (root != nullptr && isVariablePlace(*root)) {
                const std::string name = normalize(root->symbol->name);
                if (!stateNames.contains(name)) {
                    tombstoneLocals_.insert(name);
                }
            }
        });
    });
}

// A variable some Link points to gets a new tombstone at each declaration;
// the previous one (an earlier round of a loop) is freed, so its Links die.
void FunctionEmitter::declareTombstone(const std::string& normalizedName)
{
    if (!tombstoneLocals_.contains(normalizedName)) {
        return;
    }
    const std::string slot = "%tomb." + safeLlvmName(normalizedName) + "." + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca ptr\n";
    registerOwnedSlot(slot, nullptr, "ptr");
    ownedSlots_.back().tombstone = true;
    const std::string block = emitNewTombstone();
    emitTombstoneFree(slot);
    output_ << "  store ptr " << block << ", ptr " << slot << '\n';
    tombstoneSlots_.insert_or_assign(normalizedName, slot);
}

std::string FunctionEmitter::emitPointerBuiltin(const tir::Expr& call)
{
    const semantic::BuiltinOp op = call.op;
    const tir::Expr& argument = *call.operands[0];

    if (op == semantic::BuiltinOp::New) {
        const TypeRef ownType = call.type;
        const TypeRef targetType = ownType->pointerTarget();
        const std::string llvmType = valueLlvmType(targetType);
        requireRuntime(RuntimeFunction::OwnAlloc);
        const std::string end = "%tmp" + std::to_string(nextTemporary_++);
        const std::string size = "%tmp" + std::to_string(nextTemporary_++);
        const std::string object = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << end << " = getelementptr " << llvmType << ", ptr null, i32 1\n";
        output_ << "  " << size << " = ptrtoint ptr " << end << " to i64\n";
        output_ << "  " << object << " = call ptr @__inox_own_alloc(i64 " << size << ")\n";
        const std::string label = std::to_string(nextLabel_++);
        const std::string isNull = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << isNull << " = icmp eq ptr " << object << ", null\n";
        output_ << "  br i1 " << isNull << ", label %new.oom" << label << ", label %new.ok" << label << "\n\n";
        output_ << "new.oom" << label << ":\n  call void @__inox_arith_fault(i32 12)\n  unreachable\n\n";
        output_ << "new.ok" << label << ":\n";
        if (bare(argument).kind == tir::ExprKind::TypeValue) {
            if (!targetType->isStruct()) {
                throw CodegenError("New(T) of a type without defaults");
            }
            emitStructDefaults(object, targetType);
        } else {
            const std::string value = emitValue(argument);
            emitOwningStore(object, targetType, llvmType, value);
        }
        holdOwnedResult(object, ownType, "ptr");
        return object;
    }

    if (op == semantic::BuiltinOp::Move || op == semantic::BuiltinOp::Free) {
        const TypeRef type = bare(argument).type;
        const std::string llvmType = valueLlvmType(type);
        const Place place = emitPlace(argument);
        if (op == semantic::BuiltinOp::Free) {
            emitReleaseAt(place.pointer, type);
            output_ << "  store " << llvmType << " zeroinitializer, ptr " << place.pointer << '\n';
            return {};
        }
        const std::string value = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << value << " = load " << llvmType << ", ptr " << place.pointer << '\n';
        output_ << "  store " << llvmType << " zeroinitializer, ptr " << place.pointer << '\n';
        holdOwnedResult(value, type, llvmType);
        return value;
    }

    if (op == semantic::BuiltinOp::Link) {
        return emitLinkOf(argument);
    }

    // Alive and Same: whether each pointer reaches an object, and which.
    const auto aliveAndTarget = [&](const tir::Expr& pointer) {
        const TypeRef type = bare(pointer).type;
        const std::string value = emitValue(pointer);
        if (isLinkType(type)) {
            const LinkParts parts = extractLink(value);
            return std::make_pair(emitLinkAlive(parts), parts.target);
        }
        const std::string alive = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << alive << " = icmp ne ptr " << value << ", null\n";
        return std::make_pair(alive, value);
    };
    if (op == semantic::BuiltinOp::Alive) {
        return aliveAndTarget(argument).first;
    }
    const auto [firstAlive, firstTarget] = aliveAndTarget(argument);
    const auto [secondAlive, secondTarget] = aliveAndTarget(*call.operands[1]);
    const std::string both = "%tmp" + std::to_string(nextTemporary_++);
    const std::string same = "%tmp" + std::to_string(nextTemporary_++);
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << both << " = and i1 " << firstAlive << ", " << secondAlive << '\n';
    output_ << "  " << same << " = icmp eq ptr " << firstTarget << ", " << secondTarget << '\n';
    output_ << "  " << result << " = and i1 " << both << ", " << same << '\n';
    return result;
}

bool FunctionEmitter::isPointerBuiltin(semantic::BuiltinOp op)
{
    switch (op) {
    case semantic::BuiltinOp::New:
    case semantic::BuiltinOp::Free:
    case semantic::BuiltinOp::Move:
    case semantic::BuiltinOp::Link:
    case semantic::BuiltinOp::Alive:
    case semantic::BuiltinOp::Same:
        return true;
    default:
        return false;
    }
}

} // namespace inox::compiler::codegen::detail
