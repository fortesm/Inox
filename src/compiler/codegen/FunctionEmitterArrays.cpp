// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: array indexing, bounds, slices and element access (CANON-13, ADR-0017).

#include "FunctionEmitter.h"

#include "ExceptionAbi.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "MathBuiltins.h"
#include "Operators.h"
#include "../support/Platform.h"

#include <algorithm>
#include <limits>
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

// The LLVM type of a value in SSA form; i64 for a type without one.
std::string FunctionEmitter::valueLlvmType(TypeRef type)
{
    const std::string llvmType = llvmValueType(type);
    return llvmType.empty() ? std::string("i64") : llvmType;
}

// The address of an array value: a local's slot, an element of an outer
// array, a field, or a temporary holding any other array value.
std::string FunctionEmitter::emitArrayBaseAddress(const tir::Place& base, const ArrayLayout& layout)
{
    if (isVariablePlace(base)) {
        const auto local = locals_.find(normalize(base.symbol->name));
        if (local != locals_.end()) {
            emitStorageGuard(local->first);
            return local->second.slot;
        }
    }
    if (base.kind == tir::PlaceKind::Element) {
        return emitElementAddress(base).pointer;
    }
    if (base.kind == tir::PlaceKind::Field) {
        return emitPlace(base).pointer;
    }
    return storeArrayTemporary(base.kind == tir::PlaceKind::Value ? emitValue(*base.value) : emitPlaceValue(base),
                               layout);
}

std::string FunctionEmitter::emitArrayBaseAddress(const tir::Expr& base, const ArrayLayout& layout)
{
    if (bare(base).kind == tir::ExprKind::Read) {
        return emitArrayBaseAddress(*bare(base).place, layout);
    }
    return storeArrayTemporary(emitValue(base), layout);
}

std::string FunctionEmitter::storeArrayTemporary(const std::string& value, const ArrayLayout& layout)
{
    const std::string slot = "%arraytmp" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = alloca " << layout.llvmType << '\n';
    output_ << "  store " << layout.llvmType << ' ' << value << ", ptr " << slot << '\n';
    return slot;
}

FunctionEmitter::SliceParts FunctionEmitter::extractSlice(const std::string& value)
{
    SliceParts parts;
    parts.data = "%tmp" + std::to_string(nextTemporary_++);
    parts.low = "%tmp" + std::to_string(nextTemporary_++);
    parts.high = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << parts.data << " = extractvalue " << kSliceLlvmType << ' ' << value << ", 0\n";
    output_ << "  " << parts.low << " = extractvalue " << kSliceLlvmType << ' ' << value << ", 1\n";
    output_ << "  " << parts.high << " = extractvalue " << kSliceLlvmType << ' ' << value << ", 2\n";
    return parts;
}

// The base of a slice expression: its data pointer and bounds, and the
// LLVM storage of one element.
FunctionEmitter::SliceParts FunctionEmitter::emitSliceSource(const tir::Expr& base, std::string& storage,
                                                             TypeRef& elementType)
{
    const tir::Expr& value = bare(base);
    if (value.kind == tir::ExprKind::Read) {
        return emitSliceSource(*value.place, storage, elementType);
    }
    const TypeRef baseType = value.type;
    if (isSliceType(baseType)) {
        elementType = baseType->element();
        storage = llvmStorageType(elementType);
        return extractSlice(emitValue(base));
    }
    const auto layout = arrayLayout(baseType);
    if (!layout) {
        throw CodegenUnsupported("slicing of this value is not lowered yet");
    }
    elementType = layout->element;
    storage = layout->elementStorage;
    return arraySliceParts(emitArrayBaseAddress(base, *layout), *layout);
}

FunctionEmitter::SliceParts FunctionEmitter::emitSliceSource(const tir::Place& base, std::string& storage,
                                                             TypeRef& elementType)
{
    if (base.kind == tir::PlaceKind::Value) {
        return emitSliceSource(*base.value, storage, elementType);
    }
    const TypeRef baseType = base.type;
    if (isSliceType(baseType)) {
        elementType = baseType->element();
        storage = llvmStorageType(elementType);
        return extractSlice(emitPlaceValue(base));
    }
    const auto layout = arrayLayout(baseType);
    if (!layout) {
        throw CodegenUnsupported("slicing of this value is not lowered yet");
    }
    elementType = layout->element;
    storage = layout->elementStorage;
    return arraySliceParts(emitArrayBaseAddress(base, *layout), *layout);
}

