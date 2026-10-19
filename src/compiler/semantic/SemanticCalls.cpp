// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: calls: user routines, methods, construction, ref arguments (ADR-0016).

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"
#include "../exceptions/ExceptionTypes.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

namespace inox::compiler::semantic {

using namespace detail;

TypeRef SemanticAnalyzer::analyzeCallExpression(const ast::CallExpression& call)
{
    // CANON-24: the intrinsics written by the contract lowering.
    if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
        const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
        if (name == "__check") {
            return analyzeContractCheck(call);
        }
        if (name == "__pure") {
            return analyzePureCall(call);
        }
    }

    // `Name := Value` inside an argument list is named struct construction
    // (CANON-9). It is not an assignment, and only a struct constructor takes it.
    const bool calleeIsType =
        call.callee().kind() == ast::AstNodeKind::IdentifierExpression &&
        (types().isDeclared(static_cast<const ast::IdentifierExpression&>(call.callee()).name()) ||
         genericStruct(static_cast<const ast::IdentifierExpression&>(call.callee()).name()) != nullptr);
    if (!calleeIsType) {
        for (const auto& argument : call.arguments()) {
            if (const ast::BinaryExpression* named = namedArgument(*argument)) {
                throw SemanticError(
                    "named argument '" +
                    static_cast<const ast::IdentifierExpression&>(named->left()).name() +
                    " := ...' is only allowed in struct construction (CANON-9); "
                    "':=' is a statement, not an expression");
            }
        }
    }

    if (call.callee().kind() == ast::AstNodeKind::CallExpression &&
        isMemberCall(static_cast<const ast::CallExpression&>(call.callee()))) {
        return analyzeMethodCallExpression(call);
    }

