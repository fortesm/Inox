#pragma once

#include "SemanticResult.h"
#include "../ast/Ast.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
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
    void declareOrThrow(std::string_view name, SymbolKind kind, std::string typeName = {});
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
    void registerTypeSectionSymbols(const ast::SectionDeclaration& section);
    void registerStructDeclaration(const std::vector<std::string>& tokens, std::size_t& index);
    void validateSectionTypes(const ast::SectionDeclaration& section) const;
    void validateStructDeclaration(const std::vector<std::string>& tokens, std::size_t& index) const;
    void analyzeFunction(const ast::FunctionDeclaration& function);

    void analyzeStatements(const std::vector<ast::StatementPtr>& statements, bool createScope);
    void analyzeStatement(const ast::Statement& statement);
    void analyzeVarBlock(const ast::VarBlockStatement& statement);
    std::string analyzeExpression(const ast::Expression& expression);
    std::string inferExpressionType(const ast::Expression& expression);
    std::string analyzeCallExpression(const ast::CallExpression& expression);
    std::string analyzeMemberExpression(const ast::CallExpression& expression);
    std::string analyzeBinaryExpression(const ast::BinaryExpression& expression);
    std::string analyzeUnaryExpression(const ast::UnaryExpression& expression);
    std::string analyzePreludeCall(std::string_view name,
                                   const std::vector<std::string>& argumentTypes) const;
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
    static bool isMemberCall(const ast::CallExpression& expression);
    static std::string normalizeName(std::string_view name);
    static bool canAssign(std::string_view targetType, std::string_view valueType);
    static bool typesMatch(std::string_view left, std::string_view right);

    ResolvedType resolvedType(std::string typeName) const;

    SymbolTable symbols_;
    TypeTable types_;
    SemanticResult result_;
    std::unordered_map<std::string, FunctionSignature> functions_;
    std::unordered_map<std::string, StructType> structs_;
    std::string currentFunctionReturnType_;
    bool currentFunctionSawReturn_ = false;
    std::size_t loopDepth_ = 0;
    std::size_t repeatDepth_ = 0;
    bool hasMain_ = false;
};

} // namespace inox::compiler::semantic