FunctionEmitter::SliceParts FunctionEmitter::arraySliceParts(const std::string& pointer, const ArrayLayout& layout)
{
    const std::string data = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << data << " = getelementptr inbounds " << layout.llvmType << ", ptr " << pointer
            << ", i64 0, i64 0\n";
    return SliceParts{data, std::to_string(layout.low), std::to_string(layout.high)};
}

// ADR-0017: `B[L..H]` checks Low(B) <= L <= H <= High(B) (a slice is never
// empty, like an array), then points at element L.
std::string FunctionEmitter::emitSlice(const tir::Expr& slice)
{
    std::string storage;
    TypeRef elementType = nullptr;
    beginSliceGuard(slice);
    const SliceParts source = emitSliceSource(*slice.operands[0], storage, elementType);
    // An omitted bound is the base's own.
    const std::string low = slice.hasLow ? emitValue(*slice.operands[1]) : source.low;
    const std::string high = slice.hasHigh ? emitValue(*slice.operands[slice.hasLow ? 2 : 1]) : source.high;
    endPlaceGuard();
    const std::string lowOk = "%tmp" + std::to_string(nextTemporary_++);
    const std::string highOk = "%tmp" + std::to_string(nextTemporary_++);
    const std::string orderOk = "%tmp" + std::to_string(nextTemporary_++);
    const std::string both = "%tmp" + std::to_string(nextTemporary_++);
    const std::string valid = "%tmp" + std::to_string(nextTemporary_++);
    const std::string flag = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << lowOk << " = icmp sge i64 " << low << ", " << source.low << '\n';
    output_ << "  " << highOk << " = icmp sle i64 " << high << ", " << source.high << '\n';
    output_ << "  " << orderOk << " = icmp sle i64 " << low << ", " << high << '\n';
    output_ << "  " << both << " = and i1 " << lowOk << ", " << highOk << '\n';
    output_ << "  " << valid << " = and i1 " << both << ", " << orderOk << '\n';
    output_ << "  " << flag << " = zext i1 " << valid << " to i64\n";
    (void)emitRangeCheck(flag, IntegerRange{1, 1}, 10);
    const std::string offset = "%tmp" + std::to_string(nextTemporary_++);
    const std::string data = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << offset << " = sub i64 " << low << ", " << source.low << '\n';
    output_ << "  " << data << " = getelementptr inbounds " << storage << ", ptr " << source.data
            << ", i64 " << offset << '\n';
    const std::string withData = "%tmp" + std::to_string(nextTemporary_++);
    const std::string withLow = "%tmp" + std::to_string(nextTemporary_++);
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << withData << " = insertvalue " << kSliceLlvmType << " undef, ptr " << data << ", 0\n";
    output_ << "  " << withLow << " = insertvalue " << kSliceLlvmType << ' ' << withData << ", i64 " << low
            << ", 1\n";
    output_ << "  " << result << " = insertvalue " << kSliceLlvmType << ' ' << withLow << ", i64 " << high
            << ", 2\n";
    return result;
}

// CANON-13: every index is checked against Low..High (fault kind 9).
FunctionEmitter::ElementAddress FunctionEmitter::emitElementAddress(const tir::Place& element)
{
    const tir::Place& base = *element.base;
    const tir::Expr& indexValue = *element.indices[0];
    if (isSliceType(base.type)) {
        std::string storage;
        TypeRef elementType = nullptr;
        beginElementGuard(element);
        const SliceParts slice = emitSliceSource(base, storage, elementType);
        std::string index = emitValue(indexValue);
        endPlaceGuard();
        if (isUInt64Type(bare(indexValue).type)) {
            // Above Int64.Max is outside every slice (change uint64).
            index = emitRangeCheck(index, IntegerRange{0, std::numeric_limits<std::int64_t>::max()}, 9);
        }
        const std::string checked = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << checked << " = call i64 @__inox_range_i64(i64 " << index << ", i64 "
                << slice.low << ", i64 " << slice.high << ", i32 9)\n";
        const std::string offset = "%tmp" + std::to_string(nextTemporary_++);
        const std::string pointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << offset << " = sub i64 " << checked << ", " << slice.low << '\n';
        output_ << "  " << pointer << " = getelementptr inbounds " << storage << ", ptr " << slice.data
                << ", i64 " << offset << '\n';
        return ElementAddress{pointer, storage, elementType};
    }
    // Safe pointers: through a pointer, the array is the object it reaches.
    const bool throughPointer = base.kind == tir::PlaceKind::Through;
    const auto layout = arrayLayout(base.type);
    if (!layout) {
        throw CodegenUnsupported("indexing of this value is not lowered yet");
    }
    beginElementGuard(element);
    const std::string basePointer =
        throughPointer ? emitPointerTarget(*base.base) : emitArrayBaseAddress(base, *layout);
    const std::string index = emitValue(indexValue);
    endPlaceGuard();
    // A UInt64 index above Int64.Max is a negative i64: a low bound of at
    // least 0 rejects it (change uint64).
    const std::int64_t low =
        isUInt64Type(bare(indexValue).type) ? std::max<std::int64_t>(layout->low, 0) : layout->low;
    const std::string checked = emitRangeCheck(index, IntegerRange{low, layout->high}, 9);
    std::string offset = checked;
    if (layout->low != 0) {
        offset = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << offset << " = sub i64 " << checked << ", " << layout->low << '\n';
    }
    const std::string pointer = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << pointer << " = getelementptr inbounds " << layout->llvmType << ", ptr "
            << basePointer << ", i64 0, i64 " << offset << '\n';
    return ElementAddress{pointer, layout->elementStorage, layout->element};
}

