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

#include "AstQueries.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "../support/ArrayType.h"
#include "../support/PointerType.h"

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>

namespace inox::compiler::codegen::detail {

namespace {

constexpr std::string_view kTombstoneType = "__Tombstone";

const std::string& calleeName(const ast::CallExpression& call)
{
    return static_cast<const ast::IdentifierExpression&>(call.callee()).name();
}

bool isOwnerPointer(std::string_view type)
{
    return support::isOwnTypeName(type) || support::isOptionOwnTypeName(type);
}

} // namespace

bool FunctionEmitter::isMoveOnly(std::string_view inoxType) const
{
    if (isOwnerPointer(inoxType)) {
        return true;
    }
    if (support::isLinkTypeName(inoxType) || support::isSliceTypeName(inoxType)) {
        return false;
    }
    if (const auto layout = arrayLayout(inoxType)) {
        return isMoveOnly(layout->elementType);
    }
    if (const StructDefinition* structType = findStruct(structs_, inoxType)) {
        for (const StructFieldInfo& field : structType->fields) {
            if (isMoveOnly(field.inoxType)) {
                return true;
            }
        }
    }
    return false;
}

bool FunctionEmitter::isPointerBuiltin(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
        return false;
    }
    const std::string& name = calleeName(call);
    const std::size_t count = call.arguments().size();
    const bool known = ((equalsIgnoreCase(name, "New") || equalsIgnoreCase(name, "Free") ||
                         equalsIgnoreCase(name, "Move") || equalsIgnoreCase(name, "Value") ||
                         equalsIgnoreCase(name, "Link") || equalsIgnoreCase(name, "Alive")) &&
                        count == 1) ||
                       (equalsIgnoreCase(name, "Same") && count == 2);
    return known && signatures_.find(normalize(name)) == signatures_.end();
}

