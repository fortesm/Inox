// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace inox::compiler::ast {

enum class AstNodeKind {
    Module,
    UseDeclaration,
    SectionDeclaration,
    RawDeclaration,
    FunctionDeclaration,
    LiteralExpression,
    IdentifierExpression,
    BinaryExpression,
    UnaryExpression,
    CallExpression,
    ConditionalExpression,
    BlockStatement,
    ExpressionStatement,
    VarStatement,
    VarBlockStatement,
    IfStatement,
    UnlessStatement,
    WhileStatement,
    RepeatStatement,
    UntilStatement,
    ForInStatement,
    CaseStatement,
    TryStatement,
    RaiseStatement,
    RetryStatement,
    ReturnStatement,
    ExitStatement,
    LeaveStatement,
    ContinueStatement,
    WithStatement
};

enum class LiteralKind {
    Integer,
    Float,
    String,
    Char,
    Boolean
};

enum class BinaryOperator {
    Assign,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Remainder,
    ShiftLeft,
    ShiftRight,
    BitAnd,
    BitXor,
    BitOr,
    Power,
    Range,
    In,
    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    And,
    Xor,
    Or
};

enum class UnaryOperator {
    Plus,
    Minus,
    Not,
    BitNot,
    Old  // `old X` in a `pos` clause (CANON-24): X as it was on entry
};

enum class SectionKind {
    Type,
    Const,
    State
};

class AstNode {
public:
    explicit AstNode(AstNodeKind kind);
    virtual ~AstNode() = default;

    AstNodeKind kind() const;

private:
    AstNodeKind kind_;
};

using AstNodePtr = std::unique_ptr<AstNode>;

class Expression : public AstNode {
public:
    explicit Expression(AstNodeKind kind);
    ~Expression() override = default;

    // Height of the expression tree rooted here (a leaf is 1). The parser uses
    // it to enforce the documented implementation nesting limit (CANON-19, checked arithmetic and limits) so
    // that later recursive passes can never exhaust the native stack.
    std::size_t depth() const { return depth_; }

protected:
    std::size_t depth_ = 1;
};

class Statement : public AstNode {
public:
    explicit Statement(AstNodeKind kind);
    ~Statement() override = default;
};

using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;

struct ElseIfClause {
    ExpressionPtr condition;
    std::vector<StatementPtr> body;
};

struct CaseArm {
    std::vector<ExpressionPtr> choices;
    std::vector<StatementPtr> body;
};

struct ExceptionHandler {
    std::string bindingName;
    std::string typeName;
    std::vector<StatementPtr> body;
};

class ModuleNode final : public AstNode {
public:
    explicit ModuleNode(std::string name);

    const std::string& name() const;
    std::vector<AstNodePtr>& items();
    const std::vector<AstNodePtr>& items() const;
    std::vector<StatementPtr>& statements();
    const std::vector<StatementPtr>& statements() const;

    // The modules a program was built from (the entry module and every
    // module reached through Use), recorded by the driver; the analyzer
    // makes the intrinsics of a standard module visible only when the
    // program uses it (Std.Files, Std.IOUtils).
    void addLoadedModule(std::string name);
    const std::vector<std::string>& loadedModules() const;

    // r3.99 (module resolution): the modules each module sees (itself, what
    // it uses and what those re-export), the first words of the module names
    // and aliases a module writes in qualified names (no variable may take
    // one), and the explanation of a name a module cannot see, for the
    // analyzer's "unknown symbol" error. Keys are module names as written.
    std::vector<std::string> visibleModules(const std::string& module) const;
    void setVisibleModules(const std::string& module, std::vector<std::string> modules);
    bool reservesRoot(const std::string& module, std::string_view root) const;
    void addReservedRoot(const std::string& module, std::string root);
    std::string nameHint(const std::string& module, std::string_view name) const;
    void setNameHint(const std::string& module, std::string_view name, std::string hint);

private:
    std::vector<std::pair<std::string, std::vector<std::string>>> visibleModules_;
    std::vector<std::pair<std::string, std::string>> reservedRoots_;
    std::vector<std::pair<std::string, std::string>> nameHints_;
    std::string name_;
    std::vector<AstNodePtr> items_;
    std::vector<StatementPtr> statements_;
    std::vector<std::string> loadedModules_;
};

