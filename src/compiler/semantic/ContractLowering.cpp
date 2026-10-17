// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Design by contract, the routine and struct part (CANON-24). Eiffel's rules
// (Bertrand Meyer, "Object-Oriented Software Construction"; eiffel.org):
//   - `pre` (Eiffel require) is checked when the routine is called;
//   - `pos` (Eiffel ensure) when it returns normally, with `old X` (X on
//     entry) and `Result` (the returned value);
//   - an Override inherits the contracts of the methods it replaces: its
//     precondition is (inherited) or else (its own `pre else`), its
//     postcondition (inherited) and then (its own `pos then`); writing a
//     plain `pre` or `pos` in an Override is an error, as in Eiffel;
//   - a Struct's `invariant` is checked by the backend where Eiffel checks a
//     class invariant (see SemanticAnalyzer::recordInvariantChecks).
// Loop `invariant` and `variant` are lowered by the parser.

#include "ContractLowering.h"
#include "../ast/AstClone.h"

#include "SemanticAnalyzer.h"
#include "../types/TypeSyntax.h"

#include <cctype>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace inox::compiler::semantic {

namespace {

using ast::ContractClause;

std::string lower(std::string_view text)
{
    std::string result(text);
    for (char& ch : result) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return result;
}

ast::ExpressionPtr identifier(const std::string& name)
{
    return std::make_unique<ast::IdentifierExpression>(name);
}

ast::ExpressionPtr member(ast::ExpressionPtr base, const std::string& name)
{
    std::vector<ast::ExpressionPtr> arguments;
    arguments.push_back(std::move(base));
    arguments.push_back(identifier(name));
    return std::make_unique<ast::CallExpression>(identifier("__member"), std::move(arguments));
}

ast::ExpressionPtr literal(ast::LiteralKind kind, std::string value)
{
    return std::make_unique<ast::LiteralExpression>(kind, std::move(value));
}

ast::StatementPtr statement(ast::ExpressionPtr expression)
{
    return std::make_unique<ast::ExpressionStatement>(std::move(expression));
}

ast::StatementPtr check(int kind, ast::ExpressionPtr condition, const std::string& message)
{
    std::vector<ast::ExpressionPtr> arguments;
    arguments.push_back(literal(ast::LiteralKind::Integer, std::to_string(kind)));
    arguments.push_back(std::move(condition));
    arguments.push_back(literal(ast::LiteralKind::String, message));
    return statement(std::make_unique<ast::CallExpression>(identifier("__check"), std::move(arguments)));
}

ast::StatementPtr declare(const std::string& name, ast::ExpressionPtr value)
{
    return statement(std::make_unique<ast::BinaryExpression>(ast::BinaryOperator::Assign, identifier(name),
                                                             std::move(value)));
}

// One clause as an Override sees it: the condition is a copy in which Self
// names the part the replaced method runs on (`Self.TPessoa`).
struct Clause {
    ContractClause::Kind kind = ContractClause::Kind::Pre;
    const ast::Expression* condition = nullptr;  // owned by the routine that wrote it
    std::vector<std::string> path;               // parts from the overriding struct
    std::string text;
    std::string message;
    std::string origin;                          // the routine that wrote it
};

struct Effective {
    // The precondition holds when one alternative holds, and an
    // alternative holds when all its clauses do; no alternative = no
    // precondition. An Override adds its `pre else` clauses as one more
    // alternative (Eiffel: or else).
    std::vector<std::vector<Clause>> preAlternatives;
    bool preIsTrue = true;
    std::vector<Clause> pos;  // all must hold (Eiffel: and then)
};

struct Routine {
    ast::FunctionDeclaration* declaration = nullptr;
    std::size_t index = 0;
    std::vector<ContractClause> clauses;
};

class ContractLowerer {
public:
    explicit ContractLowerer(ast::ModuleNode& program) : program_(program) {}

