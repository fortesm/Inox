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
#include "../support/ArrayType.h"

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

std::string SemanticAnalyzer::analyzeCallExpression(const ast::CallExpression& call)
{
    // `Name := Value` inside an argument list is named struct construction
    // (CANON-9). It is not an assignment, and only a struct constructor takes it.
    const bool calleeIsType =
        call.callee().kind() == ast::AstNodeKind::IdentifierExpression &&
        types_.resolve(static_cast<const ast::IdentifierExpression&>(call.callee()).name()) != nullptr;
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
        if ((equalsIgnoreCase(callee.name(), "Low") || equalsIgnoreCase(callee.name(), "High") ||
             equalsIgnoreCase(callee.name(), "Length")) &&
            call.arguments().size() == 1) {
            const std::string argumentType = canonicalTypeName(analyzeExpression(*call.arguments()[0]));
            if (support::isSliceTypeName(argumentType)) {
                return "Int64";  // ADR-0017: a slice's bounds are known at run time
            }
            if (const auto parts = support::splitArrayTypeName(argumentType)) {
                // CANON-13: Low, High and Length of a fixed array are
                // compile-time constants of its type.
                const std::int64_t low = std::stoll(parts->low);
                const std::int64_t high = std::stoll(parts->high);
                const std::int64_t value = equalsIgnoreCase(callee.name(), "Low")    ? low
                                         : equalsIgnoreCase(callee.name(), "High")   ? high
                                                                                     : high - low + 1;
                constants_[&call] = value;
                return "Int64";
            }
        }
        if (const TypeSymbol* type = types_.resolve(callee.name())) {
            const Symbol& symbol = resolveOrThrow(callee.name());
            result_.bind(callee, symbol);
            result_.bind(call, symbol);
            result_.setExpressionType(callee, resolvedType(canonicalTypeName(type->name)));
            const StructType* structType = resolveStruct(type->name);
            if (structType == nullptr) {
                // A conversion such as `Float32(0.0)` or `Currency(19.99)`.
                for (const auto& argument : call.arguments()) {
                    if (const ast::BinaryExpression* named = namedArgument(*argument)) {
                        throw SemanticError(
                            "named argument '" +
                            static_cast<const ast::IdentifierExpression&>(named->left()).name() +
                            " := ...' is only allowed in struct construction (CANON-9)");
                    }
                    analyzeExpression(*argument);
                }
                // CANON-8 (v3.39): an integer conversion checks the range of
                // its target. A constant argument outside it, such as
                // Natural(-1) or Byte(300), is a compile-time error.
                if (call.arguments().size() == 1) {
                    std::int64_t constant = 0;
                    std::int64_t low = 0;
                    std::int64_t high = 0;
                    if (integerTypeRange(canonicalTypeName(type->name), low, high) &&
                        constantIntegerValue(*call.arguments()[0], constant) &&
                        (constant < low || constant > high)) {
                        throw SemanticError("value out of range for " + callee.name() + ": " +
                                            std::to_string(constant));
                    }
                }
                return canonicalTypeName(type->name);
            }

            // Named struct construction (CANON-9): every argument is
            // `Field := Value`. The left side names a field, never a variable;
            // nothing is declared or assigned.
            std::vector<std::string> namedFields;
            for (const auto& argument : call.arguments()) {
                const ast::BinaryExpression* named = namedArgument(*argument);
                if (named == nullptr) {
                    throw SemanticError(
                        "positional construction is not canonical (CANON-9): write " +
                        type->name + "(Field := Value, ...)");
                }
                const std::string& fieldName =
                    static_cast<const ast::IdentifierExpression&>(named->left()).name();
                const StructField* field = resolveStructField(type->name, fieldName);
                if (field == nullptr) {
                    if (const auto reused = findReusedMember(type->name, fieldName); reused && !reused->isMethod) {
                        throw SemanticError(fieldName + " belongs to the reused part " + reused->path.front() +
                                            " (CANON-9): construct the part, " + type->name + "(" +
                                            reused->path.front() + " := " + reused->path.front() + "(...))");
                    }
                    throw SemanticError("unknown field in construction: " +
                                        type->name + "." + fieldName);
                }
                for (const std::string& seen : namedFields) {
                    if (equalsIgnoreCase(seen, fieldName)) {
                        throw SemanticError("duplicate field in construction: " +
                                            type->name + "." + fieldName);
                    }
                }
                namedFields.push_back(fieldName);
                const std::string valueType = analyzeExpression(named->right());
                if (!canAssignValue(field->typeName, valueType, named->right())) {
                    throw SemanticError("field " + type->name + "." + fieldName +
                                        " expects " + field->typeName);
                }
                result_.setExpressionType(*named, resolvedType(field->typeName));
            }
            for (const StructField& field : structType->fields) {
                bool named = false;
                for (const std::string& seen : namedFields) {
                    named = named || equalsIgnoreCase(seen, field.name);
                }
                if (!named && !field.hasDefault && resolveStruct(field.typeName) == nullptr) {
                    throw SemanticError(
                        "construction omits scalar field without default (CANON-9): " +
                        type->name + "." + field.name);
                }
            }
            return canonicalTypeName(type->name);
        }

        // Std.Debug (debug-assert): Assert exists only with `Use Std.Debug`;
        // a routine of the program named Assert is called otherwise.
        if (equalsIgnoreCase(callee.name(), "Assert") &&
            (debugVisible_ || resolveFunctionSignature(callee.name()) == nullptr)) {
            return analyzeAssertCall(call);
        }

        // Std.Files / Std.IOUtils (file-io, module-pairs): without the `Use`
        // that declares it, a file function's name stays free for the
        // program's own routines; a call to none of them gets a hint.
        if (isFileFunction(callee.name(), call.arguments().size()) &&
            (fileFunctionVisible(callee.name()) || resolveFunctionSignature(callee.name()) == nullptr)) {
            return analyzeFileCall(callee.name(), call);
        }

        // Length of anything but an array or a slice, and Ord before enums
        // exist, are not Inox (CANON-13, CANON-14); a routine of the program
        // with that name is called instead.
        if ((equalsIgnoreCase(callee.name(), "Length") || equalsIgnoreCase(callee.name(), "Ord")) &&
            resolveFunctionSignature(callee.name()) == nullptr) {
            analyzePreludeCall(callee.name(), {});
        }

        if (isPreludeCall(callee.name())) {
            const Symbol& symbol = resolveOrThrow(callee.name());
            result_.bind(callee, symbol);
            result_.bind(call, symbol);
            if (equalsIgnoreCase(callee.name(), "Get") || equalsIgnoreCase(callee.name(), "GetLn")) {
                return analyzeInputCall(callee.name(), call.arguments());
            }
            std::vector<std::string> argumentTypes;
            argumentTypes.reserve(call.arguments().size());
            for (const auto& argument : call.arguments()) {
                argumentTypes.push_back(analyzeExpression(*argument));
            }
            return analyzePreludeCall(callee.name(), argumentTypes);
        }

        if (const FunctionSignature* signature = resolveFunctionSignature(callee.name())) {
            const Symbol& symbol = resolveOrThrow(callee.name());
            result_.bind(callee, symbol);
            result_.bind(call, symbol);
            return analyzeUserFunctionCall(*signature, call.arguments());
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
    return {};
}

std::string SemanticAnalyzer::analyzeMemberExpression(const ast::CallExpression& call)
{
    if (call.arguments().size() != 2 ||
        call.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
        throw SemanticError("member access requires object and field name");
    }

    const std::string baseType = analyzeExpression(*call.arguments()[0]);
    const auto& fieldIdentifier =
        static_cast<const ast::IdentifierExpression&>(*call.arguments()[1]);
    const StructField* field = resolveStructField(baseType, fieldIdentifier.name());
    if (field != nullptr) {
        result_.setExpressionType(fieldIdentifier, resolvedType(field->typeName));
        return field->typeName;
    }

    const std::string qualifiedName = baseType + "." + fieldIdentifier.name();
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
        if (!canAssign(signature->parameters.front().typeName, baseType)) {
            throw SemanticError(
                "method " + signature->name + " receiver expects " +
                signature->parameters.front().typeName + ", got " + baseType);
        }
        requireWritableReceiver(*signature, *call.arguments()[0]);
        const Symbol& symbol = resolveOrThrow(signature->name);
        result_.bind(fieldIdentifier, symbol);
        result_.bind(call, symbol);
        return signature->returnType;
    }

    throw SemanticError("unknown field or zero-argument method " + fieldIdentifier.name() + " in " + baseType);
}

