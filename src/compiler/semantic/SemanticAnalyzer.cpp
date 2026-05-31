#include "SemanticAnalyzer.h"

#include <array>
#include <cstddef>
#include <cctype>
#include <utility>

namespace inox::compiler::semantic {

namespace {

bool equalsIgnoreCase(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t index = 0; index < left.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(left[index])) !=
            std::tolower(static_cast<unsigned char>(right[index]))) {
            return false;
        }
    }

    return true;
}

std::string_view binaryOperatorName(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::Assign:
        return ":=";
    case ast::BinaryOperator::Add:
        return "+";
    case ast::BinaryOperator::Subtract:
        return "-";
    case ast::BinaryOperator::Multiply:
        return "*";
    case ast::BinaryOperator::Divide:
        return "/";
    case ast::BinaryOperator::IntegerDivide:
        return "div";
    case ast::BinaryOperator::Modulo:
        return "mod";
    case ast::BinaryOperator::ShiftLeft:
        return "shl";
    case ast::BinaryOperator::ShiftRight:
        return "shr";
    case ast::BinaryOperator::BitAnd:
        return "bitand";
    case ast::BinaryOperator::BitXor:
        return "bitxor";
    case ast::BinaryOperator::BitOr:
        return "bitor";
    case ast::BinaryOperator::Power:
        return "^";
    case ast::BinaryOperator::Range:
        return "..";
    case ast::BinaryOperator::In:
        return "in";
    case ast::BinaryOperator::Equal:
        return "=";
    case ast::BinaryOperator::NotEqual:
        return "#";
    case ast::BinaryOperator::Less:
        return "<";
    case ast::BinaryOperator::Greater:
        return ">";
    case ast::BinaryOperator::LessEqual:
        return "<=";
    case ast::BinaryOperator::GreaterEqual:
        return ">=";
    case ast::BinaryOperator::And:
        return "and";
    case ast::BinaryOperator::Xor:
        return "xor";
    case ast::BinaryOperator::Or:
        return "or";
    }
    return "?";
}

struct PreludeSignature {
    std::string_view name;
    std::vector<std::string_view> parameterTypes;
    std::string_view returnType;
};

const std::array<PreludeSignature, 10>& preludeSignatures()
{
    static const std::array<PreludeSignature, 10> signatures = {{
        {"Put", {"*"}, "Void"},
        {"PutLn", {"*"}, "Void"},
        {"ReadLn", {}, "String"},
        {"Length", {"String"}, "Int64"},
        {"Ord", {"Char"}, "Int64"},
        {"Sin", {"Float64"}, "Float64"},
        {"Cos", {"Float64"}, "Float64"},
        {"Sqrt", {"Float64"}, "Float64"},
        {"Abs", {"Int64"}, "Int64"},
        {"Abs", {"Float64"}, "Float64"}
    }};
    return signatures;
}

} // namespace

SemanticError::SemanticError(std::string message)
    : std::runtime_error(std::move(message))
{
}

SemanticAnalyzer::SemanticAnalyzer()
{
    declareBuiltins();
    declareBuiltinTypes();
}

const SemanticResult& SemanticAnalyzer::analyze(const ast::ModuleNode& module)
{
    result_.clear();

    if (module.name().empty()) {
        throw SemanticError("module must have a name");
    }

    declareOrThrow(module.name(), SymbolKind::Module);

    for (const auto& item : module.items()) {
        declareModuleItem(*item);
    }

    registerFunctionSignatures(module);

    if (!hasMain_) {
        throw SemanticError("module must declare Main()");
    }

    for (const auto& item : module.items()) {
        analyzeModuleItem(*item);
    }

    return result_;
}

const SemanticResult& SemanticAnalyzer::result() const
{
    return result_;
}

void SemanticAnalyzer::registerFunctionSignatures(const ast::ModuleNode& module)
{
    for (const auto& item : module.items()) {
        if (item->kind() == ast::AstNodeKind::FunctionDeclaration) {
            registerFunctionSignature(static_cast<const ast::FunctionDeclaration&>(*item));
        }
    }
}

