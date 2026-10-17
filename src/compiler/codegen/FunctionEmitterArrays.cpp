// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: array indexing, bounds, slices and element access (CANON-13, ADR-0017).

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

// ADR-0015: an Integer expression stored into a Natural carries an
// implicit range check, recorded by semantic analysis.
bool FunctionEmitter::isIndexCall(const ast::Expression& expression)
{
    if (expression.kind() != ast::AstNodeKind::CallExpression) {
        return false;
    }
    const auto& call = static_cast<const ast::CallExpression&>(expression);
    return call.callee().kind() == ast::AstNodeKind::IdentifierExpression &&
           static_cast<const ast::IdentifierExpression&>(call.callee()).name() == "__index" &&
           call.arguments().size() == 2;
}

// The LLVM type of a value in SSA form; i64 for a type without one.
std::string FunctionEmitter::valueLlvmType(TypeRef type)
{
    const std::string llvmType = llvmValueType(type);
    return llvmType.empty() ? std::string("i64") : llvmType;
}

// Low(A), High(A) and Length(A) of a fixed array are constants (CANON-13).
std::optional<std::int64_t> FunctionEmitter::arrayBoundCall(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 1) {
        return std::nullopt;
    }
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    const bool low = equalsIgnoreCase(callee.name(), "Low");
    const bool high = equalsIgnoreCase(callee.name(), "High");
    if (!low && !high && !equalsIgnoreCase(callee.name(), "Length")) {
        return std::nullopt;
    }
    const TypeRef type = typeOf(*call.arguments()[0]);
    if (!isArrayType(type)) {
        return std::nullopt;
    }
    return low ? type->low() : high ? type->high() : type->length();
}

// The address of an array value: a local's slot, an element of an outer
// array, or a temporary holding any other array value.
std::string FunctionEmitter::emitArrayBaseAddress(const ast::Expression& base, const ArrayLayout& layout)
{
    if (base.kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto local = locals_.find(normalize(static_cast<const ast::IdentifierExpression&>(base).name()));
        if (local != locals_.end()) {
            return local->second.slot;
        }
    }
    if (isIndexCall(base)) {
        return emitElementAddress(static_cast<const ast::CallExpression&>(base)).pointer;
    }
    if (isMemberAccessCall(base) && memberField(static_cast<const ast::CallExpression&>(base))) {
        return emitPlace(base).pointer;
    }
    const std::string value = emitExpression(base);
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
FunctionEmitter::SliceParts FunctionEmitter::emitSliceSource(const ast::Expression& base, std::string& storage, TypeRef& elementType)
{
    const TypeRef baseType = typeOf(base);
    if (isSliceType(baseType)) {
        elementType = baseType->element();
        storage = llvmStorageType(elementType);
        return extractSlice(emitExpression(base));
    }
    const auto layout = arrayLayout(baseType);
    if (!layout) {
        throw CodegenUnsupported("slicing of this value is not lowered yet");
    }
    elementType = layout->element;
    storage = layout->elementStorage;
    const std::string pointer = emitArrayBaseAddress(base, *layout);
    const std::string data = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << data << " = getelementptr inbounds " << layout->llvmType << ", ptr " << pointer
            << ", i64 0, i64 0\n";
    return SliceParts{data, std::to_string(layout->low), std::to_string(layout->high)};
}

// ADR-0017: `B[L..H]` checks Low(B) <= L <= H <= High(B) (a slice is never
// empty, like an array), then points at element L.
std::string FunctionEmitter::emitSlice(const ast::CallExpression& call)
{
    std::string storage;
    TypeRef elementType = nullptr;
    const SliceParts source = emitSliceSource(*call.arguments()[0], storage, elementType);
    const auto bound = [&](const ast::Expression& expression, const std::string& fallback) {
        if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
            const std::string& name = static_cast<const ast::IdentifierExpression&>(expression).name();
            if (name == "__low" || name == "__high") {
                return fallback;
            }
        }
        return emitExpression(expression);
    };
    const std::string low = bound(*call.arguments()[1], source.low);
    const std::string high = bound(*call.arguments()[2], source.high);
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
FunctionEmitter::ElementAddress FunctionEmitter::emitElementAddress(const ast::CallExpression& call)
{
    const ast::Expression& base = *call.arguments()[0];
    if (isSliceType(typeOf(base))) {
        std::string storage;
        TypeRef elementType = nullptr;
        const SliceParts slice = emitSliceSource(base, storage, elementType);
        const std::string index = emitExpression(*call.arguments()[1]);
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
    const TypeRef baseType = typeOf(base);
    const bool throughPointer = isPointerType(baseType);
    const auto layout = arrayLayout(throughPointer ? baseType->pointerTarget() : baseType);
    if (!layout) {
        throw CodegenUnsupported("indexing of this value is not lowered yet");
    }
    const std::string basePointer =
        throughPointer ? emitPointerTarget(base) : emitArrayBaseAddress(base, *layout);
    const std::string index = emitExpression(*call.arguments()[1]);
    const std::string checked = emitRangeCheck(index, IntegerRange{layout->low, layout->high}, 9);
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

} // namespace inox::compiler::codegen::detail
