// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: routine signatures, scopes and symbols, module items, Type/Const/State sections, structs.

#include "SemanticAnalyzer.h"
#include "StdIntrinsics.h"

#include "SemanticSupport.h"
#include "../exceptions/ExceptionTypes.h"
#include "../types/TypeSyntax.h"
#include "../lexer/Lexer.h"

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

void SemanticAnalyzer::registerFunctionSignatures(const ast::ModuleNode& module)
{
    for (const auto& item : module.items()) {
        if (item->kind() == ast::AstNodeKind::FunctionDeclaration) {
            const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
            if (!function.isGeneric()) {  // a template: its instances get signatures
                registerFunctionSignature(function);
            }
        }
    }
}

void SemanticAnalyzer::registerFunctionSignature(const ast::FunctionDeclaration& function)
{
    const std::string previousModule = currentModule_;
    enterModule(function.module());
    registerFunctionSignatureIn(function);
    enterModule(previousModule);
}

void SemanticAnalyzer::registerFunctionSignatureIn(const ast::FunctionDeclaration& function)
{
    const auto& tokens = function.signatureTokens();

    FunctionSignature signature;
    signature.name = function.name();
    signature.isOverride = function.isOverride();
    signature.isPublic = function.isPublic();
    signature.module = function.module();

    const std::size_t dot = ast::methodDot(function.name());
    const bool isAssociatedMethod = dot != std::string::npos;
    const std::string receiverType = isAssociatedMethod ? function.name().substr(0, dot) : std::string{};

    std::size_t index = 0;
    if (!tokens.empty() && tokens.front() == "(") {
        index = 1;
        if (index < tokens.size() && tokens[index] == ")") {
            throw SemanticError("empty parentheses are not allowed in declarations: " + function.name());
        }

        while (index < tokens.size() && tokens[index] != ")") {
            // ADR-0016: `ref X T` and `ref mut X T` pass by reference.
            PassMode mode = PassMode::Value;
            if (equalsIgnoreCase(tokens[index], "ref")) {
                ++index;
                mode = PassMode::Ref;
                if (index < tokens.size() && equalsIgnoreCase(tokens[index], "mut")) {
                    ++index;
                    mode = PassMode::RefMut;
                }
                if (isAssociatedMethod && signature.parameters.empty()) {
                    throw SemanticError("the receiver of a method is Self or Self mut, never ref (ADR-0016)");
                }
            }
            if (equalsIgnoreCase(tokens[index], "mut")) {
                throw SemanticError("mutable parameters are reserved for a future Inox version: use a local variable or return a new value");
            }
            if (!looksLikeIdentifier(tokens[index])) {
                throw SemanticError("expected parameter name in function: " + function.name());
            }
            const std::string parameterName = tokens[index++];
            TypeRef parameterType = nullptr;

            if (isAssociatedMethod && signature.parameters.empty() && equalsIgnoreCase(parameterName, "Self")) {
                // `Self mut` may change the receiver; `Self` only reads it
                // (CANON-9). A method of a built-in type (`String.Trim`) has
                // the same two receivers: `Self mut` is the caller's variable.
                if (index < tokens.size() && equalsIgnoreCase(tokens[index], "mut")) {
                    ++index;
                    mode = PassMode::RefMut;
                }
                parameterType = resolveWrittenType(receiverType);
                if (!parameterType->isStruct() &&
                    (types::isArraySpelling(receiverType) || isFileType(parameterType))) {
                    throw SemanticError("methods belong to structs and built-in value types; " +
                                        receiverType + " cannot have methods: " + function.name());
                }
            } else {
                if (index < tokens.size() && equalsIgnoreCase(tokens[index], "mut")) {
                    throw SemanticError("mutable parameters are reserved for a future Inox version: use a local variable or return a new value");
                }
                if (index >= tokens.size() ||
                    (!looksLikeIdentifier(tokens[index]) && !types::isArraySpelling(tokens[index]) &&
                     !types::isGenericSpelling(tokens[index]))) {
                    throw SemanticError("expected type for parameter: " + parameterName);
                }
                parameterType = resolveWrittenType(tokens[index]);
                if (mode != PassMode::Value && parameterType->isSlice()) {
                    throw SemanticError("a slice is already a view: pass it without ref (ADR-0017)");
                }
                ++index;
            }

            for (const FunctionParameter& parameter : signature.parameters) {
                if (equalsIgnoreCase(parameter.name, parameterName)) {
                    throw SemanticError("duplicate parameter: " + parameterName);
                }
            }
            if (mode != PassMode::RefMut) {
                rejectFileIn(parameterType, "passed by value or as ref; write `ref mut " + parameterName + " File`");
            }
            signature.parameters.push_back(FunctionParameter{parameterName, parameterType, mode});

            if (index < tokens.size() && tokens[index] == ",") {
                ++index;
            } else if (index >= tokens.size() || tokens[index] != ")") {
                throw SemanticError("expected ',' or ')' in function: " + function.name());
            }
        }

        if (index >= tokens.size() || tokens[index] != ")") {
            throw SemanticError("expected ')' in function: " + function.name());
        }
        ++index;
    }

    if (index < tokens.size()) {
        if (index + 1 != tokens.size() ||
            (!looksLikeIdentifier(tokens[index]) && !types::isArraySpelling(tokens[index]) &&
             !types::isGenericSpelling(tokens[index]))) {
            throw SemanticError("invalid return type in function: " + function.name());
        }
        signature.returnType = resolveWrittenType(tokens[index]);
        rejectFileIn(signature.returnType, "returned by " + function.name());
        if (signature.returnType->isSlice()) {
            throw SemanticError("a slice cannot be returned: it would outlive its array (ADR-0017)");
        }
    }

    if (isAssociatedMethod && resolveStruct(receiverType) == nullptr) {
        // A method of a built-in type is found under the type's canonical
        // name (`Integer.Twice` is called on an Int64 value).
        if (signature.isOverride) {
            throw SemanticError(function.name() + " is marked Override, but " + receiverType +
                                " reuses no struct: Override replaces a reused method (CANON-9)");
        }
        const TypeRef canonical = resolveWrittenType(receiverType);
        const std::string canonicalKey = normalizeName(canonical->name() + function.name().substr(dot));
        if (canonicalKey != normalizeName(function.name())) {
            if (functions_.contains(canonicalKey)) {
                throw SemanticError("method " + function.name().substr(dot + 1) + " is declared twice for " +
                                    canonical->name() + " (" + receiverType + " is " + canonical->name() + ")");
            }
            functions_.emplace(canonicalKey, signature);
        }
    }
    if (functions_.contains(normalizeName(function.name()))) {
        throw SemanticError("duplicate routine: " + function.name());
    }
    result_.setSignature(function, signature);
    functions_.emplace(normalizeName(function.name()), std::move(signature));
}

