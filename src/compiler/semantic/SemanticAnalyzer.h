#pragma once

#include "Symbol.h"
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

class SemanticAnalyzer {
public:
    SemanticAnalyzer();

    void analyze(const ast::ModuleNode& module);

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
    void validateSectionTypes(const ast::SectionDeclaration& section) const;
    void analyzeFunction(const ast::FunctionDeclaration& function);

    void analyzeStatements(const std::vector<ast::StatementPtr>& statements, bool createScope);
    void analyzeStatement(const ast::Statement& statement);
    void analyzeVarBlock(const ast::VarBlockStatement& statement);
    std::string analyzeExpression(const ast::Expression& expression);
    std::string analyzeBinaryExpression(const ast::BinaryExpression& expression);
    std::string analyzeUnaryExpression(const ast::UnaryExpression& expression);
    std::string analyzePreludeCall(std::string_view name,
                                   const std::vector<std::string>& argumentTypes) const;
    std::string analyzeUserFunctionCall(const FunctionSignature& signature,
                                        const std::vector<ast::ExpressionPtr>& arguments);
    void requireBoolCondition(const ast::Expression& expression);
    std::string inferSectionDeclarationType(const std::vector<std::string>& tokens,
                                            std::size_t nameIndex) const;

    static bool looksLikeIdentifier(std::string_view text);
    static bool isInternalSyntheticName(std::string_view name);
    static bool isNumericType(std::string_view typeName);
    static bool isIntegerType(std::string_view typeName);
    static bool isPreludeCall(std::string_view name);
    static std::string normalizeName(std::string_view name);
    static bool canAssign(std::string_view targetType, std::string_view valueType);
    static bool typesMatch(std::string_view left, std::string_view right);

    SymbolTable symbols_;
    TypeTable types_;
    std::unordered_map<std::string, FunctionSignature> functions_;
    std::string currentFunctionReturnType_;
    bool currentFunctionSawReturn_ = false;
    std::size_t loopDepth_ = 0;
    bool hasMain_ = false;
};

} // namespace inox::compiler::semantic