// `Use A.B`, `Export Use A.B` (r3.99: the users of this module also see the
// public declarations of A.B), `Use A.B as AB` (an alias for qualified names
// in this file) and `Use A.B.(X, Y)` (only X and Y without qualification).
class UseDeclaration final : public AstNode {
public:
    explicit UseDeclaration(std::vector<std::string> path, bool exported = false, std::string alias = {},
                            std::vector<std::string> names = {}, bool selective = false, std::size_t line = 0);

    const std::vector<std::string>& path() const;
    bool isExported() const { return exported_; }
    const std::string& alias() const { return alias_; }
    const std::vector<std::string>& names() const { return names_; }
    bool isSelective() const { return selective_; }
    std::size_t line() const { return line_; }

private:
    std::vector<std::string> path_;
    bool exported_ = false;
    std::string alias_;
    std::vector<std::string> names_;
    bool selective_ = false;
    std::size_t line_ = 0;
};

// A field of a Struct declaration (CANON-9). `typeName` uses the front end's
// spelling (`Array[L..H] T`, `Slice<T>` or a name); a default is one literal,
// with a leading `-` for a negative number.
struct StructFieldDeclaration {
    std::string name;
    std::string typeName;
    bool hasDefault = false;
    std::string defaultValue;
    // r3.82: `:= Fill(Literal)`, the default of an array field;
    // defaultValue is the literal.
    bool fillDefault = false;
    std::size_t line = 0;
    // Method reuse (CANON-9): `TName Struct (TPart)` reuses TPart. The part
    // is a field named after its type, whose fields and methods the struct
    // also offers directly.
    bool isPart = false;
    // r3.99: a field after the `Public` line of its struct is seen by other
    // modules; the others only by the module that declares the struct.
    bool isPublic = false;
};

// Design by contract (CANON-24): one `pre`, `pos`, `invariant` or `variant`
// line. `text` is the condition as written, for the run-time message;
// `message` is the optional String literal after a comma.
struct ContractClause {
    enum class Kind {
        Pre,            // pre Condition
        PreElse,        // pre else Condition (an Override weakens)
        Pos,            // pos Condition
        PosThen,        // pos then Condition (an Override strengthens)
        Invariant,      // invariant Condition, in a Struct
        LoopInvariant,  // invariant Condition, first lines of a loop
        LoopVariant     // variant IntegerExpression, first lines of a loop
    };
    Kind kind = Kind::Pre;
    ExpressionPtr condition;
    std::string text;
    std::string message;
    std::size_t line = 0;
};

enum class TypeDeclarationKind {
    Struct,
    Range,
    Enum
};

// One declaration of a Type section (CANON-5, CANON-13): a Struct with its
// fields, a Range with its bounds, or an Enum with its literals.
struct TypeDeclaration {
    TypeDeclarationKind kind = TypeDeclarationKind::Struct;
    std::string name;
    std::size_t line = 0;
    std::vector<StructFieldDeclaration> fields;
    ExpressionPtr bounds;  // Range: the `Low..High` expression
    std::vector<std::string> literals;
    std::vector<ContractClause> invariants;  // Struct: `invariant` lines (CANON-24)
    // CANON-8 (r3.77): `TPair<K, V> Struct` declares K and V; empty otherwise.
    std::vector<std::string> typeParameters;
    // r3.99: `TName Struct ... Public`; the module that declares the type
    // (set by module resolution).
    bool isPublic = false;
    std::string module;
};

// One Const or State declaration (CANON-5). `typeName` is empty for the
// inferred form `Name := Expr`; `value` is null for a State declaration that
// takes its type's defaults (`Name TStruct`). The value is parsed as an
// expression so semantic analysis checks it like any other (ADR-0009, CANON-9).
struct VariableDeclaration {
    std::string name;
    std::string typeName;
    ExpressionPtr value;
    std::size_t line = 0;
    // r3.99: `Const Name Public := Value`; a State variable is never public.
    bool isPublic = false;
};

class SectionDeclaration final : public AstNode {
public:
    SectionDeclaration(SectionKind sectionKind, std::vector<TypeDeclaration> types,
                       std::vector<VariableDeclaration> variables);