const FunctionSignature* SemanticAnalyzer::resolveFunctionSignature(std::string_view name) const
{
    const auto iterator = functions_.find(normalizeName(name));
    return iterator != functions_.end() ? &iterator->second : nullptr;
}

// CANON-2 (r3.74): True, False, Exit, bitand, bitor, bitxor and bitnot
// are predefined names, not reserved words; they keep their meaning and can
// never be declared.
void SemanticAnalyzer::rejectPredefinedName(std::string_view name)
{
    std::string normalized(name);
    for (char& ch : normalized) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    if (inox::compiler::lexer::Lexer::isPredefinedName(normalized)) {
        throw SemanticError(std::string(name) + " is a predefined name of Inox and cannot be declared (CANON-2)");
    }
}

// r3.99: the text of an unknown name, with why it is not seen when another
// module declares it.
std::string SemanticAnalyzer::unknownNameText(std::string_view name) const
{
    if (program_ != nullptr) {
        if (std::string hint = program_->nameHint(currentModule_, name); !hint.empty()) {
            return hint;
        }
    }
    return std::string(name);
}

// r3.99: the analysis of a declaration of `module` sees the library
// functions of the modules it uses (with what they re-export), not those
// another module of the program uses.
void SemanticAnalyzer::enterModule(const std::string& module)
{
    if (program_ == nullptr || module.empty()) {
        return;
    }
    currentModule_ = module;
    const std::vector<std::string> visible = program_->visibleModules(module);
    const auto sees = [&](std::string_view name) {
        for (const std::string& entry : visible) {
            if (equalsIgnoreCase(entry, name)) {
                return true;
            }
        }
        return false;
    };
    ioUtilsVisible_ = sees("Std.IOUtils");
    filesVisible_ = ioUtilsVisible_ || sees("Std.Files");
    debugVisible_ = sees("Std.Debug");
    mathVisible_ = sees("Std.Math");
    strUtilsVisible_ = sees("Std.StrUtils");
    stringsVisible_ = strUtilsVisible_ || sees("Std.Strings");
    intrinsicModules_.clear();
    for (const ModuleIntrinsic& function : kModuleIntrinsics) {
        if (sees(function.module)) {
            intrinsicModules_.insert(std::string(function.module));
        }
    }
}

// r3.99: the module that declares a struct (a generic instance is its
// template's), "" for a type of no module.
std::string SemanticAnalyzer::moduleOfType(TypeRef type) const
{
    if (type == nullptr) {
        return {};
    }
    std::string name = type->name();
    if (const std::size_t angle = name.find('<'); angle != std::string::npos) {
        name.resize(angle);
    }
    const auto found = typeModules_.find(normalizeName(name));
    return found != typeModules_.end() ? found->second : std::string();
}

bool SemanticAnalyzer::outsideModuleOf(TypeRef type) const
{
    const std::string module = moduleOfType(type);
    return !module.empty() && !currentModule_.empty() && !equalsIgnoreCase(module, currentModule_);
}

