#include "LlvmIrEmitter.h"

#include <cctype>
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

struct IntegerParameter {
    std::string inoxName;
    std::string llvmName;
};

struct IntegerFunctionSignature {
    std::string llvmName;
    std::vector<IntegerParameter> parameters;
};

using IntegerFunctionSignatures =
    std::unordered_map<std::string, IntegerFunctionSignature>;

std::vector<IntegerParameter> parseIntegerSignature(const ast::FunctionDeclaration& function)
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

    if (index + 1 != tokens.size() || !equalsIgnoreCase(tokens[index], "Integer")) {
        throw CodegenError(
            "LLVM emission currently supports only Integer return types");
    }

    return parameters;
}

class IntegerFunctionEmitter {
public:
    IntegerFunctionEmitter(std::ostringstream& output,
                           const ast::FunctionDeclaration& function,
                           const IntegerFunctionSignature& signature,
                           const IntegerFunctionSignatures& signatures)
        : output_(output), function_(function), signatures_(signatures)
    {
        for (const IntegerParameter& parameter : signature.parameters) {
            parameters_.emplace(normalize(parameter.inoxName), "%" + parameter.llvmName);
        }
    }

    void emit()
    {
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
        output_ << "  ret i64 " << value << '\n';
    }

private:
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
            emitLocalAssignment(
                static_cast<const ast::ExpressionStatement&>(statement).expression());
            return;
        }

        throw CodegenError(
            "LLVM emission currently supports only local variables and assignments before Return");
    }

    void emitVarBlockDeclaration(const ast::Statement& statement)
    {
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
        locals_.emplace(normalizedName, slot);
    }

    void emitLocalAssignment(const ast::Expression& expression)
    {
        if (expression.kind() != ast::AstNodeKind::BinaryExpression) {
            throw CodegenError(
                "LLVM emission currently supports only simple local assignments");
        }

        const auto& assignment = static_cast<const ast::BinaryExpression&>(expression);
        if (assignment.op() != ast::BinaryOperator::Assign ||
            assignment.left().kind() != ast::AstNodeKind::IdentifierExpression) {
            throw CodegenError(
                "LLVM emission currently supports only simple local assignments");
        }

        const auto& identifier =
            static_cast<const ast::IdentifierExpression&>(assignment.left());
        const auto local = locals_.find(normalize(identifier.name()));
        if (local == locals_.end()) {
            throw CodegenError(
                "LLVM emission currently supports assignment only to local variables");
        }

        const std::string value = emitExpression(assignment.right());
        output_ << "  store i64 " << value << ", ptr " << local->second << '\n';
    }

    std::string emitExpression(const ast::Expression& expression)
    {
        switch (expression.kind()) {
        case ast::AstNodeKind::LiteralExpression: {
            const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
            if (literal.literalKind() != ast::LiteralKind::Integer) {
                break;
            }
            return literal.value();
        }
        case ast::AstNodeKind::IdentifierExpression: {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
            const std::string normalizedName = normalize(identifier.name());
            const auto local = locals_.find(normalizedName);
            if (local != locals_.end()) {
                const std::string result = "%tmp" + std::to_string(nextTemporary_++);
                output_ << "  " << result << " = load i64, ptr " << local->second << '\n';
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
            const std::string operation = llvmOperation(binary.op());
            const std::string left = emitExpression(binary.left());
            const std::string right = emitExpression(binary.right());
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << result << " = " << operation << " i64 "
                    << left << ", " << right << '\n';
            return result;
        }
        case ast::AstNodeKind::CallExpression: {
            const auto& call = static_cast<const ast::CallExpression&>(expression);
            if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
                break;
            }

            const auto& callee =
                static_cast<const ast::IdentifierExpression&>(call.callee());
            const auto signature = signatures_.find(normalize(callee.name()));
            if (signature == signatures_.end()) {
                break;
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
        default:
            throw CodegenError(
                "LLVM emission currently supports only +, -, and *");
        }
    }

    std::ostringstream& output_;
    const ast::FunctionDeclaration& function_;
    const IntegerFunctionSignatures& signatures_;
    std::unordered_map<std::string, std::string> parameters_;
    std::unordered_map<std::string, std::string> locals_;
    std::size_t nextTemporary_ = 0;
};

void emitIntegerFunction(std::ostringstream& output,
                         const ast::FunctionDeclaration& function,
                         const IntegerFunctionSignature& signature,
                         const IntegerFunctionSignatures& signatures)
{
    output << "define i64 @" << signature.llvmName << '(';
    for (std::size_t index = 0; index < signature.parameters.size(); ++index) {
        if (index != 0) {
            output << ", ";
        }
        output << "i64 %" << signature.parameters[index].llvmName;
    }
    output << ") {\n"
           << "entry:\n";
    IntegerFunctionEmitter(output, function, signature, signatures).emit();
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
    IntegerFunctionSignatures signatures;
    std::ostringstream output;

    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::FunctionDeclaration) {
            continue;
        }

        const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
        if (equalsIgnoreCase(function.name(), "Main")) {
            mainFunction = &function;
        } else {
            const std::string normalizedName = normalize(function.name());
            signatures.emplace(
                normalizedName,
                IntegerFunctionSignature{
                    normalizedName,
                    parseIntegerSignature(function)});
        }
    }

    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::FunctionDeclaration) {
            continue;
        }

        const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
        if (!equalsIgnoreCase(function.name(), "Main")) {
            const auto signature = signatures.find(normalize(function.name()));
            emitIntegerFunction(output, function, signature->second, signatures);
        }
    }

    if (mainFunction == nullptr) {
        throw CodegenError("LLVM emission requires Main()");
    }
    if (!mainFunction->body().empty()) {
        throw CodegenError(
            "LLVM emission currently supports only an empty Main()");
    }

    output << "define i32 @main() {\n"
           << "entry:\n"
           << "  ret i32 0\n"
           << "}\n";
    return output.str();
}

} // namespace inox::compiler::codegen