    void run()
    {
        collect();
        for (auto& [key, routine] : routines_) {
            validateClauses(routine);
        }
        // Effective contracts first (an Override reads the clauses of the
        // methods it replaces before they are lowered), then lowering.
        std::map<std::string, Effective> effective;
        for (auto& [key, routine] : routines_) {
            effective.emplace(key, effectiveOf(key));
        }
        for (auto& [key, routine] : routines_) {
            const Effective& contract = effective.at(key);
            if (!contract.preIsTrue || !contract.pos.empty()) {
                lowerRoutine(routine, contract);
            }
        }
        for (std::unique_ptr<ast::FunctionDeclaration>& added : added_) {
            program_.items().push_back(std::move(added));
        }
        addInvariantMethods();
    }

private:
    void collect()
    {
        for (std::size_t index = 0; index < program_.items().size(); ++index) {
            ast::AstNode& item = *program_.items()[index];
            if (item.kind() == ast::AstNodeKind::FunctionDeclaration) {
                auto& function = static_cast<ast::FunctionDeclaration&>(item);
                Routine routine;
                routine.declaration = &function;
                routine.index = index;
                routine.clauses = function.takeContracts();
                routines_.emplace(lower(function.name()), std::move(routine));
            } else if (item.kind() == ast::AstNodeKind::SectionDeclaration) {
                const auto& section = static_cast<const ast::SectionDeclaration&>(item);
                for (const ast::TypeDeclaration& type : section.types()) {
                    if (type.kind != ast::TypeDeclarationKind::Struct) {
                        continue;
                    }
                    std::vector<std::string> parts;
                    for (const ast::StructFieldDeclaration& field : type.fields) {
                        if (field.isPart) {
                            parts.push_back(field.typeName);
                        }
                    }
                    structs_.emplace(lower(type.name), StructInfo{type.name, std::move(parts), &type});
                }
            }
        }
    }

    void validateClauses(const Routine& routine) const
    {
        const ast::FunctionDeclaration& function = *routine.declaration;
        const bool isFunction = hasResult(function);
        if (!routine.clauses.empty() && lower(function.name()) == "main") {
            throw SemanticError("Main has no contracts: nobody calls it (CANON-24); check with Assert instead");
        }
        for (const ContractClause& clause : routine.clauses) {
            const bool inheriting = clause.kind == ContractClause::Kind::PreElse ||
                                    clause.kind == ContractClause::Kind::PosThen;
            if (inheriting && !function.isOverride()) {
                throw SemanticError(function.name() + ": 'pre else' and 'pos then' belong to an Override, "
                                    "which inherits the contracts it replaces; write 'pre' or 'pos' (CANON-24)");
            }
            if (!inheriting && function.isOverride()) {
                throw SemanticError(function.name() + " is an Override: it weakens the precondition with 'pre "
                                    "else' and strengthens the postcondition with 'pos then' (CANON-24)");
            }
            const bool isPre = clause.kind == ContractClause::Kind::Pre || clause.kind == ContractClause::Kind::PreElse;
            if (usesResult(*clause.condition) && (isPre || !isFunction)) {
                throw SemanticError(function.name() + ": Result is the value a function returns, valid only in "
                                    "its 'pos' (CANON-24)");
            }
        }
        if (!isFunction) {
            return;
        }
        // Result names the returned value in a pos: no parameter may take it.
        for (const std::string& token : function.signatureTokens()) {
            if (lower(token) == "result" && hasPos(routine)) {
                throw SemanticError(function.name() + ": a parameter named Result would hide the value 'pos' "
                                    "calls Result (CANON-24)");
            }
        }
    }

    static bool hasPos(const Routine& routine)
    {
        for (const ContractClause& clause : routine.clauses) {
            if (clause.kind == ContractClause::Kind::Pos || clause.kind == ContractClause::Kind::PosThen) {
                return true;
            }
        }
        return false;
    }

    static bool usesResult(const ast::Expression& expression)
    {
        bool found = false;
        ast::cloneExpression(expression, [&](const ast::Expression& node) -> ast::ExpressionPtr {
            if (node.kind() == ast::AstNodeKind::IdentifierExpression &&
                lower(static_cast<const ast::IdentifierExpression&>(node).name()) == "result") {
                found = true;
            }
            return nullptr;
        });
        return found;
    }