// r3.99 (C1): a private field is reached only in its struct's module.
void SemanticAnalyzer::requireFieldVisible(TypeRef structType, const types::StructField& field) const
{
    if (!field.isPublic && outsideModuleOf(structType)) {
        throw SemanticError(field.name + " is not Public in " + moduleOfType(structType) + " (r3.99): only " +
                            moduleOfType(structType) + " reaches the private fields of " + structType->name());
    }
}

// r3.99: a method without Public is called only in its module. The methods
// the compiler makes (`__invariant`, `__Name_body`) are its own.
void SemanticAnalyzer::requireMethodVisible(const FunctionSignature& signature) const
{
    const std::size_t dot = ast::methodDot(signature.name);
    const std::string method = dot == std::string::npos ? signature.name : signature.name.substr(dot + 1);
    if (signature.isPublic || signature.module.empty() || currentModule_.empty() ||
        method.rfind("__", 0) == 0 || equalsIgnoreCase(signature.module, currentModule_)) {
        return;
    }
    throw SemanticError(method + " is not Public in " + signature.module + " (r3.99): only " + signature.module +
                        " calls the private methods of its types");
}

// r3.99 (C2): outside its module, a struct whose private fields have no
// default is built only by a routine of its module.
void SemanticAnalyzer::requireBuildableHere(TypeRef type) const
{
    if (type == nullptr || !type->isStruct() || !outsideModuleOf(type)) {
        return;
    }
    for (const types::StructField& field : type->fields()) {
        const bool bornEmpty = field.type != nullptr && (field.type->isLink() || field.type->isOptionOwn());
        if (!field.isPublic && !field.hasDefault && !bornEmpty &&
            (field.type == nullptr || !field.type->isStruct() || !requiredFieldWithoutDefault(field.type).empty())) {
            throw SemanticError(type->name() + " is built only in " + moduleOfType(type) + " (r3.99): its private "
                                "field " + field.name + " has no default; call a routine of " +
                                moduleOfType(type) + " that builds it");
        }
    }
}

void SemanticAnalyzer::declareOrThrow(
    std::string_view name,
    SymbolKind kind,
    TypeRef type,
    bool isMutable)
{
    rejectPredefinedName(name);
    // r3.99 (A2): `Bank.Accounts.Fee` must mean the module, so no variable
    // takes the first name of a module or an alias its module uses.
    if ((kind == SymbolKind::Variable || kind == SymbolKind::LoopIterator) && program_ != nullptr &&
        program_->reservesRoot(currentModule_, name)) {
        throw SemanticError(std::string(name) + " is the name of a module " + currentModule_ +
                            " uses, in its qualified names: no variable takes it (r3.99)");
    }
    Scope& scope = symbols_.currentScope();
    if (scope.containsLocal(name)) {
        throw SemanticError("duplicate symbol in same scope: " + std::string(name));
    }
    if (scope.containsInAncestors(name)) {
        throw SemanticError("shadowing is forbidden: " + std::string(name));
    }
    scope.declare(std::string(name), kind, type, isMutable);
}

// OPEN-4 (decided by Marcelo Fortes, 2026-10-10): a local variable that is
// never read is a compile error, as in Go. A misspelled name in FORM 1
// (`Coutner := Counter + 1`) declares a new variable that nothing reads, so
// the typo is caught. Parameters, `for` iterators, exception bindings, State
// and Const are not tracked; assigning to a variable is not reading it.
void SemanticAnalyzer::trackLocal(std::string_view name)
{
    if (const Symbol* symbol = symbols_.currentScope().resolve(name)) {
        localsToRead_.push_back({symbol, &symbols_.currentScope()});
    }
}

void SemanticAnalyzer::leaveScope()
{
    const Scope* scope = &symbols_.currentScope();
    forgetMovedOwnersOf(scope);
    std::string unread;
    for (auto it = localsToRead_.begin(); it != localsToRead_.end();) {
        if (it->scope != scope) {
            ++it;
            continue;
        }
        if (!readSymbols_.contains(it->symbol)) {
            unread += (unread.empty() ? "" : ", ") + it->symbol->name;
        }
        it = localsToRead_.erase(it);
    }
    symbols_.popScope();
    if (!unread.empty()) {
        throw SemanticError(
            std::string(unread.find(',') == std::string::npos ? "local variable never read: "
                                                               : "local variables never read: ") +
            unread + " (OPEN-4: read it or remove it; a misspelled name declares a new variable)");
    }
}

const Symbol& SemanticAnalyzer::resolveOrThrow(std::string_view name) const
{
    if (isInternalSyntheticName(name)) {
        const Symbol* symbol = symbols_.currentScope().resolve(name);
        return *symbol;
    }
    const Symbol* symbol = symbols_.currentScope().resolve(name);
    if (symbol == nullptr) {
        // r3.99: a name another module declares says why it is not seen.
        if (program_ != nullptr) {
            if (std::string hint = program_->nameHint(currentModule_, name); !hint.empty()) {
                throw SemanticError("unknown symbol: " + hint);
            }
        }
        throw SemanticError("unknown symbol: " + std::string(name));
    }
    return *symbol;
}