    SectionKind sectionKind() const;
    // The declarations of a Type section (empty for Const and State).
    const std::vector<TypeDeclaration>& types() const;
    // The declarations of a Const or State section (empty for Type).
    const std::vector<VariableDeclaration>& variables() const;
    std::vector<TypeDeclaration>& types() { return types_; }
    std::vector<VariableDeclaration>& variables() { return variables_; }
    // r3.99: the module the section was written in (module resolution).
    const std::string& module() const { return module_; }
    void setModule(std::string module) { module_ = std::move(module); }

private:
    std::string module_;
    SectionKind sectionKind_;
    std::vector<TypeDeclaration> types_;
    std::vector<VariableDeclaration> variables_;
};

class RawDeclaration final : public AstNode {
public:
    RawDeclaration(std::string head, std::vector<std::string> tokens);

    const std::string& head() const;
    const std::vector<std::string>& tokens() const;

private:
    std::string head_;
    std::vector<std::string> tokens_;
};

// The position of the dot of a method name, `TName.Method`; npos for a
// routine. The name of a generic instance has dots only inside its type
// arguments (`Max<Array[1..3] Int64>`), which never make it a method.
std::size_t methodDot(std::string_view routineName);

class FunctionDeclaration final : public AstNode {
public:
    FunctionDeclaration(std::string name,
                        std::vector<std::string> signatureTokens,
                        std::vector<StatementPtr> body,
                        bool isOverride = false,
                        std::vector<ContractClause> contracts = {},
                        std::vector<std::string> typeParameters = {});

    const std::string& name() const;
    // CANON-8 (r3.77): `Max<T>(A T, B T) T :` declares the type parameter T;
    // empty for an ordinary routine. A generic routine is a template: the
    // analyzer checks and compiles a copy per use.
    const std::vector<std::string>& typeParameters() const;
    bool isGeneric() const { return !typeParameters_.empty(); }
    const std::vector<std::string>& signatureTokens() const;
    const std::vector<StatementPtr>& body() const;
    // `TName.Method(...) [Type] Override :` replaces a reused method (CANON-9).
    bool isOverride() const;
    // `pre` / `pos` lines (CANON-24); the contract lowering pass moves them
    // into checks and leaves this empty.
    const std::vector<ContractClause>& contracts() const;
    std::vector<ContractClause> takeContracts();
    std::vector<StatementPtr> takeBody();
    std::vector<ContractClause>& contracts() { return contracts_; }
    std::vector<StatementPtr>& body() { return body_; }
    std::vector<std::string>& signatureTokens() { return signatureTokens_; }
    void setName(std::string name) { name_ = std::move(name); }
    // r3.99: `... Public :` puts the routine in its module's interface; the
    // module is the one it was written in (module resolution). A routine
    // made from another (contract wrappers, generic instances) takes both.
    bool isPublic() const { return isPublic_; }
    void setPublic(bool isPublic) { isPublic_ = isPublic; }
    const std::string& module() const { return module_; }
    void setModule(std::string module) { module_ = std::move(module); }
    void copyModuleFrom(const FunctionDeclaration& other)
    {
        isPublic_ = other.isPublic_;
        module_ = other.module_;
    }

private:
    bool isPublic_ = false;
    std::string module_;
    std::string name_;
    std::vector<std::string> signatureTokens_;
    std::vector<StatementPtr> body_;
    bool isOverride_ = false;
    std::vector<ContractClause> contracts_;
    std::vector<std::string> typeParameters_;
};

class LiteralExpression final : public Expression {
public:
    LiteralExpression(LiteralKind literalKind, std::string value);

    LiteralKind literalKind() const;
    const std::string& value() const;

private:
    LiteralKind literalKind_;
    std::string value_;
};

class IdentifierExpression final : public Expression {
public:
    explicit IdentifierExpression(std::string name);

    const std::string& name() const;

private:
    std::string name_;
};

class BinaryExpression final : public Expression {
public:
    BinaryExpression(BinaryOperator op, ExpressionPtr left, ExpressionPtr right);

    BinaryOperator op() const;
    const Expression& left() const;
    const Expression& right() const;
    ExpressionPtr takeLeft();
    ExpressionPtr takeRight();

private:
    BinaryOperator op_;
    ExpressionPtr left_;
    ExpressionPtr right_;
};

class UnaryExpression final : public Expression {
public:
    UnaryExpression(UnaryOperator op, ExpressionPtr operand);

