#include "LlvmIrEmitter.h"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace inox::compiler::codegen {

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

std::string normalize(std::string_view name)
{
    std::string normalized;
    normalized.reserve(name.size());
    for (const char ch : name) {
        normalized.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

std::string llvmIntegerLiteral(std::string_view value)
{
    if (value.empty() || value.front() != '$') {
        return std::string(value);
    }

    return std::to_string(std::stoull(std::string(value.substr(1)), nullptr, 16));
}


struct LlvmStringConstant {
    std::size_t size = 0;
    std::string bytes;
};

void appendEscapedLlvmByte(std::ostringstream& output, unsigned char byte)
{
    if (byte >= 0x20 && byte <= 0x7e && byte != '"' && byte != '\\') {
        output << static_cast<char>(byte);
        return;
    }

    output << '\\'
           << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(byte)
           << std::nouppercase << std::dec << std::setfill(' ');
}

LlvmStringConstant llvmStringConstant(std::string_view value)
{
    std::ostringstream bytes;
    std::size_t size = 0;

    for (std::size_t index = 0; index < value.size(); ++index) {
        unsigned char byte = static_cast<unsigned char>(value[index]);
        if (byte == '\\' && index + 1 < value.size()) {
            const char escape = value[++index];
            switch (escape) {
            case 'n':
                byte = '\n';
                break;
            case 'r':
                byte = '\r';
                break;
            case 't':
                byte = '\t';
                break;
            case '\\':
                byte = '\\';
                break;
            case '"':
                byte = '"';
                break;
            case '0':
                byte = '\0';
                break;
            default:
                appendEscapedLlvmByte(bytes, '\\');
                ++size;
                byte = static_cast<unsigned char>(escape);
                break;
            }
        }

        appendEscapedLlvmByte(bytes, byte);
        ++size;
    }

    appendEscapedLlvmByte(bytes, '\0');
    ++size;
    return LlvmStringConstant{size, bytes.str()};
}

struct IntegerParameter {
    std::string inoxName;
    std::string llvmName;
};

struct FunctionSignature {
    std::string llvmName;
    std::string llvmReturnType;
    std::vector<IntegerParameter> parameters;
};

using FunctionSignatures =
    std::unordered_map<std::string, FunctionSignature>;

struct StructFieldInfo {
    std::string inoxName;
    std::string llvmType;
    std::size_t index = 0;
};

struct StructDefinition {
    std::string inoxName;
    std::string llvmName;
    std::vector<StructFieldInfo> fields;
};

using StructDefinitions = std::unordered_map<std::string, StructDefinition>;

std::string llvmStructName(std::string_view inoxName)
{
    return "%" + normalize(inoxName);
}

std::string llvmTypeForScalar(std::string_view inoxType)
{
    if (equalsIgnoreCase(inoxType, "Integer") || equalsIgnoreCase(inoxType, "Int64")) {
        return "i64";
    }
    if (equalsIgnoreCase(inoxType, "Bool")) {
        return "i1";
    }
    return {};
}

std::string llvmTypeForInoxType(std::string_view inoxType, const StructDefinitions& structs)
{
    if (const std::string scalar = llvmTypeForScalar(inoxType); !scalar.empty()) {
        return scalar;
    }
    const auto structType = structs.find(normalize(inoxType));
    if (structType != structs.end()) {
        return structType->second.llvmName;
    }
    return {};
}

const StructDefinition* findStruct(const StructDefinitions& structs, std::string_view inoxName)
{
    const auto iterator = structs.find(normalize(inoxName));
    return iterator != structs.end() ? &iterator->second : nullptr;
}

const StructFieldInfo* findStructField(const StructDefinition& structType, std::string_view fieldName)
{
    for (const StructFieldInfo& field : structType.fields) {
        if (equalsIgnoreCase(field.inoxName, fieldName)) {
            return &field;
        }
    }
    return nullptr;
}

void collectStructDefinitions(const ast::SectionDeclaration& section, StructDefinitions& structs)
{
    if (section.sectionKind() != ast::SectionKind::Type) {
        return;
    }

    const auto& tokens = section.tokens();
    for (std::size_t index = 0; index + 1 < tokens.size();) {
        if (!equalsIgnoreCase(tokens[index + 1], "Struct")) {
            ++index;
            continue;
        }

        StructDefinition definition;
        definition.inoxName = tokens[index];
        definition.llvmName = llvmStructName(tokens[index]);
        index += 2;

        while (index < tokens.size() && tokens[index] != ";") {
            if (index + 1 >= tokens.size()) {
                throw CodegenError("invalid Struct declaration: " + definition.inoxName);
            }
            const std::string fieldName = tokens[index++];
            const std::string fieldType = tokens[index++];
            const std::string llvmFieldType = llvmTypeForScalar(fieldType);
            if (llvmFieldType.empty()) {
                throw CodegenError(
                    "LLVM emission currently supports only Integer and Bool struct fields");
            }
            definition.fields.push_back(
                StructFieldInfo{fieldName, llvmFieldType, definition.fields.size()});
        }

        if (index >= tokens.size() || tokens[index] != ";") {
            throw CodegenError("expected ';' to close Struct: " + definition.inoxName);
        }
        ++index;
        structs.emplace(normalize(definition.inoxName), std::move(definition));
    }
}


FunctionSignature parseFunctionSignature(const ast::FunctionDeclaration& function)
{
    const auto& tokens = function.signatureTokens();
    if (tokens.size() < 3 || tokens.front() != "(") {
        throw CodegenError("unsupported function signature: " + function.name());
    }

    std::vector<IntegerParameter> parameters;
    std::size_t index = 1;
    while (index < tokens.size() && tokens[index] != ")") {
        if (index + 1 >= tokens.size() || !equalsIgnoreCase(tokens[index + 1], "Integer")) {
            throw CodegenError(
                "LLVM emission currently supports only Integer parameters");
        }

        parameters.push_back(IntegerParameter{tokens[index], normalize(tokens[index])});
        index += 2;
        if (index < tokens.size() && tokens[index] == ",") {
            ++index;
        } else if (index >= tokens.size() || tokens[index] != ")") {
            throw CodegenError("unsupported function signature: " + function.name());
        }
    }

    if (index >= tokens.size() || tokens[index] != ")") {
        throw CodegenError("unsupported function signature: " + function.name());
    }
    ++index;

    std::string llvmReturnType;
    if (index == tokens.size()) {
        llvmReturnType = "void";
    } else if (index + 1 != tokens.size()) {
        throw CodegenError(
            "LLVM emission currently supports only Integer and Bool return types, or subroutines without return type");
    } else if (equalsIgnoreCase(tokens[index], "Integer")) {
        llvmReturnType = "i64";
    } else if (equalsIgnoreCase(tokens[index], "Bool")) {
        llvmReturnType = "i1";
    } else {
        throw CodegenError(
            "LLVM emission currently supports only Integer and Bool return types, or subroutines without return type");
    }

    return FunctionSignature{
        normalize(function.name()),
        std::move(llvmReturnType),
        std::move(parameters)};
}

class FunctionEmitter {
public:
    FunctionEmitter(std::ostringstream& output,
                    const ast::FunctionDeclaration& function,
                    const FunctionSignature& signature,
                    const FunctionSignatures& signatures,
                    const StructDefinitions& structs,
                    std::vector<std::string>& stringGlobals,
                    std::size_t& nextStringLiteral)
        : output_(output),
          function_(function),
          signature_(signature),
          signatures_(signatures),
          structs_(structs),
          stringGlobals_(stringGlobals),
          nextStringLiteral_(nextStringLiteral)
    {
        for (const IntegerParameter& parameter : signature.parameters) {
            parameters_.emplace(normalize(parameter.inoxName), "%" + parameter.llvmName);
        }
    }

    void emit()
    {
        if (signature_.llvmReturnType == "i32" || signature_.llvmReturnType == "void") {
            for (const auto& statement : function_.body()) {
                emitLocalDeclaration(*statement);
            }
            if (signature_.llvmReturnType == "i32") {
                output_ << "  ret i32 0\n";
            } else {
                output_ << "  ret void\n";
            }
            return;
        }

        if (function_.body().size() == 1 &&
            function_.body().front()->kind() == ast::AstNodeKind::IfStatement) {
            emitIfReturn(
                static_cast<const ast::IfStatement&>(*function_.body().front()));
            return;
        }

        if (function_.body().empty() ||
            function_.body().back()->kind() != ast::AstNodeKind::ReturnStatement) {
            throw CodegenError(
                "LLVM emission currently requires a final Return expression");
        }

        for (std::size_t index = 0; index + 1 < function_.body().size(); ++index) {
            emitLocalDeclaration(*function_.body()[index]);
        }

        const auto& returnStatement =
            static_cast<const ast::ReturnStatement&>(*function_.body().back());
        const std::string value = emitExpression(returnStatement.expression());
        output_ << "  ret " << signature_.llvmReturnType << ' ' << value << '\n';
    }

private:
    struct LoopTargets {
        std::string continueTarget;
        std::string breakTarget;
    };

    struct LocalInfo {
        std::string slot;
        std::string inoxType;
        std::string llvmType;
    };

    struct FieldAddress {
        std::string pointer;
        std::string llvmType;
    };

    void emitIfReturn(const ast::IfStatement& statement)
    {
        if (signature_.llvmReturnType != "i64") {
            throw CodegenError(
                "LLVM emission currently supports if/else only in Integer functions");
        }
        if (statement.elseBody().empty()) {
            throw CodegenError(
                "LLVM emission currently requires else for direct-return if chains");
        }
        if (statement.thenBody().size() != 1 ||
            statement.thenBody().front()->kind() != ast::AstNodeKind::ReturnStatement ||
            statement.elseBody().size() != 1 ||
            statement.elseBody().front()->kind() != ast::AstNodeKind::ReturnStatement) {
            throw CodegenError(
                "LLVM emission currently requires a single Return in each if branch");
        }
        for (const auto& clause : statement.elseIfClauses()) {
            if (clause.body.size() != 1 ||
                clause.body.front()->kind() != ast::AstNodeKind::ReturnStatement) {
                throw CodegenError(
                    "LLVM emission currently requires a single Return in each elif branch");
            }
        }

        const std::size_t label = nextLabel_++;
        const bool hasElseIf = !statement.elseIfClauses().empty();
        const std::string condition = emitExpression(statement.condition());
        output_ << "  br i1 " << condition
                << ", label %then" << label
                << ", label %" << (hasElseIf ? "elifcond" : "else") << label;
        if (hasElseIf) {
            output_ << "_0";
        }
        output_ << "\n\n";

        output_ << "then" << label << ":\n";
        emitReturn(static_cast<const ast::ReturnStatement&>(*statement.thenBody().front()));

        for (std::size_t index = 0; index < statement.elseIfClauses().size(); ++index) {
            const auto& clause = statement.elseIfClauses()[index];
            const bool hasNext = index + 1 < statement.elseIfClauses().size();
            output_ << "\nelifcond" << label << '_' << index << ":\n";
            const std::string elseIfCondition = emitExpression(*clause.condition);
            output_ << "  br i1 " << elseIfCondition
                    << ", label %elifthen" << label << '_' << index
                    << ", label %" << (hasNext ? "elifcond" : "else") << label;
            if (hasNext) {
                output_ << '_' << index + 1;
            }
            output_ << "\n\n";

            output_ << "elifthen" << label << '_' << index << ":\n";
            emitReturn(static_cast<const ast::ReturnStatement&>(*clause.body.front()));
        }

        output_ << "\nelse" << label << ":\n";
        emitReturn(static_cast<const ast::ReturnStatement&>(*statement.elseBody().front()));
    }

    void emitReturn(const ast::ReturnStatement& statement)
    {
        const std::string value = emitExpression(statement.expression());
        output_ << "  ret " << signature_.llvmReturnType << ' ' << value << '\n';
    }

    void emitIfMerge(const ast::IfStatement& statement)
    {
        if (!statement.elseIfClauses().empty()) {
            throw CodegenError(
                "LLVM emission currently supports if without elif");
        }

        const std::size_t label = nextLabel_++;
        const bool hasElse = !statement.elseBody().empty();
        const std::string condition = emitExpression(statement.condition());
        output_ << "  br i1 " << condition
                << ", label %then" << label
                << ", label %" << (hasElse ? "else" : "endif") << label << "\n\n";

        output_ << "then" << label << ":\n";
        emitAssignmentBranch(statement.thenBody());
        output_ << "  br label %endif" << label << "\n\n";

        if (hasElse) {
            output_ << "else" << label << ":\n";
            emitAssignmentBranch(statement.elseBody());
            output_ << "  br label %endif" << label << "\n\n";
        }

        output_ << "endif" << label << ":\n";
    }

    void emitAssignmentBranch(const std::vector<ast::StatementPtr>& statements)
    {
        if (statements.empty()) {
            throw CodegenError(
                "LLVM emission currently requires assignments in each if branch");
        }

        for (const auto& statement : statements) {
            if (statement->kind() != ast::AstNodeKind::ExpressionStatement) {
                throw CodegenError(
                    "LLVM emission currently supports only assignments in continuing if branches");
            }
            emitLocalAssignment(
                static_cast<const ast::ExpressionStatement&>(*statement).expression());
        }
    }

    void emitWhile(const ast::WhileStatement& statement)
    {
        const std::size_t label = nextLabel_++;
        const std::string conditionTarget = "whilecond" + std::to_string(label);
        const std::string endTarget = "whileend" + std::to_string(label);
        output_ << "  br label %whilecond" << label << "\n\n";

        output_ << "whilecond" << label << ":\n";
        const std::string condition = emitExpression(statement.condition());
        output_ << "  br i1 " << condition
                << ", label %whilebody" << label
                << ", label %whileend" << label << "\n\n";

        output_ << "whilebody" << label << ":\n";
        loopTargets_.push_back(LoopTargets{conditionTarget, endTarget});
        const bool terminated = emitLoopStatements(statement.body());
        loopTargets_.pop_back();
        if (!terminated) {
            output_ << "  br label %" << conditionTarget << '\n';
        }
        output_ << '\n';

        output_ << "whileend" << label << ":\n";
    }

    void emitRepeat(const ast::RepeatStatement& statement)
    {
        const std::size_t label = nextLabel_++;
        const std::string bodyTarget = "repeatbody" + std::to_string(label);
        const std::string endTarget = "repeatend" + std::to_string(label);
        output_ << "  br label %" << bodyTarget << "\n\n";

        output_ << bodyTarget << ":\n";
        loopTargets_.push_back(LoopTargets{bodyTarget, endTarget});
        const bool terminated = emitRepeatStatements(statement.body(), bodyTarget, endTarget, label);
        loopTargets_.pop_back();
        if (!terminated) {
            output_ << "  br label %" << bodyTarget << '\n';
        }
        output_ << '\n';

        output_ << endTarget << ":\n";
    }

    void emitForIn(const ast::ForInStatement& statement)
    {
        if (statement.iterable().kind() != ast::AstNodeKind::BinaryExpression) {
            throw CodegenError(
                "LLVM emission currently supports only range expressions in for loops");
        }

        const auto& range = static_cast<const ast::BinaryExpression&>(statement.iterable());
        if (range.op() != ast::BinaryOperator::Range) {
            throw CodegenError(
                "LLVM emission currently supports only Start..End for ranges");
        }

        const std::string iteratorName = normalize(statement.iterator());
        if (locals_.find(iteratorName) != locals_.end()) {
            throw CodegenError(
                "LLVM emission does not support for iterator shadowing local variables");
        }

        const std::size_t label = nextLabel_++;
        const std::string conditionTarget = "forcond" + std::to_string(label);
        const std::string bodyTarget = "forbody" + std::to_string(label);
        const std::string stepTarget = "forstep" + std::to_string(label);
        const std::string endTarget = "forend" + std::to_string(label);
        const std::string slot = "%" + iteratorName;

        output_ << "  " << slot << " = alloca i64\n";
        const std::string startValue = emitExpression(range.left());
        output_ << "  store i64 " << startValue << ", ptr " << slot << '\n';
        locals_.emplace(iteratorName, LocalInfo{slot, "Integer", "i64"});

        output_ << "  br label %" << conditionTarget << "\n\n";

        output_ << conditionTarget << ":\n";
        const std::string iteratorValue = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << iteratorValue << " = load i64, ptr " << slot << '\n';
        const std::string endValue = emitExpression(range.right());
        const std::string condition = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << condition << " = icmp sle i64 "
                << iteratorValue << ", " << endValue << '\n';
        output_ << "  br i1 " << condition
                << ", label %" << bodyTarget
                << ", label %" << endTarget << "\n\n";

        output_ << bodyTarget << ":\n";
        loopTargets_.push_back(LoopTargets{stepTarget, endTarget});
        const bool terminated = emitLoopStatements(statement.body());
        loopTargets_.pop_back();
        if (!terminated) {
            output_ << "  br label %" << stepTarget << '\n';
        }
        output_ << '\n';

        output_ << stepTarget << ":\n";
        const std::string stepValue = "%tmp" + std::to_string(nextTemporary_++);
        const std::string incrementedValue = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << stepValue << " = load i64, ptr " << slot << '\n';
        const std::string increment = statement.step() != nullptr
            ? emitExpression(*statement.step())
            : "1";
        output_ << "  " << incrementedValue << " = add i64 "
                << stepValue << ", " << increment << "\n";
        output_ << "  store i64 " << incrementedValue << ", ptr " << slot << '\n';
        output_ << "  br label %" << conditionTarget << "\n\n";

        output_ << endTarget << ":\n";
        locals_.erase(iteratorName);
    }

    bool emitRepeatStatements(const std::vector<ast::StatementPtr>& statements,
                              const std::string& bodyTarget,
                              const std::string& endTarget,
                              std::size_t repeatLabel)
    {
        std::size_t continueIndex = 0;
        bool terminated = false;
        for (std::size_t index = 0; index < statements.size(); ++index) {
            if (terminated) {
                throw CodegenError(
                    "LLVM emission does not support statements after terminating repeat branch");
            }

            const ast::Statement& statement = *statements[index];
            if (statement.kind() == ast::AstNodeKind::ExpressionStatement) {
                emitLocalAssignment(
                    static_cast<const ast::ExpressionStatement&>(statement).expression());
                continue;
            }
            if (statement.kind() == ast::AstNodeKind::IfStatement) {
                emitLoopIf(static_cast<const ast::IfStatement&>(statement));
                continue;
            }
            if (statement.kind() == ast::AstNodeKind::BreakStatement) {
                output_ << "  br label %" << currentLoopTargets().breakTarget << '\n';
                terminated = true;
                continue;
            }
            if (statement.kind() == ast::AstNodeKind::ContinueStatement) {
                output_ << "  br label %" << currentLoopTargets().continueTarget << '\n';
                terminated = true;
                continue;
            }
            if (statement.kind() == ast::AstNodeKind::UntilStatement) {
                const auto& untilStatement =
                    static_cast<const ast::UntilStatement&>(statement);
                const std::string condition = emitExpression(untilStatement.condition());
                const bool hasFollowingStatements = index + 1 < statements.size();
                const std::string continueTarget = hasFollowingStatements
                    ? "repeatcontinue" + std::to_string(repeatLabel) + "_" +
                          std::to_string(continueIndex++)
                    : bodyTarget;
                output_ << "  br i1 " << condition
                        << ", label %" << endTarget
                        << ", label %" << continueTarget << '\n';
                if (hasFollowingStatements) {
                    output_ << "\n" << continueTarget << ":\n";
                } else {
                    terminated = true;
                }
                continue;
            }

            throw CodegenError(
                "LLVM emission currently supports only assignments, if, break, continue, and until in repeat bodies");
        }

        return terminated;
    }

    bool emitLoopStatements(const std::vector<ast::StatementPtr>& statements)
    {
        bool terminated = false;
        for (const auto& statement : statements) {
            if (terminated) {
                throw CodegenError(
                    "LLVM emission does not support statements after break or continue");
            }
            terminated = emitLoopStatement(*statement);
        }
        return terminated;
    }

    bool emitLoopStatement(const ast::Statement& statement)
    {
        if (statement.kind() == ast::AstNodeKind::ExpressionStatement) {
            emitLocalAssignment(
                static_cast<const ast::ExpressionStatement&>(statement).expression());
            return false;
        }
        if (statement.kind() == ast::AstNodeKind::IfStatement) {
            emitLoopIf(static_cast<const ast::IfStatement&>(statement));
            return false;
        }
        if (statement.kind() == ast::AstNodeKind::BreakStatement) {
            output_ << "  br label %" << currentLoopTargets().breakTarget << '\n';
            return true;
        }
        if (statement.kind() == ast::AstNodeKind::ContinueStatement) {
            output_ << "  br label %" << currentLoopTargets().continueTarget << '\n';
            return true;
        }

        throw CodegenError(
            "LLVM emission currently supports only assignments, if, break, and continue in loop bodies");
    }

    void emitLoopIf(const ast::IfStatement& statement)
    {
        if (!statement.elseIfClauses().empty() || !statement.elseBody().empty()) {
            throw CodegenError(
                "LLVM emission currently supports loop if without elif or else");
        }

        const std::size_t label = nextLabel_++;
        const std::string condition = emitExpression(statement.condition());
        output_ << "  br i1 " << condition
                << ", label %loopthen" << label
                << ", label %loopendif" << label << "\n\n";

        output_ << "loopthen" << label << ":\n";
        if (!emitLoopStatements(statement.thenBody())) {
            output_ << "  br label %loopendif" << label << '\n';
        }
        output_ << "\nloopendif" << label << ":\n";
    }

    const LoopTargets& currentLoopTargets() const
    {
        if (loopTargets_.empty()) {
            throw CodegenError(
                "LLVM emission supports break and continue only inside loops");
        }
        return loopTargets_.back();
    }

    void emitLocalDeclaration(const ast::Statement& statement)
    {
        if (statement.kind() == ast::AstNodeKind::VarStatement) {
            const auto& variable = static_cast<const ast::VarStatement&>(statement);
            if (variable.initializer() == nullptr) {
                throw CodegenError(
                    "LLVM emission currently requires local variable initialization");
            }
            emitLocalVariable(variable.name(), *variable.initializer());
            return;
        }

        if (statement.kind() == ast::AstNodeKind::VarBlockStatement) {
            const auto& block = static_cast<const ast::VarBlockStatement&>(statement);
            for (const auto& declaration : block.declarations()) {
                emitVarBlockDeclaration(*declaration);
            }
            return;
        }

        if (statement.kind() == ast::AstNodeKind::ExpressionStatement) {
            const auto& expression =
                static_cast<const ast::ExpressionStatement&>(statement).expression();
            if (isAssignmentExpression(expression)) {
                emitLocalAssignment(expression);
            } else {
                emitExpressionStatement(expression);
            }
            return;
        }

        if (statement.kind() == ast::AstNodeKind::IfStatement) {
            emitIfMerge(static_cast<const ast::IfStatement&>(statement));
            return;
        }

        if (statement.kind() == ast::AstNodeKind::WhileStatement) {
            emitWhile(static_cast<const ast::WhileStatement&>(statement));
            return;
        }

        if (statement.kind() == ast::AstNodeKind::RepeatStatement) {
            emitRepeat(static_cast<const ast::RepeatStatement&>(statement));
            return;
        }

        if (statement.kind() == ast::AstNodeKind::ForInStatement) {
            emitForIn(static_cast<const ast::ForInStatement&>(statement));
            return;
        }

        throw CodegenError(
            "LLVM emission currently supports only local variables, assignments, if, while, repeat, and for before Return");
    }

    void emitVarBlockDeclaration(const ast::Statement& statement)
    {
        if (statement.kind() == ast::AstNodeKind::VarStatement) {
            const auto& variable = static_cast<const ast::VarStatement&>(statement);
            if (variable.initializer() == nullptr) {
                emitTypedLocalVariable(variable.name(), variable.typeName());
            } else if (!variable.typeName().empty()) {
                emitTypedLocalVariable(variable.name(), variable.typeName(), variable.initializer());
            } else {
                emitLocalVariable(variable.name(), *variable.initializer());
            }
            return;
        }

        if (statement.kind() != ast::AstNodeKind::ExpressionStatement) {
            throw CodegenError(
                "unsupported local variable declaration in Integer function");
        }

        const auto& expression =
            static_cast<const ast::ExpressionStatement&>(statement).expression();
        if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
            throw CodegenError(
                "unsupported local variable declaration in Integer function");
        }

        const auto& assignment = static_cast<const ast::BinaryExpression&>(expression);
        if (assignment.op() != ast::BinaryOperator::Assign ||
            assignment.left().kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenError(
                "unsupported local variable declaration in Integer function");
        }

        const auto& identifier =
            static_cast<const ast::IdentifierExpression&>(assignment.left());
        emitLocalVariable(identifier.name(), assignment.right());
    }

    void emitLocalVariable(std::string_view name, const ast::Expression& initializer)
    {
        const std::string normalizedName = normalize(name);
        const std::string slot = "%" + normalizedName;
        output_ << "  " << slot << " = alloca i64\n";
        const std::string value = emitExpression(initializer);
        output_ << "  store i64 " << value << ", ptr " << slot << '\n';
        locals_.emplace(normalizedName, LocalInfo{slot, "Integer", "i64"});
    }

    void emitTypedLocalVariable(std::string_view name, std::string_view typeName, const ast::Expression* initializer = nullptr)
    {
        const std::string llvmType = llvmTypeForInoxType(typeName, structs_);
        if (llvmType.empty()) {
            throw CodegenError("unsupported local variable type for LLVM emission");
        }

        const std::string normalizedName = normalize(name);
        const std::string slot = "%" + normalizedName;
        output_ << "  " << slot << " = alloca " << llvmType << "\n";

        if (const StructDefinition* structType = findStruct(structs_, typeName)) {
            if (initializer != nullptr) {
                throw CodegenError("LLVM emission does not support struct initializers yet");
            }
            output_ << "  store " << structType->llvmName
                    << " zeroinitializer, ptr " << slot << '\n';
            locals_.emplace(normalizedName,
                            LocalInfo{slot, std::string(typeName), structType->llvmName});
            return;
        }

        const std::string value = initializer != nullptr ? emitExpression(*initializer) : "0";
        output_ << "  store " << llvmType << ' ' << value << ", ptr " << slot << '\n';
        locals_.emplace(normalizedName, LocalInfo{slot, std::string(typeName), llvmType});
    }

    static bool isAssignmentExpression(const ast::Expression& expression)
    {
        if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
            return false;
        }

        const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
        return binary.op() == ast::BinaryOperator::Assign;
    }

    void emitExpressionStatement(const ast::Expression& expression)
    {
        if (expression.kind() != ast::AstNodeKind::CallExpression) {
            throw CodegenError(
                "LLVM emission currently supports only assignments and calls as statements");
        }

        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenError(
                "LLVM emission currently supports only direct calls as statements");
        }

        const auto& callee =
            static_cast<const ast::IdentifierExpression&>(call.callee());
        const bool isPut = equalsIgnoreCase(callee.name(), "Put");
        const bool isPutLn = equalsIgnoreCase(callee.name(), "PutLn");
        if (isPut || isPutLn) {
            if (call.arguments().size() != 1) {
                throw CodegenError("Put/PutLn LLVM emission expects exactly one argument");
            }
            emitOutputCall(*call.arguments().front(), isPutLn);
            return;
        }

        emitSubroutineCall(call, callee.name());
    }

    void emitSubroutineCall(const ast::CallExpression& call, std::string_view calleeName)
    {
        const auto signature = signatures_.find(normalize(calleeName));
        if (signature == signatures_.end()) {
            throw CodegenError(
                "LLVM emission currently supports only Put/PutLn and user subroutine calls as statements");
        }
        if (signature->second.llvmReturnType != "void") {
            throw CodegenError(
                "LLVM emission currently supports only subroutine calls as expression statements");
        }
        if (call.arguments().size() != signature->second.parameters.size()) {
            throw CodegenError("unsupported subroutine argument count: " + function_.name());
        }

        std::vector<std::string> arguments;
        arguments.reserve(call.arguments().size());
        for (const auto& argument : call.arguments()) {
            arguments.push_back(emitExpression(*argument));
        }

        output_ << "  call void @" << signature->second.llvmName << '(';
        for (std::size_t index = 0; index < arguments.size(); ++index) {
            if (index != 0) {
                output_ << ", ";
            }
            output_ << "i64 " << arguments[index];
        }
        output_ << ")\n";
    }

    void emitOutputCall(const ast::Expression& argument, bool newline)
    {
        if (argument.kind() == ast::AstNodeKind::LiteralExpression) {
            const auto& literal = static_cast<const ast::LiteralExpression&>(argument);
            if (literal.literalKind() == ast::LiteralKind::String) {
                const std::string value = emitStringLiteral(literal.value());
                output_ << "  call i32 (ptr, ...) @printf(ptr "
                        << (newline ? "@.inox.fmt.str.nl" : "@.inox.fmt.str")
                        << ", ptr " << value << ")\n";
                return;
            }
        }

        if (isBoolExpression(argument)) {
            const std::string value = emitExpression(argument);
            const std::string selected = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << selected
                    << " = select i1 " << value
                    << ", ptr @.inox.true, ptr @.inox.false\n";
            output_ << "  call i32 (ptr, ...) @printf(ptr "
                    << (newline ? "@.inox.fmt.str.nl" : "@.inox.fmt.str")
                    << ", ptr " << selected << ")\n";
            return;
        }

        const std::string value = emitExpression(argument);
        output_ << "  call i32 (ptr, ...) @printf(ptr "
                << (newline ? "@.inox.fmt.i64.nl" : "@.inox.fmt.i64")
                << ", i64 " << value << ")\n";
    }

    std::string emitStringLiteral(std::string_view value)
    {
        const std::string name = ".inox.str." + std::to_string(nextStringLiteral_++);
        const LlvmStringConstant constant = llvmStringConstant(value);
        std::ostringstream global;
        global << "@" << name << " = private unnamed_addr constant ["
               << constant.size << " x i8] c\"" << constant.bytes << "\"";
        stringGlobals_.push_back(global.str());
        return "@" + name;
    }

    bool isBoolExpression(const ast::Expression& expression) const
    {
        if (expression.kind() == ast::AstNodeKind::LiteralExpression) {
            const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
            return literal.literalKind() == ast::LiteralKind::Boolean;
        }

        if (expression.kind() == ast::AstNodeKind::UnaryExpression) {
            const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
            return unary.op() == ast::UnaryOperator::Not;
        }

        if (expression.kind() == ast::AstNodeKind::BinaryExpression) {
            const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
            return !llvmComparisonPredicate(binary.op()).empty() ||
                   !llvmBooleanOperation(binary.op()).empty();
        }

        return false;
    }

    FieldAddress emitMemberAddress(const ast::CallExpression& call)
    {
        if (call.arguments().size() != 2 ||
            call.arguments()[0]->kind() != ast::AstNodeKind::IdentifierExpression ||
            call.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenError("LLVM emission currently supports only simple local field access");
        }

        const auto& base = static_cast<const ast::IdentifierExpression&>(*call.arguments()[0]);
        const auto& fieldName = static_cast<const ast::IdentifierExpression&>(*call.arguments()[1]);
        const auto local = locals_.find(normalize(base.name()));
        if (local == locals_.end()) {
            throw CodegenError("LLVM emission supports field access only on local struct variables");
        }

        const StructDefinition* structType = findStruct(structs_, local->second.inoxType);
        if (structType == nullptr) {
            throw CodegenError("LLVM emission field access target is not a struct");
        }
        const StructFieldInfo* field = findStructField(*structType, fieldName.name());
        if (field == nullptr) {
            throw CodegenError("unknown struct field for LLVM emission");
        }

        const std::string pointer = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << pointer << " = getelementptr " << structType->llvmName
                << ", ptr " << local->second.slot
                << ", i32 0, i32 " << field->index << '\n';
        return FieldAddress{pointer, field->llvmType};
    }

    static bool isMemberAccessCall(const ast::Expression& expression)
    {
        if (expression.kind() != ast::AstNodeKind::CallExpression) {
            return false;
        }
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
            return false;
        }
        const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
        return equalsIgnoreCase(callee.name(), "__member");
    }

    void emitLocalAssignment(const ast::Expression& expression)
    {
        if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
            throw CodegenError(
                "LLVM emission currently supports only simple local assignments");
        }

        const auto& assignment = static_cast<const ast::BinaryExpression&>(expression);
        if (assignment.op() != ast::BinaryOperator::Assign) {
            throw CodegenError(
                "LLVM emission currently supports only simple local assignments");
        }

        if (assignment.left().kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto& identifier =
                static_cast<const ast::IdentifierExpression&>(assignment.left());
            const auto local = locals_.find(normalize(identifier.name()));
            if (local == locals_.end()) {
                throw CodegenError(
                    "LLVM emission currently supports assignment only to local variables");
            }

            const std::string value = emitExpression(assignment.right());
            output_ << "  store " << local->second.llvmType << ' ' << value
                    << ", ptr " << local->second.slot << '\n';
            return;
        }

        if (isMemberAccessCall(assignment.left())) {
            const FieldAddress field = emitMemberAddress(
                static_cast<const ast::CallExpression&>(assignment.left()));
            const std::string value = emitExpression(assignment.right());
            output_ << "  store " << field.llvmType << ' ' << value
                    << ", ptr " << field.pointer << '\n';
            return;
        }

        throw CodegenError(
            "LLVM emission currently supports only local variable or field assignments");
    }

    std::string emitExpression(const ast::Expression& expression)
    {
        switch (expression.kind()) {
        case ast::AstNodeKind::LiteralExpression: {
            const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
            if (literal.literalKind() == ast::LiteralKind::Integer) {
                return llvmIntegerLiteral(literal.value());
            }
            if (literal.literalKind() == ast::LiteralKind::Boolean) {
                return equalsIgnoreCase(literal.value(), "true") ? "1" : "0";
            }
            break;
        }
        case ast::AstNodeKind::IdentifierExpression: {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
            const std::string normalizedName = normalize(identifier.name());
            const auto local = locals_.find(normalizedName);
            if (local != locals_.end()) {
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                if (local->second.llvmType.empty() || local->second.llvmType.front() == '%') {
                    throw CodegenError("LLVM emission cannot use whole struct as scalar expression");
                }
                output_ << "  " << result << " = load " << local->second.llvmType
                        << ", ptr " << local->second.slot << '\n';
                return result;
            }
            const auto parameter = parameters_.find(normalizedName);
            if (parameter != parameters_.end()) {
                return parameter->second;
            }
            break;
        }
        case ast::AstNodeKind::BinaryExpression: {
            const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
            const std::string left = emitExpression(binary.left());
            const std::string right = emitExpression(binary.right());
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            if (const std::string predicate = llvmComparisonPredicate(binary.op());
                !predicate.empty()) {
                output_ << "  " << result << " = icmp " << predicate << " i64 "
                        << left << ", " << right << '\n';
            } else if (const std::string operation = llvmBooleanOperation(binary.op());
                       !operation.empty()) {
                output_ << "  " << result << " = " << operation << " i1 "
                        << left << ", " << right << '\n';
            } else {
                output_ << "  " << result << " = " << llvmOperation(binary.op()) << " i64 "
                        << left << ", " << right << '\n';
            }
            return result;
        }
        case ast::AstNodeKind::UnaryExpression: {
            const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
            if (unary.op() == ast::UnaryOperator::Not) {
                const std::string operand = emitExpression(unary.operand());
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << result << " = xor i1 " << operand << ", true\n";
                return result;
            }
            if (unary.op() == ast::UnaryOperator::Plus) {
                return emitExpression(unary.operand());
            }
            if (unary.op() == ast::UnaryOperator::Minus) {
                if (unary.operand().kind() == ast::AstNodeKind::LiteralExpression) {
                    const auto& literal =
                        static_cast<const ast::LiteralExpression&>(unary.operand());
                    if (literal.literalKind() == ast::LiteralKind::Integer) {
                        return "-" + llvmIntegerLiteral(literal.value());
                    }
                }
                const std::string operand = emitExpression(unary.operand());
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << result << " = sub i64 0, " << operand << '\n';
                return result;
            }
            break;
        }
        case ast::AstNodeKind::CallExpression: {
            const auto& call = static_cast<const ast::CallExpression&>(expression);
            if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
                break;
            }

            const auto& callee =
                static_cast<const ast::IdentifierExpression&>(call.callee());
            if (equalsIgnoreCase(callee.name(), "__member")) {
                const FieldAddress field = emitMemberAddress(call);
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << result << " = load " << field.llvmType
                        << ", ptr " << field.pointer << '\n';
                return result;
            }

            const auto signature = signatures_.find(normalize(callee.name()));
            if (signature == signatures_.end()) {
                break;
            }
            if (signature->second.llvmReturnType != "i64") {
                throw CodegenError(
                    "LLVM emission currently supports calls only to Integer functions");
            }
            if (call.arguments().size() != signature->second.parameters.size()) {
                throw CodegenError(
                    "unsupported call argument count in Integer function: " +
                    function_.name());
            }

            std::vector<std::string> arguments;
            arguments.reserve(call.arguments().size());
            for (const auto& argument : call.arguments()) {
                arguments.push_back(emitExpression(*argument));
            }

            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << result << " = call i64 @" << signature->second.llvmName << '(';
            for (std::size_t index = 0; index < arguments.size(); ++index) {
                if (index != 0) {
                    output_ << ", ";
                }
                output_ << "i64 " << arguments[index];
            }
            output_ << ")\n";
            return result;
        }
        default:
            break;
        }

        throw CodegenError(
            "unsupported expression in Integer function: " + function_.name());
    }

    static std::string llvmOperation(ast::BinaryOperator op)
    {
        switch (op) {
        case ast::BinaryOperator::Add:
            return "add";
        case ast::BinaryOperator::Subtract:
            return "sub";
        case ast::BinaryOperator::Multiply:
            return "mul";
        case ast::BinaryOperator::Divide:
        case ast::BinaryOperator::IntegerDivide:
            return "sdiv";
        case ast::BinaryOperator::Modulo:
            return "srem";
        case ast::BinaryOperator::ShiftLeft:
            return "shl";
        case ast::BinaryOperator::ShiftRight:
            return "ashr";
        case ast::BinaryOperator::BitAnd:
            return "and";
        case ast::BinaryOperator::BitOr:
            return "or";
        case ast::BinaryOperator::BitXor:
            return "xor";
        default:
            throw CodegenError(
                "unsupported Integer operator for LLVM emission");
        }
    }

    static std::string llvmComparisonPredicate(ast::BinaryOperator op)
    {
        switch (op) {
        case ast::BinaryOperator::Equal:
            return "eq";
        case ast::BinaryOperator::NotEqual:
            return "ne";
        case ast::BinaryOperator::Less:
            return "slt";
        case ast::BinaryOperator::Greater:
            return "sgt";
        case ast::BinaryOperator::LessEqual:
            return "sle";
        case ast::BinaryOperator::GreaterEqual:
            return "sge";
        default:
            return {};
        }
    }

    static std::string llvmBooleanOperation(ast::BinaryOperator op)
    {
        switch (op) {
        case ast::BinaryOperator::And:
            return "and";
        case ast::BinaryOperator::Xor:
            return "xor";
        case ast::BinaryOperator::Or:
            return "or";
        default:
            return {};
        }
    }

    std::ostringstream& output_;
    const ast::FunctionDeclaration& function_;
    const FunctionSignature& signature_;
    const FunctionSignatures& signatures_;
    const StructDefinitions& structs_;
    std::vector<std::string>& stringGlobals_;
    std::size_t& nextStringLiteral_;
    std::unordered_map<std::string, std::string> parameters_;
    std::unordered_map<std::string, LocalInfo> locals_;
    std::vector<LoopTargets> loopTargets_;
    std::size_t nextTemporary_ = 0;
    std::size_t nextLabel_ = 0;
};