void SemanticAnalyzer::declareModuleItem(const ast::AstNode& item)
{
    switch (item.kind()) {
    case ast::AstNodeKind::FunctionDeclaration: {
        const auto& function = static_cast<const ast::FunctionDeclaration&>(item);
        declareOrThrow(function.name(), SymbolKind::Function);
        if (equalsIgnoreCase(function.name(), "Main")) {
            hasMain_ = true;
        }
        break;
    }
    case ast::AstNodeKind::SectionDeclaration:
        declareSectionSymbols(static_cast<const ast::SectionDeclaration&>(item));
        break;
    default:
        break;
    }
}

void SemanticAnalyzer::analyzeModuleItem(const ast::AstNode& item)
{
    if (item.kind() == ast::AstNodeKind::FunctionDeclaration) {
        const auto& function = static_cast<const ast::FunctionDeclaration&>(item);
        if (!function.isGeneric()) {  // a template is checked in each instance
            analyzeFunction(function);
        }
    } else if (item.kind() == ast::AstNodeKind::SectionDeclaration) {
        const auto& section = static_cast<const ast::SectionDeclaration&>(item);
        enterModule(section.module());
        validateSectionTypes(section);
        // Const and State initializers are expressions: they are analyzed like
        // any other (CANON-9 named construction, types), not skipped.
        for (const ast::VariableDeclaration& declaration : section.variables()) {
            if (declaration.value == nullptr) {
                result_.setDeclaredType(declaration, resolveWrittenType(declaration.typeName));
                continue;
            }
            const TypeRef declaredType =
                declaration.typeName.empty() ? nullptr : resolveWrittenType(declaration.typeName);
            const TypeRef valueType = analyzeExpressionFor(*declaration.value, declaredType);
            rejectFileIn(valueType, std::string(section.sectionKind() == ast::SectionKind::State
                                                    ? "a State variable, "
                                                    : "a Const, ") +
                                        declaration.name);
            if (declaration.typeName.empty() && valueType != nullptr) {
                symbols_.currentScope().inferType(declaration.name, valueType);
            }
            if (section.sectionKind() == ast::SectionKind::Const) {
                recordConstantExpression(declaration.name, *declaration.value);
            }
            if (!declaration.typeName.empty()) {
                const TypeRef declared = resolveWrittenType(declaration.typeName);
                if (valueType != nullptr && !canAssignValue(declared, valueType, *declaration.value)) {
                    throw SemanticError("initializer of " + declaration.name + " has type " +
                                        valueType->name() + ", expected " + declared->name());
                }
                result_.setDeclaredType(declaration, declared);
            } else {
                result_.setDeclaredType(declaration, valueType);
            }
        }
    }
}

void SemanticAnalyzer::declareSectionSymbols(const ast::SectionDeclaration& section)
{
    if (section.sectionKind() == ast::SectionKind::Type) {
        registerTypeSectionSymbols(section);
        return;
    }

    const SymbolKind kind = section.sectionKind() == ast::SectionKind::Const ? SymbolKind::Constant
                                                                              : SymbolKind::State;
    const bool isMutable = kind == SymbolKind::State;
    for (const ast::VariableDeclaration& declaration : section.variables()) {
        // An array type is made canonical when the section is analyzed: its
        // bounds may name a Const declared later in the module.
        const TypeRef type =
            declaration.typeName.empty() ? inferSectionDeclarationType(declaration.value.get())
            : types::isArraySpelling(declaration.typeName) ? nullptr
                                                           : lookupWrittenType(declaration.typeName);
        declareOrThrow(declaration.name, kind, type, isMutable);
        if (type == nullptr && !declaration.typeName.empty() && !types::isArraySpelling(declaration.typeName)) {
            pendingSectionTypes_.emplace_back(declaration.name, declaration.typeName);
        }
        if (declaration.value == nullptr) {
            if (kind == SymbolKind::State) {
                stateDeclarationsWithoutInitializer_.emplace_back(declaration.name, declaration.typeName);
            }
            continue;
        }
        // A Const value is recorded here only when it is a single literal or
        // name; other initializers are folded when the section is analyzed.
        if (kind == SymbolKind::Constant) {
            if (const std::string token = singleTokenText(*declaration.value); !token.empty()) {
                recordConstantValue(declaration.name, token);
            }
        }
    }
}

// The text of a one-token expression (a literal or a name), or empty.
std::string SemanticAnalyzer::singleTokenText(const ast::Expression& expression)
{
    if (expression.kind() == ast::AstNodeKind::LiteralExpression) {
        return static_cast<const ast::LiteralExpression&>(expression).value();
    }
    if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
        return static_cast<const ast::IdentifierExpression&>(expression).name();
    }
    return {};
}

