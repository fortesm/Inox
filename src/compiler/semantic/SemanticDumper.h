#pragma once

#include "SemanticResult.h"

#include <ostream>
#include <string_view>
#include <vector>

namespace inox::compiler::semantic {

class SemanticDumper {
public:
    SemanticDumper(std::ostream& output, const SemanticResult& result)
        : output_(output), result_(result)
    {
    }

    void dump(const ast::ModuleNode& module)
    {
        output_ << "Module " << module.name() << '\n';
        for (const auto& item : module.items()) {
            if (item->kind() == ast::AstNodeKind::FunctionDeclaration) {
                const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
                line(1, "Function " + function.name());
                dumpStatements(function.body(), 2);
            }
        }
        dumpStatements(module.statements(), 1);
    }

private:
    void line(std::size_t depth, std::string_view text)
    {
        for (std::size_t index = 0; index < depth; ++index) {
            output_ << "  ";
        }
        output_ << text << '\n';
    }

    std::string typeSuffix(const ast::Expression& expression) const
    {
        const ResolvedType* type = result_.typeOf(expression);
        return " : " + (type != nullptr && !type->name.empty() ? type->name : "<unknown>");
    }

    static std::string symbolSuffix(const Symbol* symbol)
    {
        if (symbol == nullptr) {
            return {};
        }
        return " [symbol=" + symbol->name + ", kind=" + symbolKindName(symbol->kind) + "]";
    }

    void dumpExpression(const ast::Expression& expression, std::size_t depth)
    {
        switch (expression.kind()) {
        case ast::AstNodeKind::LiteralExpression: {
            const auto& literal = static_cast<const ast::LiteralExpression&>(expression);
            line(depth, "Literal " + literal.value() + typeSuffix(expression));
            break;
        }
        case ast::AstNodeKind::IdentifierExpression: {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(expression);
            line(depth,
                 "Identifier " + identifier.name() + typeSuffix(expression) +
                     symbolSuffix(result_.symbolOf(identifier)));
            break;
        }
        case ast::AstNodeKind::BinaryExpression: {
            const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
            line(depth, "Binary " + binaryOperatorName(binary.op()) + typeSuffix(expression));
            dumpExpression(binary.left(), depth + 1);
            dumpExpression(binary.right(), depth + 1);
            break;
        }
        case ast::AstNodeKind::UnaryExpression: {
            const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
            line(depth, "Unary " + unaryOperatorName(unary.op()) + typeSuffix(expression));
            dumpExpression(unary.operand(), depth + 1);
            break;
        }
        case ast::AstNodeKind::CallExpression: {
            const auto& call = static_cast<const ast::CallExpression&>(expression);
            line(depth, "Call" + typeSuffix(expression) + symbolSuffix(result_.symbolOf(call)));
            dumpExpression(call.callee(), depth + 1);
            for (const auto& argument : call.arguments()) {
                dumpExpression(*argument, depth + 1);
            }
            break;
        }
        default:
            line(depth, "Expression" + typeSuffix(expression));
            break;
        }
    }

    void dumpStatements(const std::vector<ast::StatementPtr>& statements, std::size_t depth)
    {
        for (const auto& statement : statements) {
            dumpStatement(*statement, depth);
        }
    }

