// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "LlvmIrEmitter.h"
#include "../support/Platform.h"
#include "../exceptions/ExceptionTypes.h"

#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
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

std::uint64_t exceptionTypeId(std::string_view name)
{
    const auto* info = exceptions::findExceptionType(name);
    if (info == nullptr) {
        throw CodegenUnsupported("unknown exception type for LLVM emission: " + std::string(name));
    }
    return info->typeId;
}

bool statementContainsTry(const ast::Statement& statement);

bool statementsContainTry(const std::vector<ast::StatementPtr>& statements)
{
    for (const auto& statement : statements) {
        if (statementContainsTry(*statement)) return true;
    }
    return false;
}

bool statementContainsTry(const ast::Statement& statement)
{
    switch (statement.kind()) {
    case ast::AstNodeKind::TryStatement:
        return true;
    case ast::AstNodeKind::BlockStatement:
        return statementsContainTry(static_cast<const ast::BlockStatement&>(statement).statements());
    case ast::AstNodeKind::IfStatement: {
        const auto& node = static_cast<const ast::IfStatement&>(statement);
        if (statementsContainTry(node.thenBody()) || statementsContainTry(node.elseBody())) return true;
        for (const auto& clause : node.elseIfClauses()) if (statementsContainTry(clause.body)) return true;
        return false;
    }
    case ast::AstNodeKind::UnlessStatement:
        return statementsContainTry(static_cast<const ast::UnlessStatement&>(statement).body());
    case ast::AstNodeKind::WhileStatement:
        return statementsContainTry(static_cast<const ast::WhileStatement&>(statement).body());
    case ast::AstNodeKind::RepeatStatement:
        return statementsContainTry(static_cast<const ast::RepeatStatement&>(statement).body());
    case ast::AstNodeKind::ForInStatement:
        return statementsContainTry(static_cast<const ast::ForInStatement&>(statement).body());
    case ast::AstNodeKind::CaseStatement: {
        const auto& node = static_cast<const ast::CaseStatement&>(statement);
        for (const auto& arm : node.arms()) if (statementsContainTry(arm.body)) return true;
        return statementsContainTry(node.otherwiseBody());
    }
    case ast::AstNodeKind::WithStatement:
        return statementsContainTry(static_cast<const ast::WithStatement&>(statement).body());
    default:
        return false;
    }
}

bool functionContainsTry(const ast::FunctionDeclaration& function)
{
    return statementsContainTry(function.body());
}

bool statementUsesExceptions(const ast::Statement& statement)
{
    if (statement.kind() == ast::AstNodeKind::TryStatement ||
        statement.kind() == ast::AstNodeKind::RaiseStatement ||
        statement.kind() == ast::AstNodeKind::RetryStatement) return true;
    switch (statement.kind()) {
    case ast::AstNodeKind::BlockStatement:
        for (const auto& s : static_cast<const ast::BlockStatement&>(statement).statements()) if (statementUsesExceptions(*s)) return true;
        break;
    case ast::AstNodeKind::IfStatement: {
        const auto& n = static_cast<const ast::IfStatement&>(statement);
        for (const auto& s : n.thenBody()) if (statementUsesExceptions(*s)) return true;
        for (const auto& c : n.elseIfClauses()) for (const auto& s : c.body) if (statementUsesExceptions(*s)) return true;
        for (const auto& s : n.elseBody()) if (statementUsesExceptions(*s)) return true;
        break;
    }
    case ast::AstNodeKind::UnlessStatement:
        for (const auto& s : static_cast<const ast::UnlessStatement&>(statement).body()) if (statementUsesExceptions(*s)) return true;
        break;
    case ast::AstNodeKind::WhileStatement:
        for (const auto& s : static_cast<const ast::WhileStatement&>(statement).body()) if (statementUsesExceptions(*s)) return true;
        break;
    case ast::AstNodeKind::RepeatStatement:
        for (const auto& s : static_cast<const ast::RepeatStatement&>(statement).body()) if (statementUsesExceptions(*s)) return true;
        break;
    case ast::AstNodeKind::ForInStatement:
        for (const auto& s : static_cast<const ast::ForInStatement&>(statement).body()) if (statementUsesExceptions(*s)) return true;
        break;
    case ast::AstNodeKind::CaseStatement: {
        const auto& n = static_cast<const ast::CaseStatement&>(statement);
        for (const auto& a : n.arms()) for (const auto& s : a.body) if (statementUsesExceptions(*s)) return true;
        for (const auto& s : n.otherwiseBody()) if (statementUsesExceptions(*s)) return true;
        break;
    }
    case ast::AstNodeKind::WithStatement:
        for (const auto& s : static_cast<const ast::WithStatement&>(statement).body()) if (statementUsesExceptions(*s)) return true;
        break;
    default: break;
    }
    return false;
}

bool moduleUsesExceptions(const ast::ModuleNode& module)
{
    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::FunctionDeclaration) continue;
        const auto& fn = static_cast<const ast::FunctionDeclaration&>(*item);
        for (const auto& st : fn.body()) if (statementUsesExceptions(*st)) return true;
    }
    return false;
}