    if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
        if (equalsIgnoreCase(callee.name(), "__member")) {
            return analyzeMemberExpression(call);
        }
        if (callee.name() == "__index") {
            return analyzeIndexExpression(call);
        }
        if (callee.name() == "__slice") {
            return analyzeSliceExpression(call);
        }
        // Sets (CANON-13): a literal, and the six set functions.
        if (callee.name() == "__set") {
            return analyzeSetLiteral(call);
        }
        if (isSetFunctionCall(call)) {
            const Symbol* symbol = symbols_.currentScope().resolve(callee.name());
            if (symbol != nullptr) {
                result_.bind(callee, *symbol);
            }
            return analyzeSetFunction(call);
        }
        if (equalsIgnoreCase(callee.name(), "Without") && call.arguments().size() == 2 &&
            !userRoutineNamed(callee.name())) {
            throw SemanticError("the set function Without(S, X) is now Exclude(S, X), and With(S, X) is "
                                "Include(S, X) (r3.81)");
        }
        // r3.81: an array is filled where it is declared, `Fill(Value)`.
        if (isFillCall(call)) {
            return analyzeFill(call);
        }
        // Enums: Ord, Succ, Pred, and Low and High of an enum type.
        if (const TypeRef ordinal = analyzeOrdinalBuiltin(call)) {
            return ordinal;
        }
        // Safe pointers: New, Free, Move, Value, Link, Alive, Same.
        if (isPointerBuiltinCall(callee.name(), call.arguments().size())) {
            return analyzePointerBuiltin(callee.name(), call);
        }
        if ((equalsIgnoreCase(callee.name(), "Low") || equalsIgnoreCase(callee.name(), "High") ||
             equalsIgnoreCase(callee.name(), "Length")) &&
            call.arguments().size() == 1) {
            const TypeRef argumentType = analyzeExpression(*call.arguments()[0]);
            if (argumentType != nullptr && argumentType->isSlice()) {
                return types().int64Type();  // ADR-0017: a slice's bounds are known at run time
            }
            // Std.Strings: Length of a String counts its characters.
            if (argumentType != nullptr && argumentType->isString() && equalsIgnoreCase(callee.name(), "Length") &&
                (stringsVisible_ || !userRoutineNamed(callee.name()))) {
                const std::vector<TypeRef> argumentTypes{argumentType};
                return analyzeStringCall(call, &argumentTypes);
            }
            if (argumentType != nullptr && argumentType->isArray()) {
                // CANON-13: Low, High and Length of a fixed array are
                // compile-time constants of its type.
                const std::int64_t low = argumentType->low();
                const std::int64_t high = argumentType->high();
                const std::int64_t value = equalsIgnoreCase(callee.name(), "Low")    ? low
                                         : equalsIgnoreCase(callee.name(), "High")   ? high
                                                                                     : high - low + 1;
                constants_[&call] = value;
                return types().int64Type();
            }
        }
        if (const ast::TypeDeclaration* generic = genericStruct(callee.name())) {
            return analyzeGenericConstruction(*generic, call);
        }
        if (const TypeRef type = types().lookup(callee.name())) {
            const Symbol& symbol = resolveOrThrow(callee.name());
            result_.bind(callee, symbol);
            result_.bind(call, symbol);
            result_.setExpressionType(callee, type);
            if (!type->isStruct()) {
                // A conversion such as `Float32(0.0)` or `Currency(19.99)`.
                for (const auto& argument : call.arguments()) {
                    if (const ast::BinaryExpression* named = namedArgument(*argument)) {
                        throw SemanticError(
                            "named argument '" +
                            static_cast<const ast::IdentifierExpression&>(named->left()).name() +
                            " := ...' is only allowed in struct construction (CANON-9)");
                    }
                    // UInt64(18446744073709551615): the argument of a
                    // UInt64 conversion is a UInt64 context (change uint64).
                    analyzeWithIntegerContext(*argument, type);
                }
                // CANON-8 (v3.39): an integer conversion checks the range of
                // its target. A constant argument outside it, such as
                // Natural(-1) or Byte(300), is a compile-time error.
                if (type->isEnum()) {
                    checkEnumConversion(call, type);
                    return type;
                }
                // Char values (change char-values): Char(I) and String(C).
                if (type->isChar() || type->isString()) {
                    checkCharConversion(call, type);
                    return type;
                }
                if (call.arguments().size() == 1) {
                    if (const TypeRef from = result_.typeOf(*call.arguments()[0]);
                        from != nullptr && from->isChar() && !type->isInteger()) {
                        throw SemanticError(callee.name() + "(C) of a Char is not a conversion: a Char becomes "
                                            "Integer(C), its code point, or String(C) (CANON-14)");
                    }
                }
                // An enum becomes an integer only through Ord (CANON-13).
                if (call.arguments().size() == 1 && type->isInteger()) {
                    if (const TypeRef from = result_.typeOf(*call.arguments()[0]); from != nullptr && from->isEnum()) {
                        throw SemanticError(callee.name() + "(E) of an enum value is written Ord(E) (CANON-13)");
                    }
                }
                if (call.arguments().size() == 1 && type->isInteger()) {
                    IntegerConstant constant;
                    if (integerConstant(*call.arguments()[0], constant) && !constantFits(constant, type)) {
                        throw SemanticError("value out of range for " + callee.name() + ": " +
                                            constantText(constant));
                    }
                }
                return type;
            }

            return analyzeConstruction(call, type, nullptr);
        }

        // Std.Debug (debug-assert): Assert exists only with `Use Std.Debug`;
        // a routine of the program named Assert is called otherwise.
        if (equalsIgnoreCase(callee.name(), "Assert") && (debugVisible_ || !userRoutineNamed(callee.name()))) {
            return analyzeAssertCall(call);
        }

        // Std.Files / Std.IOUtils (file-io, module-pairs): without the `Use`
        // that declares it, a file function's name stays free for the
        // program's own routines; a call to none of them gets a hint.
        if (isFileFunction(callee.name(), call.arguments().size()) &&
            (fileFunctionVisible(callee.name()) || !userRoutineNamed(callee.name()))) {
            return analyzeFileCall(callee.name(), call);
        }

        // Std.Strings / Std.StrUtils (std-strings), the same way; Length of
        // an array, a slice or a String was handled above.
        if (findStringFunction(callee.name()) != nullptr && !equalsIgnoreCase(callee.name(), "Length") &&
            (stringFunctionVisible(callee.name()) || !userRoutineNamed(callee.name()))) {
            return analyzeStringCall(call, nullptr);
        }