    UnaryOperator op() const;
    const Expression& operand() const;
    ExpressionPtr takeOperand();

private:
    UnaryOperator op_;
    ExpressionPtr operand_;
};

class CallExpression final : public Expression {
public:
    CallExpression(ExpressionPtr callee, std::vector<ExpressionPtr> arguments);

    const Expression& callee() const;
    const std::vector<ExpressionPtr>& arguments() const;
    ExpressionPtr takeCallee();
    std::vector<ExpressionPtr> takeArguments();
    // Method reuse (CANON-9): the analyzer replaces the receiver of a member
    // access that reaches a reused part (`S.FName`) with the explicit path
    // (`S.TPerson.FName`), so every later phase sees ordinary field access.
    ExpressionPtr takeArgument(std::size_t index);
    void setArgument(std::size_t index, ExpressionPtr argument);

private:
    ExpressionPtr callee_;
    std::vector<ExpressionPtr> arguments_;
};

// ADR-0013: `if Condition then A else B`, an expression that selects a value.
// Only the chosen branch is evaluated; `else` is mandatory.
class ConditionalExpression final : public Expression {
public:
    ConditionalExpression(ExpressionPtr condition, ExpressionPtr thenValue, ExpressionPtr elseValue);

    const Expression& condition() const;
    const Expression& thenValue() const;
    const Expression& elseValue() const;

private:
    ExpressionPtr condition_;
    ExpressionPtr thenValue_;
    ExpressionPtr elseValue_;
};

class BlockStatement final : public Statement {
public:
    explicit BlockStatement(std::vector<StatementPtr> statements);

    const std::vector<StatementPtr>& statements() const;
    std::vector<StatementPtr> takeStatements();

private:
    std::vector<StatementPtr> statements_;
};

class ExpressionStatement final : public Statement {
public:
    explicit ExpressionStatement(ExpressionPtr expression, bool compoundAssignment = false, bool discard = false);

    const Expression& expression() const;
    ExpressionPtr takeExpression();
    // ADR-0012: `L += R` is stored as `L := L + R`; this flag records the
    // compound form, so the read of L it implies is not counted as a read.
    bool isCompoundAssignment() const;
    // r3.93 (E2): `_ := F(X)` calls F and drops its result on purpose. `_`
    // is no variable: the statement holds only the call.
    bool isDiscard() const;

private:
    ExpressionPtr expression_;
    bool compoundAssignment_ = false;
    bool discard_ = false;
};

class VarStatement final : public Statement {
public:
    VarStatement(bool isMutable,
                 std::string name,
                 ExpressionPtr initializer,
                 std::string typeName = {});

    bool isMutable() const;
    const std::string& name() const;
    const std::string& typeName() const;
    const Expression* initializer() const;
    ExpressionPtr takeInitializer();

private:
    bool isMutable_ = false;
    std::string name_;
    std::string typeName_;
    ExpressionPtr initializer_;
};

class VarBlockStatement final : public Statement {
public:
    explicit VarBlockStatement(std::vector<StatementPtr> declarations);

    const std::vector<StatementPtr>& declarations() const;
    std::vector<StatementPtr> takeDeclarations();

private:
    std::vector<StatementPtr> declarations_;
};

class IfStatement final : public Statement {
public:
    IfStatement(ExpressionPtr condition,
                std::vector<StatementPtr> thenBody,
                std::vector<ElseIfClause> elseIfClauses,
                std::vector<StatementPtr> elseBody);

    const Expression& condition() const;
    const std::vector<StatementPtr>& thenBody() const;
    const std::vector<ElseIfClause>& elseIfClauses() const;
    const std::vector<StatementPtr>& elseBody() const;

private:
    ExpressionPtr condition_;
    std::vector<StatementPtr> thenBody_;
    std::vector<ElseIfClause> elseIfClauses_;
    std::vector<StatementPtr> elseBody_;
};

class UnlessStatement final : public Statement {
public:
    UnlessStatement(ExpressionPtr condition, std::vector<StatementPtr> body);

    const Expression& condition() const;
    const std::vector<StatementPtr>& body() const;

private:
    ExpressionPtr condition_;
    std::vector<StatementPtr> body_;
};