// The type of `Name := Expr` known before the section is analyzed: the type of
// a literal, or of the name or routine the expression starts with. Other
// initializers get their type when the section is analyzed.
TypeRef SemanticAnalyzer::inferSectionDeclarationType(const ast::Expression* value) const
{
    while (value != nullptr && value->kind() == ast::AstNodeKind::BinaryExpression) {
        value = &static_cast<const ast::BinaryExpression*>(value)->left();
    }
    if (value != nullptr && value->kind() == ast::AstNodeKind::CallExpression) {
        value = &static_cast<const ast::CallExpression*>(value)->callee();
    }
    if (value == nullptr) {
        return nullptr;
    }
    if (value->kind() == ast::AstNodeKind::LiteralExpression) {
        switch (static_cast<const ast::LiteralExpression*>(value)->literalKind()) {
        case ast::LiteralKind::Boolean:
            return types().boolType();
        case ast::LiteralKind::String:
            return types().stringType();
        case ast::LiteralKind::Char:
            return types().charType();
        case ast::LiteralKind::Integer:
            return types().int64Type();
        case ast::LiteralKind::Float:
            return types().float64Type();
        default:
            return nullptr;
        }
    }
    if (value->kind() == ast::AstNodeKind::IdentifierExpression) {
        const Symbol* symbol =
            symbols_.currentScope().resolve(static_cast<const ast::IdentifierExpression*>(value)->name());
        return symbol != nullptr ? symbol->type : nullptr;
    }
    return nullptr;
}

void SemanticAnalyzer::registerTypeSectionSymbols(const ast::SectionDeclaration& section)
{
    for (const ast::TypeDeclaration& declaration : section.types()) {
        if (declaration.kind == ast::TypeDeclarationKind::Struct && declaration.typeParameters.empty()) {
            registerStructDeclaration(declaration);
        } else if (declaration.kind == ast::TypeDeclarationKind::Enum) {
            registerEnumDeclaration(declaration);
        } else if (declaration.kind == ast::TypeDeclarationKind::Range) {
            if (types().isDeclared(declaration.name)) {
                throw SemanticError("type already declared: " + declaration.name);
            }
            declareOrThrow(declaration.name, SymbolKind::Type);
            const auto& bounds = static_cast<const ast::BinaryExpression&>(*declaration.bounds);
            const bool ofChar = bounds.left().kind() == ast::AstNodeKind::LiteralExpression &&
                                static_cast<const ast::LiteralExpression&>(bounds.left()).literalKind() ==
                                    ast::LiteralKind::Char;
            types().declareRange(declaration.name, ofChar);
        }
    }
}

// CANON-13 Range: `TMonth Range 1..12`, its bounds static values (literals or
// Consts) with Low <= High. A Char range keeps its bounds as code points.
void SemanticAnalyzer::resolveRangeBounds(const ast::TypeDeclaration& declaration)
{
    const TypeRef rangeType = types().lookup(declaration.name);
    const auto& bounds = static_cast<const ast::BinaryExpression&>(*declaration.bounds);
    std::int64_t low = 0;
    std::int64_t high = 0;
    if (rangeType->isChar()) {
        analyzeExpression(bounds.left());
        analyzeExpression(bounds.right());
        if (!staticCharValue(bounds.left(), low) || !staticCharValue(bounds.right(), high)) {
            throw SemanticError("the bounds of Range " + declaration.name + " must be static Char values");
        }
    } else {
        const TypeRef lowType = analyzeExpression(bounds.left());
        const TypeRef highType = analyzeExpression(bounds.right());
        if (!isIntegerType(lowType) || !isIntegerType(highType) || isUInt64Type(lowType) || isUInt64Type(highType) ||
            !constantIntegerValue(bounds.left(), low) || !constantIntegerValue(bounds.right(), high)) {
            throw SemanticError("the bounds of Range " + declaration.name +
                                " must be static Integer values: literals or Consts (CANON-13)");
        }
    }
    if (low > high) {
        throw SemanticError("Range " + declaration.name + " is empty: its low bound " + std::to_string(low) +
                            " is above its high bound " + std::to_string(high) + " (CANON-13)");
    }
    types().setRangeBounds(rangeType, low, high);
}

// CANON-13 (change enum-range-set): an enum is a nominal type; each literal
// is a constant of that type whose value is its ordinal, 0..N-1. A literal
// belongs to one enum and shares the module's namespace.
void SemanticAnalyzer::registerEnumDeclaration(const ast::TypeDeclaration& declaration)
{
    if (types().isDeclared(declaration.name)) {
        throw SemanticError("type already declared: " + declaration.name);
    }
    declareOrThrow(declaration.name, SymbolKind::Type);
    const TypeRef enumType = types().declareEnum(declaration.name, declaration.literals);
    for (std::size_t ordinal = 0; ordinal < declaration.literals.size(); ++ordinal) {
        const std::string& literal = declaration.literals[ordinal];
        if (const Symbol* existing = symbols_.currentScope().resolve(literal)) {
            if (existing->kind == SymbolKind::Constant && existing->type != nullptr && existing->type->isEnum()) {
                throw SemanticError("enum literal " + literal + " is already a literal of " +
                                    existing->type->name() +
                                    ": a literal belongs to one enum (CANON-13)");
            }
        }
        declareOrThrow(literal, SymbolKind::Constant, enumType, false);
        ConstantValue value;
        value.kind = ConstantValue::Kind::Integer;
        value.integer = static_cast<std::int64_t>(ordinal);
        result_.setConstantValue(*symbols_.currentScope().resolve(literal), value);
    }
}

