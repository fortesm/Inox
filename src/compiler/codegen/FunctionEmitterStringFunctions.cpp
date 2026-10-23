// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: String concatenation and the functions of Std.Strings and
// Std.StrUtils (OpenSpec change std-strings, design sections 1 and 5).
//
// - `A + B + C` is one call to __inox_str_concat: one allocation, each part
//   copied once.
// - `S := S + X ...` and `S += X` append to S in place when nothing else
//   holds its block (__inox_str_append), so a loop of appends costs O(N).
// - Every String made is owned (+1) and held by a statement temporary, like
//   any call result; a failure status stops the program (kind 15 out of
//   range, 12 out of memory).

#include "FunctionEmitter.h"

#include "AstQueries.h"
#include "LlvmIrEmitter.h"
#include "LlvmText.h"

#include <string>
#include <vector>

namespace inox::compiler::codegen::detail {

namespace {

bool isConcatenation(const ast::Expression& expression, const semantic::SemanticResult& semantics)
{
    if (expression.kind() != ast::AstNodeKind::BinaryExpression ||
        static_cast<const ast::BinaryExpression&>(expression).op() != ast::BinaryOperator::Add) {
        return false;
    }
    return isStringType(semantics.typeOf(expression));
}

// The operands of a `+` chain of Strings, left to right (`A + B + C` is
// `(A + B) + C`; a parenthesized `A + (B + C)` flattens the same way).
void collectParts(const ast::Expression& expression, const semantic::SemanticResult& semantics,
                  std::vector<const ast::Expression*>& parts)
{
    if (isConcatenation(expression, semantics)) {
        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        collectParts(binary.left(), semantics, parts);
        collectParts(binary.right(), semantics, parts);
        return;
    }
    parts.push_back(&expression);
}

} // namespace

bool FunctionEmitter::isStringConcatenation(const ast::Expression& expression) const
{
    return isConcatenation(expression, semantics_);
}

// The status slot of a runtime String function.
std::string FunctionEmitter::newStringStatus()
{
    const std::string status = "%strstatus" + std::to_string(nextTemporary_++);
    output_ << "  " << status << " = alloca i32\n";
    return status;
}

// A failed String function stops the program: one cold helper per module.
void FunctionEmitter::emitStringStatusCheck(const std::string& status)
{
    requireModuleLine("define internal void @__inox_str_fault(i32 %status) noreturn nounwind cold noinline {\n"
                      "entry:\n"
                      "  %range = icmp eq i32 %status, 1\n"
                      "  %kind = select i1 %range, i32 15, i32 12\n"
                      "  call void @__inox_arith_fault(i32 %kind)\n"
                      "  unreachable\n"
                      "}");
    const std::string label = std::to_string(nextLabel_++);
    const std::string code = "%tmp" + std::to_string(nextTemporary_++);
    const std::string failed = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << code << " = load i32, ptr " << status << '\n';
    output_ << "  " << failed << " = icmp ne i32 " << code << ", 0\n";
    output_ << "  br i1 " << failed << ", label %str.fail" << label << ", label %str.ok" << label << "\n\n";
    output_ << "str.fail" << label << ":\n  call void @__inox_str_fault(i32 " << code << ")\n  unreachable\n\n";
    output_ << "str.ok" << label << ":\n";
}

// The parts as an array of String pointers, for concat and append.
std::string FunctionEmitter::emitStringParts(const std::vector<const ast::Expression*>& parts)
{
    std::vector<std::string> values;
    values.reserve(parts.size());
    for (const ast::Expression* part : parts) {
        values.push_back(emitExpression(*part));
    }
    const std::string array = "%strparts" + std::to_string(nextTemporary_++);
    output_ << "  " << array << " = alloca [" << values.size() << " x ptr]\n";
    for (std::size_t index = 0; index < values.size(); ++index) {
        const std::string element = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << element << " = getelementptr [" << values.size() << " x ptr], ptr " << array
                << ", i64 0, i64 " << index << '\n';
        output_ << "  store ptr " << values[index] << ", ptr " << element << '\n';
    }
    return array;
}

std::string FunctionEmitter::emitConcatenation(const ast::BinaryExpression& expression)
{
    std::vector<const ast::Expression*> parts;
    collectParts(expression, semantics_, parts);
    const std::string array = emitStringParts(parts);
    requireRuntime(RuntimeFunction::StringConcat);
    requireStringRuntime();
    const std::string status = newStringStatus();
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << result << " = call ptr @__inox_str_concat(i64 " << parts.size() << ", ptr " << array
            << ", ptr " << status << ")\n";
    emitStringStatusCheck(status);
    holdOwnedResult(result, typeOf(expression), "ptr");
    return result;
}

// Two writings of the same storage: a variable, a field of the same place,
// or an element of the same place at an index that is the same name or
// literal. Nothing in them has an effect, so evaluating one is evaluating
// both.
bool FunctionEmitter::samePlace(const ast::Expression& first, const ast::Expression& second) const
{
    if (first.kind() != second.kind()) {
        return false;
    }
    if (first.kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto& a = static_cast<const ast::IdentifierExpression&>(first);
        const auto& b = static_cast<const ast::IdentifierExpression&>(second);
        return normalize(a.name()) == normalize(b.name()) && semantics_.symbolOf(a) == semantics_.symbolOf(b);
    }
    if (first.kind() == ast::AstNodeKind::LiteralExpression) {
        const auto& a = static_cast<const ast::LiteralExpression&>(first);
        const auto& b = static_cast<const ast::LiteralExpression&>(second);
        return a.literalKind() == ast::LiteralKind::Integer && b.literalKind() == ast::LiteralKind::Integer &&
               a.value() == b.value();
    }
    if (first.kind() != ast::AstNodeKind::CallExpression) {
        return false;
    }
    const auto& a = static_cast<const ast::CallExpression&>(first);
    const auto& b = static_cast<const ast::CallExpression&>(second);
    const bool member = isMemberAccessCall(a) && isMemberAccessCall(b) && memberField(a) && memberField(b);
    const bool element = isIndexCall(a) && isIndexCall(b);
    if (!member && !element) {
        return false;
    }
    if (member) {
        return samePlace(*a.arguments()[0], *b.arguments()[0]) &&
               normalize(static_cast<const ast::IdentifierExpression&>(*a.arguments()[1]).name()) ==
                   normalize(static_cast<const ast::IdentifierExpression&>(*b.arguments()[1]).name());
    }
    return samePlace(*a.arguments()[0], *b.arguments()[0]) && samePlace(*a.arguments()[1], *b.arguments()[1]);
}

// Whether evaluating `expression` may run a routine of the program, which
// could change the String being appended to before the append reads it.
bool FunctionEmitter::mayRunRoutine(const ast::Expression& expression) const
{
    bool runs = false;
    forEachSubExpression(expression, [&](const ast::Expression& node) { runs = runs || runsRoutine(node); });
    return runs;
}

// `S := S + X ...` (also `S += X`): appended in place. False when the
// assignment is not of that form, or another part may run a routine (it
// could change S first; then the ordinary concatenation keeps the order).
bool FunctionEmitter::emitStringAppend(const ast::BinaryExpression& assignment)
{
    if (!isStringConcatenation(assignment.right())) {
        return false;
    }
    std::vector<const ast::Expression*> parts;
    collectParts(assignment.right(), semantics_, parts);
    if (!samePlace(assignment.left(), *parts.front())) {
        return false;
    }
    parts.erase(parts.begin());
    for (const ast::Expression* part : parts) {
        if (mayRunRoutine(*part)) {
            return false;
        }
    }
    if (assignment.left().kind() == ast::AstNodeKind::IdentifierExpression &&
        locals_.find(normalize(static_cast<const ast::IdentifierExpression&>(assignment.left()).name())) ==
            locals_.end()) {
        return false;
    }
    const Place place = emitPlace(assignment.left());
    const std::string array = emitStringParts(parts);
    requireRuntime(RuntimeFunction::StringAppend);
    requireStringRuntime();
    const std::string status = newStringStatus();
    output_ << "  call void @__inox_str_append(ptr " << place.pointer << ", i64 " << parts.size() << ", ptr "
            << array << ", ptr " << status << ")\n";
    emitStringStatusCheck(status);
    emitInvariantChecks(assignment, true);
    return true;
}

std::string FunctionEmitter::emitStringCall(const ast::CallExpression& call, semantic::StringFunction function)
{
    std::vector<std::string> values;
    values.reserve(call.arguments().size());
    for (const auto& argument : call.arguments()) {
        values.push_back(emitExpression(*argument));
    }
    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
    using semantic::StringFunction;

    // The functions that make a String: owned, with a status.
    const auto makes = [&](RuntimeFunction runtime, const std::string& symbol, const std::string& arguments) {
        requireRuntime(runtime);
        requireStringRuntime();
        const std::string status = newStringStatus();
        output_ << "  " << result << " = call ptr @" << symbol << "(" << arguments << ", ptr " << status << ")\n";
        emitStringStatusCheck(status);
        holdOwnedResult(result, semantics_.types().stringType(), "ptr");
        return result;
    };
    const auto find = [&](int where) {
        requireRuntime(RuntimeFunction::StringFind);
        output_ << "  " << result << " = call zeroext i1 @__inox_str_find(ptr " << values[0] << ", ptr "
                << values[1] << ", i32 " << where << ")\n";
        return result;
    };

    switch (function) {
    case StringFunction::Length:
        requireRuntime(RuntimeFunction::StringLength);
        output_ << "  " << result << " = call i64 @__inox_str_length(ptr " << values[0] << ")\n";
        return result;
    case StringFunction::ByteLength: {
        // The length in bytes is at offset 8 of every String block.
        const std::string field = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << field << " = getelementptr i8, ptr " << values[0] << ", i64 8\n";
        output_ << "  " << result << " = load i64, ptr " << field << '\n';
        return result;
    }
    case StringFunction::Substr:
        return makes(RuntimeFunction::StringSubstr, "__inox_str_substr",
                     "ptr " + values[0] + ", i64 " + values[1] + ", i64 " + values[2]);
    case StringFunction::CharAt: {
        // Change char-values: the code point at a position counted from 1;
        // outside the String the status stops the program (kind 15).
        requireRuntime(RuntimeFunction::StringCharAt);
        requireStringRuntime();
        const std::string status = newStringStatus();
        output_ << "  " << result << " = call i64 @__inox_str_char_at(ptr " << values[0] << ", i64 " << values[1]
                << ", ptr " << status << ")\n";
        emitStringStatusCheck(status);
        return result;
    }
    case StringFunction::Position:
        requireRuntime(RuntimeFunction::StringPosition);
        output_ << "  " << result << " = call i64 @__inox_str_pos(ptr " << values[0] << ", ptr " << values[1]
                << ")\n";
        return result;
    case StringFunction::UpperCase:
    case StringFunction::LowerCase:
        return makes(RuntimeFunction::StringCase, "__inox_str_case",
                     "ptr " + values[0] + ", i32 " + (function == StringFunction::UpperCase ? "0" : "1"));
    case StringFunction::IsLetter:
    case StringFunction::IsDigit:
    case StringFunction::IsSpace:
    case StringFunction::IsUpper:
    case StringFunction::IsLower:
    case StringFunction::IsPunctuation: {
        // r3.89 (C3): the order of the classes in __inox_char_is.
        const int which = function == StringFunction::IsLetter  ? 0
                          : function == StringFunction::IsDigit ? 1
                          : function == StringFunction::IsSpace ? 2
                          : function == StringFunction::IsUpper ? 3
                          : function == StringFunction::IsLower ? 4
                                                                : 5;
        requireRuntime(RuntimeFunction::CharIs);
        output_ << "  " << result << " = call zeroext i1 @__inox_char_is(i64 " << values[0] << ", i32 " << which
                << ")\n";
        return result;
    }
    case StringFunction::UpperCaseChar:
    case StringFunction::LowerCaseChar:
        requireRuntime(RuntimeFunction::CharCase);
        output_ << "  " << result << " = call i64 @__inox_char_case(i64 " << values[0] << ", i32 "
                << (function == StringFunction::UpperCaseChar ? 0 : 1) << ")\n";
        return result;
    case StringFunction::FormatFloat: {
        // r3.90 (B1): fixed decimals, 0..20 ("string index out of range"
        // outside, the status of a String function).
        std::string real = values[0];
        if (expressionLlvmType(*call.arguments()[0]) == "float") {
            real = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << real << " = fpext float " << values[0] << " to double\n";
        }
        return makes(RuntimeFunction::StringFormatFloat, "__inox_str_format_float",
                     "double " + real + ", i64 " + values[1]);
    }
    case StringFunction::FieldCount:
        requireRuntime(RuntimeFunction::StringFieldCount);
        output_ << "  " << result << " = call i64 @__inox_str_field_count(ptr " << values[0] << ", ptr " << values[1]
                << ")\n";
        return result;
    case StringFunction::Field:
        return makes(RuntimeFunction::StringField, "__inox_str_field",
                     "ptr " + values[0] + ", ptr " + values[1] + ", i64 " + values[2]);
    case StringFunction::Trim:
    case StringFunction::LTrim:
    case StringFunction::RTrim:
        return makes(RuntimeFunction::StringTrim, "__inox_str_trim",
                     "ptr " + values[0] + ", i32 " +
                         (function == StringFunction::LTrim ? "1" : function == StringFunction::RTrim ? "2" : "3"));
    case StringFunction::Left:
    case StringFunction::Right:
        return makes(RuntimeFunction::StringEdge, "__inox_str_edge",
                     "ptr " + values[0] + ", i64 " + values[1] + ", i32 " +
                         (function == StringFunction::Left ? "1" : "2"));
    case StringFunction::PadLeft:
    case StringFunction::PadRight:
        return makes(RuntimeFunction::StringPad, "__inox_str_pad",
                     "ptr " + values[0] + ", i64 " + values[1] + ", i32 " +
                         (function == StringFunction::PadLeft ? "1" : "2"));
    case StringFunction::Replicate:
        return makes(RuntimeFunction::StringReplicate, "__inox_str_replicate",
                     "ptr " + values[0] + ", i64 " + values[1]);
    case StringFunction::Replace:
        return makes(RuntimeFunction::StringReplace, "__inox_str_replace",
                     "ptr " + values[0] + ", ptr " + values[1] + ", ptr " + values[2]);
    case StringFunction::Contains:
        return find(0);
    case StringFunction::StartsWith:
        return find(1);
    case StringFunction::EndsWith:
        return find(2);
    case StringFunction::SameText:
    case StringFunction::CompareText: {
        requireRuntime(RuntimeFunction::StringCompareText);
        const std::string order = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << order << " = call i32 @__inox_str_compare_text(ptr " << values[0] << ", ptr "
                << values[1] << ")\n";
        if (function == StringFunction::SameText) {
            output_ << "  " << result << " = icmp eq i32 " << order << ", 0\n";
        } else {
            output_ << "  " << result << " = sext i32 " << order << " to i64\n";
        }
        return result;
    }
    }
    throw CodegenError("unknown Std.Strings function");
}

} // namespace inox::compiler::codegen::detail