        // Length of anything but an array or a slice, and Ord before enums
        // exist, are not Inox (CANON-13, CANON-14); a routine of the program
        // with that name is called instead.
        if ((equalsIgnoreCase(callee.name(), "Length") || equalsIgnoreCase(callee.name(), "Ord")) &&
            !userRoutineNamed(callee.name())) {
            analyzePreludeCall(callee.name(), {});
        }

        if (isPreludeCall(callee.name())) {
            const Symbol& symbol = resolveOrThrow(callee.name());
            result_.bind(callee, symbol);
            result_.bind(call, symbol);
            if (equalsIgnoreCase(callee.name(), "Get") || equalsIgnoreCase(callee.name(), "GetLn")) {
                return analyzeInputCall(callee.name(), call.arguments());
            }
            std::vector<TypeRef> argumentTypes;
            argumentTypes.reserve(call.arguments().size());
            for (const auto& argument : call.arguments()) {
                argumentTypes.push_back(analyzeExpression(*argument));
            }
            return analyzePreludeCall(callee.name(), argumentTypes);
        }

        if (const ast::FunctionDeclaration* generic = genericRoutine(callee.name())) {
            return analyzeGenericCall(*generic, callee, &call);
        }
        if (const FunctionSignature* signature = resolveFunctionSignature(callee.name())) {
            const Symbol& symbol = resolveOrThrow(callee.name());
            result_.bind(callee, symbol);
            result_.bind(call, symbol);
            const TypeRef type = analyzeUserFunctionCall(*signature, call.arguments());
            // CANON-24: a ref mut argument may break its struct's invariant.
            for (std::size_t index = 0; index < call.arguments().size(); ++index) {
                if (signature->parameters[index].mode == PassMode::RefMut) {
                    rejectInContract("calling " + signature->name + ", which changes a ref mut argument,");
                    recordInvariantPlaces(call, *call.arguments()[index], false, true);
                }
            }
            return type;
        }
    }

    analyzeExpression(call.callee());
    if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
        const Symbol& symbol = resolveOrThrow(callee.name());
        if (symbol.kind == SymbolKind::Variable || symbol.kind == SymbolKind::LoopIterator ||
            symbol.kind == SymbolKind::Constant || symbol.kind == SymbolKind::State) {
            throw SemanticError("'" + callee.name() + "' is a value, not a function; it cannot be called");
        }
        result_.bind(call, symbol);
    }
    for (const auto& argument : call.arguments()) {
        analyzeExpression(*argument);
    }
    return nullptr;
}

TypeRef SemanticAnalyzer::analyzeMemberExpression(const ast::CallExpression& call)
{
    if (call.arguments().size() != 2 ||
        call.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
        throw SemanticError("member access requires object and field name");
    }

    // Through a pointer, `P.Field` is a field of its object (safe-pointers).
    const TypeRef baseType = typeThroughPointer(analyzeExpression(*call.arguments()[0]));
    const auto& fieldIdentifier =
        static_cast<const ast::IdentifierExpression&>(*call.arguments()[1]);
    const types::StructField* field = resolveStructField(baseType, fieldIdentifier.name());
    if (field != nullptr) {
        result_.setExpressionType(fieldIdentifier, field->type);
        return field->type;
    }

    const std::string qualifiedName = types::nameOf(baseType) + "." + fieldIdentifier.name();
    const FunctionSignature* signature = resolveFunctionSignature(qualifiedName);
    if (signature == nullptr) {
        // CANON-9: a field or method of a reused part, reached through it.
        if (const auto reused = findReusedMember(baseType, fieldIdentifier.name())) {
            routeThroughParts(call, reused->path);
            return analyzeMemberExpression(call);
        }
    }
    if (signature != nullptr) {
        if (signature->parameters.empty()) {
            throw SemanticError("method " + signature->name + " must declare an explicit receiver parameter");
        }
        if (signature->parameters.size() != 1) {
            throw SemanticError("method " + signature->name + " requires arguments; use parentheses only when passing arguments");
        }
        if (!canAssign(signature->parameters.front().type, baseType)) {
            throw SemanticError(
                "method " + signature->name + " receiver expects " +
                types::nameOf(signature->parameters.front().type) + ", got " + types::nameOf(baseType));
        }
        requireWritableReceiver(*signature, *call.arguments()[0]);
        if (signature->parameters.front().mode == PassMode::RefMut) {
            rejectInContract("calling " + signature->name + ", which changes its receiver (Self mut),");
        }
        recordInvariantPlaces(call, *call.arguments()[0], true,
                              signature->parameters.front().mode == PassMode::RefMut);
        const Symbol& symbol = resolveOrThrow(signature->name);
        result_.bind(fieldIdentifier, symbol);
        result_.bind(call, symbol);
        return signature->returnType;
    }

    throw SemanticError("unknown field or zero-argument method " + fieldIdentifier.name() + " in " +
                        types::nameOf(baseType));
}

