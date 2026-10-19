// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: enums (CANON-13, OpenSpec change enum-range-set). An enum
// value is its ordinal in an i64, like every integer; Ord is the value,
// Succ and Pred are checked steps, and the name of a literal comes from one
// table of immortal Strings per enum.

#include "FunctionEmitter.h"

#include "LlvmIrEmitter.h"
#include "LlvmText.h"

#include <string>

namespace inox::compiler::codegen::detail {

namespace {

const std::string& calleeName(const ast::CallExpression& call)
{
    return static_cast<const ast::IdentifierExpression&>(call.callee()).name();
}

} // namespace

// The enum type an argument names (`Low(TSuit)`), or null.
TypeRef FunctionEmitter::enumTypeNamed(const ast::Expression& expression) const
{
    if (expression.kind() != ast::AstNodeKind::IdentifierExpression) {
        return nullptr;
    }
    const TypeRef type =
        semantics_.types().lookup(static_cast<const ast::IdentifierExpression&>(expression).name());
    return type != nullptr && (type->isEnum() || type->isRange() || type->isCharRange()) ? type : nullptr;
}

bool FunctionEmitter::isOrdinalBuiltin(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().size() != 1 ||
        calledRoutine(static_cast<const ast::IdentifierExpression&>(call.callee())) != nullptr) {
        return false;
    }
    const std::string& name = calleeName(call);
    const ast::Expression& argument = *call.arguments()[0];
    if (equalsIgnoreCase(name, "Low") || equalsIgnoreCase(name, "High")) {
        return enumTypeNamed(argument) != nullptr;
    }
    const TypeRef type = typeOf(argument);
    if (equalsIgnoreCase(name, "Ord")) {
        return type != nullptr && type->isEnum();
    }
    return (equalsIgnoreCase(name, "Succ") || equalsIgnoreCase(name, "Pred")) && type != nullptr &&
           (type->isEnum() || type->isRange());
}

std::string FunctionEmitter::emitOrdinalBuiltin(const ast::CallExpression& call)
{
    const std::string& name = calleeName(call);
    const ast::Expression& argument = *call.arguments()[0];
    if (const TypeRef type = enumTypeNamed(argument)) {
        return std::to_string(equalsIgnoreCase(name, "Low") ? type->minValue() : type->maxValue());
    }
    const std::string value = emitExpression(argument);
    if (equalsIgnoreCase(name, "Ord")) {
        return value;
    }
    // Succ and Pred: past the last or before the first literal is "value
    // out of range" (kind 11).
    const TypeRef type = typeOf(argument);
    const std::string stepped = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << stepped << " = " << (equalsIgnoreCase(name, "Succ") ? "add" : "sub") << " i64 " << value
            << ", 1\n";
    return emitRangeCheck(stepped, IntegerRange{type->minValue(), type->maxValue()}, 11);
}

// The names of an enum's literals, as immortal Strings (CANON-13 Q3: PutLn
// prints the name as declared). One table per enum and module.
std::string FunctionEmitter::enumNameTable(TypeRef enumType)
{
    if (const auto known = module_.enumNameTables.find(enumType); known != module_.enumNameTables.end()) {
        return known->second;
    }
    const std::string table = "@.inox.enum." + llvmGlobalName(normalize(enumType->name())) + ".names";
    module_.enumNameTables.emplace(enumType, table);
    std::string definition = table + " = private unnamed_addr constant [" +
                             std::to_string(enumType->literals().size()) + " x ptr] [";
    for (std::size_t index = 0; index < enumType->literals().size(); ++index) {
        definition += (index == 0 ? "ptr " : ", ptr ") + emitStringValue(enumType->literals()[index]);
    }
    definition += "]";
    module_.moduleLines.push_back(definition);
    return table;
}

// The literal's name of an enum value, an immortal String.
std::string FunctionEmitter::emitEnumName(const std::string& value, TypeRef enumType)
{
    const std::string table = enumNameTable(enumType);
    const std::string slot = "%tmp" + std::to_string(nextTemporary_++);
    const std::string name = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << slot << " = getelementptr [" << enumType->literals().size() << " x ptr], ptr " << table
            << ", i64 0, i64 " << value << '\n';
    output_ << "  " << name << " = load ptr, ptr " << slot << '\n';
    return name;
}

} // namespace inox::compiler::codegen::detail