std::string FunctionEmitter::loadElement(const ElementAddress& element)
{
    const std::string loaded = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << loaded << " = load " << element.storage << ", ptr " << element.pointer << '\n';
    const auto range = narrowIntegerRange(element.elementType);
    if (!range) {
        return loaded;
    }
    const std::string widened = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << widened << " = " << (range->low < 0 ? "sext " : "zext ") << element.storage
            << ' ' << loaded << " to i64\n";
    return widened;
}

// A narrow integer is already in range; storing it keeps its low bits.
std::string FunctionEmitter::narrowForStorage(const std::string& value, const ElementAddress& element)
{
    if (!narrowIntegerRange(element.elementType)) {
        return value;
    }
    const std::string truncated = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << truncated << " = trunc i64 " << value << " to " << element.storage << '\n';
    return truncated;
}

// `Fill(Value)` (r3.81): Value is evaluated once and copied into every
// element (or row) of a temporary array, which is the call's value; the
// store that takes it retains what it holds, and the temporary is released
// at the end of the statement. An array of owners is only filled with None:
// all of it is null.
std::string FunctionEmitter::emitFill(const tir::Expr& call)
{
    const TypeRef arrayType = call.type;
    const TypeRef unit = call.unit;
    const std::string llvmType = llvmValueType(arrayType);
    if (isMoveOnly(unit)) {
        return "zeroinitializer";
    }
    std::string value = emitValue(*call.operands[0]);
    const std::string storage = llvmStorageType(unit);
    if (narrowIntegerRange(unit)) {
        const std::string truncated = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << truncated << " = trunc i64 " << value << " to " << storage << '\n';
        value = truncated;
    }
    const std::int64_t units = flattenArray(arrayType).count / (isArrayType(unit) ? flattenArray(unit).count : 1);
    const std::string array = "%fill" + std::to_string(nextTemporary_++);
    output_ << "  " << array << " = alloca " << llvmType << '\n';
    output_ << "  store " << llvmType << " zeroinitializer, ptr " << array << '\n';
    const bool owns = typeOwnsReferences(unit);
    const std::string label = std::to_string(nextLabel_++);
    const std::string counter = "%fill.i" + label;
    output_ << "  " << counter << " = alloca i64\n";
    output_ << "  store i64 0, ptr " << counter << '\n';
    output_ << "  br label %fill.cond" << label << "\n\n";
    output_ << "fill.cond" << label << ":\n";
    const std::string index = "%tmp" + std::to_string(nextTemporary_++);
    const std::string more = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << index << " = load i64, ptr " << counter << '\n';
    output_ << "  " << more << " = icmp slt i64 " << index << ", " << units << '\n';
    output_ << "  br i1 " << more << ", label %fill.body" << label << ", label %fill.end" << label << "\n\n";
    output_ << "fill.body" << label << ":\n";
    const std::string element = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << element << " = getelementptr inbounds " << storage << ", ptr " << array << ", i64 " << index
            << '\n';
    if (owns) {
        emitOwningStore(element, unit, storage, value);
    } else {
        output_ << "  store " << storage << ' ' << value << ", ptr " << element << '\n';
    }
    const std::string next = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << next << " = add i64 " << index << ", 1\n";
    output_ << "  store i64 " << next << ", ptr " << counter << '\n';
    output_ << "  br label %fill.cond" << label << "\n\n";
    output_ << "fill.end" << label << ":\n";
    const std::string filled = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << filled << " = load " << llvmType << ", ptr " << array << '\n';
    if (owns) {
        holdOwnedResult(filled, arrayType, llvmType);
    }
    return filled;
}

} // namespace inox::compiler::codegen::detail
