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
                           const std::vector<IntegerParameter>& parameters)
        : output_(output), function_(function)
    {
        for (const IntegerParameter& parameter : parameters) {
            parameters_.emplace(normalize(parameter.inoxName), "%" + parameter.llvmName);
        }
    }

    void emit()
    {
        if (function_.body().size() != 1 ||
            function_.body().front()->kind() != ast::AstNodeKind::ReturnStatement) {
            throw CodegenError(
                "LLVM emission currently supports only a single Return expression");
        }

        const auto& returnStatement =
            static_cast<const ast::ReturnStatement&>(*function_.body().front());
        const std::string value = emitExpression(returnStatement.expression());
        output_ << "  ret i64 " << value << '\n';
    }

private:
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
            const auto iterator = parameters_.find(normalize(identifier.name()));
            if (iterator != parameters_.end()) {
                return iterator->second;
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
    std::unordered_map<std::string, std::string> parameters_;
    std::size_t nextTemporary_ = 0;
};

void emitIntegerFunction(std::ostringstream& output,
                         const ast::FunctionDeclaration& function)
{
    const std::vector<IntegerParameter> parameters = parseIntegerSignature(function);

    output << "define i64 @" << normalize(function.name()) << '(';
    for (std::size_t index = 0; index < parameters.size(); ++index) {
        if (index != 0) {
            output << ", ";
        }
        output << "i64 %" << parameters[index].llvmName;
    }
    output << ") {\n"
           << "entry:\n";
    IntegerFunctionEmitter(output, function, parameters).emit();
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
    std::ostringstream output;

    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::FunctionDeclaration) {
            continue;
        }

        const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
        if (equalsIgnoreCase(function.name(), "Main")) {
            mainFunction = &function;
        } else {
            emitIntegerFunction(output, function);
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
