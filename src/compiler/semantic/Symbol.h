#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace inox::compiler::semantic {

enum class SymbolKind {
    Module,
    Function,
    Variable,
    Constant,
    State,
    Type,
    Builtin
};

struct Symbol {
    std::string name;
    std::string normalizedName;
    SymbolKind kind = SymbolKind::Variable;
    std::string typeName;
};

struct TypeSymbol {
    std::string name;
    std::string normalizedName;
    std::string aliasOf;
    bool isBuiltin = false;
};

class Scope {
public:
    explicit Scope(Scope* parent = nullptr);

    Scope* parent() const;
    const Symbol* declare(std::string name, SymbolKind kind, std::string typeName = {});
    const Symbol* resolve(std::string_view name) const;
    bool containsLocal(std::string_view name) const;
    bool containsInAncestors(std::string_view name) const;

private:
    static std::string normalize(std::string_view name);

    Scope* parent_ = nullptr;
    std::unordered_map<std::string, Symbol> symbols_;
};

class TypeTable {
public:
    const TypeSymbol* declare(std::string name, bool isBuiltin, std::string aliasOf = {});
    const TypeSymbol* resolve(std::string_view name) const;
    bool contains(std::string_view name) const;

private:
    static std::string normalize(std::string_view name);

    std::unordered_map<std::string, TypeSymbol> types_;
};

class SymbolTable {
public:
    SymbolTable();

    Scope& globalScope();
    const Scope& globalScope() const;

    Scope& pushScope();
    void popScope();
    Scope& currentScope();
    const Scope& currentScope() const;

private:
    std::vector<std::unique_ptr<Scope>> scopes_;
    Scope* current_ = nullptr;
};

} // namespace inox::compiler::semantic