// LLVM shares one namespace between local values and basic-block labels, and the
// emitter's own temporaries (`%tmpN`) and labels (`thenN`, `forcondN`, ...) always
// end in a digit. A user identifier must therefore never be emitted verbatim when
// it could collide with one of those names.
std::string safeLlvmName(const std::string& name)
{
    if (name.empty()) {
        return name;
    }
    const bool endsWithDigit = std::isdigit(static_cast<unsigned char>(name.back())) != 0;
    const bool reserved = name == "entry" || (name.size() >= 2 && name[0] == '_' && name[1] == '_');
    return (endsWithDigit || reserved) ? name + ".v" : name;
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
                throw CodegenUnsupported(
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
                    throw CodegenUnsupported("unsupported function signature: " + function.name());
                }
                parameterType = tokens[index++];
            }

            std::string llvmParameterType = llvmTypeForScalar(parameterType);
            if (llvmParameterType.empty()) {
                const StructDefinition* structType = findStruct(structs, parameterType);
                if (structType == nullptr) {
                    throw CodegenUnsupported(
                        "LLVM emission currently supports only scalar and struct parameters");
                }
                llvmParameterType = isReceiver ? "ptr" : structType->llvmName;
            }

            parameters.push_back(FunctionParameter{
                parameterName,
                safeLlvmName(normalize(parameterName)),
                parameterType,
                llvmParameterType});
            if (index < tokens.size() && tokens[index] == ",") {
                ++index;
            } else if (index >= tokens.size() || tokens[index] != ")") {
                throw CodegenUnsupported("unsupported function signature: " + function.name());
            }
        }

        if (index >= tokens.size() || tokens[index] != ")") {
            throw CodegenUnsupported("unsupported function signature: " + function.name());
        }
        ++index;
    }

    std::string llvmReturnType;
    if (index == tokens.size()) {
        llvmReturnType = "void";
    } else if (index + 1 != tokens.size()) {
        throw CodegenUnsupported(
            "LLVM emission currently supports scalar, struct return types, or subroutines without return type");
    } else if (const std::string scalarReturnType = llvmTypeForScalar(tokens[index]); !scalarReturnType.empty()) {
        llvmReturnType = scalarReturnType;
    } else if (const StructDefinition* structType = findStruct(structs, tokens[index])) {
        llvmReturnType = structType->llvmName;
    } else {
        throw CodegenUnsupported(
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
                    std::size_t& nextStringLiteral,
                    const semantic::SemanticResult& semantics)
        : semantics_(semantics),
          output_(output),
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

        if (signature_.llvmReturnType != "void" && signature_.llvmReturnType != "i32") {
            returnValueSlot_ = "%__inox.return.value";
            output_ << "  " << returnValueSlot_ << " = alloca " << signature_.llvmReturnType << "\n";
        }
    }

    void emit()
    {
        if (signature_.llvmReturnType == "i32" || signature_.llvmReturnType == "void") {
            for (const auto& statement : function_.body()) {
                emitStatement(*statement);
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
            throw CodegenUnsupported(
                "LLVM emission currently requires a final Return expression");
        }

        for (std::size_t index = 0; index + 1 < function_.body().size(); ++index) {
            emitStatement(*function_.body()[index]);
        }

        const auto& returnStatement =
            static_cast<const ast::ReturnStatement&>(*function_.body().back());
        const std::string value = emitExpression(returnStatement.expression());
        output_ << "  ret " << signature_.llvmReturnType << ' ' << value << '\n';
    }

private:
    const semantic::SemanticResult& semantics_;
    struct LoopTargets {
        std::string continueTarget;
        std::string leaveTarget;
    };

    struct RetryContext {
        std::string counterSlot;
        std::string actionSlot;
        std::string cleanupTarget;
        std::string rethrowRequestTarget;
    };

    struct CleanupContext {
        std::string actionSlot;
        std::string returnRequestTarget;
        std::string exitRequestTarget;
        std::string leaveRequestTarget;
        std::string continueRequestTarget;
        std::size_t loopDepthAtEntry = 0;
        std::string leaveDestination;
        std::string continueDestination;
        // `until` exits the nearest repeat enclosing this try, possibly across
        // intermediate loops. Empty when the try is not inside a repeat.
        std::string untilRequestTarget;
        std::size_t untilTargetDepth = 0;
        std::string untilDestination;
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
            throw CodegenUnsupported(
                "LLVM emission currently supports if/else only in Integer functions");
        }
        if (statement.elseBody().empty()) {
            throw CodegenUnsupported(
                "LLVM emission currently requires else for direct-return if chains");
        }
        if (statement.thenBody().size() != 1 ||
            statement.thenBody().front()->kind() != ast::AstNodeKind::ReturnStatement ||
            statement.elseBody().size() != 1 ||
            statement.elseBody().front()->kind() != ast::AstNodeKind::ReturnStatement) {
            throw CodegenUnsupported(
                "LLVM emission currently requires a single Return in each if branch");
        }
        for (const auto& clause : statement.elseIfClauses()) {
            if (clause.body.size() != 1 ||
                clause.body.front()->kind() != ast::AstNodeKind::ReturnStatement) {
                throw CodegenUnsupported(
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
        if (!cleanupContexts_.empty()) {
            if (returnValueSlot_.empty()) {
                throw CodegenError("internal error: Return cleanup requires a return-value slot");
            }
            output_ << "  store " << signature_.llvmReturnType << ' ' << value
                    << ", ptr " << returnValueSlot_ << "\n";
            output_ << "  br label %" << cleanupContexts_.back().returnRequestTarget << "\n";
            const std::string dead = newDeadLabel("eh.after.return");
            output_ << "\n" << dead << ":\n";
            return;
        }
        output_ << "  ret " << signature_.llvmReturnType << ' ' << value << '\n';
    }

    void emitExit()
    {
        if (!cleanupContexts_.empty()) {
            output_ << "  br label %" << cleanupContexts_.back().exitRequestTarget << "\n";
            const std::string dead = newDeadLabel("eh.after.exit");
            output_ << "\n" << dead << ":\n";
            return;
        }
        if (signature_.llvmReturnType == "i32") {
            output_ << "  ret i32 0\n";
        } else if (signature_.llvmReturnType == "void") {
            output_ << "  ret void\n";
        } else {
            throw CodegenError("Exit is not valid in a value-returning function");
        }
    }

    const CleanupContext* cleanupForLoopTransfer() const
    {
        if (loopTargets_.empty()) {
            return nullptr;
        }
        const std::size_t targetDepth = loopTargets_.size();
        for (auto it = cleanupContexts_.rbegin(); it != cleanupContexts_.rend(); ++it) {
            if (it->loopDepthAtEntry >= targetDepth) {
                return &*it;
            }
        }
        return nullptr;
    }

    void emitLeaveTransfer()
    {
        if (loopTargets_.empty()) {
            throw CodegenError("leave outside loop");
        }
        if (const CleanupContext* cleanup = cleanupForLoopTransfer()) {
            output_ << "  br label %" << cleanup->leaveRequestTarget << "\n";
        } else {
            output_ << "  br label %" << currentLoopTargets().leaveTarget << "\n";
        }
        const std::string dead = newDeadLabel("eh.after.leave");
        output_ << "\n" << dead << ":\n";
    }

    void emitContinueTransfer()
    {
        if (loopTargets_.empty()) {
            throw CodegenError("continue outside loop");
        }
        if (const CleanupContext* cleanup = cleanupForLoopTransfer()) {
            output_ << "  br label %" << cleanup->continueRequestTarget << "\n";
        } else {
            output_ << "  br label %" << currentLoopTargets().continueTarget << "\n";
        }
        const std::string dead = newDeadLabel("eh.after.continue");
        output_ << "\n" << dead << ":\n";
    }

    void emitIfMerge(const ast::IfStatement& statement)
    {
        // if / elif* / else? lowered as a chain of conditional branches that all
        // join at one merge block. Each body is lowered by the general statement
        // dispatcher, so any statement (loops, try, leave, continue, nested if)
        // composes inside any branch.
        const std::size_t label = nextLabel_++;
        const std::string endTarget = "endif" + std::to_string(label);
        const auto& clauses = statement.elseIfClauses();
        const bool hasElse = !statement.elseBody().empty();

        auto emitArm = [&](const ast::Expression& condition,
                           const std::vector<ast::StatementPtr>& body,
                           std::size_t armIndex,
                           bool isLastConditionalArm) {
            const std::string suffix = std::to_string(label) + "_" + std::to_string(armIndex);
            const std::string thenTarget = armIndex == 0 ? "then" + std::to_string(label) : "elifthen" + suffix;
            const std::string nextTarget = isLastConditionalArm
                ? (hasElse ? "else" + std::to_string(label) : endTarget)
                : "elifcond" + std::to_string(label) + "_" + std::to_string(armIndex + 1);
            const std::string value = emitExpression(condition);
            output_ << "  br i1 " << value << ", label %" << thenTarget
                    << ", label %" << nextTarget << "\n\n";
            output_ << thenTarget << ":\n";
            emitAssignmentBranch(body);
            output_ << "  br label %" << endTarget << "\n\n";
            if (!isLastConditionalArm) {
                output_ << nextTarget << ":\n";
            }
        };

        emitArm(statement.condition(), statement.thenBody(), 0, clauses.empty());
        for (std::size_t index = 0; index < clauses.size(); ++index) {
            emitArm(*clauses[index].condition, clauses[index].body, index + 1,
                    index + 1 == clauses.size());
        }

        if (hasElse) {
            output_ << "else" << label << ":\n";
            emitAssignmentBranch(statement.elseBody());
            output_ << "  br label %" << endTarget << "\n\n";
        }

        output_ << endTarget << ":\n";
    }

    void emitAssignmentBranch(const std::vector<ast::StatementPtr>& statements)
    {
        for (const auto& statement : statements) {
            emitStatement(*statement);
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
        repeatLoopDepths_.push_back(loopTargets_.size());
        const bool terminated = emitLoopStatements(statement.body());
        repeatLoopDepths_.pop_back();
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
            throw CodegenUnsupported(
                "LLVM emission currently supports only range expressions in for loops");
        }

        const auto& range = static_cast<const ast::BinaryExpression&>(statement.iterable());
        if (range.op() != ast::BinaryOperator::Range) {
            throw CodegenUnsupported(
                "LLVM emission currently supports only Start..End for ranges");
        }

        const std::string iteratorName = normalize(statement.iterator());
        // Semantic analysis already rejected iterators that conflict with a visible
        // symbol; an entry left in `locals_` here belongs to a finished sibling scope.
        std::optional<LocalInfo> shadowedLocal;
        if (const auto existing = locals_.find(iteratorName); existing != locals_.end()) {
            shadowedLocal = existing->second;
        }

        const std::size_t label = nextLabel_++;
        const std::string conditionTarget = "forcond" + std::to_string(label);
        const std::string bodyTarget = "forbody" + std::to_string(label);
        const std::string stepTarget = "forstep" + std::to_string(label);
        const std::string endTarget = "forend" + std::to_string(label);
        const std::string slot = newSlot(iteratorName);

        auto temporary = [this]() { return "%tmp" + std::to_string(nextTemporary_++); };

        // CANON-11 `for in range`: both endpoints and the step are evaluated ONCE,
        // before the first iteration (start, end, step order). The direction comes
        // from the endpoints: A<B ascending, A>B descending, A=B runs once. The step
        // is a positive magnitude; a step <= 0 traps before the loop starts.
        output_ << "  " << slot << " = alloca i64\n";
        const std::string startValue = emitExpression(range.left());
        const std::string endValue = emitExpression(range.right());
        std::string increment = "1";
        if (statement.step() != nullptr) {
            const std::string rawStep = emitExpression(*statement.step());
            increment = temporary();
            output_ << "  " << increment << " = call i64 @__inox_for_step_i64(i64 " << rawStep << ")\n";
        }
        const std::string descending = temporary();
        output_ << "  " << descending << " = icmp sgt i64 " << startValue << ", " << endValue << '\n';
        output_ << "  store i64 " << startValue << ", ptr " << slot << '\n';
        locals_.insert_or_assign(iteratorName, LocalInfo{slot, "Integer", "i64"});

        output_ << "  br label %" << conditionTarget << "\n\n";

        output_ << conditionTarget << ":\n";
        const std::string iteratorValue = temporary();
        const std::string ascendingCondition = temporary();
        const std::string descendingCondition = temporary();
        const std::string condition = temporary();
        output_ << "  " << iteratorValue << " = load i64, ptr " << slot << '\n';
        output_ << "  " << ascendingCondition << " = icmp sle i64 " << iteratorValue << ", " << endValue << '\n';
        output_ << "  " << descendingCondition << " = icmp sge i64 " << iteratorValue << ", " << endValue << '\n';
        output_ << "  " << condition << " = select i1 " << descending << ", i1 " << descendingCondition
                << ", i1 " << ascendingCondition << '\n';
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

        // The next iterator value is computed with overflow detection: if it does
        // not fit in Int64 the range is exhausted, so the loop ends instead of
        // wrapping around (a range ending at Int64.Max or Int64.Min must terminate).
        output_ << stepTarget << ":\n";
        const std::string current = temporary();
        const std::string addPair = temporary();
        const std::string addValue = temporary();
        const std::string addOverflow = temporary();
        const std::string subPair = temporary();
        const std::string subValue = temporary();
        const std::string subOverflow = temporary();
        const std::string nextValue = temporary();
        const std::string nextOverflow = temporary();
        output_ << "  " << current << " = load i64, ptr " << slot << '\n';
        output_ << "  " << addPair << " = call { i64, i1 } @llvm.sadd.with.overflow.i64(i64 "
                << current << ", i64 " << increment << ")\n";
        output_ << "  " << addValue << " = extractvalue { i64, i1 } " << addPair << ", 0\n";
        output_ << "  " << addOverflow << " = extractvalue { i64, i1 } " << addPair << ", 1\n";
        output_ << "  " << subPair << " = call { i64, i1 } @llvm.ssub.with.overflow.i64(i64 "
                << current << ", i64 " << increment << ")\n";
        output_ << "  " << subValue << " = extractvalue { i64, i1 } " << subPair << ", 0\n";
        output_ << "  " << subOverflow << " = extractvalue { i64, i1 } " << subPair << ", 1\n";
        output_ << "  " << nextValue << " = select i1 " << descending << ", i64 " << subValue
                << ", i64 " << addValue << '\n';
        output_ << "  " << nextOverflow << " = select i1 " << descending << ", i1 " << subOverflow
                << ", i1 " << addOverflow << '\n';
        output_ << "  store i64 " << nextValue << ", ptr " << slot << '\n';
        output_ << "  br i1 " << nextOverflow << ", label %" << endTarget
                << ", label %" << conditionTarget << "\n\n";

        output_ << endTarget << ":\n";
        if (shadowedLocal.has_value()) {
            locals_.insert_or_assign(iteratorName, *shadowedLocal);
        } else {
            locals_.erase(iteratorName);
        }
    }

    bool emitLoopStatements(const std::vector<ast::StatementPtr>& statements)
    {
        // Loop bodies are ordinary blocks: every statement goes through the
        // general dispatcher. leave/continue end the current block and open an
        // unreachable continuation label, so the caller may always close the body
        // with a branch back to the loop header.
        for (const auto& statement : statements) {
            emitStatement(*statement);
        }
        return false;
    }

    // CANON-11 `until Condition` exits the nearest repeat when true, wherever it
    // appears in the repeat body: also inside if/elif/else, try, and loops nested
    // in the repeat. It is a transfer to an explicit target (the repeat), not to
    // the innermost loop: every ensure between the until and that repeat runs,
    // innermost first, and no other.
    void emitUntil(const ast::UntilStatement& statement)
    {
        if (repeatLoopDepths_.empty()) {
            throw CodegenError("until outside repeat");
        }
        const std::size_t targetDepth = repeatLoopDepths_.back();
        const std::size_t label = nextLabel_++;
        const std::string exitTarget = "untilexit" + std::to_string(label);
        const std::string nextTarget = "untilnext" + std::to_string(label);
        const std::string condition = emitExpression(statement.condition());
        output_ << "  br i1 " << condition << ", label %" << exitTarget
                << ", label %" << nextTarget << "\n\n";
        output_ << exitTarget << ":\n";
        const CleanupContext* crossed = nullptr;
        for (auto it = cleanupContexts_.rbegin(); it != cleanupContexts_.rend(); ++it) {
            if (it->loopDepthAtEntry >= targetDepth) {
                crossed = &*it;
                break;
            }
        }
        if (crossed != nullptr) {
            output_ << "  br label %" << crossed->untilRequestTarget << "\n\n";
        } else {
            output_ << "  br label %" << loopTargets_[targetDepth - 1].leaveTarget << "\n\n";
        }
        output_ << nextTarget << ":\n";
    }

    const LoopTargets& currentLoopTargets() const
    {
        if (loopTargets_.empty()) {
            throw CodegenError(
                "LLVM emission supports leave and continue only inside loops");
        }
        return loopTargets_.back();
    }

    void emitWith(const ast::WithStatement& statement)
    {
        // The target must be an identifier naming an already-declared local.
        if (statement.target().kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenUnsupported(
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
        locals_.insert_or_assign(normalize(statement.bindingName()), targetIt->second);

        // Emit body statements; dot-prefixed members were already expanded by
        // the parser to __member(__with_N, Field), which resolves via the alias.
        for (const auto& bodyStatement : statement.body()) {
            emitStatement(*bodyStatement);
        }
    }

    std::string emitExceptionTypeMatch(std::string_view actualTypeValue,
                                       std::string_view expectedTypeName)
    {
        const auto ids = exceptions::matchingTypeIds(expectedTypeName);
        if (ids.empty()) {
            throw CodegenUnsupported(
                "unknown exception type for handler matching: " +
                std::string(expectedTypeName));
        }

        std::string combined;
        for (const std::uint64_t id : ids) {
            const std::string cmp = "%eh.match" + std::to_string(nextTemporary_++);
            output_ << "  " << cmp << " = icmp eq i64 " << actualTypeValue
                    << ", " << id << "\n";
            if (combined.empty()) {
                combined = cmp;
            } else {
                const std::string joined = "%eh.match.any" + std::to_string(nextTemporary_++);
                output_ << "  " << joined << " = or i1 " << combined << ", " << cmp << "\n";
                combined = joined;
            }
        }
        return combined;
    }

    void emitRetry(const ast::RetryStatement& statement)
    {
        if (retryContexts_.empty()) {
            throw CodegenError("Retry requires an active On/Else exception handler");
        }

        const RetryContext& context = retryContexts_.back();
        const std::string limit = emitExpression(statement.count());
        const std::string current = "%eh.retry.count" + std::to_string(nextTemporary_++);
        const std::string allowed = "%eh.retry.allowed" + std::to_string(nextTemporary_++);
        const std::string allowedLabel = "eh.retry.allow" + std::to_string(nextLabel_++);
        const std::string exhaustedLabel = "eh.retry.exhausted" + std::to_string(nextLabel_++);
        const std::string deadLabel = newDeadLabel("eh.after.retry");

        output_ << "  " << current << " = load i64, ptr " << context.counterSlot << "\n";
        // Signed comparison intentionally makes a dynamic negative N behave as
        // an exhausted budget. Semantic analysis rejects literal negatives.
        output_ << "  " << allowed << " = icmp slt i64 " << current << ", " << limit << "\n";
        output_ << "  br i1 " << allowed << ", label %" << allowedLabel
                << ", label %" << exhaustedLabel << "\n\n";

        output_ << allowedLabel << ":\n";
        const std::string next = "%eh.retry.next" + std::to_string(nextTemporary_++);
        output_ << "  " << next << " = add i64 " << current << ", 1\n";
        output_ << "  store i64 " << next << ", ptr " << context.counterSlot << "\n";
        output_ << "  store i32 2, ptr " << context.actionSlot << "\n";
        output_ << "  br label %" << context.cleanupTarget << "\n\n";

        output_ << exhaustedLabel << ":\n";
        output_ << "  br label %" << context.rethrowRequestTarget << "\n\n";

        output_ << deadLabel << ":\n";
    }

    const std::string& currentUnwindTarget() const
    {
        if (unwindTargets_.empty()) {
            throw CodegenError("internal error: no active exception unwind target");
        }
        return unwindTargets_.back();
    }

    std::string newDeadLabel(std::string_view prefix)
    {
        return std::string(prefix) + std::to_string(nextLabel_++);
    }

    void emitExceptionCapture(std::string_view landingPadLabel,
                              std::string_view stateSlot,
                              std::string_view nextLabel,
                              bool replaceExisting = false)
    {
        output_ << landingPadLabel << ":\n";
        const std::string landing = "%eh.lp" + std::to_string(nextTemporary_++);
        const std::string raw = "%eh.raw" + std::to_string(nextTemporary_++);
        const std::string state = "%eh.state" + std::to_string(nextTemporary_++);
        output_ << "  " << landing << " = landingpad { ptr, i32 } catch ptr null\n";
        output_ << "  " << raw << " = extractvalue { ptr, i32 } " << landing << ", 0\n";
        output_ << "  " << state << " = call ptr @__inox_exception_capture(ptr " << raw << ")\n";
        if (replaceExisting) {
            const std::string old = "%eh.old" + std::to_string(nextTemporary_++);
            const std::string hasOld = "%eh.hasold" + std::to_string(nextTemporary_++);
            const std::string releaseLabel = "eh.release.old" + std::to_string(nextLabel_++);
            const std::string storeLabel = "eh.store.new" + std::to_string(nextLabel_++);
            output_ << "  " << old << " = load ptr, ptr " << stateSlot << "\n";
            output_ << "  " << hasOld << " = icmp ne ptr " << old << ", null\n";
            output_ << "  br i1 " << hasOld << ", label %" << releaseLabel << ", label %" << storeLabel << "\n\n";
            output_ << releaseLabel << ":\n";
            output_ << "  call void @__inox_exception_release(ptr " << old << ")\n";
            output_ << "  br label %" << storeLabel << "\n\n";
            output_ << storeLabel << ":\n";
        }
        output_ << "  store ptr " << state << ", ptr " << stateSlot << "\n";
        output_ << "  br label %" << nextLabel << "\n\n";
    }

    void emitReleaseExceptionState(std::string_view stateSlot)
    {
        const std::string state = "%eh.release" + std::to_string(nextTemporary_++);
        output_ << "  " << state << " = load ptr, ptr " << stateSlot << "\n";
        output_ << "  call void @__inox_exception_release(ptr " << state << ")\n";
        output_ << "  store ptr null, ptr " << stateSlot << "\n";
    }

    void emitRethrowState(std::string_view stateSlot)
    {
        const std::string state = "%eh.rethrow.state" + std::to_string(nextTemporary_++);
        output_ << "  " << state << " = load ptr, ptr " << stateSlot << "\n";
        if (!unwindTargets_.empty()) {
            const std::string impossible = newDeadLabel("eh.rethrow.unreachable");
            output_ << "  invoke void @__inox_exception_rethrow(ptr " << state << ") to label %"
                    << impossible << " unwind label %" << currentUnwindTarget() << "\n\n";
            output_ << impossible << ":\n  unreachable\n";
        } else {
            output_ << "  call void @__inox_exception_rethrow(ptr " << state << ")\n";
            output_ << "  unreachable\n";
        }
    }

    void emitRaise(const ast::RaiseStatement& statement)
    {
        if (statement.expression() == nullptr) {
            if (caughtExceptionStates_.empty() || bareRethrowTargets_.empty()) {
                throw CodegenError("bare Raise requires an active exception handler");
            }
            const std::string dead = newDeadLabel("eh.after.rethrow");
            output_ << "  br label %" << bareRethrowTargets_.back() << "\n\n";
            output_ << dead << ":\n";
            return;
        }

        if (statement.expression()->kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenUnsupported("Raise currently requires an exception type name");
        }
        const auto& identifier = static_cast<const ast::IdentifierExpression&>(*statement.expression());
        const std::uint64_t typeId = exceptionTypeId(identifier.name());
        if (!unwindTargets_.empty()) {
            const std::string impossible = newDeadLabel("eh.raise.unreachable");
            const std::string dead = newDeadLabel("eh.after.raise");
            output_ << "  invoke void @__inox_raise(i64 " << typeId << ") to label %" << impossible
                    << " unwind label %" << currentUnwindTarget() << "\n\n";
            output_ << impossible << ":\n  unreachable\n\n";
            output_ << dead << ":\n";
        } else {
            const std::string dead = newDeadLabel("eh.after.raise");
            output_ << "  call void @__inox_raise(i64 " << typeId << ")\n";
            output_ << "  unreachable\n\n" << dead << ":\n";
        }
    }

    void emitTry(const ast::TryStatement& statement)
    {
        const std::size_t id = nextLabel_++;
        const std::string stateSlot = "%eh.state.slot" + std::to_string(id);
        const std::string retryCounterSlot = "%eh.retry.slot" + std::to_string(id);
        const std::string actionSlot = "%eh.action.slot" + std::to_string(id);
        const std::string bodyLabel = "eh.try.body" + std::to_string(id);
        const std::string landing = "eh.lpad" + std::to_string(id);
        const std::string dispatch = "eh.dispatch" + std::to_string(id);
        const std::string handlerUnwind = "eh.handler.lpad" + std::to_string(id);
        const std::string handlerUnwindCaptured = "eh.handler.captured" + std::to_string(id);
        const std::string ensureLabel = "eh.ensure" + std::to_string(id);
        const std::string ensureUnwind = "eh.ensure.lpad" + std::to_string(id);
        const std::string ensureUnwindCaptured = "eh.ensure.captured" + std::to_string(id);
        const std::string afterEnsure = "eh.after.ensure" + std::to_string(id);
        const std::string retryPerform = "eh.retry.perform" + std::to_string(id);
        const std::string rethrowRequest = "eh.rethrow.request" + std::to_string(id);
        const std::string returnRequest = "eh.return.request" + std::to_string(id);
        const std::string exitRequest = "eh.exit.request" + std::to_string(id);
        const std::string leaveRequest = "eh.leave.request" + std::to_string(id);
        const std::string continueRequest = "eh.loop.continue.request" + std::to_string(id);
        const std::string returnPerform = "eh.return.perform" + std::to_string(id);
        const std::string exitPerform = "eh.exit.perform" + std::to_string(id);
        const std::string leavePerform = "eh.leave.perform" + std::to_string(id);
        const std::string continuePerform = "eh.loop.continue.perform" + std::to_string(id);
        const std::string untilRequest = "eh.until.request" + std::to_string(id);
        const std::string untilPerform = "eh.until.perform" + std::to_string(id);
        const std::string continueLabel = "eh.continue" + std::to_string(id);
        const std::string rethrowLabel = "eh.rethrow" + std::to_string(id);
        const std::string handledLabel = statement.hasEnsure() ? ensureLabel : continueLabel;
        const std::string cleanupForRetry = statement.hasEnsure() ? ensureLabel : retryPerform;

        const std::size_t loopDepthAtEntry = loopTargets_.size();
        const std::string leaveDestination = loopTargets_.empty() ? std::string{} : loopTargets_.back().leaveTarget;
        const std::string continueDestination = loopTargets_.empty() ? std::string{} : loopTargets_.back().continueTarget;
        const std::string outerReturnRequest = cleanupContexts_.empty()
            ? std::string{} : cleanupContexts_.back().returnRequestTarget;
        const std::string outerExitRequest = cleanupContexts_.empty()
            ? std::string{} : cleanupContexts_.back().exitRequestTarget;

        std::string outerLeaveRequest;
        std::string outerContinueRequest;
        if (loopDepthAtEntry != 0) {
            for (auto it = cleanupContexts_.rbegin(); it != cleanupContexts_.rend(); ++it) {
                if (it->loopDepthAtEntry >= loopDepthAtEntry) {
                    outerLeaveRequest = it->leaveRequestTarget;
                    outerContinueRequest = it->continueRequestTarget;
                    break;
                }
            }
        }

        const std::size_t untilTargetDepth = repeatLoopDepths_.empty() ? 0 : repeatLoopDepths_.back();
        const std::string untilDestination = untilTargetDepth == 0
            ? std::string{} : loopTargets_[untilTargetDepth - 1].leaveTarget;
        std::string outerUntilRequest;
        if (untilTargetDepth != 0) {
            for (auto it = cleanupContexts_.rbegin(); it != cleanupContexts_.rend(); ++it) {
                if (it->loopDepthAtEntry >= untilTargetDepth) {
                    outerUntilRequest = it->untilRequestTarget;
                    break;
                }
            }
        }

        const CleanupContext cleanupContext{
            actionSlot,
            returnRequest,
            exitRequest,
            leaveRequest,
            continueRequest,
            loopDepthAtEntry,
            leaveDestination,
            continueDestination,
            untilTargetDepth != 0 ? untilRequest : std::string{},
            untilTargetDepth,
            untilDestination};

        output_ << "  " << stateSlot << " = alloca ptr\n";
        output_ << "  " << retryCounterSlot << " = alloca i64\n";
        output_ << "  " << actionSlot << " = alloca i32\n";
        output_ << "  store ptr null, ptr " << stateSlot << "\n";
        output_ << "  store i64 0, ptr " << retryCounterSlot << "\n";
        output_ << "  store i32 0, ptr " << actionSlot << "\n";
        output_ << "  br label %" << bodyLabel << "\n\n";

        if (statement.hasEnsure()) {
            cleanupContexts_.push_back(cleanupContext);
        }

        output_ << bodyLabel << ":\n";
        unwindTargets_.push_back(landing);
        for (const auto& bodyStatement : statement.body()) {
            emitStatement(*bodyStatement);
        }
        unwindTargets_.pop_back();
        output_ << "  store i32 0, ptr " << actionSlot << "\n";
        output_ << "  br label %" << handledLabel << "\n\n";

        emitExceptionCapture(landing, stateSlot, dispatch);

        output_ << dispatch << ":\n";
        if (!statement.hasExcept()) {
            output_ << "  br label %" << rethrowRequest << "\n\n";
        } else if (statement.hasPlainExcept()) {
            const std::string catchAll = "eh.catchall" + std::to_string(id);
            output_ << "  br label %" << catchAll << "\n\n";
            output_ << catchAll << ":\n";
            caughtExceptionStates_.push_back(stateSlot);
            bareRethrowTargets_.push_back(rethrowRequest);
            unwindTargets_.push_back(handlerUnwind);
            for (const auto& st : statement.exceptBody()) emitStatement(*st);
            unwindTargets_.pop_back();
            bareRethrowTargets_.pop_back();
            caughtExceptionStates_.pop_back();
            emitReleaseExceptionState(stateSlot);
            output_ << "  store i32 0, ptr " << actionSlot << "\n";
            output_ << "  br label %" << handledLabel << "\n\n";
        } else {
            const std::string state = "%eh.dispatch.state" + std::to_string(nextTemporary_++);
            const std::string type = "%eh.type" + std::to_string(nextTemporary_++);
            output_ << "  " << state << " = load ptr, ptr " << stateSlot << "\n";
            output_ << "  " << type << " = call i64 @__inox_exception_type(ptr " << state << ")\n";

            std::vector<std::string> labels;
            labels.reserve(statement.handlers().size());
            for (std::size_t index = 0; index < statement.handlers().size(); ++index) {
                labels.push_back("eh.handler" + std::to_string(id) + "_" + std::to_string(index));
            }
            const std::string elseLabel = !statement.elseBody().empty()
                ? "eh.else" + std::to_string(id) : rethrowRequest;

            std::string nextCheck = "eh.check" + std::to_string(id) + "_0";
            if (statement.handlers().empty()) {
                output_ << "  br label %" << elseLabel << "\n\n";
            } else {
                output_ << "  br label %" << nextCheck << "\n\n";
                for (std::size_t index = 0; index < statement.handlers().size(); ++index) {
                    const auto& handler = statement.handlers()[index];
                    output_ << nextCheck << ":\n";
                    const std::string matched = emitExceptionTypeMatch(type, handler.typeName);
                    const bool last = index + 1 == statement.handlers().size();
                    const std::string noMatch = last ? elseLabel
                        : "eh.check" + std::to_string(id) + "_" + std::to_string(index + 1);
                    output_ << "  br i1 " << matched << ", label %" << labels[index]
                            << ", label %" << noMatch << "\n\n";
                    nextCheck = noMatch;
                }
            }

            const RetryContext retryContext{
                retryCounterSlot, actionSlot, cleanupForRetry, rethrowRequest};

            for (std::size_t index = 0; index < statement.handlers().size(); ++index) {
                const auto& handler = statement.handlers()[index];
                output_ << labels[index] << ":\n";
                caughtExceptionStates_.push_back(stateSlot);
                bareRethrowTargets_.push_back(rethrowRequest);
                retryContexts_.push_back(retryContext);
                if (!handler.bindingName.empty()) {
                    exceptionBindings_.emplace(normalize(handler.bindingName), stateSlot);
                }
                unwindTargets_.push_back(handlerUnwind);
                for (const auto& st : handler.body) emitStatement(*st);
                unwindTargets_.pop_back();
                if (!handler.bindingName.empty()) {
                    exceptionBindings_.erase(normalize(handler.bindingName));
                }
                retryContexts_.pop_back();
                bareRethrowTargets_.pop_back();
                caughtExceptionStates_.pop_back();
                emitReleaseExceptionState(stateSlot);
                output_ << "  store i32 0, ptr " << actionSlot << "\n";
                output_ << "  br label %" << handledLabel << "\n\n";
            }

            if (!statement.elseBody().empty()) {
                output_ << elseLabel << ":\n";
                caughtExceptionStates_.push_back(stateSlot);
                bareRethrowTargets_.push_back(rethrowRequest);
                retryContexts_.push_back(retryContext);
                unwindTargets_.push_back(handlerUnwind);
                for (const auto& st : statement.elseBody()) emitStatement(*st);
                unwindTargets_.pop_back();
                retryContexts_.pop_back();
                bareRethrowTargets_.pop_back();
                caughtExceptionStates_.pop_back();
                emitReleaseExceptionState(stateSlot);
                output_ << "  store i32 0, ptr " << actionSlot << "\n";
                output_ << "  br label %" << handledLabel << "\n\n";
            }
        }

        if (statement.hasExcept()) {
            emitExceptionCapture(handlerUnwind, stateSlot, handlerUnwindCaptured, true);
            output_ << handlerUnwindCaptured << ":\n";
            output_ << "  br label %" << rethrowRequest << "\n\n";
        }

        output_ << rethrowRequest << ":\n";
        output_ << "  store i32 1, ptr " << actionSlot << "\n";
        output_ << "  br label %" << (statement.hasEnsure() ? ensureLabel : rethrowLabel) << "\n\n";

        if (statement.hasEnsure()) {
            output_ << returnRequest << ":\n";
            output_ << "  store i32 3, ptr " << actionSlot << "\n";
            output_ << "  br label %" << ensureLabel << "\n\n";

            output_ << exitRequest << ":\n";
            output_ << "  store i32 4, ptr " << actionSlot << "\n";
            output_ << "  br label %" << ensureLabel << "\n\n";

            if (loopDepthAtEntry != 0) {
                output_ << leaveRequest << ":\n";
                output_ << "  store i32 5, ptr " << actionSlot << "\n";
                output_ << "  br label %" << ensureLabel << "\n\n";

                output_ << continueRequest << ":\n";
                output_ << "  store i32 6, ptr " << actionSlot << "\n";
                output_ << "  br label %" << ensureLabel << "\n\n";
            }

            if (untilTargetDepth != 0) {
                output_ << untilRequest << ":\n";
                output_ << "  store i32 7, ptr " << actionSlot << "\n";
                output_ << "  br label %" << ensureLabel << "\n\n";
            }

            cleanupContexts_.pop_back();

            output_ << ensureLabel << ":\n";
            unwindTargets_.push_back(ensureUnwind);
            for (const auto& st : statement.ensureBody()) emitStatement(*st);
            unwindTargets_.pop_back();
            output_ << "  br label %" << afterEnsure << "\n\n";

            emitExceptionCapture(ensureUnwind, stateSlot, ensureUnwindCaptured, true);
            output_ << ensureUnwindCaptured << ":\n";
            output_ << "  br label %" << rethrowLabel << "\n\n";

            output_ << afterEnsure << ":\n";
            const std::string action = "%eh.action" + std::to_string(nextTemporary_++);
            output_ << "  " << action << " = load i32, ptr " << actionSlot << "\n";
            output_ << "  switch i32 " << action << ", label %" << continueLabel << " [\n";
            output_ << "    i32 1, label %" << rethrowLabel << "\n";
            output_ << "    i32 2, label %" << retryPerform << "\n";
            if (!returnValueSlot_.empty()) {
                output_ << "    i32 3, label %" << returnPerform << "\n";
            }
            if (signature_.llvmReturnType == "i32" || signature_.llvmReturnType == "void") {
                output_ << "    i32 4, label %" << exitPerform << "\n";
            }
            if (loopDepthAtEntry != 0) {
                output_ << "    i32 5, label %" << leavePerform << "\n";
                output_ << "    i32 6, label %" << continuePerform << "\n";
            }
            if (untilTargetDepth != 0) {
                output_ << "    i32 7, label %" << untilPerform << "\n";
            }
            output_ << "  ]\n\n";

            if (!returnValueSlot_.empty()) {
                output_ << returnPerform << ":\n";
                if (!outerReturnRequest.empty()) {
                    output_ << "  br label %" << outerReturnRequest << "\n\n";
                } else {
                    const std::string returnValue = "%eh.return.value" + std::to_string(nextTemporary_++);
                    output_ << "  " << returnValue << " = load " << signature_.llvmReturnType
                            << ", ptr " << returnValueSlot_ << "\n";
                    output_ << "  ret " << signature_.llvmReturnType << " " << returnValue << "\n\n";
                }
            }

            if (signature_.llvmReturnType == "i32" || signature_.llvmReturnType == "void") {
                output_ << exitPerform << ":\n";
                if (!outerExitRequest.empty()) {
                    output_ << "  br label %" << outerExitRequest << "\n\n";
                } else if (signature_.llvmReturnType == "i32") {
                    output_ << "  ret i32 0\n\n";
                } else {
                    output_ << "  ret void\n\n";
                }
            }

            if (loopDepthAtEntry != 0) {
                output_ << leavePerform << ":\n";
                if (!outerLeaveRequest.empty()) {
                    output_ << "  br label %" << outerLeaveRequest << "\n\n";
                } else {
                    output_ << "  br label %" << leaveDestination << "\n\n";
                }

                output_ << continuePerform << ":\n";
                if (!outerContinueRequest.empty()) {
                    output_ << "  br label %" << outerContinueRequest << "\n\n";
                } else {
                    output_ << "  br label %" << continueDestination << "\n\n";
                }
            }

            if (untilTargetDepth != 0) {
                output_ << untilPerform << ":\n";
                if (!outerUntilRequest.empty()) {
                    output_ << "  br label %" << outerUntilRequest << "\n\n";
                } else {
                    output_ << "  br label %" << untilDestination << "\n\n";
                }
            }
        }

        output_ << retryPerform << ":\n";
        emitReleaseExceptionState(stateSlot);
        output_ << "  store i32 0, ptr " << actionSlot << "\n";
        output_ << "  br label %" << bodyLabel << "\n\n";

        output_ << rethrowLabel << ":\n";
        emitRethrowState(stateSlot);
        const std::string dead = newDeadLabel("eh.after.unhandled");
        output_ << "\n" << dead << ":\n";
        output_ << "  br label %" << continueLabel << "\n\n";

        output_ << continueLabel << ":\n";
    }

    void emitStatement(const ast::Statement& statement)
    {
        if (statement.kind() == ast::AstNodeKind::VarStatement) {
            const auto& variable = static_cast<const ast::VarStatement&>(statement);
            if (variable.initializer() == nullptr) {
                if (variable.typeName().empty()) {
                    throw CodegenUnsupported(
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

        if (statement.kind() == ast::AstNodeKind::TryStatement) {
            emitTry(static_cast<const ast::TryStatement&>(statement));
            return;
        }

        if (statement.kind() == ast::AstNodeKind::RaiseStatement) {
            emitRaise(static_cast<const ast::RaiseStatement&>(statement));
            return;
        }

        if (statement.kind() == ast::AstNodeKind::RetryStatement) {
            emitRetry(static_cast<const ast::RetryStatement&>(statement));
            return;
        }

        if (statement.kind() == ast::AstNodeKind::ReturnStatement) {
            emitReturn(static_cast<const ast::ReturnStatement&>(statement));
            return;
        }

        if (statement.kind() == ast::AstNodeKind::ExitStatement) {
            emitExit();
            return;
        }

        if (statement.kind() == ast::AstNodeKind::LeaveStatement) {
            emitLeaveTransfer();
            return;
        }

        if (statement.kind() == ast::AstNodeKind::ContinueStatement) {
            emitContinueTransfer();
            return;
        }

        if (statement.kind() == ast::AstNodeKind::WithStatement) {
            emitWith(static_cast<const ast::WithStatement&>(statement));
            return;
        }

        if (statement.kind() == ast::AstNodeKind::UntilStatement) {
            emitUntil(static_cast<const ast::UntilStatement&>(statement));
            return;
        }

        if (statement.kind() == ast::AstNodeKind::CaseStatement) {
            throw CodegenUnsupported("LLVM emission does not lower case statements yet");
        }
        if (statement.kind() == ast::AstNodeKind::UnlessStatement) {
            throw CodegenUnsupported("LLVM emission does not lower unless statements yet");
        }
        throw CodegenUnsupported("LLVM emission does not lower this statement kind yet");
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
            throw CodegenUnsupported(
                "unsupported local variable declaration in Integer function");
        }

        const auto& expression =
            static_cast<const ast::ExpressionStatement&>(statement).expression();
        if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
            throw CodegenUnsupported(
                "unsupported local variable declaration in Integer function");
        }

        const auto& assignment = static_cast<const ast::BinaryExpression&>(expression);
        if (assignment.op() != ast::BinaryOperator::Assign ||
            assignment.left().kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenUnsupported(
                "unsupported local variable declaration in Integer function");
        }

        const auto& identifier =
            static_cast<const ast::IdentifierExpression&>(assignment.left());
        emitLocalVariable(identifier.name(), assignment.right());
    }

    // Returns a slot name that is unique inside the current function, so sibling
    // scopes (two `for` loops over the same iterator, the same local in two `if`
    // bodies, ...) never produce a duplicate LLVM definition.
    std::string newSlot(const std::string& normalizedName)
    {
        const std::string base = "%" + safeLlvmName(normalizedName);
        std::string candidate = base;
        while (!usedSlots_.insert(candidate).second) {
            candidate = base + "." + std::to_string(slotCounter_++);
        }
        return candidate;
    }

    void emitLocalVariable(std::string_view name, const ast::Expression& initializer)
    {
        const std::string normalizedName = normalize(name);
        const std::string llvmType = expressionLlvmType(initializer);
        const std::string inoxType = llvmType == "double" ? "Float64" :
                                    llvmType == "i1" ? "Bool" : "Integer";
        const std::string slot = newSlot(normalizedName);
        output_ << "  " << slot << " = alloca " << llvmType << "\n";
        const std::string value = emitExpression(initializer);
        output_ << "  store " << llvmType << ' ' << value << ", ptr " << slot << '\n';
        locals_.insert_or_assign(normalizedName, LocalInfo{slot, inoxType, llvmType});
    }

    void emitTypedLocalVariable(std::string_view name, std::string_view typeName, const ast::Expression* initializer = nullptr)
    {
        const std::string llvmType = llvmTypeForInoxType(typeName, structs_);
        if (llvmType.empty()) {
            throw CodegenUnsupported("unsupported local variable type for LLVM emission");
        }

        const std::string normalizedName = normalize(name);
        const std::string slot = newSlot(normalizedName);
        output_ << "  " << slot << " = alloca " << llvmType << "\n";

        if (const StructDefinition* structType = findStruct(structs_, typeName)) {
            if (initializer != nullptr) {
                throw CodegenUnsupported("LLVM emission does not support struct initializers yet");
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
            locals_.insert_or_assign(normalizedName,
                                     LocalInfo{slot, std::string(typeName), structType->llvmName});
            return;
        }

        // A declared-but-uninitialized scalar starts at its type's zero. LLVM
        // requires a floating-point literal for float and double, so `0` is not
        // valid there.
        const std::string zero = isFloatLlvmType(llvmType) ? "0.0" : "0";
        const std::string value = initializer != nullptr ? emitExpression(*initializer) : zero;
        output_ << "  store " << llvmType << ' ' << value << ", ptr " << slot << '\n';
        locals_.insert_or_assign(normalizedName, LocalInfo{slot, std::string(typeName), llvmType});
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
            throw CodegenUnsupported(
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
            throw CodegenUnsupported(
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

    std::string emitUserCall(const FunctionSignature& signature,
                             const std::vector<std::string>& arguments,
                             bool requireValue)
    {
        if (arguments.size() != signature.parameters.size()) {
            throw CodegenError("internal error: LLVM user-call argument count mismatch");
        }
        if (requireValue && signature.llvmReturnType == "void") {
            throw CodegenError("void function call cannot be used as an expression");
        }

        std::string result;
        if (signature.llvmReturnType != "void") {
            result = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << result << " = ";
        } else {
            output_ << "  ";
        }

        output_ << (unwindTargets_.empty() ? "call " : "invoke ")
                << signature.llvmReturnType << " @" << signature.llvmName << '(';
        for (std::size_t index = 0; index < arguments.size(); ++index) {
            if (index != 0) output_ << ", ";
            output_ << signature.parameters[index].llvmType << ' ' << arguments[index];
        }
        output_ << ')';

        if (unwindTargets_.empty()) {
            output_ << "\n";
        } else {
            const std::string continuation = "eh.invoke.cont" + std::to_string(nextLabel_++);
            output_ << " to label %" << continuation
                    << " unwind label %" << currentUnwindTarget() << "\n\n";
            output_ << continuation << ":\n";
        }
        return result;
    }

    void emitNoArgumentSubroutineCall(std::string_view calleeName)
    {
        const auto signature = signatures_.find(normalize(calleeName));
        if (signature == signatures_.end()) {
            throw CodegenUnsupported(
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
        emitUserCall(signature->second, {}, false);
    }

    void emitSubroutineCall(const ast::CallExpression& call, std::string_view calleeName)
    {
        const auto signature = signatures_.find(normalize(calleeName));
        if (signature == signatures_.end()) {
            throw CodegenUnsupported(
                "LLVM emission currently supports only Put/PutLn and user subroutine calls as statements");
        }
        if (signature->second.llvmReturnType != "void") {
            throw CodegenUnsupported(
                "LLVM emission currently supports only subroutine calls as expression statements");
        }
        if (call.arguments().size() != signature->second.parameters.size()) {
            throw CodegenUnsupported("unsupported subroutine argument count: " + function_.name());
        }

        std::vector<std::string> arguments;
        arguments.reserve(call.arguments().size());
        for (const auto& argument : call.arguments()) {
            arguments.push_back(emitExpression(*argument));
        }

        emitUserCall(signature->second, arguments, false);
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
            throw CodegenUnsupported("Get/GetLn LLVM emission requires assignable local variables");
        }

        const auto& identifier = static_cast<const ast::IdentifierExpression&>(argument);
        const auto local = locals_.find(normalize(identifier.name()));
        if (local == locals_.end()) {
            throw CodegenUnsupported("Get/GetLn LLVM emission currently supports only local variables");
        }
        if (local->second.llvmType != "i64") {
            throw CodegenUnsupported("Get/GetLn LLVM emission currently supports only Integer/Int64 variables");
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
            throw CodegenUnsupported("LLVM emission currently supports only simple local field access");
        }

        const auto& base = static_cast<const ast::IdentifierExpression&>(*call.arguments()[0]);
        const auto& fieldName = static_cast<const ast::IdentifierExpression&>(*call.arguments()[1]);
        const auto local = locals_.find(normalize(base.name()));
        if (local == locals_.end()) {
            throw CodegenError("LLVM emission supports field access only on local struct variables");
        }

        const StructDefinition* structType = findStruct(structs_, local->second.inoxType);
        if (structType == nullptr) {
            throw CodegenUnsupported("LLVM emission field access target is not a struct");
        }
        const StructFieldInfo* field = findStructField(*structType, fieldName.name());
        if (field == nullptr) {
            throw CodegenUnsupported("unknown struct field for LLVM emission");
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
            throw CodegenUnsupported("LLVM emission currently supports only local method calls");
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
            throw CodegenUnsupported("LLVM emission method receiver is not a struct");
        }

        const std::string qualifiedName = local->second.inoxType + "." + method.name();
        const auto signature = signatures_.find(normalize(qualifiedName));
        if (signature == signatures_.end()) {
            throw CodegenUnsupported("unknown method for LLVM emission: " + qualifiedName);
        }
        if (signature->second.parameters.empty() ||
            signature->second.parameters.front().llvmType != "ptr") {
            throw CodegenUnsupported("LLVM method emission requires an explicit struct receiver parameter");
        }
        if (call.arguments().size() + 1 != signature->second.parameters.size()) {
            throw CodegenUnsupported("unsupported method argument count: " + qualifiedName);
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
            throw CodegenUnsupported("LLVM emission currently supports only local zero-argument method calls");
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
            throw CodegenUnsupported("LLVM emission method receiver is not a struct");
        }

        const std::string qualifiedName = local->second.inoxType + "." + method.name();
        const auto signature = signatures_.find(normalize(qualifiedName));
        if (signature == signatures_.end()) {
            throw CodegenUnsupported("unknown field or zero-argument method for LLVM emission: " + qualifiedName);
        }
        if (signature->second.parameters.size() != 1 ||
            signature->second.parameters.front().llvmType != "ptr") {
            throw CodegenUnsupported("LLVM zero-argument method emission requires only an explicit struct receiver parameter");
        }
        return MethodCallTarget{&signature->second, local->second.slot};
    }

    std::string emitNoArgumentMethodCall(const ast::CallExpression& member, bool requireValue)
    {
        const MethodCallTarget target = resolveNoArgumentMethodAccess(member);
        const FunctionSignature& signature = *target.signature;

        return emitUserCall(signature, {target.receiverPointer}, requireValue);
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

        return emitUserCall(signature, arguments, requireValue);
    }

    void emitLocalAssignment(const ast::Expression& expression)
    {
        if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
            throw CodegenUnsupported(
                "LLVM emission currently supports only simple local assignments");
        }

        const auto& assignment = static_cast<const ast::BinaryExpression&>(expression);
        if (assignment.op() != ast::BinaryOperator::Assign) {
            throw CodegenUnsupported(
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

        throw CodegenUnsupported(
            "LLVM emission currently supports only local variable or field assignments");
    }

    // P-C stage 1: a module Const is read from the semantic result, already
    // resolved, never re-parsed here.
    const semantic::ConstantValue* resolvedConstant(const ast::IdentifierExpression& identifier) const
    {
        const semantic::Symbol* symbol = semantics_.symbolOf(identifier);
        if (symbol == nullptr || symbol->kind != semantic::SymbolKind::Constant) {
            return nullptr;
        }
        return semantics_.constantValueOf(*symbol);
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
            if (const semantic::ConstantValue* constant = resolvedConstant(identifier)) {
                return constant->kind == semantic::ConstantValue::Kind::Boolean ? "i1" : "i64";
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
            if (const semantic::ConstantValue* constant = resolvedConstant(identifier)) {
                if (constant->kind == semantic::ConstantValue::Kind::Boolean) {
                    return constant->boolean ? "1" : "0";
                }
                return std::to_string(constant->integer);
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
            } else if (const std::string helper = checkedIntegerHelper(binary.op()); !helper.empty()) {
                output_ << "  " << result << " = call i64 @" << helper << "(i64 "
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
                    output_ << "  " << result << " = call i64 @__inox_neg_i64(i64 " << operand << ")\n";
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
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << result << " = call i64 @__inox_abs_i64(i64 " << value << ")\n";
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
                throw CodegenUnsupported("unsupported call argument count in function: " + function_.name());
            }

            std::vector<std::string> arguments;
            arguments.reserve(call.arguments().size());
            for (const auto& argument : call.arguments()) {
                arguments.push_back(emitExpression(*argument));
            }

            return emitUserCall(signature->second, arguments, true);
        }
        default:
            break;
        }

        throw CodegenUnsupported("unsupported expression in function: " + function_.name());
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
            throw CodegenUnsupported("unsupported math builtin: " + std::string(name));
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
            throw CodegenUnsupported("unsupported Float operator for LLVM emission");
        }
    }

    // Integer arithmetic is checked (CANON-19, checked arithmetic and limits): overflow, division by zero and an
    // out-of-range shift count trap instead of wrapping or being undefined.
    // These operators lower to always-inlined runtime helpers.
    static std::string checkedIntegerHelper(ast::BinaryOperator op)
    {
        switch (op) {
        case ast::BinaryOperator::Add:
            return "__inox_add_i64";
        case ast::BinaryOperator::Subtract:
            return "__inox_sub_i64";
        case ast::BinaryOperator::Multiply:
            return "__inox_mul_i64";
        case ast::BinaryOperator::Divide:
        case ast::BinaryOperator::IntegerDivide:
            return "__inox_div_i64";
        case ast::BinaryOperator::Modulo:
            return "__inox_mod_i64";
        case ast::BinaryOperator::ShiftLeft:
            return "__inox_shl_i64";
        case ast::BinaryOperator::ShiftRight:
            return "__inox_shr_i64";
        default:
            return {};
        }
    }

    static std::string llvmOperation(ast::BinaryOperator op)
    {
        switch (op) {
        case ast::BinaryOperator::BitAnd:
            return "and";
        case ast::BinaryOperator::BitOr:
            return "or";
        case ast::BinaryOperator::BitXor:
            return "xor";
        default:
            throw CodegenUnsupported(
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
    std::unordered_set<std::string> usedSlots_;
    std::size_t slotCounter_ = 0;
    std::vector<LoopTargets> loopTargets_;
    std::vector<std::size_t> repeatLoopDepths_;
    std::vector<std::string> unwindTargets_;
    std::vector<std::string> caughtExceptionStates_;
    std::vector<std::string> bareRethrowTargets_;
    std::vector<RetryContext> retryContexts_;
    std::vector<CleanupContext> cleanupContexts_;
    std::unordered_map<std::string, std::string> exceptionBindings_;
    std::string returnValueSlot_;
    std::size_t nextTemporary_ = 0;
    std::size_t nextLabel_ = 0;
};

void emitFunction(std::ostringstream& output,
                  const ast::FunctionDeclaration& function,
                  const FunctionSignature& signature,
                  const FunctionSignatures& signatures,
                  const StructDefinitions& structs,
                  std::vector<std::string>& stringGlobals,
                  std::size_t& nextStringLiteral,
                  const semantic::SemanticResult& semantics)
{
    output << "define " << signature.llvmReturnType << " @" << signature.llvmName << '(';
    for (std::size_t index = 0; index < signature.parameters.size(); ++index) {
        if (index != 0) {
            output << ", ";
        }
        output << signature.parameters[index].llvmType << " %" << signature.parameters[index].llvmName;
    }
    output << ')';
    if (functionContainsTry(function)) {
        output << " personality ptr @__gxx_personality_v0";
    }
    output << " {\n"
           << "entry:\n";

    std::ostringstream body;
    FunctionEmitter(body, function, signature, signatures, structs, stringGlobals, nextStringLiteral, semantics).emit();

    // Static allocas belong in the entry block: an alloca inside a loop would grow
    // the stack on every iteration.
    std::istringstream bodyLines(body.str());
    std::string hoisted;
    std::string remaining;
    for (std::string line; std::getline(bodyLines, line);) {
        const bool isAlloca = line.rfind("  %", 0) == 0 && line.find(" = alloca ") != std::string::npos;
        (isAlloca ? hoisted : remaining) += line + "\n";
    }
    output << hoisted << remaining;
    output << "}\n\n";
}

} // namespace

CodegenError::CodegenError(std::string message)
    : std::runtime_error(std::move(message))
{
}

CodegenUnsupported::CodegenUnsupported(std::string message)
    : CodegenError(std::move(message))
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
  %c0 = call i32 @getchar()
  %eof0 = icmp eq i32 %c0, -1
  br i1 %eof0, label %trap, label %ws_check

ws_check:
  %is_sp = icmp eq i32 %c0, 32
  %is_tb = icmp eq i32 %c0, 9
  %is_lf = icmp eq i32 %c0, 10
  %is_cr = icmp eq i32 %c0, 13
  %ws_a = or i1 %is_sp, %is_tb
  %ws_b = or i1 %is_lf, %is_cr
  %is_ws = or i1 %ws_a, %ws_b
  br i1 %is_ws, label %skip_ws, label %sign_check

sign_check:
  %is_minus = icmp eq i32 %c0, 45
  br i1 %is_minus, label %minus, label %first_digit

minus:
  %cm = call i32 @getchar()
  br label %first_digit

first_digit:
  %neg = phi i1 [ true, %minus ], [ false, %sign_check ]
  %cf = phi i32 [ %cm, %minus ], [ %c0, %sign_check ]
  %fge = icmp sge i32 %cf, 48
  %fle = icmp sle i32 %cf, 57
  %fis = and i1 %fge, %fle
  br i1 %fis, label %digits, label %trap

digits:
  %ch = phi i32 [ %cf, %first_digit ], [ %next_ch, %digit_ok ]
  %acc = phi i64 [ 0, %first_digit ], [ %sub_v, %digit_ok ]
  %dge = icmp sge i32 %ch, 48
  %dle = icmp sle i32 %ch, 57
  %dis = and i1 %dge, %dle
  br i1 %dis, label %digit_body, label %finish

digit_body:
  %d32 = sub i32 %ch, 48
  %d64 = sext i32 %d32 to i64
  %mul_p = call { i64, i1 } @llvm.smul.with.overflow.i64(i64 %acc, i64 10)
  %mul_v = extractvalue { i64, i1 } %mul_p, 0
  %mul_o = extractvalue { i64, i1 } %mul_p, 1
  %sub_p = call { i64, i1 } @llvm.ssub.with.overflow.i64(i64 %mul_v, i64 %d64)
  %sub_v = extractvalue { i64, i1 } %sub_p, 0
  %sub_o = extractvalue { i64, i1 } %sub_p, 1
  %ovf = or i1 %mul_o, %sub_o
  br i1 %ovf, label %trap, label %digit_ok

digit_ok:
  %next_ch = call i32 @getchar()
  br label %digits

finish:
  %e_eof = icmp eq i32 %ch, -1
  %e_sp = icmp eq i32 %ch, 32
  %e_tb = icmp eq i32 %ch, 9
  %e_lf = icmp eq i32 %ch, 10
  %e_cr = icmp eq i32 %ch, 13
  %e_a = or i1 %e_eof, %e_sp
  %e_b = or i1 %e_tb, %e_lf
  %e_c = or i1 %e_a, %e_b
  %e_ok = or i1 %e_c, %e_cr
  br i1 %e_ok, label %sign_fix, label %trap

sign_fix:
  br i1 %neg, label %store_neg, label %store_pos

store_neg:
  store i64 %acc, ptr %out
  ret void

store_pos:
  %is_min = icmp eq i64 %acc, -9223372036854775808
  br i1 %is_min, label %trap, label %store_pos_ok

store_pos_ok:
  %pos = sub i64 0, %acc
  store i64 %pos, ptr %out
  ret void

trap:
  call void @__inox_arith_fault(i32 6)
  unreachable
}

)llvm";
}

struct RuntimeFaultKind {
    int code;
    const char* message;
};

// CANON-19: a runtime arithmetic fault is a deterministic Inox trap. The
// program flushes its output, prints "Inox runtime error: <category>" on
// standard error and terminates with kRuntimeFaultExitStatus. There is no
// unwinding: try/except/ensure cannot intercept it.
constexpr RuntimeFaultKind kRuntimeFaultKinds[] = {
    {1, "integer overflow"},
    {2, "division by zero"},
    {3, "invalid shift count"},
    {4, "for-loop step must be positive"},
    {5, "negative exponent"},
    {6, "invalid integer input"},
};

std::string llvmByteString(const std::string& text)
{
    std::string encoded;
    for (const unsigned char ch : text) {
        if (ch == '\n') {
            encoded += "\\0A";
        } else if (ch == '"' || ch == '\\' || ch < 0x20 || ch >= 0x7f) {
            constexpr char hex[] = "0123456789ABCDEF";
            encoded += '\\';
            encoded += hex[ch >> 4];
            encoded += hex[ch & 0x0f];
        } else {
            encoded += static_cast<char>(ch);
        }
    }
    return encoded;
}

std::string runtimeFaultSeam()
{
    const support::NativeErrorWriter writer = support::nativeErrorWriter();
    const std::string lengthType = "i" + std::to_string(writer.lengthBits);
    const std::string resultType = "i" + std::to_string(writer.resultBits);

    std::ostringstream ir;
    ir << "; Every deterministic runtime fault goes through this one function (CANON-19).\n"
       << "; It flushes the program's output, prints \"Inox runtime error: <category>\" on\n"
       << "; standard error (file descriptor 2) and terminates with status "
       << kRuntimeFaultExitStatus << " without\n"
       << "; unwinding, so try/except/ensure cannot intercept it. Kinds: 1 integer\n"
       << "; overflow, 2 division or modulo by zero, 3 shift count outside 0..63, 4 for\n"
       << "; step <= 0, 5 negative exponent, 6 invalid integer input.\n";
    for (const RuntimeFaultKind& kind : kRuntimeFaultKinds) {
        const std::string text = std::string("Inox runtime error: ") + kind.message + "\n";
        ir << "@.inox.fault." << kind.code << " = private unnamed_addr constant ["
           << text.size() << " x i8] c\"" << llvmByteString(text) << "\"\n";
    }
    ir << "declare i32 @fflush(ptr)\n"
       << "declare " << resultType << " @" << writer.symbol << "(i32, ptr, " << lengthType << ")\n"
       << "declare void @_exit(i32) noreturn nounwind\n\n"
       << "define internal void @__inox_arith_fault(i32 %kind) noreturn nounwind cold noinline {\n"
       << "entry:\n"
       << "  %flushed = call i32 @fflush(ptr null)\n"
       << "  switch i32 %kind, label %report [\n";
    for (const RuntimeFaultKind& kind : kRuntimeFaultKinds) {
        ir << "    i32 " << kind.code << ", label %kind" << kind.code << "\n";
    }
    ir << "  ]\n\n";
    for (const RuntimeFaultKind& kind : kRuntimeFaultKinds) {
        ir << "kind" << kind.code << ":\n  br label %report\n\n";
    }
    const RuntimeFaultKind& fallback = kRuntimeFaultKinds[0];
    ir << "report:\n  %message = phi ptr [ @.inox.fault." << fallback.code << ", %entry ]";
    for (const RuntimeFaultKind& kind : kRuntimeFaultKinds) {
        ir << ", [ @.inox.fault." << kind.code << ", %kind" << kind.code << " ]";
    }
    const std::size_t fallbackLength = std::string("Inox runtime error: ").size() +
                                       std::string(fallback.message).size() + 1;
    ir << "\n  %length = phi " << lengthType << " [ " << fallbackLength << ", %entry ]";
    for (const RuntimeFaultKind& kind : kRuntimeFaultKinds) {
        const std::size_t length = std::string("Inox runtime error: ").size() +
                                   std::string(kind.message).size() + 1;
        ir << ", [ " << length << ", %kind" << kind.code << " ]";
    }
    ir << "\n  %written = call " << resultType << " @" << writer.symbol << "(i32 2, ptr %message, "
       << lengthType << " %length)\n"
       << "  call void @_exit(i32 " << kRuntimeFaultExitStatus << ")\n"
       << "  unreachable\n"
       << "}\n";
    return ir.str();
}

std::string mathRuntimeHelpers()
{
    return R"llvm(declare { i64, i1 } @llvm.smul.with.overflow.i64(i64, i64)

)llvm" + runtimeFaultSeam() + R"llvm(
declare { i64, i1 } @llvm.sadd.with.overflow.i64(i64, i64)
declare { i64, i1 } @llvm.ssub.with.overflow.i64(i64, i64)

define internal i64 @__inox_add_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %p = call { i64, i1 } @llvm.sadd.with.overflow.i64(i64 %a, i64 %b)
  %v = extractvalue { i64, i1 } %p, 0
  %o = extractvalue { i64, i1 } %p, 1
  br i1 %o, label %trap, label %ok

ok:
  ret i64 %v

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_sub_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %p = call { i64, i1 } @llvm.ssub.with.overflow.i64(i64 %a, i64 %b)
  %v = extractvalue { i64, i1 } %p, 0
  %o = extractvalue { i64, i1 } %p, 1
  br i1 %o, label %trap, label %ok

ok:
  ret i64 %v

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_mul_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %p = call { i64, i1 } @llvm.smul.with.overflow.i64(i64 %a, i64 %b)
  %v = extractvalue { i64, i1 } %p, 0
  %o = extractvalue { i64, i1 } %p, 1
  br i1 %o, label %trap, label %ok

ok:
  ret i64 %v

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_neg_i64(i64 %a) alwaysinline nounwind {
entry:
  %o = icmp eq i64 %a, -9223372036854775808
  br i1 %o, label %trap, label %ok

ok:
  %r = sub i64 0, %a
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_abs_i64(i64 %a) alwaysinline nounwind {
entry:
  %o = icmp eq i64 %a, -9223372036854775808
  br i1 %o, label %trap, label %ok

ok:
  %n = icmp slt i64 %a, 0
  %m = sub i64 0, %a
  %r = select i1 %n, i64 %m, i64 %a
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_div_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %z = icmp eq i64 %b, 0
  br i1 %z, label %zero, label %check

check:
  %m1 = icmp eq i64 %b, -1
  %mn = icmp eq i64 %a, -9223372036854775808
  %ov = and i1 %m1, %mn
  br i1 %ov, label %overflow, label %ok

ok:
  %r = sdiv i64 %a, %b
  ret i64 %r

zero:
  call void @__inox_arith_fault(i32 2)
  unreachable

overflow:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

define internal i64 @__inox_mod_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %z = icmp eq i64 %b, 0
  br i1 %z, label %trap, label %ok

ok:
  %m1 = icmp eq i64 %b, -1
  %d = select i1 %m1, i64 1, i64 %b
  %q = srem i64 %a, %d
  %r = select i1 %m1, i64 0, i64 %q
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 2)
  unreachable
}

define internal i64 @__inox_shl_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %bad = icmp ugt i64 %b, 63
  br i1 %bad, label %trap, label %ok

ok:
  %r = shl i64 %a, %b
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 3)
  unreachable
}

define internal i64 @__inox_shr_i64(i64 %a, i64 %b) alwaysinline nounwind {
entry:
  %bad = icmp ugt i64 %b, 63
  br i1 %bad, label %trap, label %ok

ok:
  %r = ashr i64 %a, %b
  ret i64 %r

trap:
  call void @__inox_arith_fault(i32 3)
  unreachable
}

define internal i64 @__inox_for_step_i64(i64 %s) alwaysinline nounwind {
entry:
  %bad = icmp sle i64 %s, 0
  br i1 %bad, label %trap, label %ok

ok:
  ret i64 %s

trap:
  call void @__inox_arith_fault(i32 4)
  unreachable
}


define internal i64 @__inox_ipow_i64(i64 %base, i64 %exponent) {
entry:
  %negative = icmp slt i64 %exponent, 0
  br i1 %negative, label %negexp, label %loop

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
  br i1 %mul.result.overflow, label %overflow, label %after_mul_result

after_mul_result:
  %result.after = phi i64 [ %mul.result, %mul_result ], [ %result.cur, %body ]
  %exp.next = ashr i64 %exp.cur, 1
  %need_square = icmp ne i64 %exp.next, 0
  br i1 %need_square, label %square_base, label %continue

square_base:
  %square.pair = call { i64, i1 } @llvm.smul.with.overflow.i64(i64 %base.cur, i64 %base.cur)
  %square = extractvalue { i64, i1 } %square.pair, 0
  %square.overflow = extractvalue { i64, i1 } %square.pair, 1
  br i1 %square.overflow, label %overflow, label %continue

continue:
  %result.next = phi i64 [ %result.after, %after_mul_result ], [ %result.after, %square_base ]
  %base.next = phi i64 [ %base.cur, %after_mul_result ], [ %square, %square_base ]
  br label %loop

exit:
  ret i64 %result.cur

negexp:
  call void @__inox_arith_fault(i32 5)
  unreachable

overflow:
  call void @__inox_arith_fault(i32 1)
  unreachable
}

)llvm";
}

std::string LlvmIrEmitter::emit(const ast::ModuleNode& module, const semantic::SemanticResult& semantics) const
{
    const ast::FunctionDeclaration* mainFunction = nullptr;
    FunctionSignatures signatures;
    StructDefinitions structs;
    std::ostringstream functionOutput;
    std::ostringstream output;
    std::vector<std::string> stringGlobals;
    std::size_t nextStringLiteral = 0;
    const bool usesExceptions = moduleUsesExceptions(module);

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
            emitFunction(functionOutput, function, signature->second, signatures, structs, stringGlobals, nextStringLiteral, semantics);
        }
    }

    if (mainFunction == nullptr) {
        throw CodegenError("LLVM emission requires Main");
    }
    const FunctionSignature mainSignature{"main", "i32", {}};
    emitFunction(functionOutput, *mainFunction, mainSignature, signatures, structs, stringGlobals, nextStringLiteral, semantics);

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
    output << "declare double @hypot(double, double)\n";
    if (usesExceptions) {
        output << "declare i32 @__gxx_personality_v0(...)\n";
        output << "declare void @__inox_raise(i64)\n";
        output << "declare ptr @__inox_exception_capture(ptr)\n";
        output << "declare i64 @__inox_exception_type(ptr)\n";
        output << "declare void @__inox_exception_release(ptr)\n";
        output << "declare void @__inox_exception_rethrow(ptr)\n";
    }
    output << '\n';
    output << inputRuntimeHelpers();
    output << mathRuntimeHelpers();
    output << functionOutput.str();
    return output.str();
}

} // namespace inox::compiler::codegen