// Walks a value of `inoxType` stored at `pointer` (struct fields and array
// elements, recursively) and calls `action` on every leaf `isLeaf` accepts.
void FunctionEmitter::forEachLeafAt(const std::string& pointer, std::string_view inoxType,
                                    const std::function<bool(std::string_view)>& isLeaf,
                                    const std::function<void(const std::string&, std::string_view)>& action)
{
    if (isLeaf(inoxType)) {
        action(pointer, inoxType);
        return;
    }
    if (const StructDefinition* structType = findStruct(structs_, inoxType)) {
        for (const StructFieldInfo& field : structType->fields) {
            if (!typeOwnsReferences(field.inoxType)) {
                continue;
            }
            const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << fieldPointer << " = getelementptr " << structType->llvmName << ", ptr " << pointer
                    << ", i32 0, i32 " << field.index << '\n';
            forEachLeafAt(fieldPointer, field.inoxType, isLeaf, action);
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
    if (!typeOwnsReferences(element)) {
        return;
    }
    const std::string storage = llvmTypeForInoxType(element, structs_);
    const std::string label = std::to_string(nextLabel_++);
    const std::string counter = "%leafwalk.i" + label;
    output_ << "  " << counter << " = alloca i64\n";
    output_ << "  store i64 0, ptr " << counter << '\n';
    output_ << "  br label %leafwalk.cond" << label << "\n\n";
    output_ << "leafwalk.cond" << label << ":\n";
    const std::string index = "%tmp" + std::to_string(nextTemporary_++);
    const std::string more = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << index << " = load i64, ptr " << counter << '\n';
    output_ << "  " << more << " = icmp slt i64 " << index << ", " << count << '\n';
    output_ << "  br i1 " << more << ", label %leafwalk.body" << label << ", label %leafwalk.end" << label
            << "\n\n";
    output_ << "leafwalk.body" << label << ":\n";
    const std::string elementPointer = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << elementPointer << " = getelementptr inbounds " << storage << ", ptr " << pointer << ", i64 "
            << index << '\n';
    forEachLeafAt(elementPointer, element, isLeaf, action);
    const std::string next = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << next << " = add i64 " << index << ", 1\n";
    output_ << "  store i64 " << next << ", ptr " << counter << '\n';
    output_ << "  br label %leafwalk.cond" << label << "\n\n";
    output_ << "leafwalk.end" << label << ":\n";
}

// Frees every owner held by the value at `pointer` (and the tombstone of a
// tombstone slot).
void FunctionEmitter::emitOwnerFreesAt(const std::string& pointer, std::string_view inoxType)
{
    if (inoxType == kTombstoneType) {
        requireRuntime(RuntimeFunction::OwnFree);
        const std::string tombstone = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << tombstone << " = load ptr, ptr " << pointer << '\n';
        output_ << "  call void @__inox_own_free(ptr " << tombstone << ", ptr null)\n";
        return;
    }
    forEachLeafAt(
        pointer, inoxType, [](std::string_view type) { return isOwnerPointer(type); },
        [&](const std::string& at, std::string_view type) {
            requireRuntime(RuntimeFunction::OwnFree);
            const std::string drop = dropFunctionFor(support::pointerTargetTypeName(type));
            const std::string object = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << object << " = load ptr, ptr " << at << '\n';
            output_ << "  call void @__inox_own_free(ptr " << object << ", ptr " << drop << ")\n";
        });
}

// The function that releases what an object of `targetType` holds (its
// Strings and owners) before its block is freed; "null" when it holds none.
std::string FunctionEmitter::dropFunctionFor(const std::string& targetType)
{
    if (!typeOwnsReferences(targetType)) {
        return "null";
    }
    const std::string key = normalize(targetType);
    if (const auto existing = module_.dropFunctions.find(key); existing != module_.dropFunctions.end()) {
        return existing->second;
    }
    const std::string name = "@__inox_drop." + std::to_string(module_.dropFunctions.size());
    module_.dropFunctions.emplace(key, name);
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
    requireModuleLine("; the drop function of " + targetType + " (safe pointers)\n" + definition);
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

std::string FunctionEmitter::emitPointerTarget(const ast::Expression& pointer)
{
    const std::string type = semanticTypeName(pointer);
    const std::string value = emitExpression(pointer);
    if (support::isLinkTypeName(type)) {
        const LinkParts parts = extractLink(value);
        emitLinkCheck(parts);
        return parts.target;
    }
    return emitOwnerNullCheck(value);
}

// The header and generation of the storage that holds a place.
FunctionEmitter::LinkParts FunctionEmitter::emitHeaderOfPlace(const ast::Expression& place)
{
    if (place.kind() == ast::AstNodeKind::IdentifierExpression) {
        const std::string name = normalize(static_cast<const ast::IdentifierExpression&>(place).name());
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
    if (place.kind() == ast::AstNodeKind::CallExpression) {
        const auto& call = static_cast<const ast::CallExpression&>(place);
        if (isPointerBuiltin(call) && equalsIgnoreCase(calleeName(call), "Value")) {
            return emitHeaderOfPointer(*call.arguments()[0]);
        }
        if (isMemberAccessCall(call) || isIndexCall(call)) {
            const ast::Expression& base = *call.arguments()[0];
            if (support::isPointerTypeName(semanticTypeName(base))) {
                return emitHeaderOfPointer(base);
            }
            return emitHeaderOfPlace(base);
        }
    }
    throw CodegenUnsupported("a Link to this place is not lowered yet");
}

FunctionEmitter::LinkParts FunctionEmitter::emitHeaderOfPointer(const ast::Expression& pointer)
{
    const std::string type = semanticTypeName(pointer);
    const std::string value = emitExpression(pointer);
    if (support::isLinkTypeName(type)) {
        return extractLink(value);
    }
    const std::string object = emitOwnerNullCheck(value);
    const std::string header = "%tmp" + std::to_string(nextTemporary_++);
    const std::string generation = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << header << " = getelementptr i8, ptr " << object << ", i64 -16\n";
    output_ << "  " << generation << " = load i64, ptr " << header << '\n';
    return LinkParts{object, header, generation};
}

std::string FunctionEmitter::emitLinkOf(const ast::Expression& argument)
{
    const std::string type = semanticTypeName(argument);
    if (support::isLinkTypeName(type)) {
        return emitExpression(argument);
    }
    if (support::isOwnTypeName(type)) {
        return buildLink(emitHeaderOfPointer(argument));
    }
    if (support::isOptionOwnTypeName(type)) {
        // None gives an empty Link.
        const std::string object = emitExpression(argument);
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
    LinkParts parts = emitHeaderOfPlace(argument);
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
    forEachStatement(function_.body(), [&](const ast::Statement& statement) {
        forEachExpressionOf(statement, [&](const ast::Expression& expression) {
            if (expression.kind() != ast::AstNodeKind::CallExpression) {
                return;
            }
            const auto& call = static_cast<const ast::CallExpression&>(expression);
            if (!isPointerBuiltin(call) || !equalsIgnoreCase(calleeName(call), "Link") ||
                support::isPointerTypeName(semanticTypeName(*call.arguments()[0]))) {
                return;
            }
            const ast::Expression* root = call.arguments()[0].get();
            while (root != nullptr && root->kind() == ast::AstNodeKind::CallExpression) {
                const auto& inner = static_cast<const ast::CallExpression&>(*root);
                if (!(isMemberAccessCall(inner) || isIndexCall(inner))) {
                    root = nullptr;
                    break;
                }
                const ast::Expression& base = *inner.arguments()[0];
                root = support::isPointerTypeName(semanticTypeName(base)) ? nullptr : &base;
            }
            if (root != nullptr && root->kind() == ast::AstNodeKind::IdentifierExpression) {
                const std::string name = normalize(static_cast<const ast::IdentifierExpression&>(*root).name());
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
    registerOwnedSlot(slot, kTombstoneType, "ptr");
    const std::string block = emitNewTombstone();
    emitOwningStore(slot, kTombstoneType, "ptr", block);
    tombstoneSlots_.insert_or_assign(normalizedName, slot);
}

std::string FunctionEmitter::emitPointerBuiltin(const ast::CallExpression& call)
{
    const std::string& name = calleeName(call);
    const ast::Expression& argument = *call.arguments()[0];

    if (equalsIgnoreCase(name, "New")) {
        const std::string ownType = semanticTypeName(call);
        const std::string targetType = support::pointerTargetTypeName(ownType);
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
        const semantic::Symbol* typeSymbol =
            argument.kind() == ast::AstNodeKind::IdentifierExpression
                ? semantics_.symbolOf(static_cast<const ast::IdentifierExpression&>(argument))
                : nullptr;
        if (typeSymbol != nullptr && typeSymbol->kind == semantic::SymbolKind::Type) {
            const StructDefinition* structType = findStruct(structs_, targetType);
            if (structType == nullptr) {
                throw CodegenError("New(T) of a type without defaults");
            }
            emitStructDefaults(object, *structType);
        } else {
            const std::string value = emitExpression(argument);
            emitOwningStore(object, targetType, llvmType, value);
        }
        holdOwnedResult(object, ownType, "ptr");
        return object;
    }

    if (equalsIgnoreCase(name, "Move") || equalsIgnoreCase(name, "Free")) {
        const std::string type = semanticTypeName(argument);
        const std::string llvmType = valueLlvmType(type);
        const Place place = emitPlace(argument);
        if (equalsIgnoreCase(name, "Free")) {
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

    if (equalsIgnoreCase(name, "Value")) {
        const std::string type = semanticTypeName(call);
        const std::string target = emitPointerTarget(argument);
        const std::string value = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << value << " = load " << valueLlvmType(type) << ", ptr " << target << '\n';
        return value;
    }

    if (equalsIgnoreCase(name, "Link")) {
        return emitLinkOf(argument);
    }

    // Alive and Same: whether each pointer reaches an object, and which.
    const auto aliveAndTarget = [&](const ast::Expression& pointer) {
        const std::string type = semanticTypeName(pointer);
        const std::string value = emitExpression(pointer);
        if (support::isLinkTypeName(type)) {
            const LinkParts parts = extractLink(value);
            return std::make_pair(emitLinkAlive(parts), parts.target);
        }
        const std::string alive = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << alive << " = icmp ne ptr " << value << ", null\n";
        return std::make_pair(alive, value);
    };
    if (equalsIgnoreCase(name, "Alive")) {
        return aliveAndTarget(argument).first;
    }
    const auto [firstAlive, firstTarget] = aliveAndTarget(argument);
    const auto [secondAlive, secondTarget] = aliveAndTarget(*call.arguments()[1]);
    const std::string both = "%tmp" + std::to_string(nextTemporary_++);
    const std::string same = "%tmp" + std::to_string(nextTemporary_++);
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << both << " = and i1 " << firstAlive << ", " << secondAlive << '\n';
    output_ << "  " << same << " = icmp eq ptr " << firstTarget << ", " << secondTarget << '\n';
    output_ << "  " << result << " = and i1 " << both << ", " << same << '\n';
    return result;
}

} // namespace inox::compiler::codegen::detail
