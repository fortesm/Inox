// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: generic routines and structs (CANON-8, r3.77, OpenSpec
// change generics), in their simplest form, to grow later (Marcelo Fortes,
// 2026-10-13):
//
// - A generic routine or struct is a template. It is never checked alone:
//   each use with its types makes an instance (`Max<Int64>`, `TPair<String,
//   Int64>`), a copy with the type parameters replaced, checked and compiled
//   like ordinary code. There are no constraints: an operation a type lacks
//   is an error of the instance that needs it, named in the message.
// - Type arguments are inferred from the arguments or, when they do not fix
//   them, from the type of the target (`Z Integer := Zero`); an expression
//   never writes them.
// - T may be any type; what the body does with it decides (an owner can only
//   be moved, a File only passed by `ref mut`).
//
// Instances are checked after the program's own routines, outside any
// routine, so the caller's locals are never visible in a generic body.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"
#include "../ast/AstClone.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace inox::compiler::semantic {

using namespace detail;

namespace {

// More instances, or type arguments nested deeper, than any real program
// makes: a generic routine that calls itself with a bigger type each time
// (`Grow(N - 1, TBox(FItem := X))`) would never end.
constexpr std::size_t kMaxInstances = 1000;
constexpr std::size_t kMaxNesting = 32;

bool isLiteralArgument(const ast::Expression& expression)
{
    if (expression.kind() == ast::AstNodeKind::LiteralExpression) {
        return true;
    }
    return expression.kind() == ast::AstNodeKind::UnaryExpression &&
           static_cast<const ast::UnaryExpression&>(expression).operand().kind() ==
               ast::AstNodeKind::LiteralExpression;
}

struct WrittenParameter {
    std::string type;
    PassMode mode = PassMode::Value;
};

// The written parameter types and result of a generic routine's signature;
// its full check happens when an instance's signature is registered.
std::vector<WrittenParameter> writtenParameters(const std::vector<std::string>& tokens, std::string& result)
{
    std::vector<WrittenParameter> parameters;
    std::size_t index = 0;
    if (!tokens.empty() && tokens.front() == "(") {
        index = 1;
        while (index < tokens.size() && tokens[index] != ")") {
            WrittenParameter parameter;
            if (equalsIgnoreCase(tokens[index], "ref")) {
                parameter.mode = PassMode::Ref;
                ++index;
                if (index < tokens.size() && equalsIgnoreCase(tokens[index], "mut")) {
                    parameter.mode = PassMode::RefMut;
                    ++index;
                }
            }
            index += 1;  // the name
            if (index < tokens.size()) {
                parameter.type = tokens[index++];
            }
            parameters.push_back(std::move(parameter));
            if (index < tokens.size() && tokens[index] == ",") {
                ++index;
            }
        }
        ++index;
    }
    result = index < tokens.size() ? tokens[index] : std::string{};
    return parameters;
}

std::string parameterList(const std::vector<std::string>& parameters)
{
    std::string text;
    for (const std::string& parameter : parameters) {
        text += (text.empty() ? "" : ", ") + parameter;
    }
    return text;
}

} // namespace

// How deeply type arguments nest in `type`: Int64 is 0, TBox<Int64> is 1,
// TBox<TBox<Int64>> is 2.
std::size_t SemanticAnalyzer::instanceNesting(TypeRef type) const
{
    if (type == nullptr) {
        return 0;
    }
    if (type->isArray() || type->isSlice()) {
        return instanceNesting(type->element());
    }
    if (type->isPointer()) {
        return instanceNesting(type->pointerTarget());
    }
    const auto instance = structInstanceOf_.find(type);
    if (instance == structInstanceOf_.end()) {
        return 0;
    }
    std::size_t deepest = 0;
    for (const TypeRef argument : instance->second.arguments) {
        deepest = std::max(deepest, instanceNesting(argument));
    }
    return deepest + 1;
}