void SemanticAnalyzer::registerFunctionSignature(const ast::FunctionDeclaration& function)
{
    const auto& tokens = function.signatureTokens();
    if (tokens.size() < 2 || tokens.front() != "(") {
        throw SemanticError("expected parameter list for function: " + function.name());
    }

    FunctionSignature signature;
    signature.name = function.name();

    std::size_t index = 1;
    while (index < tokens.size() && tokens[index] != ")") {
        if (!looksLikeIdentifier(tokens[index])) {
            throw SemanticError("expected parameter name in function: " + function.name());
        }
        const std::string parameterName = tokens[index++];

        if (index >= tokens.size() || !looksLikeIdentifier(tokens[index])) {
            throw SemanticError("expected type for parameter: " + parameterName);
        }
        resolveTypeOrThrow(tokens[index]);
        const std::string parameterType = canonicalTypeName(tokens[index++]);

        for (const FunctionParameter& parameter : signature.parameters) {
            if (equalsIgnoreCase(parameter.name, parameterName)) {
                throw SemanticError("duplicate parameter: " + parameterName);
            }
        }
        signature.parameters.push_back(FunctionParameter{parameterName, parameterType});

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

    if (index < tokens.size()) {
        if (index + 1 != tokens.size() || !looksLikeIdentifier(tokens[index])) {
            throw SemanticError("invalid return type in function: " + function.name());
        }
        resolveTypeOrThrow(tokens[index]);
        signature.returnType = canonicalTypeName(tokens[index]);
    }

    functions_.emplace(normalizeName(function.name()), std::move(signature));
}

const FunctionSignature* SemanticAnalyzer::resolveFunctionSignature(std::string_view name) const
{
    const auto iterator = functions_.find(normalizeName(name));
    return iterator != functions_.end() ? &iterator->second : nullptr;
}

void SemanticAnalyzer::declareBuiltins()
{
    constexpr std::array<std::string_view, 22> valueBuiltins = {
        "Put", "PutLn", "ReadLn",
        "Sin", "Cos", "Sqrt", "Abs",
        "Length", "Ord",
        "True", "False",
        "RangeError", "IndexError", "DivisionByZero", "OverflowError", "IOError",
        "__index", "__member",
        "Sys", "IO", "Math", "Std"
    };

    for (std::string_view name : valueBuiltins) {
        symbols_.globalScope().declare(std::string(name), SymbolKind::Builtin);
    }
}

void SemanticAnalyzer::declareBuiltinTypes()
{
    constexpr std::array<std::string_view, 16> builtinTypes = {
        "Bool",
        "Int8", "Int16", "Int32", "Int64",
        "UInt8", "UInt16", "UInt32", "UInt64",
        "Natural",
        "Float32", "Float64",
        "Currency", "Crypto", "Char", "String"
    };

    for (std::string_view name : builtinTypes) {
        declareTypeOrThrow(name, true);
    }

    declareTypeOrThrow("Integer", true, "Int64");
    declareTypeOrThrow("UInteger", true, "UInt64");
    declareTypeOrThrow("Float", true, "Float64");
}

void SemanticAnalyzer::declareOrThrow(std::string_view name, SymbolKind kind, std::string typeName)
{
    Scope& scope = symbols_.currentScope();
    if (scope.containsLocal(name)) {
        throw SemanticError("duplicate symbol in same scope: " + std::string(name));
    }
    if (scope.containsInAncestors(name)) {
        throw SemanticError("shadowing is forbidden: " + std::string(name));
    }
    scope.declare(std::string(name), kind, std::move(typeName));
}

void SemanticAnalyzer::declareTypeOrThrow(std::string_view name, bool isBuiltin, std::string aliasOf)
{
    if (types_.contains(name)) {
        throw SemanticError("type already declared: " + std::string(name));
    }
    declareOrThrow(name, SymbolKind::Type);
    types_.declare(std::string(name), isBuiltin, std::move(aliasOf));
}

void SemanticAnalyzer::resolveTypeOrThrow(std::string_view name) const
{
    if (types_.resolve(name) == nullptr) {
        throw SemanticError("unknown type: " + std::string(name));
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
        throw SemanticError("unknown symbol: " + std::string(name));
    }
    return *symbol;
}

std::string SemanticAnalyzer::canonicalTypeName(std::string_view name) const
{
    const TypeSymbol* type = types_.resolve(name);
    if (type == nullptr) {
        return std::string(name);
    }
    if (!type->aliasOf.empty()) {
        return canonicalTypeName(type->aliasOf);
    }
    return type->name;
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
        analyzeFunction(static_cast<const ast::FunctionDeclaration&>(item));
    } else if (item.kind() == ast::AstNodeKind::SectionDeclaration) {
        validateSectionTypes(static_cast<const ast::SectionDeclaration&>(item));
    }
}