void SemanticAnalyzer::registerStructDeclaration(const ast::TypeDeclaration& declaration)
{
    if (types().isDeclared(declaration.name)) {
        throw SemanticError("type already declared: " + declaration.name);
    }
    declareOrThrow(declaration.name, SymbolKind::Type);
    typeModules_[normalizeName(declaration.name)] = declaration.module;
    const TypeRef structType = types().declareStruct(declaration.name);

    std::vector<types::StructField> fields;
    std::vector<std::string> written;
    // Duplicate detection by normalized (case-insensitive) name keeps a large
    // struct linear.
    std::unordered_set<std::string> seenFieldNames;
    for (const ast::StructFieldDeclaration& fieldDeclaration : declaration.fields) {
        if (!seenFieldNames.insert(normalizeName(fieldDeclaration.name)).second) {
            throw SemanticError("duplicate struct field: " + fieldDeclaration.name);
        }
        types::StructField field;
        field.name = fieldDeclaration.name;
        // A field's type is resolved once every Type is declared (it may name
        // a later struct); an array or pointer type once every Const is known
        // too (completeStructFields): its bounds may name a Const.
        field.hasDefault = fieldDeclaration.hasDefault;
        field.defaultValue = fieldDeclaration.defaultValue;
        field.fillDefault = fieldDeclaration.fillDefault;
        field.isPart = fieldDeclaration.isPart;
        field.isPublic = fieldDeclaration.isPublic || fieldDeclaration.isPart;
        fields.push_back(std::move(field));
        written.push_back(fieldDeclaration.typeName);
    }
    types().setStructFields(structType, std::move(fields));
    writtenFieldTypes_.emplace(structType, std::move(written));
}

// Once every Type of the module is declared: the types written as a type
// name (not an array or a pointer, which wait for the Consts).
void SemanticAnalyzer::resolveDeclaredTypeNames()
{
    for (const auto& [name, written] : pendingSectionTypes_) {
        symbols_.globalScope().inferType(name, types().lookup(written));
    }
    for (const auto& [structType, written] : writtenFieldTypes_) {
        for (std::size_t index = 0; index < written.size(); ++index) {
            if (!types::isArraySpelling(written[index]) && !types::isGenericSpelling(written[index])) {
                types().setStructFieldType(structType, index, types().lookup(written[index]));
            }
        }
    }
}

void SemanticAnalyzer::completeStructFields(const ast::TypeDeclaration& declaration)
{
    const TypeRef structType = resolveStruct(declaration.name);
    const std::vector<std::string>& written = writtenFieldTypes_.at(structType);
    for (std::size_t index = 0; index < written.size(); ++index) {
        if (types::isArraySpelling(written[index]) || types::isGenericSpelling(written[index])) {
            if (types::isGenericSpelling(written[index], "Slice")) {
                continue;  // rejected by validateStructDeclaration
            }
            const TypeRef type = resolveWrittenType(written[index]);
            types().setStructFieldType(structType, index, type);
        }
    }
}

void SemanticAnalyzer::validateStructDeclaration(const ast::TypeDeclaration& declaration)
{
    for (const ast::StructFieldDeclaration& field : declaration.fields) {
        if (types::isGenericSpelling(field.typeName, "Slice")) {
            throw SemanticError("a slice cannot be a struct field (ADR-0017): " + field.name);
        }
        const TypeRef type = resolveWrittenType(field.typeName);
        rejectFileIn(type, "a struct field, " + field.name);
        if (field.hasDefault && field.fillDefault) {
            validateFillDefault(field.name, type, field.defaultValue);
        } else if (field.hasDefault) {
            if (type->isArray()) {
                throw SemanticError("the default of an array field fills it, " + field.name + " " + field.typeName +
                                    " := Fill(Value) (r3.82)");
            }
            if (type->isPointer()) {
                throw SemanticError("a pointer field has no default literal: a Link and an Option are born "
                                    "empty, an Own is given in the construction (" + field.name + ")");
            }
            if (type->isArray() || type->isStruct()) {
                throw SemanticError("only a scalar struct field has a default (CANON-9): " + field.name);
            }
            validateStructFieldDefault(field.name, type, field.defaultValue);
        }
    }
    rejectRecursiveStruct(resolveStruct(declaration.name));
    validateReuse(declaration);
}

