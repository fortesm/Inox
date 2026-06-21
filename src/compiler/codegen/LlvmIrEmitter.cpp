// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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

std::string llvmDefaultLiteral(std::string_view value, std::string_view llvmType)
{
    if (llvmType == "i1") {
        return equalsIgnoreCase(value, "true") ? "1" : "0";
    }

    return llvmIntegerLiteral(value);
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

struct FunctionParameter {
    std::string inoxName;
    std::string llvmName;
    std::string inoxType;
    std::string llvmType;
};

struct FunctionSignature {
    std::string llvmName;
    std::string llvmReturnType;
    std::vector<FunctionParameter> parameters;
};

using FunctionSignatures =
    std::unordered_map<std::string, FunctionSignature>;

struct StructFieldInfo {
    std::string inoxName;
    std::string llvmType;
    std::size_t index = 0;
    bool hasDefault = false;
    std::string defaultValue;
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
    if (equalsIgnoreCase(inoxType, "Float") || equalsIgnoreCase(inoxType, "Float64")) {
        return "double";
    }
    if (equalsIgnoreCase(inoxType, "Float32")) {
        return "float";
    }
    return {};
}

bool isFloatLlvmType(std::string_view llvmType)
{
    return llvmType == "double" || llvmType == "float";
}

std::string fcmpPredicate(ast::BinaryOperator op)
{
    switch (op) {
    case ast::BinaryOperator::Equal:
        return "oeq";
    case ast::BinaryOperator::NotEqual:
        return "one";
    case ast::BinaryOperator::Less:
        return "olt";
    case ast::BinaryOperator::Greater:
        return "ogt";
    case ast::BinaryOperator::LessEqual:
        return "ole";
    case ast::BinaryOperator::GreaterEqual:
        return "oge";
    default:
        return {};
    }
}

std::string mathIntrinsicName(std::string_view inoxName)
{
    if (equalsIgnoreCase(inoxName, "Sqrt")) return "llvm.sqrt.f64";
    if (equalsIgnoreCase(inoxName, "Sin")) return "llvm.sin.f64";
    if (equalsIgnoreCase(inoxName, "Cos")) return "llvm.cos.f64";
    if (equalsIgnoreCase(inoxName, "Exp")) return "llvm.exp.f64";
    if (equalsIgnoreCase(inoxName, "Ln")) return "llvm.log.f64";
    if (equalsIgnoreCase(inoxName, "Log2")) return "llvm.log2.f64";
    if (equalsIgnoreCase(inoxName, "Log10")) return "llvm.log10.f64";
    if (equalsIgnoreCase(inoxName, "Power")) return "llvm.pow.f64";
    if (equalsIgnoreCase(inoxName, "Floor")) return "llvm.floor.f64";
    if (equalsIgnoreCase(inoxName, "Ceil")) return "llvm.ceil.f64";
    if (equalsIgnoreCase(inoxName, "Abs")) return "llvm.fabs.f64";
    return {};
}

std::string mathLibmName(std::string_view inoxName)
{
    if (equalsIgnoreCase(inoxName, "Cbrt")) return "cbrt";
    if (equalsIgnoreCase(inoxName, "Tan")) return "tan";
    if (equalsIgnoreCase(inoxName, "ArcSin")) return "asin";
    if (equalsIgnoreCase(inoxName, "ArcCos")) return "acos";
    if (equalsIgnoreCase(inoxName, "ArcTan")) return "atan";
    if (equalsIgnoreCase(inoxName, "ArcTan2")) return "atan2";
    if (equalsIgnoreCase(inoxName, "Sinh")) return "sinh";
    if (equalsIgnoreCase(inoxName, "Cosh")) return "cosh";
    if (equalsIgnoreCase(inoxName, "Tanh")) return "tanh";
    if (equalsIgnoreCase(inoxName, "LnXP1")) return "log1p";
    if (equalsIgnoreCase(inoxName, "FMod")) return "fmod";
    if (equalsIgnoreCase(inoxName, "Hypot")) return "hypot";
    return {};
}

bool isMathBuiltin(std::string_view inoxName)
{
    return !mathIntrinsicName(inoxName).empty() ||
           !mathLibmName(inoxName).empty() ||
           equalsIgnoreCase(inoxName, "LogN") ||
           equalsIgnoreCase(inoxName, "Hypot3") ||
           equalsIgnoreCase(inoxName, "RadToDeg") ||
           equalsIgnoreCase(inoxName, "DegToRad") ||
           equalsIgnoreCase(inoxName, "RadToGrad") ||
           equalsIgnoreCase(inoxName, "GradToRad") ||
           equalsIgnoreCase(inoxName, "RadToCycle") ||
           equalsIgnoreCase(inoxName, "CycleToRad");
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

            StructFieldInfo field;
            field.inoxName = fieldName;
            field.llvmType = llvmFieldType;
            field.index = definition.fields.size();
            if (index < tokens.size() && tokens[index] == ":=") {
                ++index;
                if (index >= tokens.size() || tokens[index] == ";") {
                    throw CodegenError("expected default value for struct field: " + fieldName);
                }
                field.hasDefault = true;
                field.defaultValue = tokens[index++];
            }

            definition.fields.push_back(std::move(field));
        }

        if (index >= tokens.size() || tokens[index] != ";") {
            throw CodegenError("expected ';' to close Struct: " + definition.inoxName);
        }
        ++index;
        structs.emplace(normalize(definition.inoxName), std::move(definition));
    }
}