    static bool hasResult(const ast::FunctionDeclaration& function)
    {
        const auto& tokens = function.signatureTokens();
        if (tokens.empty()) {
            return false;
        }
        if (tokens.front() != "(") {
            return true;  // `F Integer :` — no parameters, a result type
        }
        for (std::size_t index = 0; index < tokens.size(); ++index) {
            if (tokens[index] == ")") {
                return index + 1 < tokens.size();
            }
        }
        return false;
    }

    // The methods an Override replaces: for each part (at any depth) the
    // nearest method with the same name, with the path to its part.
    std::vector<std::pair<std::vector<std::string>, std::string>> replaced(const std::string& typeName,
                                                                           const std::string& method) const
    {
        std::vector<std::pair<std::vector<std::string>, std::string>> found;
        const auto info = structs_.find(lower(typeName));
        if (info == structs_.end()) {
            return found;
        }
        for (const std::string& part : info->second.parts) {
            const std::string key = lower(part + "." + method);
            if (routines_.contains(key)) {
                found.push_back({{part}, key});
                continue;
            }
            for (auto& [path, inner] : replaced(part, method)) {
                path.insert(path.begin(), part);
                found.push_back({path, inner});
            }
        }
        return found;
    }

    Effective effectiveOf(const std::string& key)
    {
        if (const auto cached = memo_.find(key); cached != memo_.end()) {
            return cached->second;
        }
        if (visiting_.contains(key)) {
            return {};
        }
        visiting_.insert(key);
        const Routine& routine = routines_.at(key);
        const ast::FunctionDeclaration& function = *routine.declaration;
        Effective result;
        std::vector<Clause> ownPre;
        for (const ContractClause& clause : routine.clauses) {
            Clause own{clause.kind, clause.condition.get(), {}, clause.text, clause.message, function.name()};
            if (clause.kind == ContractClause::Kind::Pre || clause.kind == ContractClause::Kind::PreElse) {
                ownPre.push_back(own);
            } else {
                result.pos.push_back(own);
            }
        }

        const std::size_t dot = function.name().find('.');
        if (function.isOverride() && dot != std::string::npos) {
            bool inheritedTrue = false;
            std::vector<Clause> inheritedPos;
            for (const auto& [path, replacedKey] : replaced(function.name().substr(0, dot),
                                                            function.name().substr(dot + 1))) {
                const Effective inherited = effectiveOf(replacedKey);
                if (inherited.preIsTrue) {
                    inheritedTrue = true;
                }
                for (const std::vector<Clause>& alternative : inherited.preAlternatives) {
                    std::vector<Clause> moved = alternative;
                    for (Clause& clause : moved) {
                        clause.path.insert(clause.path.begin(), path.begin(), path.end());
                    }
                    result.preAlternatives.push_back(std::move(moved));
                }
                for (Clause clause : inherited.pos) {
                    clause.path.insert(clause.path.begin(), path.begin(), path.end());
                    inheritedPos.push_back(std::move(clause));
                }
            }
            inheritedPos.insert(inheritedPos.end(), result.pos.begin(), result.pos.end());
            result.pos = std::move(inheritedPos);
            if (inheritedTrue) {
                result.preAlternatives.clear();  // (true) or else anything is true
                result.preIsTrue = true;
            } else {
                if (!ownPre.empty()) {
                    result.preAlternatives.push_back(ownPre);
                }
                result.preIsTrue = result.preAlternatives.empty();
            }
        } else {
            if (!ownPre.empty()) {
                result.preAlternatives.push_back(ownPre);
            }
            result.preIsTrue = result.preAlternatives.empty();
        }
        visiting_.erase(key);
        memo_.emplace(key, result);
        return result;
    }

