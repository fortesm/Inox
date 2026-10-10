// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "SemanticResult.h"
#include "../ast/Ast.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace inox::compiler::semantic {

class SemanticError final : public std::runtime_error {
public:
    explicit SemanticError(std::string message);
};

struct FunctionParameter {
    std::string name;
    std::string typeName;
};

struct FunctionSignature {
    std::string name;
    std::vector<FunctionParameter> parameters;
    std::string returnType;
};

struct StructField {
    std::string name;
    std::string typeName;
    bool hasDefault = false;
    std::string defaultValue;
};

struct StructType {
    std::string name;
    std::vector<StructField> fields;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();

    const SemanticResult& analyze(const ast::ModuleNode& module);
    const SemanticResult& result() const;

private:
    void declareBuiltins();
    void declareBuiltinTypes();
    void declareOrThrow(std::string_view name, SymbolKind kind, std::string typeName = {}, bool isMutable = false);
    const Symbol& resolveOrThrow(std::string_view name) const;
    void declareTypeOrThrow(std::string_view name, bool isBuiltin, std::string aliasOf = {});
    void resolveTypeOrThrow(std::string_view name) const;
    std::string canonicalTypeName(std::string_view name) const;

    void declareModuleItem(const ast::AstNode& item);
    void registerFunctionSignatures(const ast::ModuleNode& module);
    void registerFunctionSignature(const ast::FunctionDeclaration& function);
    const FunctionSignature* resolveFunctionSignature(std::string_view name) const;
    void analyzeModuleItem(const ast::AstNode& item);
    void declareSectionSymbols(const ast::SectionDeclaration& section);
    void recordConstantValue(std::string_view name, std::string_view valueToken);
    static void rejectInvalidConstantRightOperand(ast::BinaryOperator op, std::int64_t value);
    void registerTypeSectionSymbols(const ast::SectionDeclaration& section);
    void registerStructDeclaration(const std::vector<std::string>& tokens, std::size_t& index);
    void validateSectionTypes(const ast::SectionDeclaration& section) const;
    void validateStructDeclaration(const std::vector<std::string>& tokens, std::size_t& index) const;
    void analyzeFunction(const ast::FunctionDeclaration& function);

    void analyzeStatements(const std::vector<ast::StatementPtr>& statements, bool createScope);
    // CANON-12: "Functions must not fall through without returning a value."
    // Conservative: true only when control provably cannot reach the end.
    static bool cannotFallThrough(const std::vector<ast::StatementPtr>& statements);
    static bool cannotFallThrough(const ast::Statement& statement);
    void analyzeStatement(const ast::Statement& statement);
    void analyzeCaseStatement(const ast::CaseStatement& statement);
    void trackLocal(std::string_view name);
    void leaveScope();
    std::int64_t caseChoiceValue(const ast::Expression& choice, const std::string& selectorType);
    bool staticBoolValue(const ast::Expression& expression, bool& value) const;
    bool staticCharValue(const ast::Expression& expression, std::int64_t& value) const;
    const Symbol* resolveConstant(const ast::Expression& expression) const;
    void recordConstantExpression(const std::string& name, const ast::Expression& value);
    void analyzeVarBlock(const ast::VarBlockStatement& statement);
    std::string analyzeExpression(const ast::Expression& expression);
    std::string inferExpressionType(const ast::Expression& expression);
    std::string analyzeCallExpression(const ast::CallExpression& expression);
    std::string analyzeMemberExpression(const ast::CallExpression& expression);
    std::string analyzeMethodCallExpression(const ast::CallExpression& expression);
    std::string analyzeBinaryExpression(const ast::BinaryExpression& expression);
    std::string analyzeUnaryExpression(const ast::UnaryExpression& expression);
    std::string analyzePreludeCall(std::string_view name,
                                   const std::vector<std::string>& argumentTypes) const;
    std::string analyzeInputCall(std::string_view name,
                                 const std::vector<ast::ExpressionPtr>& arguments);
    std::string analyzeUserFunctionCall(const FunctionSignature& signature,
                                        const std::vector<ast::ExpressionPtr>& arguments);
    void requireBoolCondition(const ast::Expression& expression);
    std::string inferSectionDeclarationType(const std::vector<std::string>& tokens,
                                            std::size_t nameIndex) const;
    const StructType* resolveStruct(std::string_view name) const;
    const StructField* resolveStructField(std::string_view structName, std::string_view fieldName) const;

    static bool looksLikeIdentifier(std::string_view text);
    static bool isInternalSyntheticName(std::string_view name);
    static bool isNumericType(std::string_view typeName);
    static bool isIntegerType(std::string_view typeName);
    static bool isPreludeCall(std::string_view name);
    static bool isExceptionType(std::string_view name);
    static bool isMemberCall(const ast::CallExpression& expression);
    static std::string normalizeName(std::string_view name);
    static bool canAssign(std::string_view targetType, std::string_view valueType);
    static bool typesMatch(std::string_view left, std::string_view right);
    bool canAssignValue(std::string_view targetType, std::string_view valueType,
                        const ast::Expression& value);

    ResolvedType resolvedType(std::string typeName) const;

    // CANON-19: constant integer expressions are evaluated at compile time and
    // any overflow, division by zero or out-of-range shift is a compile error.
    void foldConstantExpression(const ast::Expression& expression);
    bool constantIntegerValue(const ast::Expression& expression, std::int64_t& value) const;
    void requireStatementExpression(const ast::Expression& expression);

    SymbolTable symbols_;
    TypeTable types_;
    SemanticResult result_;
    std::unordered_map<std::string, FunctionSignature> functions_;
    std::unordered_map<std::string, StructType> structs_;
    // State declarations written `Name Type` without ":=", checked once all
    // types are known: only structs may omit the initializer (CANON-5).
    std::vector<std::pair<std::string, std::string>> stateDeclarationsWithoutInitializer_;
    std::string currentFunctionReturnType_;
    bool currentFunctionSawReturn_ = false;
    std::size_t loopDepth_ = 0;
    std::size_t repeatDepth_ = 0;
    std::size_t exceptionHandlerDepth_ = 0;
    std::size_t retryHandlerDepth_ = 0;
    std::size_t ensureDepth_ = 0;
    bool hasMain_ = false;
    std::unordered_map<const ast::Expression*, std::int64_t> constants_;
    // Char Consts (`Const Letter := 'A'`) by normalized name, as Unicode scalar
    // values; used for static case choices (ADR-0011).
    std::unordered_map<std::string, std::int64_t> charConstants_;
    // OPEN-4: locals declared in open scopes, in declaration order, and the
    // symbols read so far.
    struct TrackedLocal {
        const Symbol* symbol;
        const Scope* scope;
    };
    std::vector<TrackedLocal> localsToRead_;
    std::unordered_set<const Symbol*> readSymbols_;
    // ADR-0012: the copy of the target inside a desugared `X += R`; analyzing
    // it is not a read of X.
    std::unordered_set<const ast::IdentifierExpression*> updateOnlyReads_;
};

} // namespace inox::compiler::semantic