void SemanticAnalyzer::declareSectionSymbols(const ast::SectionDeclaration& section)
{
    if (section.sectionKind() == ast::SectionKind::Type) {
        registerTypeSectionSymbols(section);
        return;
    }

    SymbolKind kind = SymbolKind::Variable;
    if (section.sectionKind() == ast::SectionKind::Const) {
        kind = SymbolKind::Constant;
    } else if (section.sectionKind() == ast::SectionKind::State) {
        kind = SymbolKind::State;
    }

    const auto& tokens = section.tokens();
    for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
        const bool isInitializerDeclaration =
            tokens[index + 1] == ":=" && !(index > 0 && tokens[index - 1] == ":");
        const bool isTypedDeclaration = tokens[index + 1] == ":";

        if (looksLikeIdentifier(tokens[index]) &&
            (isInitializerDeclaration || isTypedDeclaration)) {
            declareOrThrow(tokens[index], kind, inferSectionDeclarationType(tokens, index));
        }
    }
}

std::string SemanticAnalyzer::inferSectionDeclarationType(
    const std::vector<std::string>& tokens,
    std::size_t nameIndex) const
{
    if (nameIndex + 2 < tokens.size() && tokens[nameIndex + 1] == ":") {
        return canonicalTypeName(tokens[nameIndex + 2]);
    }

    if (nameIndex + 2 >= tokens.size() || tokens[nameIndex + 1] != ":=") {
        return {};
    }

    const std::string_view value = tokens[nameIndex + 2];
    if (value == "True" || value == "False" || value == "true" || value == "false") {
        return "Bool";
    }
    if (!value.empty() && value.front() == '"') {
        return "String";
    }
    if (!value.empty() && value.front() == '\'') {
        return "Char";
    }
    if (!value.empty() && (std::isdigit(static_cast<unsigned char>(value.front())) ||
                           value.front() == '$')) {
        return value.find('.') == std::string_view::npos ? "Int64" : "Float64";
    }

    const Symbol* symbol = symbols_.currentScope().resolve(value);
    return symbol != nullptr ? symbol->typeName : std::string{};
}

void SemanticAnalyzer::registerTypeSectionSymbols(const ast::SectionDeclaration& section)
{
    const auto& tokens = section.tokens();
    for (std::size_t index = 0; index < tokens.size();) {
        if (index + 1 < tokens.size() && looksLikeIdentifier(tokens[index]) &&
            equalsIgnoreCase(tokens[index + 1], "Struct")) {
            registerStructDeclaration(tokens, index);
            continue;
        }

        if (index + 1 < tokens.size() && looksLikeIdentifier(tokens[index]) &&
            tokens[index + 1] == ":=") {
            declareTypeOrThrow(tokens[index], false);
            index += 2;
            continue;
        }

        ++index;
    }
}

void SemanticAnalyzer::registerStructDeclaration(const std::vector<std::string>& tokens, std::size_t& index)
{
    const std::string typeName = tokens[index];
    declareTypeOrThrow(typeName, false);

    StructType structType;
    structType.name = typeName;
    index += 2;

    while (index < tokens.size() && tokens[index] != ";") {
        if (index + 1 >= tokens.size() || !looksLikeIdentifier(tokens[index]) ||
            !looksLikeIdentifier(tokens[index + 1])) {
            throw SemanticError("invalid struct field declaration in: " + typeName);
        }

        const std::string fieldName = tokens[index++];
        const std::string fieldType = tokens[index++];
        for (const StructField& field : structType.fields) {
            if (equalsIgnoreCase(field.name, fieldName)) {
                throw SemanticError("duplicate struct field: " + fieldName);
            }
        }
        structType.fields.push_back(StructField{fieldName, canonicalTypeName(fieldType)});
    }

    if (index >= tokens.size() || tokens[index] != ";") {
        throw SemanticError("expected ';' to close Struct: " + typeName);
    }
    ++index;

    structs_.emplace(normalizeName(typeName), std::move(structType));
}


void SemanticAnalyzer::validateStructDeclaration(const std::vector<std::string>& tokens, std::size_t& index) const
{
    const std::string typeName = tokens[index];
    index += 2;

    while (index < tokens.size() && tokens[index] != ";") {
        if (index + 1 >= tokens.size() || !looksLikeIdentifier(tokens[index]) ||
            !looksLikeIdentifier(tokens[index + 1])) {
            throw SemanticError("invalid struct field declaration in: " + typeName);
        }
        index += 1;
        resolveTypeOrThrow(tokens[index]);
        index += 1;
    }

    if (index >= tokens.size() || tokens[index] != ";") {
        throw SemanticError("expected ';' to close Struct: " + typeName);
    }
    ++index;
}