void SemanticAnalyzer::requireBoundedNesting(std::string_view generic, const std::vector<TypeRef>& arguments) const
{
    for (const TypeRef argument : arguments) {
        if (instanceNesting(argument) >= kMaxNesting) {
            throw SemanticError("the type arguments of " + std::string(generic) + " nest more than " +
                                std::to_string(kMaxNesting) +
                                " levels deep: does a generic routine call itself with a bigger type each "
                                "time? (CANON-8)");
        }
    }
}

std::string SemanticAnalyzer::instanceName(std::string_view name, const std::vector<TypeRef>& arguments)
{
    std::string text = std::string(name) + "<";
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        text += (index == 0 ? "" : ", ") + types::nameOf(arguments[index]);
    }
    return text + ">";
}

// Before any declaration: a generic struct may be named by a State or a
// signature anywhere in the module.
void SemanticAnalyzer::registerGenericTemplates(const ast::ModuleNode& module)
{
    for (const auto& item : module.items()) {
        if (item->kind() == ast::AstNodeKind::FunctionDeclaration) {
            const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
            if (function.isGeneric()) {
                genericRoutines_.emplace(normalizeName(function.name()), &function);
            }
            continue;
        }
        if (item->kind() != ast::AstNodeKind::SectionDeclaration) {
            continue;
        }
        for (const ast::TypeDeclaration& declaration : static_cast<const ast::SectionDeclaration&>(*item).types()) {
            if (declaration.kind == ast::TypeDeclarationKind::Struct && !declaration.typeParameters.empty()) {
                declareOrThrow(declaration.name, SymbolKind::Type);
                typeModules_[normalizeName(declaration.name)] = declaration.module;
                genericStructs_.emplace(normalizeName(declaration.name), &declaration);
            }
        }
    }
}

const ast::FunctionDeclaration* SemanticAnalyzer::genericRoutine(std::string_view name) const
{
    const auto found = genericRoutines_.find(normalizeName(name));
    return found != genericRoutines_.end() ? found->second : nullptr;
}

const ast::TypeDeclaration* SemanticAnalyzer::genericStruct(std::string_view name) const
{
    const auto found = genericStructs_.find(normalizeName(name));
    return found != genericStructs_.end() ? found->second : nullptr;
}

TypeRef SemanticAnalyzer::analyzeExpressionFor(const ast::Expression& expression, TypeRef target)
{
    const ast::Expression* previousExpression = std::exchange(targetExpression_, &expression);
    const TypeRef previousType = std::exchange(targetType_, target);
    try {
        // A UInt64 target makes its literals UInt64 (change uint64).
        const TypeRef type = analyzeWithIntegerContext(expression, target);
        targetExpression_ = previousExpression;
        targetType_ = previousType;
        return type;
    } catch (...) {
        targetExpression_ = previousExpression;
        targetType_ = previousType;
        throw;
    }
}