void emitFunction(std::ostringstream& output,
                  const ast::FunctionDeclaration& function,
                  const FunctionSignature& signature,
                  const FunctionSignatures& signatures,
                  const StructDefinitions& structs,
                  std::vector<std::string>& stringGlobals,
                  std::size_t& nextStringLiteral)
{
    output << "define " << signature.llvmReturnType << " @" << signature.llvmName << '(';
    for (std::size_t index = 0; index < signature.parameters.size(); ++index) {
        if (index != 0) {
            output << ", ";
        }
        output << "i64 %" << signature.parameters[index].llvmName;
    }
    output << ") {\n"
           << "entry:\n";
    FunctionEmitter(output, function, signature, signatures, structs, stringGlobals, nextStringLiteral).emit();
    output << "}\n\n";
}

} // namespace

CodegenError::CodegenError(std::string message)
    : std::runtime_error(std::move(message))
{
}

std::string LlvmIrEmitter::emit(const ast::ModuleNode& module) const
{
    const ast::FunctionDeclaration* mainFunction = nullptr;
    FunctionSignatures signatures;
    StructDefinitions structs;
    std::ostringstream functionOutput;
    std::ostringstream output;
    std::vector<std::string> stringGlobals;
    std::size_t nextStringLiteral = 0;

    for (const auto& item : module.items()) {
        if (item->kind() == ast::AstNodeKind::SectionDeclaration) {
            collectStructDefinitions(static_cast<const ast::SectionDeclaration&>(*item), structs);
        }
    }

    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::FunctionDeclaration) {
            continue;
        }

        const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
        if (equalsIgnoreCase(function.name(), "Main")) {
            mainFunction = &function;
        } else {
            const std::string normalizedName = normalize(function.name());
            signatures.emplace(normalizedName, parseFunctionSignature(function));
        }
    }

    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::FunctionDeclaration) {
            continue;
        }

        const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
        if (!equalsIgnoreCase(function.name(), "Main")) {
            const auto signature = signatures.find(normalize(function.name()));
            emitFunction(functionOutput, function, signature->second, signatures, structs, stringGlobals, nextStringLiteral);
        }
    }

    if (mainFunction == nullptr) {
        throw CodegenError("LLVM emission requires Main()");
    }
    const FunctionSignature mainSignature{"main", "i32", {}};
    emitFunction(functionOutput, *mainFunction, mainSignature, signatures, structs, stringGlobals, nextStringLiteral);

    for (const auto& [_, structType] : structs) {
        output << structType.llvmName << " = type { ";
        for (std::size_t index = 0; index < structType.fields.size(); ++index) {
            if (index != 0) {
                output << ", ";
            }
            output << structType.fields[index].llvmType;
        }
        output << " }\n";
    }
    if (!structs.empty()) {
        output << '\n';
    }

    output << "@.inox.fmt.i64.nl = private unnamed_addr constant [6 x i8] c\"%lld\\0A\\00\"\n"
           << "@.inox.fmt.i64 = private unnamed_addr constant [5 x i8] c\"%lld\\00\"\n"
           << "@.inox.fmt.str.nl = private unnamed_addr constant [4 x i8] c\"%s\\0A\\00\"\n"
           << "@.inox.fmt.str = private unnamed_addr constant [3 x i8] c\"%s\\00\"\n"
           << "@.inox.true = private unnamed_addr constant [5 x i8] c\"true\\00\"\n"
           << "@.inox.false = private unnamed_addr constant [6 x i8] c\"false\\00\"\n";
    for (const std::string& global : stringGlobals) {
        output << global << '\n';
    }
    output << "declare i32 @printf(ptr, ...)\n\n";
    output << functionOutput.str();
    return output.str();
}

} // namespace inox::compiler::codegen