void SemanticAnalyzer::validateSectionTypes(const ast::SectionDeclaration& section) const
{
    const auto& tokens = section.tokens();

    if (section.sectionKind() == ast::SectionKind::Type) {
        for (std::size_t index = 0; index < tokens.size();) {
            if (index + 1 < tokens.size() && looksLikeIdentifier(tokens[index]) &&
                equalsIgnoreCase(tokens[index + 1], "Struct")) {
                validateStructDeclaration(tokens, index);
                continue;
            }

            if (index + 2 < tokens.size() && looksLikeIdentifier(tokens[index]) &&
                tokens[index + 1] == ":=") {
                const std::string_view head = tokens[index + 2];
                if (equalsIgnoreCase(head, "Struct") || equalsIgnoreCase(head, "Enum")) {
                    index += 3;
                    continue;
                }
                if (equalsIgnoreCase(head, "Set") || equalsIgnoreCase(head, "Vector")) {
                    if (index + 4 < tokens.size() && tokens[index + 3] == "[") {
                        resolveTypeOrThrow(tokens[index + 4]);
                    }
                    index += 3;
                    continue;
                }
                if (equalsIgnoreCase(head, "Array")) {
                    for (std::size_t typeIndex = index + 3; typeIndex < tokens.size(); ++typeIndex) {
                        if (tokens[typeIndex] == "]" && typeIndex + 1 < tokens.size() &&
                            looksLikeIdentifier(tokens[typeIndex + 1])) {
                            resolveTypeOrThrow(tokens[typeIndex + 1]);
                            break;
                        }
                    }
                    index += 3;
                    continue;
                }
                if (looksLikeIdentifier(head)) {
                    resolveTypeOrThrow(head);
                }
                index += 3;
                continue;
            }

            ++index;
        }
        return;
    }

    if (section.sectionKind() != ast::SectionKind::Var &&
        section.sectionKind() != ast::SectionKind::Const &&
        section.sectionKind() != ast::SectionKind::State) {
        return;
    }

    for (std::size_t index = 0; index + 2 < tokens.size(); ++index) {
        if (looksLikeIdentifier(tokens[index]) && tokens[index + 1] == ":" &&
            looksLikeIdentifier(tokens[index + 2])) {
            resolveTypeOrThrow(tokens[index + 2]);
        }
    }
}

void SemanticAnalyzer::analyzeFunction(const ast::FunctionDeclaration& function)
{
    const std::string previousReturnType = std::move(currentFunctionReturnType_);
    const bool previousSawReturn = currentFunctionSawReturn_;
    const FunctionSignature* signature = resolveFunctionSignature(function.name());
    currentFunctionReturnType_ = signature != nullptr ? signature->returnType : std::string{};
    currentFunctionSawReturn_ = false;
    symbols_.pushScope();
    if (signature != nullptr) {
        for (const FunctionParameter& parameter : signature->parameters) {
            declareOrThrow(parameter.name, SymbolKind::Variable, parameter.typeName);
        }
    }
    analyzeStatements(function.body(), false);
    symbols_.popScope();
    if (!currentFunctionReturnType_.empty() && !currentFunctionSawReturn_) {
        throw SemanticError("function " + function.name() + " must return a value");
    }
    currentFunctionReturnType_ = previousReturnType;
    currentFunctionSawReturn_ = previousSawReturn;
}

void SemanticAnalyzer::analyzeStatements(const std::vector<ast::StatementPtr>& statements, bool createScope)
{
    if (createScope) {
        symbols_.pushScope();
    }

    for (const auto& statement : statements) {
        analyzeStatement(*statement);
    }

    if (createScope) {
        symbols_.popScope();
    }
}