void SemanticAnalyzer::unifyTypeSyntax(const types::TypeSyntax& pattern, TypeRef actual,
                                       const std::vector<std::string>& parameters, std::vector<TypeRef>& bound,
                                       const std::string& owner, const std::string& from,
                                       const std::vector<TypeRef>* fixed)
{
    if (actual == nullptr) {
        return;
    }
    switch (pattern.form) {
    case types::TypeSyntax::Form::Named:
        for (std::size_t index = 0; index < parameters.size(); ++index) {
            if (!equalsIgnoreCase(pattern.name, parameters[index])) {
                continue;
            }
            if (actual->isNone()) {
                return;  // None fixes nothing
            }
            if (fixed != nullptr && (*fixed)[index] != nullptr) {
                return;  // a literal takes the type the other arguments fixed
            }
            if (bound[index] == nullptr) {
                bound[index] = actual;
            } else if (bound[index] != actual) {
                throw SemanticError(parameters[index] + " of " + owner + " is " + bound[index]->name() +
                                    ", but " + from + " is " + actual->name() +
                                    ": the type arguments of a call are one type each (CANON-8)");
            }
            return;
        }
        return;
    case types::TypeSyntax::Form::Array:
        if (actual->isArray()) {
            unifyTypeSyntax(pattern.argument(), actual->element(), parameters, bound, owner, from, fixed);
        }
        return;
    case types::TypeSyntax::Form::Generic:
        break;
    }
    if (equalsIgnoreCase(pattern.name, "Slice")) {
        if (actual->isSlice()) {
            unifyTypeSyntax(pattern.argument(), actual->element(), parameters, bound, owner, from, fixed);
        }
        return;
    }
    if (equalsIgnoreCase(pattern.name, "Own") || equalsIgnoreCase(pattern.name, "Link")) {
        const bool matches = equalsIgnoreCase(pattern.name, "Own") ? actual->isOwn() : actual->isLink();
        if (matches) {
            unifyTypeSyntax(pattern.argument(), actual->pointerTarget(), parameters, bound, owner, from, fixed);
        }
        return;
    }
    if (equalsIgnoreCase(pattern.name, "Option")) {
        if (actual->isOptionOwn()) {
            unifyTypeSyntax(pattern.argument(), types().own(actual->pointerTarget()), parameters, bound, owner, from,
                            fixed);
        }
        return;
    }
    const auto instance = structInstanceOf_.find(actual);
    if (instance == structInstanceOf_.end() || !equalsIgnoreCase(instance->second.generic->name, pattern.name) ||
        instance->second.arguments.size() != pattern.arguments.size()) {
        return;
    }
    for (std::size_t index = 0; index < pattern.arguments.size(); ++index) {
        unifyTypeSyntax(*pattern.arguments[index], instance->second.arguments[index], parameters, bound, owner,
                        from, fixed);
    }
}

TypeRef SemanticAnalyzer::analyzeGenericCall(const ast::FunctionDeclaration& generic,
                                             const ast::IdentifierExpression& callee,
                                             const ast::CallExpression* call)
{
    const std::vector<std::string>& parameters = generic.typeParameters();
    std::string writtenResult;
    const std::vector<WrittenParameter> written = writtenParameters(generic.signatureTokens(), writtenResult);
    const std::size_t count = call != nullptr ? call->arguments().size() : 0;
    if (count != written.size()) {
        throw SemanticError("function " + generic.name() + " expects " + std::to_string(written.size()) +
                            " arguments, got " + std::to_string(count));
    }

    // Every argument is analyzed once; its type is then matched against the
    // written type of its parameter. A literal fixes a type only when nothing
    // else does (`Max(B, 1)` with B Byte is Max<UInt8>), as `B + 1` reads 1
    // in B's type (CANON-8).
    // The literals are analyzed after the other arguments: a literal whose
    // parameter is a T that they made UInt64 is a UInt64 (change uint64).
    std::vector<TypeRef> argumentTypes(count, nullptr);
    for (std::size_t index = 0; index < count; ++index) {
        if (!isLiteralArgument(*call->arguments()[index])) {
            argumentTypes[index] = analyzeExpression(*call->arguments()[index]);
        }
    }
    std::vector<TypeRef> bound(parameters.size(), nullptr);
    std::vector<TypeRef> fixedByValues;
    for (const bool literals : {false, true}) {
        if (literals) {
            fixedByValues = bound;
        }
        for (std::size_t index = 0; index < count; ++index) {
            if (isLiteralArgument(*call->arguments()[index]) != literals) {
                continue;
            }
            const auto pattern = types::readTypeSyntax(written[index].type);
            if (literals) {
                TypeRef context = nullptr;
                if (pattern && pattern->form == types::TypeSyntax::Form::Named) {
                    for (std::size_t parameter = 0; parameter < parameters.size(); ++parameter) {
                        if (equalsIgnoreCase(pattern->name, parameters[parameter])) {
                            context = bound[parameter];
                        }
                    }
                }
                argumentTypes[index] = analyzeWithIntegerContext(*call->arguments()[index], context);
            }
            if (pattern) {
                unifyTypeSyntax(*pattern, argumentTypes[index], parameters, bound, generic.name(),
                                "argument " + std::to_string(index + 1), literals ? &fixedByValues : nullptr);
            }
        }
    }
    const ast::Expression& site = call != nullptr ? static_cast<const ast::Expression&>(*call) : callee;
    bool complete = true;
    for (const TypeRef type : bound) {
        complete = complete && type != nullptr;
    }
    if (!complete && targetExpression_ == &site && targetType_ != nullptr && !writtenResult.empty()) {
        if (const auto pattern = types::readTypeSyntax(writtenResult)) {
            unifyTypeSyntax(*pattern, targetType_, parameters, bound, generic.name(), "the target", nullptr);
        }
    }
    for (std::size_t index = 0; index < bound.size(); ++index) {
        if (bound[index] == nullptr) {
            throw SemanticError("the type " + parameters[index] + " of " + generic.name() +
                                " is not known at this call: pass an argument of that type, or write the "
                                "type of the target (X Integer := " + generic.name() +
                                (count == 0 ? "" : "(...)") + ") (CANON-8)");
        }
    }

    const FunctionSignature& signature = instantiateRoutine(generic, bound);
    const Symbol* symbol = symbols_.globalScope().resolve(signature.name);
    result_.bind(callee, *symbol);
    if (call == nullptr) {
        result_.setExpressionType(callee, signature.returnType);
        return signature.returnType;
    }
    result_.bind(*call, *symbol);
    const TypeRef type = analyzeUserFunctionCall(signature, call->arguments(), &argumentTypes);
    for (std::size_t index = 0; index < call->arguments().size(); ++index) {
        if (signature.parameters[index].mode == PassMode::RefMut) {
            rejectInContract("calling " + signature.name + ", which changes a ref mut argument,");
            recordInvariantPlaces(*call, *call->arguments()[index], false, true);
        }
    }
    return type;
}