std::string SemanticAnalyzer::analyzeMethodCallExpression(const ast::CallExpression& call)
{
    const auto& member = static_cast<const ast::CallExpression&>(call.callee());
    if (member.arguments().size() != 2 ||
        member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
        throw SemanticError("method call requires object and method name");
    }

    const std::string receiverType = analyzeExpression(*member.arguments()[0]);
    const auto& methodIdentifier =
        static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
    const std::string qualifiedName = receiverType + "." + methodIdentifier.name();
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
        throw SemanticError("unknown method " + methodIdentifier.name() + " for " + receiverType);
    }

    if (signature->parameters.empty()) {
        throw SemanticError("method " + signature->name + " must declare an explicit receiver parameter");
    }
    if (!canAssign(signature->parameters.front().typeName, receiverType)) {
        throw SemanticError(
            "method " + signature->name + " receiver expects " +
            signature->parameters.front().typeName + ", got " + receiverType);
    }
    requireWritableReceiver(*signature, *member.arguments()[0]);
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
        const std::string argumentType = analyzeExpression(*call.arguments()[index]);
        const FunctionParameter& parameter = signature->parameters[index + 1];
        if (parameter.mode != PassMode::Value) {
            continue;
        }
        if (!canAssignValue(parameter.typeName, argumentType, *call.arguments()[index])) {
            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + signature->name +
                " expects " + parameter.typeName + ", got " + argumentType);
        }
    }
    checkReferenceArguments(signature->name, signature->parameters, 1, call.arguments());
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

std::string SemanticAnalyzer::analyzeUserFunctionCall(
    const FunctionSignature& signature,
    const std::vector<ast::ExpressionPtr>& arguments)
{
    if (arguments.size() != signature.parameters.size()) {
        throw SemanticError(
            "function " + signature.name + " expects " +
            std::to_string(signature.parameters.size()) + " arguments, got " +
            std::to_string(arguments.size()));
    }

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string argumentType = analyzeExpression(*arguments[index]);
        const FunctionParameter& parameter = signature.parameters[index];
        if (parameter.mode != PassMode::Value) {
            continue;
        }
        if (!canAssignValue(parameter.typeName, argumentType, *arguments[index])) {
            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + signature.name +
                " expects " + parameter.typeName + ", got " + argumentType);
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
        const ResolvedType* resolved = result_.typeOf(*arguments[index]);
        const std::string argumentType = resolved != nullptr ? canonicalTypeName(resolved->name) : std::string{};
        if (argumentType != canonicalTypeName(parameter.typeName)) {
            throw SemanticError("argument " + std::to_string(index + 1) + " of " + routine +
                                " is passed by reference and must have type " + parameter.typeName +
                                " exactly, got " + argumentType + " (ADR-0016)");
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