// r3.82: `FData Array[1..4] Byte := Fill(0)`, the default of an array
// field: one literal of its innermost element type, for every element.
void SemanticAnalyzer::validateFillDefault(const std::string& fieldName, TypeRef fieldType,
                                           const std::string& literal) const
{
    if (!fieldType->isArray()) {
        throw SemanticError("Fill is the default of an array field, and " + fieldName + " is " + fieldType->name());
    }
    TypeRef element = fieldType->element();
    while (element->isArray()) {
        element = element->element();
    }
    if (element->isStruct()) {
        throw SemanticError("the default of a field is a literal, and an array of " + element->name() +
                            " has none: give " + fieldName + " in the construction (r3.82)");
    }
    if (element->isOptionOwn()) {
        if (!equalsIgnoreCase(literal, "None")) {
            throw SemanticError("an array of owners is filled with None: " + fieldName + " := Fill(None)");
        }
        return;
    }
    if (element->isSet()) {
        if (literal != "[]") {
            throw SemanticError("the default of an array of sets is the empty set: " + fieldName + " := Fill([])");
        }
        return;
    }
    if (literal == "[]" || equalsIgnoreCase(literal, "None")) {
        throw SemanticError("Fill(" + literal + ") does not fit " + fieldType->name() + ": " + fieldName);
    }
    validateStructFieldDefault(fieldName, element, literal);
}

// The diagnostic of a struct declared without a value whose type defaults
// leave `field` without one (r3.82).
std::string SemanticAnalyzer::requiredFieldMessage(const std::string& name, TypeRef type, const std::string& field)
{
    return name + " " + type->name() + " has no value for " + field +
           ", which has no default (CANON-5, r3.82): build it, " + name + " := " + type->name() + "(... := ...)";
}

// r3.82 (CANON-5, CANON-9): the first field a value of `type` would get
// without a value written for it, as "TName.Field" (through nested
// structs), or empty when the type defaults are a complete value. An enum,
// a Char (char-values), an array, a type parameter, an owner and a struct
// that has such a field have no implicit value; a field with a default, a
// number, a Bool or a String (zero, False, ""), a Set (empty) and a Link or
// Option (None) have one.
std::string SemanticAnalyzer::requiredFieldWithoutDefault(TypeRef type)
{
    if (type == nullptr || !type->isStruct()) {
        return {};
    }
    for (const types::StructField& field : type->fields()) {
        if (field.hasDefault || field.type == nullptr) {
            continue;
        }
        // A Char has no implicit value either (change char-values).
        if (field.fromTypeParameter || field.type->isEnum() || field.type->isChar() || field.type->isArray() ||
            field.type->isOwn()) {
            return type->name() + "." + field.name;
        }
        if (std::string inner = requiredFieldWithoutDefault(field.type); !inner.empty()) {
            return type->name() + "." + field.name + " (" + inner + ")";
        }
    }
    return {};
}

void SemanticAnalyzer::validateStructFieldDefault(const std::string& fieldName,
                                                  TypeRef fieldType,
                                                  const std::string& defaultValue) const
{
    const bool quoted = defaultValue.size() >= 2 && defaultValue.front() == '"' && defaultValue.back() == '"';
    if (fieldType->isString() || quoted) {
        if (!fieldType->isString()) {
            throw SemanticError("a String literal cannot be the default of struct field " + fieldName +
                                " of type " + fieldType->name());
        }
        if (!quoted) {
            throw SemanticError("String struct field default must be a String literal: " + fieldName);
        }
        return;
    }
    // A Char field defaults to a Char literal, inside its Range if it has
    // one (char-values).
    const bool charQuoted = defaultValue.size() >= 2 && defaultValue.front() == '\'';
    if (fieldType->isChar() || charQuoted) {
        std::int64_t code = 0;
        if (!fieldType->isChar() || !charQuoted || !decodeCharLiteral(defaultValue, code)) {
            throw SemanticError("the default of struct field " + fieldName + " of type " + fieldType->name() +
                                " must be " + (fieldType->isChar() ? "one Char literal" : "of that type") +
                                ", got " + defaultValue);
        }
        if (code < fieldType->minValue() || code > fieldType->maxValue()) {
            throw SemanticError("struct field default out of range for " + fieldType->name() + ": " + fieldName);
        }
        return;
    }
    // An enum field defaults to one of its literals (CANON-13).
    if (fieldType->isEnum()) {
        for (const std::string& literal : fieldType->literals()) {
            if (equalsIgnoreCase(literal, defaultValue)) {
                return;
            }
        }
        throw SemanticError("the default of struct field " + fieldName + " must be a literal of " +
                            fieldType->name() + ", got " + defaultValue);
    }
    const bool negative = !defaultValue.empty() && defaultValue.front() == '-';
    const std::string_view magnitudeText = negative ? std::string_view(defaultValue).substr(1)
                                                    : std::string_view(defaultValue);
    if (fieldType->isBool()) {
        if (!equalsIgnoreCase(defaultValue, "true") && !equalsIgnoreCase(defaultValue, "false")) {
            throw SemanticError("Bool struct field default must be true or false: " + fieldName);
        }
        return;
    }
    if (fieldType->isFloat()) {
        const bool isReal = !magnitudeText.empty() &&
                            std::isdigit(static_cast<unsigned char>(magnitudeText.front())) != 0 &&
                            magnitudeText.find_first_of(".eE") != std::string_view::npos &&
                            magnitudeText.find_first_not_of("0123456789.eE+-") == std::string_view::npos;
        if (!isReal) {
            throw SemanticError("Float struct field default must be a real literal: " + fieldName);
        }
        return;
    }
    // Change decimals (r3.91): an integer or real literal, read exactly.
    if (fieldType->isDecimal()) {
        checkDecimalLiteral(magnitudeText, fieldType);
        return;
    }
    std::int64_t low = 0;
    std::int64_t high = 0;
    if (!integerTypeRange(fieldType, low, high)) {
        throw SemanticError("struct field defaults of type " + fieldType->name() +
                            " are not supported yet: " + fieldName);
    }
    std::uint64_t magnitude = 0;
    if (!parseIntegerLiteralMagnitude(magnitudeText, magnitude)) {
        throw SemanticError("integer struct field default must be an integer literal: " + fieldName);
    }
    // A UInt64 field may default to any value up to 2^64-1 (change uint64).
    const std::uint64_t highMagnitude = isUInt64Type(fieldType) ? kUInt64Max : static_cast<std::uint64_t>(high);
    const bool fits = negative ? (low < 0 && magnitude <= static_cast<std::uint64_t>(-(low + 1)) + 1U)
                               : magnitude <= highMagnitude;
    if (!fits) {
        throw SemanticError("struct field default out of range for " + fieldType->name() + ": " + fieldName);
    }
}