TypeRef SemanticAnalyzer::analyzeGenericConstruction(const ast::TypeDeclaration& generic,
                                                     const ast::CallExpression& call)
{
    const std::vector<std::string>& parameters = generic.typeParameters;
    std::vector<TypeRef> bound(parameters.size(), nullptr);
    // `P TPair<String, Integer> := TPair(...)`: the target names the types.
    if (targetExpression_ == &call && targetType_ != nullptr) {
        const auto instance = structInstanceOf_.find(targetType_);
        if (instance != structInstanceOf_.end() && instance->second.generic == &generic) {
            bound = instance->second.arguments;
        }
    }
    std::vector<TypeRef> valueTypes;
    if (bound.empty() || bound.front() == nullptr) {
        // Otherwise the values do, field by field.
        std::vector<const ast::StructFieldDeclaration*> fields;
        for (const auto& argument : call.arguments()) {
            const ast::BinaryExpression* named = namedArgument(*argument);
            if (named == nullptr) {
                throw SemanticError("positional construction is not canonical (CANON-9): write " + generic.name +
                                    "(Field := Value, ...)");
            }
            const std::string& fieldName = static_cast<const ast::IdentifierExpression&>(named->left()).name();
            const ast::StructFieldDeclaration* field = nullptr;
            for (const ast::StructFieldDeclaration& candidate : generic.fields) {
                if (equalsIgnoreCase(candidate.name, fieldName)) {
                    field = &candidate;
                }
            }
            if (field == nullptr) {
                throw SemanticError("unknown field in construction: " + generic.name + "." + fieldName);
            }
            fields.push_back(field);
            valueTypes.push_back(analyzeExpression(named->right()));
        }
        std::vector<TypeRef> fixedByValues;
        for (const bool literals : {false, true}) {
            if (literals) {
                fixedByValues = bound;
            }
            for (std::size_t index = 0; index < fields.size(); ++index) {
                const ast::Expression& value = static_cast<const ast::BinaryExpression&>(*call.arguments()[index]).right();
                if (isLiteralArgument(value) != literals) {
                    continue;
                }
                if (const auto pattern = types::readTypeSyntax(fields[index]->typeName)) {
                    unifyTypeSyntax(*pattern, valueTypes[index], parameters, bound, generic.name,
                                    "field " + fields[index]->name, literals ? &fixedByValues : nullptr);
                }
            }
        }
    }
    for (std::size_t index = 0; index < bound.size(); ++index) {
        if (bound[index] == nullptr) {
            throw SemanticError("the type " + parameters[index] + " of " + generic.name +
                                " is not known in this construction: give the fields that use it, or write the "
                                "type of the target (P " + generic.name + "<" + parameterList(parameters) +
                                "> := " + generic.name + "(...)) (CANON-8)");
        }
    }

    const TypeRef type = instantiateStruct(generic, bound);
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    const Symbol& symbol = resolveOrThrow(generic.name);
    result_.bind(callee, symbol);
    result_.bind(call, symbol);
    result_.setExpressionType(callee, type);
    return analyzeConstruction(call, type, valueTypes.empty() ? nullptr : &valueTypes);
}