    // A copy of a clause's condition for this routine: Self follows the
    // clause's path to its part, `old X` becomes a value saved on entry,
    // Result the returned value.
    ast::ExpressionPtr instantiate(const Clause& clause, std::vector<ast::StatementPtr>* oldValues)
    {
        return ast::cloneExpression(*clause.condition, [&](const ast::Expression& node) -> ast::ExpressionPtr {
            if (node.kind() == ast::AstNodeKind::IdentifierExpression) {
                const std::string name = lower(static_cast<const ast::IdentifierExpression&>(node).name());
                if (name == "self" && !clause.path.empty()) {
                    ast::ExpressionPtr path = identifier("Self");
                    for (const std::string& part : clause.path) {
                        path = member(std::move(path), part);
                    }
                    return path;
                }
                if (name == "result") {
                    return identifier("__result");
                }
                return nullptr;
            }
            if (node.kind() == ast::AstNodeKind::UnaryExpression &&
                static_cast<const ast::UnaryExpression&>(node).op() == ast::UnaryOperator::Old &&
                oldValues != nullptr) {
                const std::string saved = "__old_" + std::to_string(++counter_);
                Clause inner = clause;
                inner.condition = &static_cast<const ast::UnaryExpression&>(node).operand();
                std::vector<ast::ExpressionPtr> pure;
                pure.push_back(instantiate(inner, nullptr));
                oldValues->push_back(declare(
                    saved, std::make_unique<ast::CallExpression>(identifier("__pure"), std::move(pure))));
                return identifier(saved);
            }
            return nullptr;
        });
    }

    static std::string clauseText(const Clause& clause)
    {
        std::string text = clause.text;
        if (!clause.message.empty()) {
            text += ": " + clause.message;
        }
        return text;
    }

    void lowerRoutine(Routine& routine, const Effective& contract)
    {
        ast::FunctionDeclaration& function = *routine.declaration;
        const std::string& name = function.name();
        const std::size_t dot = name.find('.');
        const std::string bodyName = dot == std::string::npos
                                         ? "__" + name + "_body"
                                         : name.substr(0, dot) + ".__" + name.substr(dot + 1) + "_body";
        const bool isFunction = hasResult(function);

        std::vector<ast::StatementPtr> statements;
        // Precondition: the caller's part of the contract.
        if (contract.preAlternatives.size() == 1) {
            for (const Clause& clause : contract.preAlternatives.front()) {
                statements.push_back(check(1, instantiate(clause, nullptr),
                                           "precondition violated: " + name + ": " + clauseText(clause) +
                                               originNote(clause, name)));
            }
        } else if (contract.preAlternatives.size() > 1) {
            ast::ExpressionPtr any;
            std::string text;
            for (const std::vector<Clause>& alternative : contract.preAlternatives) {
                ast::ExpressionPtr all;
                std::string allText;
                for (const Clause& clause : alternative) {
                    ast::ExpressionPtr condition = instantiate(clause, nullptr);
                    all = all ? std::make_unique<ast::BinaryExpression>(ast::BinaryOperator::And, std::move(all),
                                                                        std::move(condition))
                              : std::move(condition);
                    allText += (allText.empty() ? "" : " and ") + clause.text;
                }
                if (!all) {
                    all = literal(ast::LiteralKind::Boolean, "true");
                    allText = "true";
                }
                any = any ? std::make_unique<ast::BinaryExpression>(ast::BinaryOperator::Or, std::move(any),
                                                                    std::move(all))
                          : std::move(all);
                text += (text.empty() ? "(" : " or else (") + allText + ")";
            }
            statements.push_back(check(1, std::move(any), "precondition violated: " + name + ": " + text));
        }

        // Postcondition clauses, with their `old` values saved now.
        std::vector<ast::StatementPtr> oldValues;
        std::vector<ast::StatementPtr> posChecks;
        for (const Clause& clause : contract.pos) {
            posChecks.push_back(check(2, instantiate(clause, &oldValues),
                                      "postcondition violated: " + name + ": " + clauseText(clause) +
                                          originNote(clause, name)));
        }
        for (ast::StatementPtr& saved : oldValues) {
            statements.push_back(std::move(saved));
        }

        // The call of the body, with the routine's own parameters. An owner
        // (or a value holding one) given to the routine is given on to its
        // body (safe-pointers), so a pos clause reads it only through old.
        std::vector<ast::ExpressionPtr> arguments;
        for (const auto& [parameter, type, byReference] : parameterList(function)) {
            if (!byReference && movesOnly(type)) {
                for (const Clause& clause : contract.pos) {
                    if (mentions(*clause.condition, parameter)) {
                        throw SemanticError("a pos clause of " + name + " cannot read " + parameter +
                                            ": the routine takes that owner; read what it holds with old (old " + parameter +
                                            ".Field) (safe pointers)");
                    }
                }
                std::vector<ast::ExpressionPtr> moved;
                moved.push_back(identifier(parameter));
                arguments.push_back(std::make_unique<ast::CallExpression>(identifier("Move"), std::move(moved)));
                continue;
            }
            arguments.push_back(identifier(parameter));
        }
        ast::ExpressionPtr call;
        if (dot != std::string::npos) {
            ast::ExpressionPtr method = member(identifier("Self"), bodyName.substr(dot + 1));
            call = arguments.empty() ? std::move(method)
                                     : std::make_unique<ast::CallExpression>(std::move(method), std::move(arguments));
        } else {
            call = arguments.empty() ? identifier(bodyName)
                                     : std::make_unique<ast::CallExpression>(identifier(bodyName), std::move(arguments));
        }
        if (isFunction) {
            statements.push_back(declare("__result", std::move(call)));
        } else {
            statements.push_back(statement(std::move(call)));
        }
        for (ast::StatementPtr& posCheck : posChecks) {
            statements.push_back(std::move(posCheck));
        }
        if (isFunction) {
            statements.push_back(std::make_unique<ast::ReturnStatement>(identifier("__result")));
        }

        added_.push_back(std::make_unique<ast::FunctionDeclaration>(bodyName, function.signatureTokens(),
                                                                    function.takeBody(), false));
        auto wrapper = std::make_unique<ast::FunctionDeclaration>(name, function.signatureTokens(),
                                                                  std::move(statements), function.isOverride());
        // The old declaration (an empty shell now) stays alive until the pass
        // ends: `routines_` still points at it.
        retired_.push_back(std::move(program_.items()[routine.index]));
        program_.items()[routine.index] = std::move(wrapper);
        routine.declaration = nullptr;
    }