// CANON-9: a struct is a value, so it cannot contain itself, directly or
// through other struct fields.
void SemanticAnalyzer::rejectRecursiveStruct(TypeRef structType) const
{
    std::vector<TypeRef> path;
    std::function<void(TypeRef)> visit = [&](TypeRef type) {
        if (type == nullptr || !type->isStruct()) {
            return;
        }
        for (const TypeRef seen : path) {
            if (seen == type) {
                throw SemanticError("struct contains itself: " + structType->name());
            }
        }
        path.push_back(type);
        for (const types::StructField& field : type->fields()) {
            visit(field.type);
        }
        path.pop_back();
    };
    visit(structType);
}

void SemanticAnalyzer::validateSectionTypes(const ast::SectionDeclaration& section)
{
    if (section.sectionKind() == ast::SectionKind::Type) {
        for (const ast::TypeDeclaration& declaration : section.types()) {
            if (declaration.kind == ast::TypeDeclarationKind::Range) {
                resolveRangeBounds(declaration);
            }
        }
        for (const ast::TypeDeclaration& declaration : section.types()) {
            if (declaration.kind == ast::TypeDeclarationKind::Struct && declaration.typeParameters.empty()) {
                completeStructFields(declaration);
            }
        }
        for (const ast::TypeDeclaration& declaration : section.types()) {
            if (declaration.kind == ast::TypeDeclarationKind::Struct && declaration.typeParameters.empty()) {
                validateStructDeclaration(declaration);
            }
        }
        return;
    }

    for (const ast::VariableDeclaration& declaration : section.variables()) {
        if (declaration.typeName.empty()) {
            continue;
        }
        if (types::isGenericSpelling(declaration.typeName, "Slice")) {
            throw SemanticError("a slice cannot be a " +
                                std::string(section.sectionKind() == ast::SectionKind::State ? "State variable"
                                                                                               : "Const") +
                                " (ADR-0017): " + declaration.name);
        }
        const TypeRef type = resolveWrittenType(declaration.typeName);
        if (section.sectionKind() == ast::SectionKind::State) {
            rejectFileIn(type, "a State variable, " + declaration.name);
        }
        if (types::isArraySpelling(declaration.typeName) || types::isGenericSpelling(declaration.typeName)) {
            symbols_.currentScope().inferType(declaration.name, type);
        }
    }

    // CANON-5: "State scalars still require initializers"; like locals, only
    // structs and sets may use the type-default form `Name TType`; an array
    // is filled, `Fill(Value)` (r3.81).
    if (section.sectionKind() == ast::SectionKind::State) {
        for (const auto& [name, typeName] : stateDeclarationsWithoutInitializer_) {
            const TypeRef type = resolveWrittenType(typeName);
            // A Link and an Option<Own<T>> are born empty (safe-pointers).
            const bool bornEmpty = type->isLink() || type->isOptionOwn();
            if (type->isArray()) {
                throw SemanticError(arrayWithoutInitializer(name, typeName, type));
            }
            if (!type->isStruct() && !type->isSet() && !bornEmpty) {
                throw SemanticError(type->isOwn()
                                        ? "an owner is born with New(...): " + name
                                    : type->isEnum() ? "an enum variable needs an initializer (CANON-5): " + name + " " +
                                                           type->name() + " := " + type->literals().front()
                                                     : "scalar declaration requires initializer: " + name);
            }
            if (const std::string owner = ownerFieldWithoutDefault(type); !owner.empty()) {
                throw SemanticError(owner + " is an owner without default: give " + name + " a value");
            }
            requireBuildableHere(type);
            if (const std::string field = requiredFieldWithoutDefault(type); !field.empty()) {
                throw SemanticError(requiredFieldMessage(name, type, field));
            }
        }
    }
}

} // namespace inox::compiler::semantic