TypeRef SemanticAnalyzer::analyzeMethodCallExpression(const ast::CallExpression& call)
{
    const auto& member = static_cast<const ast::CallExpression&>(call.callee());
    if (member.arguments().size() != 2 ||
        member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
        throw SemanticError("method call requires object and method name");
    }

    const TypeRef receiverType = typeThroughPointer(analyzeExpression(*member.arguments()[0]));
    const auto& methodIdentifier =
        static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
    const std::string qualifiedName = types::nameOf(receiverType) + "." + methodIdentifier.name();
    const FunctionSignature* signature = resolveFunctionSignature(qualifiedName);
    if (signature == nullptr && resolveStructField(receiverType, methodIdentifier.name()) == nullptr) {
        // CANON-9: a method of a reused part runs on that part.
        if (const auto reused = findReusedMember(receiverType, methodIdentifier.name());
            reused && reused->isMethod) {
            routeThroughParts(member, reused->path);
            return analyzeMethodCallExpression(call);
        }
    }
    if (signature == nullptr) {
        throw SemanticError("unknown method " + methodIdentifier.name() + " for " + types::nameOf(receiverType));
    }

    if (signature->parameters.empty()) {
        throw SemanticError("method " + signature->name + " must declare an explicit receiver parameter");
    }
    if (!canAssign(signature->parameters.front().type, receiverType)) {
        throw SemanticError(
            "method " + signature->name + " receiver expects " +
            types::nameOf(signature->parameters.front().type) + ", got " + types::nameOf(receiverType));
    }
    requireWritableReceiver(*signature, *member.arguments()[0]);
    if (signature->parameters.front().mode == PassMode::RefMut) {
        rejectInContract("calling " + signature->name + ", which changes its receiver (Self mut),");
    }
    if (call.arguments().size() + 1 != signature->parameters.size()) {
        throw SemanticError(
            "method " + signature->name + " expects " +
            std::to_string(signature->parameters.size() - 1) + " explicit arguments, got " +
            std::to_string(call.arguments().size()));
    }

    const Symbol& symbol = resolveOrThrow(signature->name);
    result_.bind(methodIdentifier, symbol);
    result_.bind(call, symbol);

    for (std::size_t index = 0; index < call.arguments().size(); ++index) {
        const TypeRef argumentType = analyzeExpression(*call.arguments()[index]);
        const FunctionParameter& parameter = signature->parameters[index + 1];
        if (parameter.mode != PassMode::Value) {
            continue;
        }
        if (!canAssignValue(parameter.type, argumentType, *call.arguments()[index])) {
            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + signature->name +
                " expects " + types::nameOf(parameter.type) + ", got " + types::nameOf(argumentType));
        }
    }
    checkReferenceArguments(signature->name, signature->parameters, 1, call.arguments());
    // CANON-24: the receiver's invariant before and (Self mut) after the
    // call; a ref mut argument's after it.
    recordInvariantPlaces(call, *member.arguments()[0], true,
                          signature->parameters.front().mode == PassMode::RefMut);
    for (std::size_t index = 0; index < call.arguments().size(); ++index) {
        if (signature->parameters[index + 1].mode == PassMode::RefMut) {
            rejectInContract("calling " + signature->name + ", which changes a ref mut argument,");
            recordInvariantPlaces(call, *call.arguments()[index], false, true);
        }
    }
    // ADR-0016: the receiver is an argument too; a `ref mut` argument may
    // not reach the receiver's variable through another name.
    std::vector<std::string> receiverNames;
    collectNames(*member.arguments()[0], receiverNames);
    for (std::size_t index = 0; index < call.arguments().size(); ++index) {
        if (signature->parameters[index + 1].mode != PassMode::RefMut) {
            continue;
        }
        const Symbol* root = writableRoot(*call.arguments()[index]);
        for (const std::string& name : receiverNames) {
            if (root != nullptr && equalsIgnoreCase(name, root->name)) {
                throw SemanticError("argument " + std::to_string(index + 1) + " of " + signature->name +
                                    " is passed as ref mut, and " + root->name +
                                    " is also the receiver; a routine never sees one variable through "
                                    "two names (ADR-0016)");
            }
        }
    }

    return signature->returnType;
}