    static std::string originNote(const Clause& clause, const std::string& name)
    {
        return lower(clause.origin) == lower(name) ? std::string() : " (from " + clause.origin + ")";
    }

    // Whether a written type moves (an owner, or a struct or array that
    // holds one): such a value is never copied (safe-pointers).
    bool movesOnly(const std::string& type, std::set<std::string> seen = {}) const
    {
        if (types::isGenericSpelling(type, "Own") || types::isGenericSpelling(type, "Option")) {
            return true;
        }
        if (const auto syntax = types::readTypeSyntax(type); syntax && syntax->form == types::TypeSyntax::Form::Array) {
            return movesOnly(syntax->argument->spelling(), seen);
        }
        const auto info = structs_.find(lower(type));
        if (info == structs_.end() || !seen.insert(lower(type)).second) {
            return false;
        }
        for (const ast::StructFieldDeclaration& field : info->second.declaration->fields) {
            if (movesOnly(field.typeName, seen)) {
                return true;
            }
        }
        return false;
    }

    // Whether an expression reads `name` outside `old`.
    static bool mentions(const ast::Expression& expression, const std::string& name)
    {
        switch (expression.kind()) {
        case ast::AstNodeKind::IdentifierExpression:
            return lower(static_cast<const ast::IdentifierExpression&>(expression).name()) == lower(name);
        case ast::AstNodeKind::UnaryExpression: {
            const auto& unary = static_cast<const ast::UnaryExpression&>(expression);
            return unary.op() != ast::UnaryOperator::Old && mentions(unary.operand(), name);
        }
        case ast::AstNodeKind::BinaryExpression: {
            const auto& binary = static_cast<const ast::BinaryExpression&>(expression);
            return mentions(binary.left(), name) || mentions(binary.right(), name);
        }
        case ast::AstNodeKind::ConditionalExpression: {
            const auto& conditional = static_cast<const ast::ConditionalExpression&>(expression);
            return mentions(conditional.condition(), name) || mentions(conditional.thenValue(), name) ||
                   mentions(conditional.elseValue(), name);
        }
        case ast::AstNodeKind::CallExpression: {
            const auto& call = static_cast<const ast::CallExpression&>(expression);
            bool found = mentions(call.callee(), name);
            for (const auto& argument : call.arguments()) {
                found = found || mentions(*argument, name);
            }
            return found;
        }
        default:
            return false;
        }
    }