class WhileStatement final : public Statement {
public:
    WhileStatement(ExpressionPtr condition, std::vector<StatementPtr> body);

    const Expression& condition() const;
    const std::vector<StatementPtr>& body() const;

private:
    ExpressionPtr condition_;
    std::vector<StatementPtr> body_;
};

class RepeatStatement final : public Statement {
public:
    explicit RepeatStatement(std::vector<StatementPtr> body);

    const std::vector<StatementPtr>& body() const;

private:
    std::vector<StatementPtr> body_;
};

class UntilStatement final : public Statement {
public:
    explicit UntilStatement(ExpressionPtr condition);

    const Expression& condition() const;

private:
    ExpressionPtr condition_;
};

class ForInStatement final : public Statement {
public:
    ForInStatement(std::string iterator,
                   ExpressionPtr iterable,
                   ExpressionPtr step,
                   std::vector<StatementPtr> body);

    const std::string& iterator() const;
    const Expression& iterable() const;
    const Expression* step() const;
    const std::vector<StatementPtr>& body() const;

private:
    std::string iterator_;
    ExpressionPtr iterable_;
    ExpressionPtr step_;
    std::vector<StatementPtr> body_;
};

class CaseStatement final : public Statement {
public:
    CaseStatement(ExpressionPtr expression,
                  std::vector<CaseArm> arms,
                  std::vector<StatementPtr> elseBody,
                  bool hasElse);

    const Expression& expression() const;
    const std::vector<CaseArm>& arms() const;
    const std::vector<StatementPtr>& elseBody() const;
    // True when the source has an `else` arm, even with an empty body.
    bool hasElse() const;

private:
    ExpressionPtr expression_;
    std::vector<CaseArm> arms_;
    std::vector<StatementPtr> elseBody_;
    bool hasElse_ = false;
};

class TryStatement final : public Statement {
public:
    TryStatement(std::vector<StatementPtr> body,
                 bool hasExcept,
                 bool plainExcept,
                 std::vector<StatementPtr> exceptBody,
                 std::vector<ExceptionHandler> handlers,
                 std::vector<StatementPtr> elseBody,
                 bool hasEnsure,
                 std::vector<StatementPtr> ensureBody);

    const std::vector<StatementPtr>& body() const;
    bool hasExcept() const;
    bool hasPlainExcept() const;
    const std::vector<StatementPtr>& exceptBody() const;
    const std::vector<ExceptionHandler>& handlers() const;
    const std::vector<StatementPtr>& elseBody() const;
    bool hasEnsure() const;
    const std::vector<StatementPtr>& ensureBody() const;
    bool hasTypedHandlers() const;

private:
    std::vector<StatementPtr> body_;
    bool hasExcept_ = false;
    bool plainExcept_ = false;
    std::vector<StatementPtr> exceptBody_;
    std::vector<ExceptionHandler> handlers_;
    std::vector<StatementPtr> elseBody_;
    bool hasEnsure_ = false;
    std::vector<StatementPtr> ensureBody_;
};

class RaiseStatement final : public Statement {
public:
    explicit RaiseStatement(ExpressionPtr expression);

    const Expression* expression() const;
    ExpressionPtr takeExpression();

private:
    ExpressionPtr expression_;
};

class RetryStatement final : public Statement {
public:
    explicit RetryStatement(ExpressionPtr count);

    const Expression& count() const;
    ExpressionPtr takeCount();

private:
    ExpressionPtr count_;
};

class ReturnStatement final : public Statement {
public:
    explicit ReturnStatement(ExpressionPtr expression);

    const Expression& expression() const;
    ExpressionPtr takeExpression();

private:
    ExpressionPtr expression_;
};

class ExitStatement final : public Statement {
public:
    ExitStatement();
};

class LeaveStatement final : public Statement {
public:
    LeaveStatement();
};

class ContinueStatement final : public Statement {
public:
    ContinueStatement();
};

class WithStatement final : public Statement {
public:
    WithStatement(ExpressionPtr target,
                  std::vector<StatementPtr> body,
                  std::string bindingName);

    const Expression& target() const;
    const std::vector<StatementPtr>& body() const;
    const std::string& bindingName() const;

private:
    ExpressionPtr target_;
    std::vector<StatementPtr> body_;
    std::string bindingName_;
};

} // namespace inox::compiler::ast
