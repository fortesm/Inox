#pragma once

#include "Symbol.h"
#include "../ast/Ast.h"

#include <string>
#include <unordered_map>
#include <utility>

namespace inox::compiler::semantic {

struct ResolvedType {
    std::string name;
    const TypeSymbol* symbol = nullptr;
};

class SemanticResult {
public:
    void clear()
    {
        expressionTypes_.clear();
        identifierSymbols_.clear();
        callSymbols_.clear();
    }

    void setExpressionType(const ast::Expression& expression, ResolvedType type)
    {
        expressionTypes_.insert_or_assign(&expression, std::move(type));
    }

    const ResolvedType* typeOf(const ast::Expression& expression) const
    {
        const auto iterator = expressionTypes_.find(&expression);
        return iterator != expressionTypes_.end() ? &iterator->second : nullptr;
    }

    void bind(const ast::IdentifierExpression& expression, const Symbol& symbol)
    {
        identifierSymbols_.insert_or_assign(&expression, &symbol);
    }

    const Symbol* symbolOf(const ast::IdentifierExpression& expression) const
    {
        const auto iterator = identifierSymbols_.find(&expression);
        return iterator != identifierSymbols_.end() ? iterator->second : nullptr;
    }

    void bind(const ast::CallExpression& expression, const Symbol& symbol)
    {
        callSymbols_.insert_or_assign(&expression, &symbol);
    }

    const Symbol* symbolOf(const ast::CallExpression& expression) const
    {
        const auto iterator = callSymbols_.find(&expression);
        return iterator != callSymbols_.end() ? iterator->second : nullptr;
    }

private:
    std::unordered_map<const ast::Expression*, ResolvedType> expressionTypes_;
    std::unordered_map<const ast::IdentifierExpression*, const Symbol*> identifierSymbols_;
    std::unordered_map<const ast::CallExpression*, const Symbol*> callSymbols_;
};

} // namespace inox::compiler::semantic