void SemanticAnalyzer::analyzeStatement(const ast::Statement& statement)
{
    switch (statement.kind()) {
    case ast::AstNodeKind::BlockStatement: {
        const auto& block = static_cast<const ast::BlockStatement&>(statement);
        analyzeStatements(block.statements(), true);
        break;
    }
    case ast::AstNodeKind::ExpressionStatement:
        analyzeExpression(static_cast<const ast::ExpressionStatement&>(statement).expression());
        break;
    case ast::AstNodeKind::VarStatement: {
        const auto& var = static_cast<const ast::VarStatement&>(statement);
        std::string typeName = var.typeName().empty() ? std::string{} : canonicalTypeName(var.typeName());
        if (!var.typeName().empty()) {
            resolveTypeOrThrow(var.typeName());
        }
        if (var.initializer() != nullptr) {
            const std::string initializerType = analyzeExpression(*var.initializer());
            if (typeName.empty()) {
                typeName = initializerType;
            } else if (!canAssign(typeName, initializerType)) {
                throw SemanticError("cannot initialize " + typeName + " with " + initializerType);
            }
        }
        if (typeName.empty()) {
            throw SemanticError("variable requires a type or initializer: " + var.name());
        }
        declareOrThrow(var.name(), SymbolKind::Variable, std::move(typeName));
        break;
    }
    case ast::AstNodeKind::VarBlockStatement:
        analyzeVarBlock(static_cast<const ast::VarBlockStatement&>(statement));
        break;
    case ast::AstNodeKind::IfStatement: {
        const auto& ifStatement = static_cast<const ast::IfStatement&>(statement);
        requireBoolCondition(ifStatement.condition());
        analyzeStatements(ifStatement.thenBody(), true);
        for (const auto& clause : ifStatement.elseIfClauses()) {
            requireBoolCondition(*clause.condition);
            analyzeStatements(clause.body, true);
        }
        analyzeStatements(ifStatement.elseBody(), true);
        break;
    }
    case ast::AstNodeKind::UnlessStatement: {
        const auto& unlessStatement = static_cast<const ast::UnlessStatement&>(statement);
        requireBoolCondition(unlessStatement.condition());
        analyzeStatements(unlessStatement.body(), true);
        break;
    }
    case ast::AstNodeKind::WhileStatement: {
        const auto& whileStatement = static_cast<const ast::WhileStatement&>(statement);
        requireBoolCondition(whileStatement.condition());
        ++loopDepth_;
        analyzeStatements(whileStatement.body(), true);
        --loopDepth_;
        break;
    }
    case ast::AstNodeKind::RepeatStatement: {
        const auto& repeatStatement = static_cast<const ast::RepeatStatement&>(statement);
        ++loopDepth_;
        ++repeatDepth_;
        analyzeStatements(repeatStatement.body(), true);
        --repeatDepth_;
        --loopDepth_;
        break;
    }
    case ast::AstNodeKind::UntilStatement: {
        const auto& untilStatement = static_cast<const ast::UntilStatement&>(statement);
        if (repeatDepth_ == 0) {
            throw SemanticError("until outside repeat");
        }
        requireBoolCondition(untilStatement.condition());
        break;
    }
    case ast::AstNodeKind::ForInStatement: {
        const auto& forStatement = static_cast<const ast::ForInStatement&>(statement);
        analyzeExpression(forStatement.iterable());
        if (forStatement.step() != nullptr) {
            analyzeExpression(*forStatement.step());
        }
        symbols_.pushScope();
        declareOrThrow(forStatement.iterator(), SymbolKind::Variable, "Int64");
        ++loopDepth_;
        analyzeStatements(forStatement.body(), false);
        --loopDepth_;
        symbols_.popScope();
        break;
    }
    case ast::AstNodeKind::CaseStatement: {
        const auto& caseStatement = static_cast<const ast::CaseStatement&>(statement);
        analyzeExpression(caseStatement.expression());
        for (const auto& arm : caseStatement.arms()) {
            for (const auto& choice : arm.choices) {
                analyzeExpression(*choice);
            }
            analyzeStatements(arm.body, true);
        }
        analyzeStatements(caseStatement.otherwiseBody(), true);
        break;
    }
    case ast::AstNodeKind::TryStatement: {
        const auto& tryStatement = static_cast<const ast::TryStatement&>(statement);
        analyzeStatements(tryStatement.body(), true);
        analyzeStatements(tryStatement.exceptBody(), true);
        analyzeStatements(tryStatement.finallyBody(), true);
        break;
    }
    case ast::AstNodeKind::RaiseStatement: {
        const auto& raiseStatement = static_cast<const ast::RaiseStatement&>(statement);
        if (raiseStatement.expression() != nullptr) {
            analyzeExpression(*raiseStatement.expression());
        }
        break;
    }
    case ast::AstNodeKind::ReturnStatement: {
        currentFunctionSawReturn_ = true;
        const std::string typeName =
            analyzeExpression(static_cast<const ast::ReturnStatement&>(statement).expression());
        if (currentFunctionReturnType_.empty()) {
            throw SemanticError("subroutine without return type cannot return a value");
        }
        if (!canAssign(currentFunctionReturnType_, typeName)) {
            throw SemanticError(
                "cannot return " + typeName + " from function returning " +
                currentFunctionReturnType_);
        }
        break;
    }
    case ast::AstNodeKind::BreakStatement:
        if (loopDepth_ == 0) {
            throw SemanticError("break outside loop");
        }
        break;
    case ast::AstNodeKind::ContinueStatement:
        if (loopDepth_ == 0) {
            throw SemanticError("continue outside loop");
        }
        break;
    default:
        break;
    }
}

