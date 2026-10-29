// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Modules and visibility (OpenSpec change modules-encapsulation, r3.99,
// decisions of Marcelo Fortes, 2026-10-23 to 2026-10-29). See the header.

#include "ModuleResolution.h"

#include "../ast/AstClone.h"
#include "SemanticAnalyzer.h"

#include <cctype>
#include <functional>
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace inox::compiler::semantic {

namespace {

std::string lower(std::string_view text)
{
    std::string result(text);
    for (char& ch : result) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return result;
}

bool isWordStart(char ch)
{
    return std::isalpha(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

bool isWordPart(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

// The name a module's file gives its declarations' internal names: the
// module name, with any character an Inox name cannot hold replaced.
std::string internalPrefix(std::string_view module)
{
    std::string result;
    for (const char ch : module) {
        result += isWordPart(ch) || ch == '.' ? ch : '_';
    }
    if (result.empty() || !isWordStart(result.front())) {
        result.insert(result.begin(), 'M');
    }
    return result;
}

enum class DeclKind { Routine, Type, Literal, Const, State };

struct Decl {
    std::string module;
    std::string original;
    std::string internal;
    DeclKind kind = DeclKind::Routine;
    bool isPublic = false;
};

struct ModuleInfo {
    ast::ModuleNode* node = nullptr;
    std::string name;
    bool isEntry = false;
    std::map<std::string, Decl*> own;
    std::vector<const ast::UseDeclaration*> uses;
};

// Collects every name written in a module (identifiers in expressions,
// words in type spellings and signatures), to know which names of the entry
// file a library also writes.
class NameCollector {
public:
    explicit NameCollector(std::set<std::string>& names) : names_(names) {}

    void module(const ast::ModuleNode& module)
    {
        for (const auto& item : module.items()) {
            if (item->kind() == ast::AstNodeKind::FunctionDeclaration) {
                const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
                for (const std::string& token : function.signatureTokens()) {
                    text(token);
                }
                for (const ast::ContractClause& clause : function.contracts()) {
                    expression(*clause.condition);
                }
                statements(function.body());
            } else if (item->kind() == ast::AstNodeKind::SectionDeclaration) {
                const auto& section = static_cast<const ast::SectionDeclaration&>(*item);
                for (const ast::TypeDeclaration& type : section.types()) {
                    for (const ast::StructFieldDeclaration& field : type.fields) {
                        text(field.typeName);
                        text(field.defaultValue);
                    }
                    if (type.bounds) {
                        expression(*type.bounds);
                    }
                    for (const ast::ContractClause& clause : type.invariants) {
                        expression(*clause.condition);
                    }
                }
                for (const ast::VariableDeclaration& variable : section.variables()) {
                    text(variable.typeName);
                    if (variable.value) {
                        expression(*variable.value);
                    }
                }
            }
        }
    }

private:
    void text(const std::string& value)
    {
        std::size_t index = 0;
        while (index < value.size()) {
            if (!isWordStart(value[index])) {
                ++index;
                continue;
            }
            std::size_t end = index;
            while (end < value.size() && isWordPart(value[end])) {
                ++end;
            }
            names_.insert(lower(value.substr(index, end - index)));
            index = end;
        }
    }

    void expression(const ast::Expression& value)
    {
        (void)ast::cloneExpression(value, recorder());
    }

    void statements(const std::vector<ast::StatementPtr>& body)
    {
        ast::CloneRewriters rewriters{recorder(), [this](const std::string& name) {
                                          text(name);
                                          return name;
                                      }};
        (void)ast::cloneStatements(body, rewriters);
    }

    ast::ExpressionRewriter recorder()
    {
        return [this](const ast::Expression& node) -> ast::ExpressionPtr {
            if (node.kind() == ast::AstNodeKind::IdentifierExpression) {
                names_.insert(lower(static_cast<const ast::IdentifierExpression&>(node).name()));
            }
            return nullptr;
        };
    }

    std::set<std::string>& names_;
};

class Resolver {
public:
    explicit Resolver(std::vector<std::unique_ptr<ast::ModuleNode>> modules) : sources_(std::move(modules)) {}

    std::unique_ptr<ast::ModuleNode> run()
    {
        if (sources_.empty()) {
            throw SemanticError("program has no modules");
        }
        for (auto& source : sources_) {
            ModuleInfo info;
            info.node = source.get();
            info.name = source->name();
            modules_.push_back(std::move(info));
        }
        modules_.back().isEntry = true;
        for (ModuleInfo& module : modules_) {
            index_[lower(module.name)] = &module;
        }

        std::set<std::string> libraryNames;
        for (ModuleInfo& module : modules_) {
            if (!module.isEntry) {
                NameCollector(libraryNames).module(*module.node);
            }
        }
        for (ModuleInfo& module : modules_) {
            collectDeclarations(module, libraryNames);
        }
        for (ModuleInfo& module : modules_) {
            prepareUses(module);
        }
        for (ModuleInfo& module : modules_) {
            checkCoherence(module);
        }

        auto program = std::make_unique<ast::ModuleNode>(modules_.back().name);
        for (ModuleInfo& module : modules_) {
            program->addLoadedModule(module.name);
        }
        for (ModuleInfo& module : modules_) {
            current_ = &module;
            rewriteModule(module, *program);
            std::vector<std::string> visible{module.name};
            for (const ast::UseDeclaration* use : module.uses) {
                if (const ModuleInfo* target = find(use->path().front())) {
                    for (const ModuleInfo* reached : reexportClosure(*target)) {
                        visible.push_back(reached->name);
                    }
                } else {
                    visible.push_back(use->path().front());
                }
            }
            program->setVisibleModules(module.name, std::move(visible));
            for (const auto& [prefix, target] : qualifiers_[&module]) {
                if (target != &module) {
                    program->addReservedRoot(module.name, prefix.substr(0, prefix.find('.')));
                }
            }
        }
        for (const auto& [module, name, hint] : hints_) {
            program->setNameHint(module, name, hint);
        }
        return program;
    }

private:
    ModuleInfo* find(std::string_view name)
    {
        const auto found = index_.find(lower(name));
        return found == index_.end() ? nullptr : found->second;
    }

    Decl* declare(ModuleInfo& module, std::string original, DeclKind kind, bool isPublic,
                  const std::set<std::string>& libraryNames)
    {
        auto decl = std::make_unique<Decl>();
        decl->module = module.name;
        decl->original = original;
        decl->kind = kind;
        decl->isPublic = isPublic;
        const bool isMain = module.isEntry && lower(original) == "main";
        if (isMain || (module.isEntry && !libraryNames.contains(lower(original)))) {
            decl->internal = original;
        } else {
            // A routine's internal name joins its module with `::`, so it
            // is never read as a method (`TAccount.Deposit`, whose type may
            // itself be qualified, `Bank.Accounts.TAccount.Deposit`).
            decl->internal = internalPrefix(module.name) + (kind == DeclKind::Routine ? "::" : ".") + original;
        }
        Decl* raw = decl.get();
        decls_.push_back(std::move(decl));
        // A second declaration of the same name keeps the first in the
        // table; the analyzer reports the duplicate with the internal name.
        module.own.emplace(lower(original), raw);
        byName_[lower(original)].push_back(raw);
        return raw;
    }

    void collectDeclarations(ModuleInfo& module, const std::set<std::string>& libraryNames)
    {
        for (const auto& item : module.node->items()) {
            switch (item->kind()) {
            case ast::AstNodeKind::UseDeclaration:
                module.uses.push_back(static_cast<const ast::UseDeclaration*>(item.get()));
                break;
            case ast::AstNodeKind::FunctionDeclaration: {
                const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
                if (ast::methodDot(function.name()) == std::string::npos) {
                    declare(module, function.name(), DeclKind::Routine, function.isPublic(), libraryNames);
                }
                break;
            }
            case ast::AstNodeKind::SectionDeclaration: {
                const auto& section = static_cast<const ast::SectionDeclaration&>(*item);
                for (const ast::TypeDeclaration& type : section.types()) {
                    declare(module, type.name, DeclKind::Type, type.isPublic, libraryNames);
                    for (const std::string& literal : type.literals) {
                        declare(module, literal, DeclKind::Literal, type.isPublic, libraryNames);
                    }
                }
                for (const ast::VariableDeclaration& variable : section.variables()) {
                    const bool isConst = section.sectionKind() == ast::SectionKind::Const;
                    declare(module, variable.name, isConst ? DeclKind::Const : DeclKind::State,
                            isConst && variable.isPublic, libraryNames);
                }
                break;
            }
            default:
                break;
            }
        }
    }

    // The modules whose public declarations `module` gives: itself and,
    // through `Export Use`, the modules it re-exports, transitively.
    std::vector<const ModuleInfo*> reexportClosure(const ModuleInfo& module)
    {
        std::vector<const ModuleInfo*> result;
        std::set<const ModuleInfo*> seen;
        std::function<void(const ModuleInfo&)> visit = [&](const ModuleInfo& reached) {
            if (!seen.insert(&reached).second) {
                return;
            }
            result.push_back(&reached);
            for (const ast::UseDeclaration* use : reached.uses) {
                if (use->isExported()) {
                    if (const ModuleInfo* next = find(use->path().front())) {
                        visit(*next);
                    }
                }
            }
        };
        visit(module);
        return result;
    }

    // The public declarations named `name` that `module` gives its users.
    std::vector<Decl*> surface(const ModuleInfo& module, const std::string& name)
    {
        std::vector<Decl*> result;
        for (const ModuleInfo* reached : reexportClosure(module)) {
            const auto found = reached->own.find(name);
            if (found != reached->own.end() && found->second->isPublic &&
                std::find(result.begin(), result.end(), found->second) == result.end()) {
                result.push_back(found->second);
            }
        }
        return result;
    }

    // Each module's qualifiers (a used module's name, an alias, a module a
    // used module re-exports, the module's own name) and the names its
    // selective uses allow.
    void prepareUses(ModuleInfo& module)
    {
        auto& qualifiers = qualifiers_[&module];
        qualifiers[lower(module.name)] = &module;
        for (const ast::UseDeclaration* use : module.uses) {
            ModuleInfo* target = find(use->path().front());
            if (target == nullptr) {
                continue;  // a library module without declarations (Std.Strings)
            }
            if (!use->alias().empty()) {
                const std::string alias = lower(use->alias());
                if (qualifiers.contains(alias) || module.own.contains(alias)) {
                    throw SemanticError("the alias " + use->alias() + " of " + target->name +
                                        " is already a name in " + module.name + " (r3.99)");
                }
                qualifiers[alias] = target;
                aliased_[&module].insert(target);
            }
            for (const ModuleInfo* reached : reexportClosure(*target)) {
                if (reached == target && !use->alias().empty()) {
                    continue;  // with an alias, only the alias names the module
                }
                qualifiers.emplace(lower(reached->name), const_cast<ModuleInfo*>(reached));
            }
            if (use->isSelective()) {
                for (const std::string& name : use->names()) {
                    if (module.own.contains(lower(name))) {
                        throw SemanticError("Use " + target->name + ".(" + name + ") takes a name " + module.name +
                                            " declares itself (r3.99): remove " + name + " from the list");
                    }
                    if (surface(*target, lower(name)).empty()) {
                        const auto found = target->own.find(lower(name));
                        if (found != target->own.end()) {
                            throw SemanticError(name + " is not Public in " + target->name + " (r3.99)");
                        }
                        throw SemanticError(target->name + " has no public " + name +
                                            " for Use " + target->name + ".(...) (r3.99)");
                    }
                    selected_[{&module, target}].insert(lower(name));
                }
            }
        }
    }

    // The declaration a name written without qualification means in the
    // current module (A1): its own first, then the one public declaration
    // among what it uses. Nothing for a local name, a parameter or a
    // built-in.
    std::optional<std::string> resolveBare(const std::string& written)
    {
        if (written.size() >= 2 && written[0] == '_' && written[1] == '_') {
            return std::nullopt;
        }
        const std::string name = lower(written);
        if (excluded_.contains(name)) {
            return std::nullopt;
        }
        ModuleInfo& module = *current_;
        if (const auto own = module.own.find(name); own != module.own.end()) {
            return own->second->internal;
        }
        std::vector<Decl*> candidates;
        std::vector<std::string> sources;
        for (const ast::UseDeclaration* use : module.uses) {
            ModuleInfo* target = find(use->path().front());
            if (target == nullptr) {
                continue;
            }
            if (use->isSelective() && !selected_[{&module, target}].contains(name)) {
                continue;
            }
            for (Decl* decl : surface(*target, name)) {
                if (std::find(candidates.begin(), candidates.end(), decl) == candidates.end()) {
                    candidates.push_back(decl);
                    sources.push_back(qualifierFor(module, *decl));
                }
            }
        }
        if (candidates.size() == 1) {
            return candidates.front()->internal;
        }
        if (candidates.size() > 1) {
            std::string forms;
            for (std::size_t index = 0; index < sources.size(); ++index) {
                forms += (index == 0 ? "" : index + 1 == sources.size() ? " or " : ", ") + sources[index];
            }
            throw SemanticError(written + " is ambiguous in " + module.name + ": write " + forms + " (r3.99)");
        }
        recordHint(module, written, name);
        return std::nullopt;
    }

    // How the current module writes `decl` qualified: through an alias of
    // its module when it has one.
    std::string qualifierFor(const ModuleInfo& module, const Decl& decl)
    {
        for (const auto& [prefix, target] : qualifiers_[&module]) {
            if (target != nullptr && lower(target->name) == lower(decl.module) && prefix != lower(decl.module)) {
                for (const ast::UseDeclaration* use : module.uses) {
                    if (lower(use->alias()) == prefix) {
                        return use->alias() + "." + decl.original;
                    }
                }
            }
        }
        return decl.module + "." + decl.original;
    }

    void recordHint(const ModuleInfo& module, const std::string& written, const std::string& name)
    {
        const auto found = byName_.find(name);
        if (found == byName_.end()) {
            return;
        }
        for (const Decl* decl : found->second) {
            if (lower(decl->module) == lower(module.name)) {
                continue;
            }
            bool used = false;
            for (const ast::UseDeclaration* use : module.uses) {
                if (const ModuleInfo* target = find(use->path().front())) {
                    for (const ModuleInfo* reached : reexportClosure(*target)) {
                        used = used || lower(reached->name) == lower(decl->module);
                    }
                }
            }
            std::string hint = used ? written + " is not Public in " + decl->module + " (r3.99)"
                                    : written + " is declared in " + decl->module + ", which " + module.name +
                                          " does not use: write Use " + decl->module + " (r3.99)";
            if (used && decl->isPublic) {
                // Public, but left out of a selective Use (A4).
                const std::string qualified = qualifierFor(module, *decl);
                hint = written + " is not in the list of Use " + decl->module + ".(...): write " + qualified +
                       " or add " + written + " to the list (r3.99)";
            }
            hints_.emplace_back(module.name, written, std::move(hint));
            return;
        }
    }

    // `Bank.Accounts.Fee` or `Accounts.Fee`: the public declaration of a
    // module the current one uses (A2). `count` is how many leading names the
    // qualified name took; nothing when the names are not qualified.
    std::optional<std::string> resolveQualified(const std::vector<std::string>& names, std::size_t& count)
    {
        ModuleInfo& module = *current_;
        const auto& qualifiers = qualifiers_[&module];
        for (std::size_t length = names.size() - 1; length >= 1; --length) {
            std::string prefix = names[0];
            for (std::size_t index = 1; index < length; ++index) {
                prefix += "." + names[index];
            }
            const auto found = qualifiers.find(lower(prefix));
            if (found != qualifiers.end() && found->second != nullptr) {
                const ModuleInfo& target = *found->second;
                const std::string name = lower(names[length]);
                count = length + 1;
                if (&target == &module) {
                    if (const auto own = module.own.find(name); own != module.own.end()) {
                        return own->second->internal;
                    }
                    throw SemanticError(module.name + " has no " + names[length] + " (r3.99)");
                }
                const std::vector<Decl*> candidates = surface(target, name);
                if (candidates.size() == 1) {
                    return candidates.front()->internal;
                }
                if (candidates.size() > 1) {
                    throw SemanticError(prefix + "." + names[length] + " is ambiguous: " + target.name +
                                        " re-exports two of them (r3.99)");
                }
                if (const auto own = target.own.find(name); own != target.own.end()) {
                    throw SemanticError(names[length] + " is not Public in " + target.name + " (r3.99)");
                }
                throw SemanticError(target.name + " has no " + names[length] + " (r3.99)");
            }
            if (length == 1) {
                break;
            }
        }
        // A qualifier written with a module's real name after an alias.
        if (aliased_.contains(&module)) {
            for (const ModuleInfo* target : aliased_[&module]) {
                const std::string name = lower(target->name);
                std::string prefix;
                for (std::size_t index = 0; index + 1 < names.size(); ++index) {
                    prefix += (index == 0 ? "" : ".") + names[index];
                    if (lower(prefix) == name) {
                        throw SemanticError(target->name + " has an alias in " + module.name +
                                            ": write the alias instead of the module's name (r3.99)");
                    }
                }
            }
        }
        return std::nullopt;
    }

    static bool memberChain(const ast::Expression& expression, std::vector<std::string>& names)
    {
        if (expression.kind() == ast::AstNodeKind::IdentifierExpression) {
            names.push_back(static_cast<const ast::IdentifierExpression&>(expression).name());
            return true;
        }
        if (expression.kind() != ast::AstNodeKind::CallExpression) {
            return false;
        }
        const auto& call = static_cast<const ast::CallExpression&>(expression);
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression ||
            static_cast<const ast::IdentifierExpression&>(call.callee()).name() != "__member" ||
            call.arguments().size() != 2 || call.arguments()[1]->kind() != ast::AstNodeKind::IdentifierExpression) {
            return false;
        }
        if (!memberChain(*call.arguments()[0], names)) {
            return false;
        }
        names.push_back(static_cast<const ast::IdentifierExpression&>(*call.arguments()[1]).name());
        return true;
    }

    static ast::ExpressionPtr member(ast::ExpressionPtr object, const std::string& name)
    {
        std::vector<ast::ExpressionPtr> arguments;
        arguments.push_back(std::move(object));
        arguments.push_back(std::make_unique<ast::IdentifierExpression>(name));
        return std::make_unique<ast::CallExpression>(std::make_unique<ast::IdentifierExpression>("__member"),
                                                     std::move(arguments));
    }

    ast::ExpressionPtr rewrite(const ast::Expression& expression)
    {
        return ast::cloneExpression(expression, [this](const ast::Expression& node) { return replace(node); });
    }

    ast::ExpressionPtr replace(const ast::Expression& node)
    {
        if (node.kind() == ast::AstNodeKind::IdentifierExpression) {
            const auto& identifier = static_cast<const ast::IdentifierExpression&>(node);
            if (const auto internal = resolveBare(identifier.name())) {
                return std::make_unique<ast::IdentifierExpression>(*internal);
            }
            return nullptr;
        }
        if (node.kind() != ast::AstNodeKind::CallExpression) {
            return nullptr;
        }
        const auto& call = static_cast<const ast::CallExpression&>(node);
        const bool isMember = call.callee().kind() == ast::AstNodeKind::IdentifierExpression &&
                              static_cast<const ast::IdentifierExpression&>(call.callee()).name() == "__member" &&
                              call.arguments().size() == 2 &&
                              call.arguments()[1]->kind() == ast::AstNodeKind::IdentifierExpression;
        if (isMember) {
            std::vector<std::string> names;
            if (memberChain(node, names) && names.size() >= 2) {
                std::size_t count = 0;
                if (const auto internal = resolveQualified(names, count)) {
                    ast::ExpressionPtr result = std::make_unique<ast::IdentifierExpression>(*internal);
                    for (std::size_t index = count; index < names.size(); ++index) {
                        result = member(std::move(result), names[index]);
                    }
                    return result;
                }
            }
            // An ordinary member: the object is rewritten, the member name
            // is a field or a method, never a module's name.
            return member(rewrite(*call.arguments()[0]),
                          static_cast<const ast::IdentifierExpression&>(*call.arguments()[1]).name());
        }
        std::vector<ast::ExpressionPtr> arguments;
        for (const auto& argument : call.arguments()) {
            // `TPoint(FX := 1)`: the name before := is a field.
            if (argument->kind() == ast::AstNodeKind::BinaryExpression &&
                static_cast<const ast::BinaryExpression&>(*argument).op() == ast::BinaryOperator::Assign &&
                static_cast<const ast::BinaryExpression&>(*argument).left().kind() ==
                    ast::AstNodeKind::IdentifierExpression) {
                const auto& named = static_cast<const ast::BinaryExpression&>(*argument);
                arguments.push_back(std::make_unique<ast::BinaryExpression>(
                    ast::BinaryOperator::Assign, ast::cloneExpression(named.left()), rewrite(named.right())));
                continue;
            }
            arguments.push_back(rewrite(*argument));
        }
        return std::make_unique<ast::CallExpression>(rewrite(call.callee()), std::move(arguments));
    }

    // A written type (`TAccount`, `Bank.Accounts.TAccount`, `Own<TNode>`,
    // `Array[1..Capacity] TItem`): each name it holds, resolved.
    std::string rewriteText(const std::string& text)
    {
        std::string result;
        std::size_t index = 0;
        while (index < text.size()) {
            if (!isWordStart(text[index]) || (index > 0 && isWordPart(text[index - 1]))) {
                result += text[index++];
                continue;
            }
            std::vector<std::string> names;
            std::size_t end = index;
            while (true) {
                std::size_t start = end;
                while (end < text.size() && isWordPart(text[end])) {
                    ++end;
                }
                names.push_back(text.substr(start, end - start));
                if (end + 1 < text.size() && text[end] == '.' && isWordStart(text[end + 1])) {
                    ++end;
                    continue;
                }
                break;
            }
            std::string replaced;
            if (names.size() > 1) {
                std::size_t count = 0;
                if (const auto internal = resolveQualified(names, count); internal && count == names.size()) {
                    replaced = *internal;
                }
            } else if (const auto internal = resolveBare(names.front())) {
                replaced = *internal;
            }
            result += replaced.empty() ? text.substr(index, end - index) : replaced;
            index = end;
        }
        return result;
    }

    std::vector<ast::StatementPtr> rewriteBody(const std::vector<ast::StatementPtr>& body)
    {
        ast::CloneRewriters rewriters{[this](const ast::Expression& node) { return replace(node); },
                                      [this](const std::string& name) { return rewriteText(name); }};
        return ast::cloneStatements(body, rewriters);
    }

    void rewriteModule(ModuleInfo& module, ast::ModuleNode& program)
    {
        for (auto& item : module.node->items()) {
            if (item->kind() == ast::AstNodeKind::UseDeclaration) {
                continue;
            }
            if (item->kind() == ast::AstNodeKind::FunctionDeclaration) {
                auto& function = static_cast<ast::FunctionDeclaration&>(*item);
                rewriteFunction(module, function);
            } else if (item->kind() == ast::AstNodeKind::SectionDeclaration) {
                auto& section = static_cast<ast::SectionDeclaration&>(*item);
                rewriteSection(module, section);
            }
            program.items().push_back(std::move(item));
        }
    }

    void rewriteFunction(ModuleInfo& module, ast::FunctionDeclaration& function)
    {
        excluded_.clear();
        for (const std::string& parameter : function.typeParameters()) {
            excluded_.insert(lower(parameter));
        }
        function.setModule(module.name);
        const std::size_t dot = ast::methodDot(function.name());
        if (dot == std::string::npos) {
            function.setName(module.own.at(lower(function.name()))->internal);
        } else {
            const std::string receiver = function.name().substr(0, dot);
            const std::string method = function.name().substr(dot + 1);
            if (const auto own = module.own.find(lower(receiver));
                own != module.own.end() && own->second->kind == DeclKind::Type) {
                function.setName(own->second->internal + "." + method);
            }
        }
        for (std::string& token : function.signatureTokens()) {
            token = rewriteText(token);
        }
        for (ast::ContractClause& clause : function.contracts()) {
            clause.condition = rewrite(*clause.condition);
        }
        function.body() = rewriteBody(function.body());
        excluded_.clear();
    }

    void rewriteSection(ModuleInfo& module, ast::SectionDeclaration& section)
    {
        section.setModule(module.name);
        for (ast::TypeDeclaration& type : section.types()) {
            excluded_.clear();
            for (const std::string& parameter : type.typeParameters) {
                excluded_.insert(lower(parameter));
            }
            type.module = module.name;
            type.name = module.own.at(lower(type.name))->internal;
            for (ast::StructFieldDeclaration& field : type.fields) {
                field.typeName = rewriteText(field.typeName);
                if (!field.defaultValue.empty() && isWordStart(field.defaultValue.front()) &&
                    lower(field.defaultValue) != "true" && lower(field.defaultValue) != "false") {
                    if (const auto internal = resolveBare(field.defaultValue)) {
                        field.defaultValue = *internal;
                    }
                }
            }
            if (type.bounds) {
                type.bounds = rewrite(*type.bounds);
            }
            for (std::string& literal : type.literals) {
                literal = module.own.at(lower(literal))->internal;
            }
            for (ast::ContractClause& clause : type.invariants) {
                clause.condition = rewrite(*clause.condition);
            }
            excluded_.clear();
        }
        for (ast::VariableDeclaration& variable : section.variables()) {
            variable.name = module.own.at(lower(variable.name))->internal;
            variable.typeName = rewriteText(variable.typeName);
            if (variable.value) {
                variable.value = rewrite(*variable.value);
            }
        }
    }

    // The own private type a written type names, if any.
    const Decl* privateTypeIn(const ModuleInfo& module, const std::string& text) const
    {
        std::size_t index = 0;
        while (index < text.size()) {
            if (!isWordStart(text[index]) || (index > 0 && isWordPart(text[index - 1]))) {
                ++index;
                continue;
            }
            std::size_t end = index;
            while (end < text.size() && isWordPart(text[end])) {
                ++end;
            }
            const auto found = module.own.find(lower(text.substr(index, end - index)));
            if (found != module.own.end() && found->second->kind == DeclKind::Type && !found->second->isPublic) {
                return found->second;
            }
            index = end;
        }
        return nullptr;
    }

    // Nothing public exposes something private (r3.99), and a method is
    // declared in the module of its type.
    void checkCoherence(const ModuleInfo& module)
    {
        for (const auto& item : module.node->items()) {
            if (item->kind() == ast::AstNodeKind::FunctionDeclaration) {
                const auto& function = static_cast<const ast::FunctionDeclaration&>(*item);
                const std::size_t dot = ast::methodDot(function.name());
                if (dot != std::string::npos) {
                    const std::string receiver = function.name().substr(0, dot);
                    const auto own = module.own.find(lower(receiver));
                    if (own == module.own.end()) {
                        const auto elsewhere = byName_.find(lower(receiver));
                        if (elsewhere != byName_.end()) {
                            for (const Decl* decl : elsewhere->second) {
                                if (decl->kind == DeclKind::Type) {
                                    throw SemanticError("the methods of " + receiver + " are declared in its module, " +
                                                        decl->module + " (r3.99): " + function.name() +
                                                        " is in " + module.name);
                                }
                            }
                        }
                    } else if (function.isPublic() && !own->second->isPublic) {
                        throw SemanticError(function.name() + " is Public, but " + receiver +
                                            " is not (r3.99): write Public at the end of the header of " + receiver +
                                            ", or remove it from the method");
                    }
                }
                if (function.isPublic()) {
                    for (const std::string& token : function.signatureTokens()) {
                        if (const Decl* hidden = privateTypeIn(module, token)) {
                            throw SemanticError(function.name() + " is Public, but its header names " +
                                                hidden->original + ", which is not (r3.99): a public routine never "
                                                "exposes a private type");
                        }
                    }
                }
                continue;
            }
            if (item->kind() != ast::AstNodeKind::SectionDeclaration) {
                continue;
            }
            const auto& section = static_cast<const ast::SectionDeclaration&>(*item);
            for (const ast::TypeDeclaration& type : section.types()) {
                for (const ast::StructFieldDeclaration& field : type.fields) {
                    if (field.isPublic && !type.isPublic) {
                        throw SemanticError("the public fields of " + type.name + " are not seen outside " +
                                            module.name + ": " + type.name + " is not Public (r3.99); write "
                                            "Public at the end of its header, or remove the line Public");
                    }
                    if (!type.isPublic || !(field.isPublic || field.isPart)) {
                        continue;
                    }
                    if (const Decl* hidden = privateTypeIn(module, field.typeName)) {
                        throw SemanticError(type.name + " is Public, but " +
                                            (field.isPart ? "it reuses " : "its public field " + field.name +
                                                                               " is a ") +
                                            hidden->original + ", which is not Public (r3.99)");
                    }
                }
            }
        }
    }

    std::vector<std::unique_ptr<ast::ModuleNode>> sources_;
    std::vector<ModuleInfo> modules_;
    std::map<std::string, ModuleInfo*> index_;
    std::vector<std::unique_ptr<Decl>> decls_;
    std::map<std::string, std::vector<Decl*>> byName_;
    std::map<const ModuleInfo*, std::map<std::string, ModuleInfo*>> qualifiers_;
    std::map<const ModuleInfo*, std::set<const ModuleInfo*>> aliased_;
    std::map<std::pair<const ModuleInfo*, const ModuleInfo*>, std::set<std::string>> selected_;
    std::vector<std::tuple<std::string, std::string, std::string>> hints_;
    std::set<std::string> excluded_;
    ModuleInfo* current_ = nullptr;
};

} // namespace

std::unique_ptr<ast::ModuleNode> resolveModules(std::vector<std::unique_ptr<ast::ModuleNode>> modules)
{
    return Resolver(std::move(modules)).run();
}

} // namespace inox::compiler::semantic