// Named struct construction (CANON-9): every argument is `Field := Value`.
// The left side names a field, never a variable; nothing is declared or
// assigned. `valueTypes`, when given, holds the types of the values, already
// analyzed in order (a generic struct infers its arguments from them).
TypeRef SemanticAnalyzer::analyzeConstruction(const ast::CallExpression& call, TypeRef type,
                                              const std::vector<TypeRef>* valueTypes)
{
    std::vector<std::string> namedFields;
    for (const auto& argument : call.arguments()) {
        const ast::BinaryExpression* named = namedArgument(*argument);
        if (named == nullptr) {
            throw SemanticError(
                "positional construction is not canonical (CANON-9): write " +
                type->name() + "(Field := Value, ...)");
        }
        const std::string& fieldName =
            static_cast<const ast::IdentifierExpression&>(named->left()).name();
        const types::StructField* field = resolveStructField(type, fieldName);
        if (field == nullptr) {
            if (const auto reused = findReusedMember(type, fieldName); reused && !reused->isMethod) {
                throw SemanticError(fieldName + " belongs to the reused part " + reused->path.front() +
                                    " (CANON-9): construct the part, " + type->name() + "(" +
                                    reused->path.front() + " := " + reused->path.front() + "(...))");
            }
            throw SemanticError("unknown field in construction: " +
                                type->name() + "." + fieldName);
        }
        for (const std::string& seen : namedFields) {
            if (equalsIgnoreCase(seen, fieldName)) {
                throw SemanticError("duplicate field in construction: " +
                                    type->name() + "." + fieldName);
            }
        }
        namedFields.push_back(fieldName);
        const TypeRef valueType = valueTypes != nullptr ? (*valueTypes)[namedFields.size() - 1]
                                                        : analyzeExpressionFor(named->right(), field->type);
        if (!canAssignValue(field->type, valueType, named->right())) {
            throw SemanticError("field " + type->name() + "." + fieldName +
                                " expects " + types::nameOf(field->type));
        }
        result_.setExpressionType(*named, field->type);
    }
    for (const types::StructField& field : type->fields()) {
        bool named = false;
        for (const std::string& seen : namedFields) {
            named = named || equalsIgnoreCase(seen, field.name);
        }
        // A Link and an Option are born empty; an Own is always given.
        const bool bornEmpty =
            field.type != nullptr && (field.type->isLink() || field.type->isOptionOwn());
        const bool isStruct = field.type != nullptr && field.type->isStruct();
        if (!named && !field.hasDefault && !isStruct && !bornEmpty) {
            if (field.type != nullptr && field.type->isOwn()) {
                throw SemanticError("construction omits the owner " + type->name() + "." + field.name +
                                    ": give it, " + field.name + " := New(...)");
            }
            if (field.type != nullptr && (field.type->isArray() || field.type->isEnum() || field.type->isChar() ||
                                          field.fromTypeParameter)) {
                throw SemanticError("construction omits " + type->name() + "." + field.name +
                                    ", which has no default (CANON-5, r3.82): give it, " + field.name + " := ...");
            }
            throw SemanticError(
                "construction omits scalar field without default (CANON-9): " +
                type->name() + "." + field.name);
        }
        if (!named && !field.hasDefault) {
            if (const std::string owner = ownerFieldWithoutDefault(field.type); !owner.empty()) {
                throw SemanticError("construction omits " + type->name() + "." + field.name +
                                    ", which holds the owner " + owner + ": give it");
            }
            if (field.fromTypeParameter) {
                throw SemanticError("construction omits " + type->name() + "." + field.name +
                                    ", which has no default (CANON-5, r3.82): give it, " + field.name + " := ...");
            }
            if (const std::string inner = requiredFieldWithoutDefault(field.type); !inner.empty()) {
                throw SemanticError("construction omits " + type->name() + "." + field.name + ", and " + inner +
                                    " has no default (CANON-5, r3.82): give it, " + field.name + " := ...");
            }
        }
    }
    // CANON-24: a new value satisfies its invariant.
    if (hasInvariant(type) && invariantChecksOn()) {
        result_.requireNewValueInvariant(call, type);
    }
    result_.setConstructedStruct(call, type);
    return type;
}