void SemanticAnalyzer::analyzeVarBlock(const ast::VarBlockStatement& statement)
{
    for (const auto& declaration : statement.declarations()) {
        if (declaration->kind() == ast::AstNodeKind::VarStatement) {
            analyzeStatement(*declaration);
            continue;
        }

        if (declaration->kind() == ast::AstNodeKind::ExpressionStatement) {
            const auto& expressionStatement = static_cast<const ast::ExpressionStatement&>(*declaration);
            const auto& expression = expressionStatement.expression();
            if (expression.kind() == ast::AstNodeKind::BinaryExpression) {
                const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
                if (binary.op() == ast::BinaryOperator::Assign &&
                    binary.left().kind() == ast::AstNodeKind::IdentifierExpression) {
                    const auto& identifier = static_cast<const ast::IdentifierExpression&>(binary.left());
                    const std::string typeName = analyzeExpression(binary.right());
                    declareOrThrow(identifier.name(), SymbolKind::Variable, typeName);
                    const Symbol& symbol = resolveOrThrow(identifier.name());
                    result_.bind(identifier, symbol);
                    result_.setExpressionType(identifier, resolvedType(typeName));
                    result_.setExpressionType(binary, resolvedType(typeName));
                    continue;
                }
            }
        }

        analyzeStatement(*declaration);
    }
}

std::string SemanticAnalyzer::analyzeExpression(const ast::Expression& expression)
{
    std::string typeName = inferExpressionType(expression);
    result_.setExpressionType(expression, resolvedType(typeName));
    return typeName;
}

std::string SemanticAnalyzer::inferExpressionType(const ast::Expression& expression)
{
    switch (expression.kind()) {
    case ast::AstNodeKind::LiteralExpression: {
        const auto literalKind = static_cast<const ast::LiteralExpression&>(expression).literalKind();
        switch (literalKind) {
        case ast::LiteralKind::Integer:
            return "Int64";
        case ast::LiteralKind::Float:
            return "Float64";
        case ast::LiteralKind::String:
            return "String";
        case ast::LiteralKind::Char:
            return "Char";
        case ast::LiteralKind::Boolean:
            return "Bool";
        }
        break;
    }
    case ast::AstNodeKind::IdentifierExpression: {
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
        const Symbol& symbol = resolveOrThrow(identifier.name());
        result_.bind(identifier, symbol);
        return symbol.typeName;
    }
    case ast::AstNodeKind::BinaryExpression:
        return analyzeBinaryExpression(static_cast<const ast::BinaryExpression&>(expression));
    case ast::AstNodeKind::UnaryExpression:
        return analyzeUnaryExpression(static_cast<const ast::UnaryExpression&>(expression));
    case ast::AstNodeKind::CallExpression:
        return analyzeCallExpression(static_cast<const ast::CallExpression&>(expression));
    default:
        break;
    }

    return {};
}

std::string SemanticAnalyzer::analyzeCallExpression(const ast::CallExpression& call)
{
    if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
        const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
        if (equalsIgnoreCase(callee.name(), "__member")) {
            return analyzeMemberExpression(call);
        }
        if (const TypeSymbol* type = types_.resolve(callee.name())) {
            const Symbol& symbol = resolveOrThrow(callee.name());
            result_.bind(callee, symbol);
            result_.bind(call, symbol);
            result_.setExpressionType(callee, resolvedType(canonicalTypeName(type->name)));
            for (const auto& argument : call.arguments()) {
                analyzeExpression(*argument);
            }
            return canonicalTypeName(type->name);
        }

        if (isPreludeCall(callee.name())) {
            const Symbol& symbol = resolveOrThrow(callee.name());
            result_.bind(callee, symbol);
            result_.bind(call, symbol);
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
        result_.bind(call, resolveOrThrow(callee.name()));
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
    if (field == nullptr) {
        throw SemanticError("unknown field " + fieldIdentifier.name() + " in " + baseType);
    }

    result_.setExpressionType(fieldIdentifier, resolvedType(field->typeName));
    return field->typeName;
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
        if (!canAssign(parameter.typeName, argumentType)) {
            throw SemanticError(
                "argument " + std::to_string(index + 1) + " of " + signature.name +
                " expects " + parameter.typeName + ", got " + argumentType);
        }
    }

    return signature.returnType;
}