TypeRef SemanticAnalyzer::resolveGenericStructType(const types::TypeSyntax& syntax, std::string_view written)
{
    const ast::TypeDeclaration* generic = genericStruct(syntax.name);
    if (generic == nullptr) {
        if (types().lookup(syntax.name) != nullptr) {
            throw SemanticError(syntax.name + " is not generic: write it without type arguments, not " +
                                std::string(written));
        }
        throw SemanticError("unknown type: " + unknownNameText(written));
    }
    if (syntax.arguments.size() != generic->typeParameters.size()) {
        throw SemanticError(generic->name + " takes " + std::to_string(generic->typeParameters.size()) +
                            " type argument" + (generic->typeParameters.size() == 1 ? "" : "s") + " (" +
                            parameterList(generic->typeParameters) + "), got " +
                            std::to_string(syntax.arguments.size()) + ": " + std::string(written));
    }
    std::vector<TypeRef> arguments;
    for (const auto& argument : syntax.arguments) {
        arguments.push_back(resolveTypeSyntax(*argument, argument->spelling()));
    }
    return instantiateStruct(*generic, arguments);
}

TypeRef SemanticAnalyzer::instantiateStruct(const ast::TypeDeclaration& generic, const std::vector<TypeRef>& arguments)
{
    requireBoundedNesting(generic.name, arguments);
    const std::string name = instanceName(generic.name, arguments);
    if (const TypeRef existing = types().lookup(name)) {
        return existing;
    }
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        if (arguments[index]->isNone() || arguments[index]->isVoid()) {
            throw SemanticError(generic.typeParameters[index] + " of " + generic.name + " cannot be " +
                                arguments[index]->name());
        }
    }
    const TypeRef type = types().declareStruct(name);
    symbols_.globalScope().declare(name, SymbolKind::Type, type);
    structInstanceOf_.emplace(type, StructInstance{&generic, arguments});
    const auto replace = [&](std::string_view written) -> std::string {
        for (std::size_t index = 0; index < arguments.size(); ++index) {
            if (equalsIgnoreCase(written, generic.typeParameters[index])) {
                return arguments[index]->name();
            }
        }
        return {};
    };
    try {
        std::vector<types::StructField> fields;
        for (const ast::StructFieldDeclaration& declaration : generic.fields) {
            const std::string written = types::substituteTypeNames(declaration.typeName, replace);
            if (types::isGenericSpelling(written, "Slice")) {
                throw SemanticError("a slice cannot be a struct field (ADR-0017): " + declaration.name);
            }
            const TypeRef fieldType = resolveWrittenType(written);
            rejectFileIn(fieldType, "a struct field, " + declaration.name);
            // r3.82: no instance may bring a default the generic struct does
            // not guarantee, so a field whose type uses a type parameter has
            // none; it is given in the construction.
            const bool usesParameter = written != declaration.typeName;
            if (declaration.hasDefault && usesParameter) {
                throw SemanticError("a field whose type uses a type parameter has no default; give " +
                                    declaration.name + " in the construction (r3.82)");
            }
            if (declaration.hasDefault && declaration.fillDefault) {
                validateFillDefault(declaration.name, fieldType, declaration.defaultValue);
            } else if (declaration.hasDefault) {
                if (fieldType->isPointer() || fieldType->isArray() || fieldType->isStruct()) {
                    throw SemanticError("only a scalar struct field has a default (CANON-9): " + declaration.name +
                                        " is " + fieldType->name());
                }
                validateStructFieldDefault(declaration.name, fieldType, declaration.defaultValue);
            }
            types::StructField field{declaration.name, fieldType, false, declaration.hasDefault,
                                     declaration.defaultValue};
            field.fillDefault = declaration.fillDefault;
            field.fromTypeParameter = !replace(declaration.typeName).empty();
            field.isPublic = declaration.isPublic;
            fields.push_back(std::move(field));
        }
        types().setStructFields(type, std::move(fields));
        rejectRecursiveStruct(type);
    } catch (const SemanticError& error) {
        throw SemanticError("in " + name + ": " + error.what());
    }
    result_.addStructInstance(type);
    return type;
}