TypeRef SemanticAnalyzer::analyzeUserFunctionCall(
    const FunctionSignature& signature,
    const std::vector<ast::ExpressionPtr>& arguments,
    const std::vector<TypeRef>* argumentTypes)
{
    if (arguments.size() != signature.parameters.size()) {
        throw SemanticError(
            "function " + signature.name + " expects " +
            std::to_string(signature.parameters.size()) + " arguments, got " +
            std::to_string(arguments.size()));
    }

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const FunctionParameter& parameter = signature.parameters[index];
        const TypeRef argumentType = argumentTypes != nullptr ? (*argumentTypes)[index]
                                                              : analyzeExpressionFor(*arguments[index], parameter.type);
        if (parameter.mode != PassMode::Value) {
            continue;
        }
        if (!canAssignValue(parameter.type, argumentType, *arguments[index])) {
            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + signature.name +
                " expects " + types::nameOf(parameter.type) + ", got " + types::nameOf(argumentType));
        }
    }
    checkReferenceArguments(signature.name, signature.parameters, 0, arguments);

    return signature.returnType;
}

// ADR-0016: a reference argument has exactly the parameter's type. A
// `ref mut` argument is a writable place (a mutable variable, a field or an
// element of one, or a `ref mut` parameter), and the variable it is rooted
// at appears in no other argument of the call, so the routine never sees one
// variable through two names.
void SemanticAnalyzer::checkReferenceArguments(const std::string& routine,
                                               const std::vector<FunctionParameter>& parameters,
                                               std::size_t parameterOffset,
                                               const std::vector<ast::ExpressionPtr>& arguments)
{
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const FunctionParameter& parameter = parameters[index + parameterOffset];
        if (parameter.mode == PassMode::Value) {
            continue;
        }
        const TypeRef argumentType = result_.typeOf(*arguments[index]);
        if (argumentType != parameter.type) {
            throw SemanticError("argument " + std::to_string(index + 1) + " of " + routine +
                                " is passed by reference and must have type " + types::nameOf(parameter.type) +
                                " exactly, got " + types::nameOf(argumentType) + " (ADR-0016)");
        }
        if (parameter.mode != PassMode::RefMut) {
            continue;
        }
        const Symbol* root = writableRoot(*arguments[index]);
        if (root == nullptr) {
            throw SemanticError("argument " + std::to_string(index + 1) + " of " + routine +
                                " is passed as ref mut and must be a mutable variable, a field or an "
                                "element of one (ADR-0016)");
        }
        for (std::size_t other = 0; other < arguments.size(); ++other) {
            if (other == index) {
                continue;
            }
            std::vector<std::string> names;
            collectNames(*arguments[other], names);
            for (const std::string& name : names) {
                if (symbols_.currentScope().resolve(name) == root) {
                    throw SemanticError("argument " + std::to_string(index + 1) + " of " + routine +
                                        " passes " + root->name +
                                        " as ref mut, so no other argument may use it (ADR-0016)");
                }
            }
        }
    }
}

} // namespace inox::compiler::semantic