    void dumpStatement(const ast::Statement& statement, std::size_t depth)
    {
        switch (statement.kind()) {
        case ast::AstNodeKind::BlockStatement:
            line(depth, "Block");
            dumpStatements(static_cast<const ast::BlockStatement&>(statement).statements(), depth + 1);
            break;
        case ast::AstNodeKind::ExpressionStatement:
            dumpExpression(static_cast<const ast::ExpressionStatement&>(statement).expression(), depth);
            break;
        case ast::AstNodeKind::VarStatement: {
            const auto& var = static_cast<const ast::VarStatement&>(statement);
            line(depth, "Var " + var.name());
            if (var.initializer() != nullptr) {
                dumpExpression(*var.initializer(), depth + 1);
            }
            break;
        }
        case ast::AstNodeKind::VarBlockStatement:
            line(depth, "Var");
            dumpStatements(static_cast<const ast::VarBlockStatement&>(statement).declarations(), depth + 1);
            break;
        case ast::AstNodeKind::IfStatement: {
            const auto& ifStatement = static_cast<const ast::IfStatement&>(statement);
            line(depth, "If");
            dumpExpression(ifStatement.condition(), depth + 1);
            dumpStatements(ifStatement.thenBody(), depth + 1);
            for (const auto& clause : ifStatement.elseIfClauses()) {
                line(depth, "Elif");
                dumpExpression(*clause.condition, depth + 1);
                dumpStatements(clause.body, depth + 1);
            }
            if (!ifStatement.elseBody().empty()) {
                line(depth, "Else");
                dumpStatements(ifStatement.elseBody(), depth + 1);
            }
            break;
        }
        case ast::AstNodeKind::UnlessStatement: {
            const auto& unlessStatement = static_cast<const ast::UnlessStatement&>(statement);
            line(depth, "Unless");
            dumpExpression(unlessStatement.condition(), depth + 1);
            dumpStatements(unlessStatement.body(), depth + 1);
            break;
        }
        case ast::AstNodeKind::WhileStatement: {
            const auto& whileStatement = static_cast<const ast::WhileStatement&>(statement);
            line(depth, "While");
            dumpExpression(whileStatement.condition(), depth + 1);
            dumpStatements(whileStatement.body(), depth + 1);
            break;
        }
        case ast::AstNodeKind::RepeatStatement: {
            const auto& repeatStatement = static_cast<const ast::RepeatStatement&>(statement);
            line(depth, "Repeat");
            dumpStatements(repeatStatement.body(), depth + 1);
            break;
        }
        case ast::AstNodeKind::UntilStatement: {
            const auto& untilStatement = static_cast<const ast::UntilStatement&>(statement);
            line(depth, "Until");
            dumpExpression(untilStatement.condition(), depth + 1);
            break;
        }
        case ast::AstNodeKind::ForInStatement: {
            const auto& forStatement = static_cast<const ast::ForInStatement&>(statement);
            line(depth, "For " + forStatement.iterator());
            dumpExpression(forStatement.iterable(), depth + 1);
            if (forStatement.step() != nullptr) {
                dumpExpression(*forStatement.step(), depth + 1);
            }
            dumpStatements(forStatement.body(), depth + 1);
            break;
        }
        case ast::AstNodeKind::CaseStatement: {
            const auto& caseStatement = static_cast<const ast::CaseStatement&>(statement);
            line(depth, "Case");
            dumpExpression(caseStatement.expression(), depth + 1);
            for (const auto& arm : caseStatement.arms()) {
                line(depth + 1, "Arm");
                for (const auto& choice : arm.choices) {
                    dumpExpression(*choice, depth + 2);
                }
                dumpStatements(arm.body, depth + 2);
            }
            if (!caseStatement.otherwiseBody().empty()) {
                line(depth + 1, "Otherwise");
                dumpStatements(caseStatement.otherwiseBody(), depth + 2);
            }
            break;
        }
        case ast::AstNodeKind::TryStatement: {
            const auto& tryStatement = static_cast<const ast::TryStatement&>(statement);
            line(depth, "Try");
            dumpStatements(tryStatement.body(), depth + 1);
            if (!tryStatement.exceptBody().empty()) {
                line(depth, "Except");
                dumpStatements(tryStatement.exceptBody(), depth + 1);
            }
            if (!tryStatement.finallyBody().empty()) {
                line(depth, "Finally");
                dumpStatements(tryStatement.finallyBody(), depth + 1);
            }
            break;
        }
        case ast::AstNodeKind::RaiseStatement: {
            const auto& raiseStatement = static_cast<const ast::RaiseStatement&>(statement);
            line(depth, "Raise");
            if (raiseStatement.expression() != nullptr) {
                dumpExpression(*raiseStatement.expression(), depth + 1);
            }
            break;
        }
        case ast::AstNodeKind::ReturnStatement:
            line(depth, "Return");
            dumpExpression(static_cast<const ast::ReturnStatement&>(statement).expression(), depth + 1);
            break;
        case ast::AstNodeKind::ExitStatement:
            line(depth, "Exit");
            break;
        case ast::AstNodeKind::BreakStatement:
            line(depth, "Break");
            break;
        case ast::AstNodeKind::ContinueStatement:
            line(depth, "Continue");
            break;
        default:
            break;
        }
    }

    static std::string symbolKindName(SymbolKind kind)
    {
        switch (kind) {
        case SymbolKind::Module:
            return "Module";
        case SymbolKind::Function:
            return "Function";
        case SymbolKind::Variable:
            return "Variable";
        case SymbolKind::Constant:
            return "Constant";
        case SymbolKind::State:
            return "State";
        case SymbolKind::Type:
            return "Type";
        case SymbolKind::Builtin:
            return "Builtin";
        }
        return "Unknown";
    }

    static std::string binaryOperatorName(ast::BinaryOperator op)
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

    static std::string unaryOperatorName(ast::UnaryOperator op)
    {
        switch (op) {
        case ast::UnaryOperator::Plus:
            return "+";
        case ast::UnaryOperator::Minus:
            return "-";
        case ast::UnaryOperator::Not:
            return "not";
        case ast::UnaryOperator::BitNot:
            return "bitnot";
        }
        return "?";
    }

    std::ostream& output_;
    const SemanticResult& result_;
};

} // namespace inox::compiler::semantic