const FunctionSignature& SemanticAnalyzer::instantiateRoutine(const ast::FunctionDeclaration& generic,
                                                              const std::vector<TypeRef>& arguments)
{
    requireBoundedNesting(generic.name(), arguments);
    const std::string name = instanceName(generic.name(), arguments);
    if (const FunctionSignature* existing = resolveFunctionSignature(name)) {
        return *existing;
    }
    if (result_.instances().size() >= kMaxInstances) {
        throw SemanticError("more than " + std::to_string(kMaxInstances) + " instances of generic routines, the "
                            "last " + name + ": does a generic routine call itself with a bigger type each time?");
    }
    const std::vector<std::string>& parameters = generic.typeParameters();
    for (const std::string& parameter : parameters) {
        if (types().lookup(parameter) != nullptr || genericStruct(parameter) != nullptr) {
            throw SemanticError("the type parameter " + parameter + " of " + generic.name() +
                                " is the name of a type; choose another name");
        }
    }
    const auto replace = [&](std::string_view written) -> std::string {
        for (std::size_t index = 0; index < parameters.size(); ++index) {
            if (equalsIgnoreCase(written, parameters[index])) {
                return arguments[index]->name();
            }
        }
        return {};
    };

    std::vector<std::string> tokens;
    for (const std::string& token : generic.signatureTokens()) {
        tokens.push_back(types::substituteTypeNames(token, replace));
    }
    ast::CloneRewriters rewrite;
    rewrite.typeName = [&](const std::string& written) { return types::substituteTypeNames(written, replace); };
    // A type parameter named in an expression (`New(T)`, `T(F := X)`).
    rewrite.expression = [&](const ast::Expression& node) -> ast::ExpressionPtr {
        if (node.kind() == ast::AstNodeKind::IdentifierExpression) {
            const std::string replacement = replace(static_cast<const ast::IdentifierExpression&>(node).name());
            if (!replacement.empty()) {
                return std::make_unique<ast::IdentifierExpression>(replacement);
            }
        }
        return nullptr;
    };
    auto made = std::make_unique<ast::FunctionDeclaration>(name, std::move(tokens),
                                                           ast::cloneStatements(generic.body(), rewrite));
    made->copyModuleFrom(generic);
    const ast::FunctionDeclaration& instance = result_.addInstance(std::move(made));
    symbols_.globalScope().declare(name, SymbolKind::Function);
    try {
        registerFunctionSignature(instance);
    } catch (const SemanticError& error) {
        throw SemanticError("in " + name + ", used in " + currentRoutine_ + ": " + error.what());
    }
    pendingInstances_.push_back(PendingInstance{&instance, currentRoutine_});
    return *resolveFunctionSignature(name);
}

// After the program's routines: each instance is checked as a routine of
// the module; checking one may make more.
void SemanticAnalyzer::analyzePendingInstances()
{
    for (std::size_t index = 0; index < pendingInstances_.size(); ++index) {
        const PendingInstance pending = pendingInstances_[index];
        try {
            analyzeFunction(*pending.declaration);
        } catch (const SemanticError& error) {
            throw SemanticError("in " + pending.declaration->name() + ", used in " + pending.origin + ": " +
                                error.what());
        }
    }
    pendingInstances_.clear();
}

} // namespace inox::compiler::semantic