std::string SemanticAnalyzer::analyzePreludeCall(
    std::string_view name,
    const std::vector<std::string>& argumentTypes) const
{
    for (const PreludeSignature& signature : preludeSignatures()) {
        if (!equalsIgnoreCase(signature.name, name) ||
            signature.parameterTypes.size() != argumentTypes.size()) {
            continue;
        }

        bool matches = true;
        for (std::size_t index = 0; index < argumentTypes.size(); ++index) {
            const std::string_view expected = signature.parameterTypes[index];
            if (expected != "*" && argumentTypes[index] != expected) {
                matches = false;
                break;
            }
        }

        if (matches) {
            return std::string(signature.returnType);
        }
    }

    if (equalsIgnoreCase(name, "Length")) {
        throw SemanticError("Length expects String");
    }
    if (equalsIgnoreCase(name, "Ord")) {
        throw SemanticError("Ord expects Char");
    }

    throw SemanticError("no matching prelude signature for call: " + std::string(name));
}

void SemanticAnalyzer::requireBoolCondition(const ast::Expression& expression)
{
    const std::string typeName = analyzeExpression(expression);
    if (!typeName.empty() && typeName != "Bool") {
        throw SemanticError("condition must be Bool");
    }
}

std::string SemanticAnalyzer::analyzeBinaryExpression(const ast::BinaryExpression& expression)
{
    const ast::BinaryOperator op = expression.op();

    if (op == ast::BinaryOperator::Assign) {
        std::string targetType;

        if (expression.left().kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression.left());
            const Symbol& target = resolveOrThrow(identifier.name());
            result_.bind(identifier, target);
            result_.setExpressionType(identifier, resolvedType(target.typeName));
            if (target.kind != SymbolKind::Variable && target.kind != SymbolKind::State) {
                throw SemanticError("assignment target is not mutable: " + identifier.name());
            }
            targetType = target.typeName;
        } else if (expression.left().kind() == ast::AstNodeKind::CallExpression &&
                   isMemberCall(static_cast<const ast::CallExpression&>(expression.left()))) {
            targetType = analyzeMemberExpression(
                static_cast<const ast::CallExpression&>(expression.left()));
        } else {
            throw SemanticError("assignment target must be an identifier or field");
        }

        const std::string rightType = analyzeExpression(expression.right());
        if (!canAssign(targetType, rightType)) {
            throw SemanticError("cannot assign " + rightType + " to " + targetType);
        }
        return targetType;
    }

    const std::string leftType = analyzeExpression(expression.left());
    const std::string rightType = analyzeExpression(expression.right());

    const auto requireMatchingTypes = [&] {
        if (!typesMatch(leftType, rightType)) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires matching operand types");
        }
    };
    const auto requireNumericOperands = [&] {
        if ((!leftType.empty() && !isNumericType(leftType)) ||
            (!rightType.empty() && !isNumericType(rightType))) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires numeric operands");
        }
        requireMatchingTypes();
    };
    const auto requireIntegerOperands = [&] {
        if ((!leftType.empty() && !isIntegerType(leftType)) ||
            (!rightType.empty() && !isIntegerType(rightType))) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires integer operands");
        }
        requireMatchingTypes();
    };

    switch (op) {
    case ast::BinaryOperator::Add:
    case ast::BinaryOperator::Subtract:
    case ast::BinaryOperator::Multiply:
    case ast::BinaryOperator::Divide:
    case ast::BinaryOperator::Power:
        requireNumericOperands();
        return leftType;
    case ast::BinaryOperator::IntegerDivide:
    case ast::BinaryOperator::Modulo:
    case ast::BinaryOperator::ShiftLeft:
    case ast::BinaryOperator::ShiftRight:
    case ast::BinaryOperator::BitAnd:
    case ast::BinaryOperator::BitXor:
    case ast::BinaryOperator::BitOr:
        requireIntegerOperands();
        return leftType;
    case ast::BinaryOperator::Equal:
    case ast::BinaryOperator::NotEqual:
    case ast::BinaryOperator::Less:
    case ast::BinaryOperator::Greater:
    case ast::BinaryOperator::LessEqual:
    case ast::BinaryOperator::GreaterEqual:
        requireMatchingTypes();
        return "Bool";
    case ast::BinaryOperator::And:
    case ast::BinaryOperator::Xor:
    case ast::BinaryOperator::Or:
        requireMatchingTypes();
        if ((!leftType.empty() && leftType != "Bool") ||
            (!rightType.empty() && rightType != "Bool")) {
            throw SemanticError(
                "operator '" + std::string(binaryOperatorName(op)) +
                "' requires Bool operands");
        }
        return "Bool";
    case ast::BinaryOperator::Range:
        requireIntegerOperands();
        return leftType;
    case ast::BinaryOperator::In:
        return "Bool";
    case ast::BinaryOperator::Assign:
        break;
    }

    return {};
}

