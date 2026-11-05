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

bool isTypeParameter(const std::string& name, const std::vector<std::string>& parameters)
{
    for (const std::string& parameter : parameters) {
        if (equalsIgnoreCase(name, parameter)) {
            return true;
        }
    }
    return false;
}

// Debug builds check the invariants of each written type substitution
// makes (design.md of structured-types).
void checkWrittenType([[maybe_unused]] const ast::TypeExpr& written)
{
#ifndef NDEBUG
    ast::checkTypeExpr(written);
#endif
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
                // A method of a generic struct is a template of its own
                // (CANON-8): instantiated with the receiver's instance.
                if (ast::methodDot(function.name()) != std::string::npos) {
                    genericMethods_.emplace(normalizeName(function.name()), &function);
                    genericMethodOrder_.push_back(&function);
                } else {
                    genericRoutines_.emplace(normalizeName(function.name()), &function);
                }
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

// Rule 7 of generic-struct-methods: a method of a generic struct writes the
// struct's own type parameters, all of them, in order (Q1).
void SemanticAnalyzer::checkGenericMethods()
{
    for (const ast::FunctionDeclaration* function : genericMethodOrder_) {
        const std::size_t dot = ast::methodDot(function->name());
        const std::string owner = function->name().substr(0, dot);
        const std::string method = function->name().substr(dot + 1);
        notePosition(*function);
        const ast::TypeDeclaration* generic = genericStruct(owner);
        if (generic == nullptr) {
            if (types().lookup(owner) != nullptr) {
                throw SemanticError(owner + " is not generic: write " + owner + "." + method + "(...)");
            }
            throw SemanticError("unknown generic struct " + owner + " of the method " + method);
        }
        std::string expected;
        for (const std::string& parameter : generic->typeParameters) {
            expected += (expected.empty() ? "" : ", ") + parameter;
        }
        const std::vector<std::string>& written = function->typeParameters();
        if (written.size() != generic->typeParameters.size()) {
            throw SemanticError(owner + " takes " + std::to_string(generic->typeParameters.size()) +
                                " type parameters (" + expected + "): write " + owner + "<" + expected + ">." +
                                method + "(...)");
        }
        for (std::size_t index = 0; index < written.size(); ++index) {
            if (!equalsIgnoreCase(written[index], generic->typeParameters[index])) {
                throw SemanticError("the type parameters of " + owner + " are " + expected + ": write " + owner +
                                    "<" + expected + ">." + method + "(...)");
            }
        }
    }
}

// `TPair.Key(Self)` for a generic TPair: its type parameters are written.
void SemanticAnalyzer::requireGenericMethodHeader(const std::string& owner, const std::string& method) const
{
    std::string expected;
    for (const std::string& parameter : genericStruct(owner)->typeParameters) {
        expected += (expected.empty() ? "" : ", ") + parameter;
    }
    throw SemanticError(owner + " is generic: write its type parameters, " + owner + "<" + expected + ">." + method +
                        "(...) (CANON-8)");
}

// The method `method` of the instance `receiver` of a generic struct, made
// on its first call (CANON-8): `TPair<String, Int64>.Swap`. Null when the
// receiver is no instance or its struct has no such method. Out of line:
// the method lookups stay small (stack budget).
const FunctionSignature* SemanticAnalyzer::genericMethodFor(TypeRef receiver, const std::string& method)
{
    const auto instance = structInstanceOf_.find(receiver);
    if (instance == structInstanceOf_.end()) {
        return nullptr;
    }
    const auto found = genericMethods_.find(normalizeName(instance->second.generic->name + "." + method));
    if (found == genericMethods_.end()) {
        return nullptr;
    }
    const std::string name = types::nameOf(receiver) + "." + method;
    return &instantiateRoutine(*found->second, instance->second.arguments, &name);
}

// A method of `receiver`'s type: declared, or of a generic struct's
// instance.
const FunctionSignature* SemanticAnalyzer::methodSignature(TypeRef receiver, const std::string& qualifiedName,
                                                           const std::string& method)
{
    if (const FunctionSignature* signature = resolveFunctionSignature(qualifiedName)) {
        return signature;
    }
    return genericMethodFor(receiver, method);
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

// The written form of a resolved type, which substitution puts where a type
// parameter was written: the form reading its name would give, so the
// instance resolves and spells as before.
ast::TypeExpr SemanticAnalyzer::writtenFormOf(TypeRef type, ast::SourcePosition position) const
{
    ast::TypeExpr written;
    written.position = position;
    const auto instance = structInstanceOf_.find(type);
    if (type->isArray()) {
        written.form = ast::TypeExpr::Form::Array;
        written.low.text = std::to_string(type->low());
        written.high.text = std::to_string(type->high());
        written.arguments.push_back(writtenFormOf(type->element(), position));
        return written;
    }
    const char* generic = type->isSlice()       ? "Slice"
                          : type->isSet()       ? "Set"
                          : type->isOwn()       ? "Own"
                          : type->isLink()      ? "Link"
                          : type->isOptionOwn() ? "Option"
                                                : nullptr;
    if (generic != nullptr) {
        written.form = ast::TypeExpr::Form::Generic;
        written.name = generic;
        written.arguments.push_back(writtenFormOf(type->element(), position));
    } else if (instance != structInstanceOf_.end()) {
        written.form = ast::TypeExpr::Form::Generic;
        written.name = instance->second.generic->name;
        for (const TypeRef argument : instance->second.arguments) {
            written.arguments.push_back(writtenFormOf(argument, position));
        }
    } else {
        written.name = type->name();
    }
    return written;
}

// Replaces each Named node that names a type parameter by the written form
// of its argument; true when one was replaced. A replaced node keeps the
// position of the node it replaces (design.md, invariant 4).
bool SemanticAnalyzer::substituteTypeParameters(ast::TypeExpr& written, const std::vector<std::string>& parameters,
                                                const std::vector<TypeRef>& arguments) const
{
    if (written.isNamed()) {
        for (std::size_t index = 0; index < parameters.size(); ++index) {
            if (equalsIgnoreCase(written.name, parameters[index])) {
                written = writtenFormOf(arguments[index], written.position);
                checkWrittenType(written);
                return true;
            }
        }
        return false;
    }
    bool replaced = false;
    for (ast::TypeExpr& argument : written.arguments) {
        replaced = substituteTypeParameters(argument, parameters, arguments) || replaced;
    }
    return replaced;
}

// An instance's header: the template's, with its type arguments for its
// type parameters. Out of line, so the frame of instantiateRoutine, which
// nested generic calls stack up, stays as it was (engineering E19).
void SemanticAnalyzer::substituteHeaderTypes(ast::RoutineHeader& header, const std::vector<std::string>& parameters,
                                             const std::vector<TypeRef>& arguments) const
{
    for (ast::ParameterDeclaration& parameter : header.parameters) {
        if (parameter.type) {
            substituteTypeParameters(*parameter.type, parameters, arguments);
        }
    }
    if (header.resultType) {
        substituteTypeParameters(*header.resultType, parameters, arguments);
    }
}

void SemanticAnalyzer::unifyTypeExpr(const ast::TypeExpr& pattern, TypeRef actual,
                                       const std::vector<std::string>& parameters, std::vector<TypeRef>& bound,
                                       const std::string& owner, const std::string& from,
                                       const std::vector<TypeRef>* fixed)
{
    if (actual == nullptr) {
        return;
    }
    switch (pattern.form) {
    case ast::TypeExpr::Form::Named:
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
    case ast::TypeExpr::Form::Array:
        if (actual->isArray()) {
            unifyTypeExpr(pattern.element(), actual->element(), parameters, bound, owner, from, fixed);
        }
        return;
    case ast::TypeExpr::Form::Generic:
        break;
    }
    if (equalsIgnoreCase(pattern.name, "Slice")) {
        if (actual->isSlice()) {
            unifyTypeExpr(pattern.element(), actual->element(), parameters, bound, owner, from, fixed);
        }
        return;
    }
    if (equalsIgnoreCase(pattern.name, "Own") || equalsIgnoreCase(pattern.name, "Link")) {
        const bool matches = equalsIgnoreCase(pattern.name, "Own") ? actual->isOwn() : actual->isLink();
        if (matches) {
            unifyTypeExpr(pattern.element(), actual->pointerTarget(), parameters, bound, owner, from, fixed);
        }
        return;
    }
    if (equalsIgnoreCase(pattern.name, "Option")) {
        if (actual->isOptionOwn()) {
            unifyTypeExpr(pattern.element(), types().own(actual->pointerTarget()), parameters, bound, owner, from,
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
        unifyTypeExpr(pattern.arguments[index], instance->second.arguments[index], parameters, bound, owner,
                        from, fixed);
    }
}

TypeRef SemanticAnalyzer::analyzeGenericCall(const ast::FunctionDeclaration& generic,
                                             const ast::IdentifierExpression& callee,
                                             const ast::CallExpression* call)
{
    const std::vector<std::string>& parameters = generic.typeParameters();
    const std::optional<ast::TypeExpr>& writtenResult = generic.resultType();
    const std::vector<ast::ParameterDeclaration>& written = generic.parameters();
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
            const ast::TypeExpr& pattern = *written[index].type;
            if (literals) {
                TypeRef context = nullptr;
                if (pattern.isNamed()) {
                    for (std::size_t parameter = 0; parameter < parameters.size(); ++parameter) {
                        if (equalsIgnoreCase(pattern.name, parameters[parameter])) {
                            context = bound[parameter];
                        }
                    }
                }
                argumentTypes[index] = analyzeWithIntegerContext(*call->arguments()[index], context);
            }
            unifyTypeExpr(pattern, argumentTypes[index], parameters, bound, generic.name(),
                            "argument " + std::to_string(index + 1), literals ? &fixedByValues : nullptr);
        }
    }
    const ast::Expression& site = call != nullptr ? static_cast<const ast::Expression&>(*call) : callee;
    bool complete = true;
    for (const TypeRef type : bound) {
        complete = complete && type != nullptr;
    }
    if (!complete && targetExpression_ == &site && targetType_ != nullptr && writtenResult) {
        unifyTypeExpr(*writtenResult, targetType_, parameters, bound, generic.name(), "the target", nullptr);
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
        recordRoutineCall(callee, signature, CallKind::Routine);
        result_.setExpressionType(callee, signature.returnType);
        return signature.returnType;
    }
    result_.bind(*call, *symbol);
    recordRoutineCall(*call, signature, CallKind::Routine);
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
                unifyTypeExpr(fields[index]->type, valueTypes[index], parameters, bound, generic.name,
                                "field " + fields[index]->name, literals ? &fixedByValues : nullptr);
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

TypeRef SemanticAnalyzer::resolveGenericStructType(const ast::TypeExpr& syntax, std::string_view written)
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
        arguments.push_back(resolveTypeExpr(argument, ast::spell(argument)));
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
    try {
        std::vector<types::StructField> fields;
        for (const ast::StructFieldDeclaration& declaration : generic.fields) {
            ast::TypeExpr written = declaration.type;
            const bool usesParameter = substituteTypeParameters(written, generic.typeParameters, arguments);
            if (written.isGeneric("Slice")) {
                throw SemanticError("a slice cannot be a struct field (ADR-0017): " + declaration.name);
            }
            const TypeRef fieldType = resolveWrittenType(written);
            rejectFileIn(fieldType, "a struct field, " + declaration.name);
            // r3.82: no instance may bring a default the generic struct does
            // not guarantee, so a field whose type uses a type parameter has
            // none; it is given in the construction.
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
            appendStructField(fields, declaration, fieldType,
                              declaration.type.isNamed() &&
                                  isTypeParameter(declaration.type.name, generic.typeParameters));
        }
        types().setStructFields(type, std::move(fields));
        rejectRecursiveStruct(type);
    } catch (const SemanticError& error) {
        throw SemanticError("in " + name + ": " + error.what());
    }
    result_.addStructInstance(type);
    if (!generic.invariants.empty()) {
        instantiateInvariant(type);
    }
    return type;
}

// The invariant of an instance of a generic struct (CANON-24): made with
// the instance, so every check of the instance finds it. Out of line: the
// recursive instantiation keeps its frame.
void SemanticAnalyzer::instantiateInvariant(TypeRef instance)
{
    static const std::string invariant = "__invariant";
    genericMethodFor(instance, invariant);
}

// The body of an instance: the template's, with its type arguments for its
// type parameters in written types, in exception handlers (names)
// and where an expression names one (`New(T)`, `T(F := X)`). Out of line,
// so the frame of instantiateRoutine, which nested generic calls stack up,
// holds none of the rewriters (design.md, stack budget).
std::vector<ast::StatementPtr> SemanticAnalyzer::cloneInstanceBody(const ast::FunctionDeclaration& generic,
                                                                    const std::vector<TypeRef>& arguments) const
{
    const std::vector<std::string>& parameters = generic.typeParameters();
    const auto replace = [&](std::string_view written) -> std::string {
        for (std::size_t index = 0; index < parameters.size(); ++index) {
            if (equalsIgnoreCase(written, parameters[index])) {
                return arguments[index]->name();
            }
        }
        return {};
    };
    // On the heap: the rewriters are larger than a pointer (stack budget).
    const auto rewriters = std::make_unique<ast::CloneRewriters>();
    ast::CloneRewriters& rewrite = *rewriters;
    rewrite.type = [&](ast::TypeExpr& written) { substituteTypeParameters(written, parameters, arguments); };
    rewrite.exceptionName = [&](const std::string& written) {
        const std::string replacement = replace(written);
        return replacement.empty() ? written : replacement;
    };
    rewrite.expression = [&](const ast::Expression& node) -> ast::ExpressionPtr {
        if (node.kind() == ast::AstNodeKind::IdentifierExpression) {
            const std::string replacement = replace(static_cast<const ast::IdentifierExpression&>(node).name());
            if (!replacement.empty()) {
                return std::make_unique<ast::IdentifierExpression>(replacement);
            }
        }
        return nullptr;
    };
    return ast::cloneStatements(generic.body(), rewrite);
}

const FunctionSignature& SemanticAnalyzer::instantiateRoutine(const ast::FunctionDeclaration& generic,
                                                              const std::vector<TypeRef>& arguments,
                                                              const std::string* instance)
{
    requireBoundedNesting(generic.name(), arguments);
    const std::string name = instance == nullptr ? instanceName(generic.name(), arguments) : *instance;
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
    auto made = std::make_unique<ast::FunctionDeclaration>(name, generic.header(),
                                                           cloneInstanceBody(generic, arguments));
    substituteHeaderTypes(made->header(), parameters, arguments);
    made->copyModuleFrom(generic);
    made->setPosition(generic.position());  // an instance is where its template is
    const ast::FunctionDeclaration& declared = result_.addInstance(std::move(made));
    symbols_.globalScope().declare(name, SymbolKind::Function);
    registerInstanceSignature(declared, name);
    pendingInstances_.push_back(PendingInstance{&declared, currentRoutine_});
    return *resolveFunctionSignature(name);
}

// The signature of an instance, registered where its template is; the call
// that made it stays the place of the diagnostics that follow. Out of line:
// the frame of instantiateRoutine holds no place (stack budget).
void SemanticAnalyzer::registerInstanceSignature(const ast::FunctionDeclaration& instance, const std::string& name)
{
    const DiagnosticPlace call = diagnosticPlace();
    try {
        registerFunctionSignature(instance);
    } catch (const SemanticError& error) {
        throw SemanticError("in " + name + ", used in " + currentRoutine_ + ": " + error.what());
    }
    diagnosticPlace() = call;
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
