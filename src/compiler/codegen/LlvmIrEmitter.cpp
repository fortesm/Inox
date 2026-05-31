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

std::string llvmIntegerLiteral(std::string_view value)
{
    if (value.empty() || value.front() != '$') {
        return std::string(value);
    }

    return std::to_string(std::stoull(std::string(value.substr(1)), nullptr, 16));
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

    if (index + 1 != tokens.size()) {
        throw CodegenError(
            "LLVM emission currently supports only Integer and Bool return types");
    }

    std::string llvmReturnType;
    if (equalsIgnoreCase(tokens[index], "Integer")) {
        llvmReturnType = "i64";
    } else if (equalsIgnoreCase(tokens[index], "Bool")) {
        llvmReturnType = "i1";
    } else {
        throw CodegenError(
            "LLVM emission currently supports only Integer and Bool return types");
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
                    const FunctionSignatures& signatures)
        : output_(output), function_(function), signature_(signature), signatures_(signatures)
    {
        for (const IntegerParameter& parameter : signature.parameters) {
            parameters_.emplace(normalize(parameter.inoxName), "%" + parameter.llvmName);
        }
    }

    void emit()
    {
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

    void emitIfReturn(const ast::IfStatement& statement)
    {
        if (signature_.llvmReturnType != "i64") {
            throw CodegenError(
                "LLVM emission currently supports if/else only in Integer functions");
        }
        if (!statement.elseIfClauses().empty() || statement.elseBody().empty()) {
            throw CodegenError(
                "LLVM emission currently requires if with else and without elif");
        }
        if (statement.thenBody().size() != 1 ||
            statement.thenBody().front()->kind() != ast::AstNodeKind::ReturnStatement ||
            statement.elseBody().size() != 1 ||
            statement.elseBody().front()->kind() != ast::AstNodeKind::ReturnStatement) {
            throw CodegenError(
                "LLVM emission currently requires a single Return in each if branch");
        }

        const std::size_t label = nextLabel_++;
        const std::string condition = emitExpression(statement.condition());
        output_ << "  br i1 " << condition
                << ", label %then" << label
                << ", label %else" << label << "\n\n";

        output_ << "then" << label << ":\n";
        emitReturn(static_cast<const ast::ReturnStatement&>(*statement.thenBody().front()));
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
            "LLVM emission currently supports only assignments, if, break, and continue in while bodies");
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
                "LLVM emission supports break and continue only inside while");
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
            emitLocalAssignment(
                static_cast<const ast::ExpressionStatement&>(statement).expression());
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

        throw CodegenError(
            "LLVM emission currently supports only local variables, assignments, if, and while before Return");
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
            if (unary.op() != ast::UnaryOperator::Not) {
                break;
            }
            const std::string operand = emitExpression(unary.operand());
            const std::string result = "%tmp" + std::to_string(nextTemporary_++);
            output_ << "  " << result << " = xor i1 " << operand << ", true\n";
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
    std::unordered_map<std::string, std::string> parameters_;
    std::unordered_map<std::string, std::string> locals_;
    std::vector<LoopTargets> loopTargets_;
    std::size_t nextTemporary_ = 0;
    std::size_t nextLabel_ = 0;
};

void emitFunction(std::ostringstream& output,
                  const ast::FunctionDeclaration& function,
                  const FunctionSignature& signature,
                  const FunctionSignatures& signatures)
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
    FunctionEmitter(output, function, signature, signatures).emit();
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
            emitFunction(output, function, signature->second, signatures);
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
