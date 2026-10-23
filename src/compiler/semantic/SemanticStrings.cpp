// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: Std.Strings and Std.StrUtils (OpenSpec change
// std-strings, CANON-14, CANON-18). Std.Strings holds what nearly every
// program needs (Length, ByteLength, Substr, Position, UpperCase, LowerCase);
// Std.StrUtils, which uses Std.Strings, the Delphi-style conveniences
// (canon r3.64 module pairs). They are compiler intrinsics that exist only
// in a program that uses the module, so their names stay free elsewhere.
// They only read their arguments, so contracts may call them.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"
#include "StdStrings.h"

#include <string>
#include <string_view>
#include <vector>

namespace inox::compiler::semantic {

using namespace detail;

const StringFunctionInfo* findStringFunction(std::string_view name) noexcept
{
    for (const StringFunctionInfo& info : kStringFunctions) {
        if (equalsIgnoreCase(info.name, name)) {
            return &info;
        }
    }
    return nullptr;
}

const StringFunctionInfo& stringFunctionInfo(StringFunction function) noexcept
{
    for (const StringFunctionInfo& info : kStringFunctions) {
        if (info.function == function) {
            return info;
        }
    }
    return kStringFunctions.front();
}

void SemanticAnalyzer::declareStringFunctionNames(const ast::ModuleNode& module)
{
    strUtilsVisible_ = usesModule(module, "Std.StrUtils");
    // Std.StrUtils uses Std.Strings, so it also gives everything of it.
    stringsVisible_ = strUtilsVisible_ || usesModule(module, "Std.Strings");
    for (const StringFunctionInfo& info : kStringFunctions) {
        if (stringFunctionVisible(info.name) && !symbols_.globalScope().containsLocal(info.name)) {
            symbols_.globalScope().declare(std::string(info.name), SymbolKind::Builtin);
        }
    }
}

bool SemanticAnalyzer::userRoutineNamed(std::string_view name) const
{
    return resolveFunctionSignature(name) != nullptr || genericRoutine(name) != nullptr;
}

bool SemanticAnalyzer::stringFunctionVisible(std::string_view name) const
{
    const StringFunctionInfo* info = findStringFunction(name);
    return info != nullptr && (info->convenience ? strUtilsVisible_ : stringsVisible_);
}

TypeRef SemanticAnalyzer::analyzeStringCall(const ast::CallExpression& call, const std::vector<TypeRef>* argumentTypes)
{
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    const StringFunctionInfo* found = findStringFunction(callee.name());
    // r3.89 (C3): UpperCase(C) and LowerCase(C) of a Char. The argument is
    // analyzed once, here, and its type passed on.
    std::vector<TypeRef> analyzedArgument;
    if ((found->function == StringFunction::UpperCase || found->function == StringFunction::LowerCase) &&
        call.arguments().size() == 1) {
        if (argumentTypes == nullptr) {
            analyzedArgument.push_back(analyzeExpressionFor(*call.arguments()[0], types().stringType()));
            argumentTypes = &analyzedArgument;
        }
        const TypeRef argument = (*argumentTypes)[0];
        if (argument != nullptr && argument->isChar()) {
            found = &stringFunctionInfo(found->function == StringFunction::UpperCase ? StringFunction::UpperCaseChar
                                                                                     : StringFunction::LowerCaseChar);
        }
    }
    const StringFunctionInfo& info = *found;
    const std::string name(info.name);
    if (info.convenience && !strUtilsVisible_) {
        throw SemanticError(name + " is declared in Std.StrUtils: add `Use Std.StrUtils`");
    }
    if (!stringsVisible_) {
        throw SemanticError(name + " is declared in Std.Strings: add `Use Std.Strings` (or Std.StrUtils, "
                                   "which gives it too)");
    }
    const auto& arguments = call.arguments();
    if (arguments.size() != info.parameters.size()) {
        throw SemanticError(name + " expects " + std::to_string(info.parameters.size()) + " argument" +
                            (info.parameters.size() == 1 ? "" : "s") + ", got " +
                            std::to_string(arguments.size()));
    }
    const Symbol& symbol = resolveOrThrow(callee.name());
    result_.bind(callee, symbol);
    result_.bind(call, symbol);
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const ast::Expression& argument = *arguments[index];
        const char kind = info.parameters[index];
        const TypeRef expected = kind == 'S'   ? types().stringType()
                                 : kind == 'C' ? types().charType()
                                 : kind == 'R' ? types().float64Type()
                                               : types().int64Type();
        const TypeRef actual = argumentTypes != nullptr ? (*argumentTypes)[index] : analyzeExpressionFor(argument, expected);
        const bool matches = kind == 'S'   ? actual != nullptr && actual->isString()
                             : kind == 'C' ? actual != nullptr && actual->isChar()
                             : kind == 'R' ? actual != nullptr && actual->isFloat()
                                           : actual != nullptr && actual->isInteger();
        if (!matches) {
            throw SemanticError("argument " + std::to_string(index + 1) + " of " + name + " is " +
                                (kind == 'S'   ? std::string("a String")
                                 : kind == 'C' ? std::string("a Char")
                                 : kind == 'R' ? std::string("a Float or a Float32")
                                               : std::string("an integer")) +
                                ", got " + types::nameOf(actual));
        }
    }
    result_.setStringCall(call, info.function);
    switch (info.result) {
    case 'S':
        return types().stringType();
    case 'B':
        return types().boolType();
    case 'C':
        return types().charType();
    default:
        return types().int64Type();
    }
}

} // namespace inox::compiler::semantic
