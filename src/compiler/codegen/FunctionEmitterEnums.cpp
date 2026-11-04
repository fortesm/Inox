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
    const semantic::BuiltinOp op = builtinOf(call);
    if (op == semantic::BuiltinOp::None || call.arguments().size() != 1) {
        return false;
    }
    const ast::Expression& argument = *call.arguments()[0];
    if (op == semantic::BuiltinOp::Low || op == semantic::BuiltinOp::High) {
        return enumTypeNamed(argument) != nullptr;
    }
    const TypeRef type = typeOf(argument);
    if (op == semantic::BuiltinOp::Ord) {
        return type != nullptr && type->isEnum();
    }
    return (op == semantic::BuiltinOp::Succ || op == semantic::BuiltinOp::Pred) && type != nullptr &&
           (type->isEnum() || type->isRange());
}

std::string FunctionEmitter::emitOrdinalBuiltin(const ast::CallExpression& call)
{
    const semantic::BuiltinOp op = builtinOf(call);
    const ast::Expression& argument = *call.arguments()[0];
    if (const TypeRef type = enumTypeNamed(argument)) {
        return std::to_string(op == semantic::BuiltinOp::Low ? type->minValue() : type->maxValue());
    }
    const std::string value = emitExpression(argument);
    if (op == semantic::BuiltinOp::Ord) {
        return value;
    }
    // Succ and Pred: past the last or before the first literal is "value
    // out of range" (kind 11).
    const TypeRef type = typeOf(argument);
    const std::string stepped = "%tmp" + std::to_string(nextTemporary_++);
    output_ << "  " << stepped << " = " << (op == semantic::BuiltinOp::Succ ? "add" : "sub") << " i64 " << value
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
        // r3.99: a literal of another module's enum has a qualified
        // internal name (`Std.Terminal.KeyUp`); it prints as written.
        const std::string& literal = enumType->literals()[index];
        const std::size_t dot = literal.rfind('.');
        definition += (index == 0 ? "ptr " : ", ptr ") +
                      emitStringValue(dot == std::string::npos ? literal : literal.substr(dot + 1));
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