std::string SemanticAnalyzer::analyzeUnaryExpression(const ast::UnaryExpression& expression)
{
    const std::string operandType = analyzeExpression(expression.operand());

    switch (expression.op()) {
    case ast::UnaryOperator::Plus:
    case ast::UnaryOperator::Minus:
        if (!operandType.empty() && !isNumericType(operandType)) {
            throw SemanticError("unary arithmetic operator requires a numeric operand");
        }
        return operandType;
    case ast::UnaryOperator::Not:
        if (!operandType.empty() && operandType != "Bool") {
            throw SemanticError("operator 'not' requires a Bool operand");
        }
        return "Bool";
    case ast::UnaryOperator::BitNot:
        if (!operandType.empty() && !isIntegerType(operandType)) {
            throw SemanticError("operator 'bitnot' requires an integer operand");
        }
        return operandType;
    }

    return {};
}

bool SemanticAnalyzer::looksLikeIdentifier(std::string_view text)
{
    if (text.empty()) {
        return false;
    }
    const auto isIdentifierStart = [](char ch) {
        return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
    };
    const auto isIdentifierPart = [](char ch) {
        return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
    };

    if (!isIdentifierStart(text.front())) {
        return false;
    }
    for (char ch : text.substr(1)) {
        if (!isIdentifierPart(ch)) {
            return false;
        }
    }
    return true;
}

const StructType* SemanticAnalyzer::resolveStruct(std::string_view name) const
{
    const auto iterator = structs_.find(normalizeName(name));
    return iterator != structs_.end() ? &iterator->second : nullptr;
}

const StructField* SemanticAnalyzer::resolveStructField(std::string_view structName, std::string_view fieldName) const
{
    const StructType* structType = resolveStruct(structName);
    if (structType == nullptr) {
        return nullptr;
    }
    for (const StructField& field : structType->fields) {
        if (equalsIgnoreCase(field.name, fieldName)) {
            return &field;
        }
    }
    return nullptr;
}

bool SemanticAnalyzer::isMemberCall(const ast::CallExpression& expression)
{
    if (expression.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
        return false;
    }
    const auto& callee = static_cast<const ast::IdentifierExpression&>(expression.callee());
    return equalsIgnoreCase(callee.name(), "__member");
}

bool SemanticAnalyzer::isInternalSyntheticName(std::string_view name)
{
    return name == "__index" || name == "__member";
}

bool SemanticAnalyzer::isNumericType(std::string_view typeName)
{
    return isIntegerType(typeName) ||
           typeName == "Float32" ||
           typeName == "Float64";
}

bool SemanticAnalyzer::isIntegerType(std::string_view typeName)
{
    return typeName == "Int8" ||
           typeName == "Int16" ||
           typeName == "Int32" ||
           typeName == "Int64" ||
           typeName == "UInt8" ||
           typeName == "UInt16" ||
           typeName == "UInt32" ||
           typeName == "UInt64" ||
           typeName == "Natural";
}

bool SemanticAnalyzer::isPreludeCall(std::string_view name)
{
    for (const PreludeSignature& signature : preludeSignatures()) {
        if (equalsIgnoreCase(signature.name, name)) {
            return true;
        }
    }
    return false;
}

std::string SemanticAnalyzer::normalizeName(std::string_view name)
{
    std::string normalized;
    normalized.reserve(name.size());
    for (const char ch : name) {
        normalized.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

bool SemanticAnalyzer::canAssign(std::string_view targetType, std::string_view valueType)
{
    return targetType.empty() || (!valueType.empty() && targetType == valueType);
}

bool SemanticAnalyzer::typesMatch(std::string_view left, std::string_view right)
{
    return left.empty() || right.empty() || left == right;
}

ResolvedType SemanticAnalyzer::resolvedType(std::string typeName) const
{
    return ResolvedType{typeName, types_.resolve(typeName)};
}

} // namespace inox::compiler::semantic