FunctionSignature parseFunctionSignature(const ast::FunctionDeclaration& function,
                                         const StructDefinitions& structs)
{
    const auto& tokens = function.signatureTokens();

    const std::size_t dot = function.name().find('.');
    const bool isAssociatedMethod = dot != std::string::npos;
    const std::string receiverType = isAssociatedMethod ? function.name().substr(0, dot) : std::string{};

    std::vector<FunctionParameter> parameters;
    std::size_t index = 0;
    if (!tokens.empty() && tokens.front() == "(") {
        index = 1;
        if (index < tokens.size() && tokens[index] == ")") {
            throw CodegenError("empty parentheses are not allowed in declarations: " + function.name());
        }

        while (index < tokens.size() && tokens[index] != ")") {
            const std::string parameterName = tokens[index++];
            std::string parameterType;
            bool isReceiver = false;
            if (isAssociatedMethod && parameters.empty() && equalsIgnoreCase(parameterName, "Self")) {
                isReceiver = true;
                if (index < tokens.size() && equalsIgnoreCase(tokens[index], "mut")) {
                    ++index;
                }
                parameterType = receiverType;
            } else {
                if (index >= tokens.size() || tokens[index] == "," || tokens[index] == ")") {
                    throw CodegenError("unsupported function signature: " + function.name());
                }
                parameterType = tokens[index++];
            }

            std::string llvmParameterType = llvmTypeForScalar(parameterType);
            if (llvmParameterType.empty()) {
                const StructDefinition* structType = findStruct(structs, parameterType);
                if (structType == nullptr) {
                    throw CodegenError(
                        "LLVM emission currently supports only scalar and struct parameters");
                }
                llvmParameterType = isReceiver ? "ptr" : structType->llvmName;
            }

            parameters.push_back(FunctionParameter{
                parameterName,
                normalize(parameterName),
                parameterType,
                llvmParameterType});
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
    }

    std::string llvmReturnType;
    if (index == tokens.size()) {
        llvmReturnType = "void";
    } else if (index + 1 != tokens.size()) {
        throw CodegenError(
            "LLVM emission currently supports scalar, struct return types, or subroutines without return type");
    } else if (const std::string scalarReturnType = llvmTypeForScalar(tokens[index]); !scalarReturnType.empty()) {
        llvmReturnType = scalarReturnType;
    } else if (const StructDefinition* structType = findStruct(structs, tokens[index])) {
        llvmReturnType = structType->llvmName;
    } else {
        throw CodegenError(
            "LLVM emission currently supports scalar, struct return types, or subroutines without return type");
    }

    return FunctionSignature{
        "inox_" + normalize(function.name()),
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
        for (const FunctionParameter& parameter : signature.parameters) {
            if (const StructDefinition* structType = findStruct(structs_, parameter.inoxType)) {
                if (parameter.llvmType == "ptr") {
                    locals_.emplace(
                        normalize(parameter.inoxName),
                        LocalInfo{"%" + parameter.llvmName, parameter.inoxType, structType->llvmName});
                } else {
                    const std::string slot = "%" + parameter.llvmName + ".addr";
                    output_ << "  " << slot << " = alloca " << structType->llvmName << "\n";
                    output_ << "  store " << structType->llvmName << " %" << parameter.llvmName
                            << ", ptr " << slot << '\n';
                    locals_.emplace(
                        normalize(parameter.inoxName),
                        LocalInfo{slot, parameter.inoxType, structType->llvmName});
                }
            } else {
                parameters_.emplace(normalize(parameter.inoxName), "%" + parameter.llvmName);
                parameterTypes_.emplace(normalize(parameter.inoxName), parameter.llvmType);
            }
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
        for (const auto& statement : statements) {
            emitLocalDeclaration(*statement);
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
                emitAssignmentOrCallStatement(
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
            emitAssignmentOrCallStatement(
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

    void emitWith(const ast::WithStatement& statement)
    {
        // The target must be an identifier naming an already-declared local.
        if (statement.target().kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenError(
                "LLVM emission currently supports only local variable targets for 'with'");
        }

        const auto& targetIdentifier =
            static_cast<const ast::IdentifierExpression&>(statement.target());
        const auto targetIt = locals_.find(normalize(targetIdentifier.name()));
        if (targetIt == locals_.end()) {
            throw CodegenError(
                "LLVM emission: 'with' target must be a declared local variable: " +
                targetIdentifier.name());
        }

        // Register the synthetic binding as an alias to the same slot/type.
        locals_.emplace(normalize(statement.bindingName()), targetIt->second);

        // Emit body statements; dot-prefixed members were already expanded by
        // the parser to __member(__with_N, Field), which resolves via the alias.
        for (const auto& bodyStatement : statement.body()) {
            emitLocalDeclaration(*bodyStatement);
        }
    }

    void emitLocalDeclaration(const ast::Statement& statement)
    {
        if (statement.kind() == ast::AstNodeKind::VarStatement) {
            const auto& variable = static_cast<const ast::VarStatement&>(statement);
            if (variable.initializer() == nullptr) {
                if (variable.typeName().empty()) {
                    throw CodegenError(
                        "LLVM emission currently requires local variable initialization");
                }
                emitTypedLocalVariable(variable.name(), variable.typeName());
            } else if (!variable.typeName().empty()) {
                emitTypedLocalVariable(variable.name(), variable.typeName(), variable.initializer());
            } else {
                emitLocalVariable(variable.name(), *variable.initializer());
            }
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

        if (statement.kind() == ast::AstNodeKind::WithStatement) {
            emitWith(static_cast<const ast::WithStatement&>(statement));
            return;
        }

        throw CodegenError(
            "LLVM emission currently supports only local variables, assignments, if, while, repeat, for, and with before Return");
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
        const std::string llvmType = expressionLlvmType(initializer);
        const std::string inoxType = llvmType == "double" ? "Float64" :
                                    llvmType == "i1" ? "Bool" : "Integer";
        const std::string slot = "%" + normalizedName;
        output_ << "  " << slot << " = alloca " << llvmType << "\n";
        const std::string value = emitExpression(initializer);
        output_ << "  store " << llvmType << ' ' << value << ", ptr " << slot << '\n';
        locals_.emplace(normalizedName, LocalInfo{slot, inoxType, llvmType});
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
            for (const StructFieldInfo& field : structType->fields) {
                if (!field.hasDefault) {
                    continue;
                }
                const std::string fieldPointer = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << fieldPointer << " = getelementptr " << structType->llvmName
                        << ", ptr " << slot
                        << ", i32 0, i32 " << field.index << '\n';
                output_ << "  store " << field.llvmType << ' '
                        << llvmDefaultLiteral(field.defaultValue, field.llvmType)
                        << ", ptr " << fieldPointer << '\n';
            }
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

    void emitAssignmentOrCallStatement(const ast::Expression& expression)
    {
        if (isAssignmentExpression(expression)) {
            emitLocalAssignment(expression);
            return;
        }
        emitExpressionStatement(expression);
    }

    void emitExpressionStatement(const ast::Expression& expression)
    {
        if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
            if (equalsIgnoreCase(identifier.name(), "Get")) {
                emitInputDiscardToken();
                return;
            }
            if (equalsIgnoreCase(identifier.name(), "GetLn")) {
                emitInputDiscardLine();
                return;
            }
            emitNoArgumentSubroutineCall(identifier.name());
            return;
        }

        if (expression.kind() != ast::AstNodeKind::CallExpression) {
            throw CodegenError(
                "LLVM emission currently supports only assignments and calls as statements");
        }

        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (isMemberAccessCall(call)) {
            emitNoArgumentMethodCall(call, false);
            return;
        }
        if (isMemberAccessCall(call.callee())) {
            emitMethodCall(call, false);
            return;
        }
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenError(
                "LLVM emission currently supports only direct calls or method calls as statements");
        }

        const auto& callee =
            static_cast<const ast::IdentifierExpression&>(call.callee());
        const bool isPut = equalsIgnoreCase(callee.name(), "Put");
        const bool isPutLn = equalsIgnoreCase(callee.name(), "PutLn");
        if (isPut || isPutLn) {
            emitOutputCallSequence(call.arguments(), isPutLn);
            return;
        }

        const bool isGet = equalsIgnoreCase(callee.name(), "Get");
        const bool isGetLn = equalsIgnoreCase(callee.name(), "GetLn");
        if (isGet || isGetLn) {
            emitInputCallSequence(call.arguments(), isGetLn);
            return;
        }

        emitSubroutineCall(call, callee.name());
    }

    void emitNoArgumentSubroutineCall(std::string_view calleeName)
    {
        const auto signature = signatures_.find(normalize(calleeName));
        if (signature == signatures_.end()) {
            throw CodegenError(
                "LLVM emission could not resolve zero-argument subroutine call: " + std::string(calleeName));
        }
        if (!signature->second.parameters.empty()) {
            throw CodegenError(
                "subroutine requires arguments; use parentheses only when passing arguments");
        }
        if (signature->second.llvmReturnType != "void") {
            throw CodegenError(
                "function result cannot be ignored in zero-argument call: " + std::string(calleeName));
        }
        output_ << "  call void @" << signature->second.llvmName << "()\n";
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
            output_ << signature->second.parameters[index].llvmType << ' ' << arguments[index];
        }
        output_ << ")\n";
    }

    void emitInputCallSequence(const std::vector<ast::ExpressionPtr>& arguments, bool consumeRestOfLine)
    {
        if (arguments.empty()) {
            throw CodegenError("Get/GetLn calls with parentheses require at least one argument");
        }

        for (const auto& argument : arguments) {
            emitInputReadInteger(*argument);
        }

        if (consumeRestOfLine) {
            emitInputDiscardLine();
        }
    }

    void emitInputReadInteger(const ast::Expression& argument)
    {
        if (argument.kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenError("Get/GetLn LLVM emission requires assignable local variables");
        }

        const auto& identifier = static_cast<const ast::IdentifierExpression&>(argument);
        const auto local = locals_.find(normalize(identifier.name()));
        if (local == locals_.end()) {
            throw CodegenError("Get/GetLn LLVM emission currently supports only local variables");
        }
        if (local->second.llvmType != "i64") {
            throw CodegenError("Get/GetLn LLVM emission currently supports only Integer/Int64 variables");
        }

        output_ << "  call void @__inox_read_i64(ptr " << local->second.slot << ")\n";
    }

    void emitInputDiscardToken()
    {
        output_ << "  call void @__inox_discard_token()\n";
    }

    void emitInputDiscardLine()
    {
        output_ << "  call void @__inox_discard_line()\n";
    }

    void emitOutputCallSequence(const std::vector<ast::ExpressionPtr>& arguments, bool newline)
    {
        if (arguments.empty()) {
            throw CodegenError("Put/PutLn LLVM emission expects at least one argument");
        }

        for (std::size_t index = 0; index < arguments.size(); ++index) {
            emitOutputCall(*arguments[index], newline && index + 1 == arguments.size());
        }
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

        const std::string argumentType = expressionLlvmType(argument);
        const std::string value = emitExpression(argument);
        if (isFloatLlvmType(argumentType)) {
            output_ << "  call i32 (ptr, ...) @printf(ptr "
                    << (newline ? "@.inox.fmt.f64.nl" : "@.inox.fmt.f64")
                    << ", double " << value << ")\n";
            return;
        }

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

        return expressionLlvmType(expression) == "i1";
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

    struct MethodCallTarget {
        const FunctionSignature* signature = nullptr;
        std::string receiverPointer;
    };

    MethodCallTarget resolveMethodCall(const ast::CallExpression& call)
    {
        if (!isMemberAccessCall(call.callee())) {
            throw CodegenError("LLVM emission expected a method call");
        }

        const auto& member = static_cast<const ast::CallExpression&>(call.callee());
        if (member.arguments().size() != 2 ||
            member.arguments()[0]->kind() != ast::AstNodeKind::IdentifierExpression ||
            member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenError("LLVM emission currently supports only local method calls");
        }

        const auto& receiver =
            static_cast<const ast::IdentifierExpression&>(*member.arguments()[0]);
        const auto& method =
            static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
        const auto local = locals_.find(normalize(receiver.name()));
        if (local == locals_.end()) {
            throw CodegenError("LLVM emission supports method calls only on local struct variables");
        }
        if (findStruct(structs_, local->second.inoxType) == nullptr) {
            throw CodegenError("LLVM emission method receiver is not a struct");
        }

        const std::string qualifiedName = local->second.inoxType + "." + method.name();
        const auto signature = signatures_.find(normalize(qualifiedName));
        if (signature == signatures_.end()) {
            throw CodegenError("unknown method for LLVM emission: " + qualifiedName);
        }
        if (signature->second.parameters.empty() ||
            signature->second.parameters.front().llvmType != "ptr") {
            throw CodegenError("LLVM method emission requires an explicit struct receiver parameter");
        }
        if (call.arguments().size() + 1 != signature->second.parameters.size()) {
            throw CodegenError("unsupported method argument count: " + qualifiedName);
        }

        return MethodCallTarget{&signature->second, local->second.slot};
    }

    MethodCallTarget resolveNoArgumentMethodAccess(const ast::CallExpression& member)
    {
        if (!isMemberAccessCall(member)) {
            throw CodegenError("LLVM emission expected member access");
        }
        if (member.arguments().size() != 2 ||
            member.arguments()[0]->kind() != ast::AstNodeKind::IdentifierExpression ||
            member.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenError("LLVM emission currently supports only local zero-argument method calls");
        }

        const auto& receiver =
            static_cast<const ast::IdentifierExpression&>(*member.arguments()[0]);
        const auto& method =
            static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
        const auto local = locals_.find(normalize(receiver.name()));
        if (local == locals_.end()) {
            throw CodegenError("LLVM emission supports method calls only on local struct variables");
        }
        if (findStruct(structs_, local->second.inoxType) == nullptr) {
            throw CodegenError("LLVM emission method receiver is not a struct");
        }

        const std::string qualifiedName = local->second.inoxType + "." + method.name();
        const auto signature = signatures_.find(normalize(qualifiedName));
        if (signature == signatures_.end()) {
            throw CodegenError("unknown field or zero-argument method for LLVM emission: " + qualifiedName);
        }
        if (signature->second.parameters.size() != 1 ||
            signature->second.parameters.front().llvmType != "ptr") {
            throw CodegenError("LLVM zero-argument method emission requires only an explicit struct receiver parameter");
        }
        return MethodCallTarget{&signature->second, local->second.slot};
    }

    std::string emitNoArgumentMethodCall(const ast::CallExpression& member, bool requireValue)
    {
        const MethodCallTarget target = resolveNoArgumentMethodAccess(member);
        const FunctionSignature& signature = *target.signature;

        if (signature.llvmReturnType == "void") {
            output_ << "  call void @" << signature.llvmName
                    << "(" << signature.parameters.front().llvmType << ' ' << target.receiverPointer << ")\n";
            if (requireValue) {
                throw CodegenError("void method call cannot be used as an expression");
            }
            return {};
        }

        const std::string result = "%tmp" + std::to_string(nextTemporary_++);
        output_ << "  " << result << " = call " << signature.llvmReturnType
                << " @" << signature.llvmName
                << "(" << signature.parameters.front().llvmType << ' ' << target.receiverPointer << ")\n";
        if (requireValue) {
            return result;
        }
        return {};
    }

    std::string emitMethodCall(const ast::CallExpression& call, bool requireValue)
    {
        const MethodCallTarget target = resolveMethodCall(call);
        const FunctionSignature& signature = *target.signature;

        std::vector<std::string> arguments;
        arguments.reserve(call.arguments().size() + 1);
        arguments.push_back(target.receiverPointer);
        for (const auto& argument : call.arguments()) {
            arguments.push_back(emitExpression(*argument));
        }

        if (signature.llvmReturnType == "void") {
            output_ << "  call void @" << signature.llvmName << '(';
        } else {
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << result << " = call " << signature.llvmReturnType
                    << " @" << signature.llvmName << '(';
            for (std::size_t index = 0; index < arguments.size(); ++index) {
                if (index != 0) {
                    output_ << ", ";
                }
                output_ << signature.parameters[index].llvmType << ' ' << arguments[index];
            }
            output_ << ")\n";
            if (requireValue) {
                return result;
            }
            return {};
        }

        for (std::size_t index = 0; index < arguments.size(); ++index) {
            if (index != 0) {
                output_ << ", ";
            }
            output_ << signature.parameters[index].llvmType << ' ' << arguments[index];
        }
        output_ << ")\n";
        if (requireValue) {
            throw CodegenError("void method call cannot be used as an expression");
        }
        return {};
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
                // CANON-5 / A6 / A7: first appearance of `Name := Expr` is an
                // inline declaration with an inferred type. The semantic layer
                // already validated this; here we allocate a fresh local and
                // store the initializer, mirroring emitLocalVariable.
                emitLocalVariable(identifier.name(), assignment.right());
                return;
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

    std::string expressionLlvmType(const ast::Expression& expression) const
    {
        switch (expression.kind()) {
        case ast::AstNodeKind::LiteralExpression: {
            const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
            switch (literal.literalKind()) {
            case ast::LiteralKind::Integer:
                return "i64";
            case ast::LiteralKind::Float:
                return "double";
            case ast::LiteralKind::Boolean:
                return "i1";
            case ast::LiteralKind::String:
                return "ptr";
            case ast::LiteralKind::Char:
                return "i32";
            }
            break;
        }
        case ast::AstNodeKind::IdentifierExpression: {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
            const std::string normalizedName = normalize(identifier.name());
            const auto local = locals_.find(normalizedName);
            if (local != locals_.end()) {
                return local->second.llvmType;
            }
            const auto parameter = parameterTypes_.find(normalizedName);
            if (parameter != parameterTypes_.end()) {
                return parameter->second;
            }
            const auto signature = signatures_.find(normalizedName);
            if (signature != signatures_.end()) {
                return signature->second.llvmReturnType;
            }
            break;
        }
        case ast::AstNodeKind::UnaryExpression: {
            const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
            if (unary.op() == ast::UnaryOperator::Not) {
                return "i1";
            }
            return expressionLlvmType(unary.operand());
        }
        case ast::AstNodeKind::BinaryExpression: {
            const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
            if (llvmComparisonPredicate(binary.op()).size() != 0 || fcmpPredicate(binary.op()).size() != 0) {
                return "i1";
            }
            if (llvmBooleanOperation(binary.op()).size() != 0) {
                return "i1";
            }
            return expressionLlvmType(binary.left());
        }
        case ast::AstNodeKind::CallExpression: {
            const auto& call = static_cast<const ast::CallExpression&>(expression);
            if (call.callee().kind() == ast::AstNodeKind::IdentifierExpression) {
                const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
                if (equalsIgnoreCase(callee.name(), "Abs") && call.arguments().size() == 1) {
                    return expressionLlvmType(*call.arguments().front());
                }
                if (isMathBuiltin(callee.name())) {
                    return "double";
                }
                const auto signature = signatures_.find(normalize(callee.name()));
                if (signature != signatures_.end()) {
                    return signature->second.llvmReturnType;
                }
            }
            if (isMemberAccessCall(call.callee())) {
                const auto& member = static_cast<const ast::CallExpression&>(call.callee());
                if (member.arguments().size() == 2 && member.arguments()[0]) {
                    const std::string receiverType = expressionInoxType(*member.arguments()[0]);
                    if (member.arguments()[1]->kind() == ast::AstNodeKind::IdentifierExpression) {
                        const auto& method = static_cast<const ast::IdentifierExpression&>(*member.arguments()[1]);
                        const auto signature = signatures_.find(normalize(receiverType + "." + method.name()));
                        if (signature != signatures_.end()) {
                            return signature->second.llvmReturnType;
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
        }
        return "i64";
    }

    std::string expressionInoxType(const ast::Expression& expression) const
    {
        switch (expression.kind()) {
        case ast::AstNodeKind::IdentifierExpression: {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
            const std::string normalizedName = normalize(identifier.name());
            const auto local = locals_.find(normalizedName);
            if (local != locals_.end()) {
                return local->second.inoxType;
            }
            break;
        }
        case ast::AstNodeKind::LiteralExpression: {
            const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
            if (literal.literalKind() == ast::LiteralKind::Float) return "Float64";
            if (literal.literalKind() == ast::LiteralKind::Boolean) return "Bool";
            if (literal.literalKind() == ast::LiteralKind::Integer) return "Integer";
            break;
        }
        default:
            break;
        }
        return {};
    }

    std::string emitExpression(const ast::Expression& expression)
    {
        switch (expression.kind()) {
        case ast::AstNodeKind::LiteralExpression: {
            const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
            if (literal.literalKind() == ast::LiteralKind::Integer) {
                return llvmIntegerLiteral(literal.value());
            }
            if (literal.literalKind() == ast::LiteralKind::Float) {
                return std::string(literal.value());
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
                if (local->second.llvmType.empty()) {
                    throw CodegenError("LLVM emission found local value with unknown LLVM type");
                }
                output_ << "  " << result << " = load " << local->second.llvmType
                        << ", ptr " << local->second.slot << '\n';
                return result;
            }
            const auto parameter = parameters_.find(normalizedName);
            if (parameter != parameters_.end()) {
                return parameter->second;
            }
            const auto signature = signatures_.find(normalizedName);
            if (signature != signatures_.end()) {
                if (!signature->second.parameters.empty()) {
                    throw CodegenError("function requires arguments; use parentheses only when passing arguments: " + identifier.name());
                }
                if (signature->second.llvmReturnType == "void") {
                    throw CodegenError("void function call cannot be used as an expression");
                }
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << result << " = call " << signature->second.llvmReturnType
                        << " @" << signature->second.llvmName << "()\n";
                return result;
            }
            break;
        }
        case ast::AstNodeKind::BinaryExpression: {
            const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
            const std::string leftType = expressionLlvmType(binary.left());
            const std::string rightType = expressionLlvmType(binary.right());
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            const std::string left = emitExpression(binary.left());
            const std::string right = emitExpression(binary.right());

            if (isFloatLlvmType(leftType) || isFloatLlvmType(rightType)) {
                if (const std::string predicate = fcmpPredicate(binary.op()); !predicate.empty()) {
                    output_ << "  " << result << " = fcmp " << predicate << ' ' << leftType
                            << ' ' << left << ", " << right << '\n';
                    return result;
                }
                if (binary.op() == ast::BinaryOperator::Power) {
                    output_ << "  " << result << " = call double @llvm.pow.f64(double "
                            << left << ", double " << right << ")\n";
                    return result;
                }
                output_ << "  " << result << " = " << llvmFloatOperation(binary.op()) << ' '
                        << leftType << ' ' << left << ", " << right << '\n';
                return result;
            }

            if (const std::string predicate = llvmComparisonPredicate(binary.op());
                !predicate.empty()) {
                output_ << "  " << result << " = icmp " << predicate << " i64 "
                        << left << ", " << right << '\n';
            } else if (const std::string operation = llvmBooleanOperation(binary.op());
                       !operation.empty()) {
                output_ << "  " << result << " = " << operation << " i1 "
                        << left << ", " << right << '\n';
            } else if (binary.op() == ast::BinaryOperator::Power) {
                output_ << "  " << result << " = call i64 @__inox_ipow_i64(i64 "
                        << left << ", i64 " << right << ")\n";
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
                const std::string operandType = expressionLlvmType(unary.operand());
                if (unary.operand().kind() == ast::AstNodeKind::LiteralExpression) {
                    const auto& literal = static_cast<const ast::LiteralExpression&>(unary.operand());
                    if (literal.literalKind() == ast::LiteralKind::Integer) {
                        return "-" + llvmIntegerLiteral(literal.value());
                    }
                    if (literal.literalKind() == ast::LiteralKind::Float) {
                        return "-" + std::string(literal.value());
                    }
                }
                const std::string operand = emitExpression(unary.operand());
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                if (isFloatLlvmType(operandType)) {
                    output_ << "  " << result << " = fsub " << operandType << " -0.0, " << operand << '\n';
                } else {
                    output_ << "  " << result << " = sub i64 0, " << operand << '\n';
                }
                return result;
            }
            break;
        }
        case ast::AstNodeKind::CallExpression: {
            const auto& call = static_cast<const ast::CallExpression&>(expression);
            if (isMemberAccessCall(call.callee())) {
                return emitMethodCall(call, true);
            }
            if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
                break;
            }

            const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
            if (equalsIgnoreCase(callee.name(), "__member")) {
                try {
                    const FieldAddress field = emitMemberAddress(call);
                    const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                    output_ << "  " << result << " = load " << field.llvmType
                            << ", ptr " << field.pointer << '\n';
                    return result;
                } catch (const CodegenError&) {
                    return emitNoArgumentMethodCall(call, true);
                }
            }

            if (equalsIgnoreCase(callee.name(), "Abs") && call.arguments().size() == 1 &&
                expressionLlvmType(*call.arguments().front()) == "i64") {
                const std::string value = emitExpression(*call.arguments().front());
                const std::string isNegative = "%tmp" + std::to_string(nextTemporary_++);
                const std::string negated = "%tmp" + std::to_string(nextTemporary_++);
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << isNegative << " = icmp slt i64 " << value << ", 0\n";
                output_ << "  " << negated << " = sub i64 0, " << value << '\n';
                output_ << "  " << result << " = select i1 " << isNegative
                        << ", i64 " << negated << ", i64 " << value << '\n';
                return result;
            }

            if (isMathBuiltin(callee.name())) {
                return emitMathBuiltinCall(callee.name(), call.arguments());
            }

            const auto signature = signatures_.find(normalize(callee.name()));
            if (signature == signatures_.end()) {
                break;
            }
            if (signature->second.llvmReturnType == "void") {
                throw CodegenError("void function call cannot be used as an expression");
            }
            if (call.arguments().size() != signature->second.parameters.size()) {
                throw CodegenError("unsupported call argument count in function: " + function_.name());
            }

            std::vector<std::string> arguments;
            arguments.reserve(call.arguments().size());
            for (const auto& argument : call.arguments()) {
                arguments.push_back(emitExpression(*argument));
            }

            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << result << " = call " << signature->second.llvmReturnType
                    << " @" << signature->second.llvmName << '(';
            for (std::size_t index = 0; index < arguments.size(); ++index) {
                if (index != 0) {
                    output_ << ", ";
                }
                output_ << signature->second.parameters[index].llvmType << ' ' << arguments[index];
            }
            output_ << ")\n";
            return result;
        }
        default:
            break;
        }

        throw CodegenError("unsupported expression in function: " + function_.name());
    }

    std::string emitMathBuiltinCall(std::string_view name, const std::vector<ast::ExpressionPtr>& arguments)
    {
        std::vector<std::string> values;
        values.reserve(arguments.size());
        for (const auto& argument : arguments) {
            values.push_back(emitExpression(*argument));
        }
        const std::string result = "%tmp" + std::to_string(nextTemporary_++);

        const auto unaryScale = [&](double factor) {
            output_ << "  " << result << " = fmul double " << values.at(0) << ", " << std::scientific << factor << std::defaultfloat << "\n";
            return result;
        };

        if (equalsIgnoreCase(name, "RadToDeg")) return unaryScale(57.295779513082320876798154814105);
        if (equalsIgnoreCase(name, "DegToRad")) return unaryScale(0.017453292519943295769236907684886);
        if (equalsIgnoreCase(name, "RadToGrad")) return unaryScale(63.661977236758134307553505349005);
        if (equalsIgnoreCase(name, "GradToRad")) return unaryScale(0.015707963267948966192313216916398);
        if (equalsIgnoreCase(name, "RadToCycle")) return unaryScale(0.15915494309189533576888376337251);
        if (equalsIgnoreCase(name, "CycleToRad")) return unaryScale(6.283185307179586476925286766559);

        if (equalsIgnoreCase(name, "LogN")) {
            const std::string logX = "%tmp" + std::to_string(nextTemporary_++);
            const std::string logBase = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << logX << " = call double @llvm.log.f64(double " << values.at(1) << ")\n";
            output_ << "  " << logBase << " = call double @llvm.log.f64(double " << values.at(0) << ")\n";
            output_ << "  " << result << " = fdiv double " << logX << ", " << logBase << "\n";
            return result;
        }
        if (equalsIgnoreCase(name, "Hypot3")) {
            const std::string xy = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << xy << " = call double @hypot(double " << values.at(0) << ", double " << values.at(1) << ")\n";
            output_ << "  " << result << " = call double @hypot(double " << xy << ", double " << values.at(2) << ")\n";
            return result;
        }

        if (const std::string intrinsic = mathIntrinsicName(name); !intrinsic.empty()) {
            output_ << "  " << result << " = call double @" << intrinsic << "(";
        } else if (const std::string libm = mathLibmName(name); !libm.empty()) {
            output_ << "  " << result << " = call double @" << libm << "(";
        } else {
            throw CodegenError("unsupported math builtin: " + std::string(name));
        }
        for (std::size_t index = 0; index < values.size(); ++index) {
            if (index != 0) output_ << ", ";
            output_ << "double " << values[index];
        }
        output_ << ")\n";
        return result;
    }

    static std::string llvmFloatOperation(ast::BinaryOperator op)
    {
        switch (op) {
        case ast::BinaryOperator::Add:
            return "fadd";
        case ast::BinaryOperator::Subtract:
            return "fsub";
        case ast::BinaryOperator::Multiply:
            return "fmul";
        case ast::BinaryOperator::Divide:
            return "fdiv";
        default:
            throw CodegenError("unsupported Float operator for LLVM emission");
        }
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
    std::unordered_map<std::string, std::string> parameterTypes_;
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
        output << signature.parameters[index].llvmType << " %" << signature.parameters[index].llvmName;
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


std::string inputRuntimeHelpers()
{
    return R"llvm(define internal void @__inox_discard_line() {
entry:
  br label %loop

loop:
  %ch = call i32 @getchar()
  %is_newline = icmp eq i32 %ch, 10
  %is_eof = icmp eq i32 %ch, -1
  %done = or i1 %is_newline, %is_eof
  br i1 %done, label %exit, label %loop

exit:
  ret void
}

define internal void @__inox_discard_token() {
entry:
  br label %skip_ws

skip_ws:
  %ch0 = call i32 @getchar()
  %is_eof0 = icmp eq i32 %ch0, -1
  %is_space0 = icmp eq i32 %ch0, 32
  %is_tab0 = icmp eq i32 %ch0, 9
  %is_lf0 = icmp eq i32 %ch0, 10
  %is_cr0 = icmp eq i32 %ch0, 13
  %ws_a0 = or i1 %is_space0, %is_tab0
  %ws_b0 = or i1 %is_lf0, %is_cr0
  %is_ws0 = or i1 %ws_a0, %ws_b0
  %keep_skipping = and i1 %is_ws0, true
  br i1 %is_eof0, label %exit, label %after_eof

after_eof:
  br i1 %keep_skipping, label %skip_ws, label %consume

consume:
  %ch1 = phi i32 [ %ch0, %after_eof ], [ %ch2, %consume_next ]
  %is_eof1 = icmp eq i32 %ch1, -1
  %is_space1 = icmp eq i32 %ch1, 32
  %is_tab1 = icmp eq i32 %ch1, 9
  %is_lf1 = icmp eq i32 %ch1, 10
  %is_cr1 = icmp eq i32 %ch1, 13
  %ws_a1 = or i1 %is_space1, %is_tab1
  %ws_b1 = or i1 %is_lf1, %is_cr1
  %is_ws1 = or i1 %ws_a1, %ws_b1
  %done1a = or i1 %is_eof1, %is_ws1
  br i1 %done1a, label %exit, label %consume_next

consume_next:
  %ch2 = call i32 @getchar()
  br label %consume

exit:
  ret void
}

define internal void @__inox_read_i64(ptr %out) {
entry:
  br label %skip_ws

skip_ws:
  %ch0 = call i32 @getchar()
  %is_eof0 = icmp eq i32 %ch0, -1
  %is_space0 = icmp eq i32 %ch0, 32
  %is_tab0 = icmp eq i32 %ch0, 9
  %is_lf0 = icmp eq i32 %ch0, 10
  %is_cr0 = icmp eq i32 %ch0, 13
  %ws_a0 = or i1 %is_space0, %is_tab0
  %ws_b0 = or i1 %is_lf0, %is_cr0
  %is_ws0 = or i1 %ws_a0, %ws_b0
  br i1 %is_eof0, label %store_zero, label %after_eof

after_eof:
  br i1 %is_ws0, label %skip_ws, label %sign_check

sign_check:
  %is_minus = icmp eq i32 %ch0, 45
  br i1 %is_minus, label %minus, label %digits_entry

minus:
  %ch_after_minus = call i32 @getchar()
  br label %digits_entry

digits_entry:
  %sign = phi i64 [ -1, %minus ], [ 1, %sign_check ]
  %first_ch = phi i32 [ %ch_after_minus, %minus ], [ %ch0, %sign_check ]
  br label %digits

digits:
  %ch = phi i32 [ %first_ch, %digits_entry ], [ %next_ch, %digit_body ]
  %acc = phi i64 [ 0, %digits_entry ], [ %next_acc, %digit_body ]
  %ge_zero = icmp sge i32 %ch, 48
  %le_nine = icmp sle i32 %ch, 57
  %is_digit = and i1 %ge_zero, %le_nine
  br i1 %is_digit, label %digit_body, label %finish

digit_body:
  %digit_i32 = sub i32 %ch, 48
  %digit_i64 = sext i32 %digit_i32 to i64
  %mul = mul i64 %acc, 10
  %next_acc = add i64 %mul, %digit_i64
  %next_ch = call i32 @getchar()
  br label %digits

finish:
  %signed = mul i64 %acc, %sign
  store i64 %signed, ptr %out
  ret void

store_zero:
  store i64 0, ptr %out
  ret void
}

)llvm";
}

std::string mathRuntimeHelpers()
{
    return R"llvm(declare { i64, i1 } @llvm.smul.with.overflow.i64(i64, i64)
declare void @llvm.trap()

define internal i64 @__inox_ipow_i64(i64 %base, i64 %exponent) {
entry:
  %negative = icmp slt i64 %exponent, 0
  br i1 %negative, label %trap, label %loop

loop:
  %result.cur = phi i64 [ 1, %entry ], [ %result.next, %continue ]
  %base.cur = phi i64 [ %base, %entry ], [ %base.next, %continue ]
  %exp.cur = phi i64 [ %exponent, %entry ], [ %exp.next, %continue ]
  %done = icmp eq i64 %exp.cur, 0
  br i1 %done, label %exit, label %body

body:
  %lowbit = and i64 %exp.cur, 1
  %is_odd = icmp ne i64 %lowbit, 0
  br i1 %is_odd, label %mul_result, label %after_mul_result

mul_result:
  %mul.result.pair = call { i64, i1 } @llvm.smul.with.overflow.i64(i64 %result.cur, i64 %base.cur)
  %mul.result = extractvalue { i64, i1 } %mul.result.pair, 0
  %mul.result.overflow = extractvalue { i64, i1 } %mul.result.pair, 1
  br i1 %mul.result.overflow, label %trap, label %after_mul_result

after_mul_result:
  %result.after = phi i64 [ %mul.result, %mul_result ], [ %result.cur, %body ]
  %exp.next = ashr i64 %exp.cur, 1
  %need_square = icmp ne i64 %exp.next, 0
  br i1 %need_square, label %square_base, label %continue

square_base:
  %square.pair = call { i64, i1 } @llvm.smul.with.overflow.i64(i64 %base.cur, i64 %base.cur)
  %square = extractvalue { i64, i1 } %square.pair, 0
  %square.overflow = extractvalue { i64, i1 } %square.pair, 1
  br i1 %square.overflow, label %trap, label %continue

continue:
  %result.next = phi i64 [ %result.after, %after_mul_result ], [ %result.after, %square_base ]
  %base.next = phi i64 [ %base.cur, %after_mul_result ], [ %square, %square_base ]
  br label %loop

exit:
  ret i64 %result.cur

trap:
  call void @llvm.trap()
  unreachable
}

)llvm";
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
            signatures.emplace(normalizedName, parseFunctionSignature(function, structs));
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
        throw CodegenError("LLVM emission requires Main");
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
           << "@.inox.fmt.f64.nl = private unnamed_addr constant [4 x i8] c\"%f\\0A\\00\"\n"
           << "@.inox.fmt.f64 = private unnamed_addr constant [3 x i8] c\"%f\\00\"\n"
           << "@.inox.fmt.str.nl = private unnamed_addr constant [4 x i8] c\"%s\\0A\\00\"\n"
           << "@.inox.fmt.str = private unnamed_addr constant [3 x i8] c\"%s\\00\"\n"
           << "@.inox.true = private unnamed_addr constant [5 x i8] c\"true\\00\"\n"
           << "@.inox.false = private unnamed_addr constant [6 x i8] c\"false\\00\"\n";
    for (const std::string& global : stringGlobals) {
        output << global << '\n';
    }
    output << "declare i32 @printf(ptr, ...)\n";
    output << "declare i32 @getchar()\n";
    output << "declare double @llvm.sqrt.f64(double)\n";
    output << "declare double @llvm.sin.f64(double)\n";
    output << "declare double @llvm.cos.f64(double)\n";
    output << "declare double @llvm.exp.f64(double)\n";
    output << "declare double @llvm.log.f64(double)\n";
    output << "declare double @llvm.log2.f64(double)\n";
    output << "declare double @llvm.log10.f64(double)\n";
    output << "declare double @llvm.pow.f64(double, double)\n";
    output << "declare double @llvm.floor.f64(double)\n";
    output << "declare double @llvm.ceil.f64(double)\n";
    output << "declare double @llvm.fabs.f64(double)\n";
    output << "declare double @cbrt(double)\n";
    output << "declare double @tan(double)\n";
    output << "declare double @asin(double)\n";
    output << "declare double @acos(double)\n";
    output << "declare double @atan(double)\n";
    output << "declare double @atan2(double, double)\n";
    output << "declare double @sinh(double)\n";
    output << "declare double @cosh(double)\n";
    output << "declare double @tanh(double)\n";
    output << "declare double @log1p(double)\n";
    output << "declare double @fmod(double, double)\n";
    output << "declare double @hypot(double, double)\n\n";
    output << inputRuntimeHelpers();
    output << mathRuntimeHelpers();
    output << functionOutput.str();
    return output.str();
}

} // namespace inox::compiler::codegen