    struct Parameter {
        std::string name;
        std::string type;
        bool byReference = false;
    };

    // The parameters of a routine, without Self, with their written types.
    static std::vector<Parameter> parameterList(const ast::FunctionDeclaration& function)
    {
        std::vector<Parameter> parameters;
        const auto& tokens = function.signatureTokens();
        if (tokens.empty() || tokens.front() != "(") {
            return parameters;
        }
        std::size_t index = 1;
        while (index < tokens.size() && tokens[index] != ")") {
            bool byReference = false;
            if (lower(tokens[index]) == "ref") {
                byReference = true;
                ++index;
                if (index < tokens.size() && lower(tokens[index]) == "mut") {
                    ++index;
                }
            }
            const std::string name = tokens[index++];
            std::string type;
            while (index < tokens.size() && tokens[index] != "," && tokens[index] != ")") {
                type += tokens[index++];  // the type (or `mut` after Self)
            }
            if (lower(name) != "self") {
                parameters.push_back(Parameter{name, type, byReference});
            }
            if (index < tokens.size() && tokens[index] == ",") {
                ++index;
            }
        }
        return parameters;
    }

    // The parameter names of a routine, without Self.
    static std::vector<std::string> parameterNames(const ast::FunctionDeclaration& function)
    {
        std::vector<std::string> names;
        const auto& tokens = function.signatureTokens();
        if (tokens.empty() || tokens.front() != "(") {
            return names;
        }
        std::size_t index = 1;
        while (index < tokens.size() && tokens[index] != ")") {
            if (lower(tokens[index]) == "ref") {
                ++index;
                if (index < tokens.size() && lower(tokens[index]) == "mut") {
                    ++index;
                }
            }
            const std::string name = tokens[index++];
            if (lower(name) != "self") {
                names.push_back(name);
            }
            while (index < tokens.size() && tokens[index] != "," && tokens[index] != ")") {
                ++index;  // the type (or `mut` after Self)
            }
            if (index < tokens.size() && tokens[index] == ",") {
                ++index;
            }
        }
        return names;
    }

    void addInvariantMethods()
    {
        for (const auto& [key, info] : structs_) {
            if (info.declaration->invariants.empty()) {
                continue;
            }
            std::vector<ast::StatementPtr> statements;
            for (const ContractClause& clause : info.declaration->invariants) {
                std::string text = clause.text;
                if (!clause.message.empty()) {
                    text += ": " + clause.message;
                }
                statements.push_back(check(3, ast::cloneExpression(*clause.condition),
                                           "invariant violated: " + info.name + ": " + text));
            }
            program_.items().push_back(std::make_unique<ast::FunctionDeclaration>(
                info.name + ".__invariant", std::vector<std::string>{"(", "Self", ")"}, std::move(statements), false));
        }
    }

    struct StructInfo {
        std::string name;
        std::vector<std::string> parts;
        const ast::TypeDeclaration* declaration = nullptr;
    };

    ast::ModuleNode& program_;
    std::map<std::string, Routine> routines_;
    std::map<std::string, StructInfo> structs_;
    std::map<std::string, Effective> memo_;
    std::set<std::string> visiting_;
    std::vector<std::unique_ptr<ast::FunctionDeclaration>> added_;
    std::vector<ast::AstNodePtr> retired_;
    std::size_t counter_ = 0;
};

} // namespace

void lowerContracts(ast::ModuleNode& program)
{
    ContractLowerer(program).run();
}

} // namespace inox::compiler::semantic
