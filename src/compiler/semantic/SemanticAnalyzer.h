// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "SemanticResult.h"
#include "StdIntrinsics.h"
#include "../ast/Ast.h"
#include "../types/TypeSyntax.h"

#include <cstdint>
#include <stdexcept>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace inox::compiler::semantic {

namespace detail {
struct IntegerConstant;
}

// A semantic diagnostic. The analyzer attaches where it was found (E14,
// change source-positions): the module and the start of the statement or
// declaration being checked; the driver turns the module into its file.
class SemanticError final : public std::runtime_error {
public:
    explicit SemanticError(std::string message);

    const ast::SourcePosition& position() const { return position_; }
    const std::string& module() const { return module_; }
    void locate(ast::SourcePosition position, std::string module)
    {
        position_ = position;
        module_ = std::move(module);
    }

private:
    ast::SourcePosition position_;
    std::string module_;
};

// A type of the analysis; null while not known (OpenSpec change
// type-representation).
using TypeRef = const types::Type*;

// A field or method a struct offers through its reused parts (CANON-9): the
// path names the parts from the struct to the member's owner.
struct ReusedMember {
    std::vector<std::string> path;
    std::string name;
    std::string owner;
    bool isMethod = false;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();

    const SemanticResult& analyze(const ast::ModuleNode& module);
    const SemanticResult& result() const;

private:
    void declareBuiltins();
    void declareBuiltinTypes();
    void declareOrThrow(std::string_view name, SymbolKind kind, TypeRef type = nullptr, bool isMutable = false);
    static void rejectPredefinedName(std::string_view name);
    const Symbol& resolveOrThrow(std::string_view name) const;
    void declareTypeName(std::string_view name, TypeRef type);
    // The type a written type denotes (`Array[1..Size] Byte`, `Own<TNode>`),
    // with the language's rules and messages; "unknown type" when a name is
    // not a type.
    TypeRef resolveWrittenType(std::string_view written);
    // Like resolveWrittenType, but null for an unknown plain name instead
    // of an error (a field type checked later, CANON-9).
    TypeRef lookupWrittenType(std::string_view written);
    TypeRef resolveTypeSyntax(const types::TypeSyntax& syntax, std::string_view written);
    TypeRef resolveArrayType(const types::TypeSyntax& syntax, std::string_view written);
    TypeRef analyzeIndexExpression(const ast::CallExpression& call);
    TypeRef analyzeSliceExpression(const ast::CallExpression& call);
    static bool isIndexTarget(const ast::Expression& expression);
    bool isSideEffectFree(const ast::Expression& expression) const;
    void checkReferenceArguments(const std::string& routine,
                                 const std::vector<FunctionParameter>& parameters,
                                 std::size_t parameterOffset,
                                 const std::vector<ast::ExpressionPtr>& arguments);
    const Symbol* writableRoot(const ast::Expression& expression) const;
    // The read-only root (a `ref` parameter, a plain `Self`, `with` on one of
    // them) whose owners a place is reached through; null when the place is
    // not owned through one. `ownerItself`: the place is an owner whose object
    // is meant (`Link(P)`, a `Self mut` call on P).
    const Symbol* readOnlyOwnerRoot(const ast::Expression& expression, bool ownerItself = false) const;
    std::string readOnlyOwnerMessage(const Symbol& root) const;
    // Throws "<prefix><message of root>". Out of line, so the recursive
    // analysis functions that call it keep their stack frames small.
    [[noreturn]] void throwReadOnlyOwner(std::string_view prefix, const Symbol& root) const;
    // Throws "<prefix><message>" when `target` is read-only through its
    // owners. Out of line for the same reason.
    void rejectReadOnlyOwnerTarget(const ast::Expression& target, const char* prefix) const;
    // `with` on a read-only reference, or on a place owned through one, only
    // reads what the target owns.
    void markWithBinding(const ast::WithStatement& withStatement);
    // A place rooted at a deep read-only symbol, through fields, elements and
    // owners (not through a Link).
    bool isDeepReadOnlyPlace(const ast::Expression& expression) const;

    void declareModuleItem(const ast::AstNode& item);
    void registerFunctionSignatures(const ast::ModuleNode& module);
    void registerFunctionSignatureIn(const ast::FunctionDeclaration& function);
    void registerFunctionSignature(const ast::FunctionDeclaration& function);
    const FunctionSignature* resolveFunctionSignature(std::string_view name) const;
    void analyzeModuleItem(const ast::AstNode& item);
    void declareSectionSymbols(const ast::SectionDeclaration& section);
    void recordConstantValue(std::string_view name, std::string_view valueToken);
    static void rejectInvalidConstantRightOperand(ast::BinaryOperator op, std::int64_t value, bool isUnsigned);
    void registerTypeSectionSymbols(const ast::SectionDeclaration& section);
    void registerStructDeclaration(const ast::TypeDeclaration& declaration);
    void registerEnumDeclaration(const ast::TypeDeclaration& declaration);
    void resolveRangeBounds(const ast::TypeDeclaration& declaration);
    // Enums (change enum-range-set; SemanticEnums.cpp).
    TypeRef enumTypeNamed(const ast::Expression& expression) const;
    TypeRef analyzeOrdinalBuiltin(const ast::CallExpression& call);
    void checkEnumConversion(const ast::CallExpression& call, TypeRef enumType);
    // Sets (change enum-range-set; SemanticSets.cpp).
    TypeRef resolveSetType(const types::TypeSyntax& syntax, std::string_view written);
    void requireSetElement(const ast::Expression& element, TypeRef setType, const std::string& where);
    TypeRef analyzeSetOperand(const ast::Expression& expression, TypeRef context);
    TypeRef analyzeSetLiteral(const ast::CallExpression& call);
    bool isSetFunctionCall(const ast::CallExpression& call) const;
    TypeRef analyzeSetFunction(const ast::CallExpression& call);
    // `Fill(Value)` (r3.81), in SemanticArrays.cpp: an array filled with
    // copies of Value, where an array is expected.
    bool isFillCall(const ast::CallExpression& call) const;
    TypeRef analyzeFill(const ast::CallExpression& call);
    // Char(I), TLetter(X) and String(C) (change char-values), SemanticEnums.cpp.
    void checkCharConversion(const ast::CallExpression& call, TypeRef target);
    // Conversions into a number (change float-conversions), SemanticEnums.cpp.
    void checkNumericConversion(const ast::CallExpression& call, TypeRef target);
    // A case on a String (change case-strings), SemanticStatements.cpp.
    void analyzeStringCase(const ast::CaseStatement& statement);
    bool staticStringValue(const ast::Expression& expression, std::string& value) const;
    static std::string arrayWithoutInitializer(const std::string& name, const std::string& written,
                                               TypeRef arrayType);
    TypeRef analyzeMembership(const ast::BinaryExpression& expression);
    // Once every Type of the module is declared: struct fields and Const and
    // State variables whose written type is a type name.
    void resolveDeclaredTypeNames();
    void completeStructFields(const ast::TypeDeclaration& declaration);
    void validateSectionTypes(const ast::SectionDeclaration& section);
    void validateStructDeclaration(const ast::TypeDeclaration& declaration);
    static std::string singleTokenText(const ast::Expression& expression);
    void validateStructFieldDefault(const std::string& fieldName,
                                    TypeRef fieldType,
                                    const std::string& defaultValue) const;
    void rejectRecursiveStruct(TypeRef structType) const;

    // Method reuse (CANON-9; SemanticReuse.cpp). A struct offers the fields
    // and methods of the structs it reuses, at every level, as long as each
    // name is reachable by exactly one path; `Override` replaces a reused
    // method. Access is rewritten to the explicit path, so a reused method
    // runs on its part (delegation).
    std::vector<std::string> ownMethodNames(std::string_view typeName) const;
    std::vector<ReusedMember> reusedMembers(TypeRef structType) const;
    std::optional<ReusedMember> findReusedMember(TypeRef structType, std::string_view name) const;
    void validateReuse(const ast::TypeDeclaration& declaration) const;
    void routeThroughParts(const ast::CallExpression& member, const std::vector<std::string>& path);
    void requireWritableReceiver(const FunctionSignature& signature, const ast::Expression& receiver) const;

    // Design by contract (CANON-24; SemanticContracts.cpp). `__check` and
    // `__pure` are the intrinsics the contract lowering writes; a contract
    // only reads (no Self mut method, ref mut argument, file or input).
    TypeRef analyzeContractCheck(const ast::CallExpression& call);
    TypeRef analyzePureCall(const ast::CallExpression& call);
    void rejectInContract(const std::string& what) const;
    bool hasInvariant(TypeRef type) const;
    void recordInvariantPlaces(const ast::AstNode& site, const ast::Expression& place, bool before, bool after);
    bool invariantChecksOn() const;
    bool inContract_ = false;
    std::string currentRoutine_;
    void analyzeFunction(const ast::FunctionDeclaration& function);

    void analyzeStatements(const std::vector<ast::StatementPtr>& statements, bool createScope);
    // CANON-12: "Functions must not fall through without returning a value."
    // Conservative: true only when control provably cannot reach the end.
    static bool cannotFallThrough(const std::vector<ast::StatementPtr>& statements);
    static bool cannotFallThrough(const ast::Statement& statement);
    void analyzeStatement(const ast::Statement& statement);
    void analyzeCaseStatement(const ast::CaseStatement& statement);
    void trackLocal(std::string_view name);
    void leaveScope();
    std::int64_t caseChoiceValue(const ast::Expression& choice, TypeRef selectorType);
    bool staticBoolValue(const ast::Expression& expression, bool& value) const;
    // A constant real expression (change float-conversions): literals, real
    // Consts, unary minus, + - * / and Float/Float32 conversions of them.
    bool staticRealValue(const ast::Expression& expression, double& value) const;
    bool staticCharValue(const ast::Expression& expression, std::int64_t& value) const;
    const Symbol* resolveConstant(const ast::Expression& expression) const;
    void recordConstantExpression(const std::string& name, const ast::Expression& value);
    void analyzeVarBlock(const ast::VarBlockStatement& statement);
    TypeRef analyzeExpression(const ast::Expression& expression);
    TypeRef inferExpressionType(const ast::Expression& expression);
    TypeRef analyzeCallExpression(const ast::CallExpression& expression);
    TypeRef analyzeMemberExpression(const ast::CallExpression& expression);
    TypeRef analyzeMethodCallExpression(const ast::CallExpression& expression);
    TypeRef analyzeBinaryExpression(const ast::BinaryExpression& expression);
    TypeRef analyzeUnaryExpression(const ast::UnaryExpression& expression);
    TypeRef analyzePreludeCall(std::string_view name, const std::vector<TypeRef>& argumentTypes) const;
    TypeRef analyzeInputCall(std::string_view name,
                             const std::vector<ast::ExpressionPtr>& arguments);
    // `argumentTypes`, when given, holds the types of the arguments, already
    // analyzed (a generic call infers its types from them first).
    TypeRef analyzeUserFunctionCall(const FunctionSignature& signature,
                                    const std::vector<ast::ExpressionPtr>& arguments,
                                    const std::vector<TypeRef>* argumentTypes = nullptr);
    TypeRef analyzeConstruction(const ast::CallExpression& call, TypeRef type,
                                const std::vector<TypeRef>* valueTypes);

    // ---- Generics (CANON-8, r3.77; SemanticGenerics.cpp) ----
    // A generic routine or struct is a template: each use with its types
    // gets a copy (an instance), checked and compiled like ordinary code.
    // Type arguments are inferred, never written in an expression; there
    // are no constraints, so an operation T lacks is an error of the
    // instance that needs it.
    void registerGenericTemplates(const ast::ModuleNode& module);
    const ast::FunctionDeclaration* genericRoutine(std::string_view name) const;
    const ast::TypeDeclaration* genericStruct(std::string_view name) const;
    // Analyzes `expression` knowing the type its value goes to (a typed
    // declaration, an assignment, a Return, a field): a generic call whose
    // types the arguments do not fix takes them from there.
    TypeRef analyzeExpressionFor(const ast::Expression& expression, TypeRef target);
    // A call of a generic routine, `Max(3, 7)`, or `Zero` without arguments.
    TypeRef analyzeGenericCall(const ast::FunctionDeclaration& generic, const ast::IdentifierExpression& callee,
                               const ast::CallExpression* call);
    // A construction of a generic struct, `TPair(FKey := "a", FValue := 1)`.
    TypeRef analyzeGenericConstruction(const ast::TypeDeclaration& generic, const ast::CallExpression& call);
    // `TPair<String, Integer>` written as a type.
    TypeRef resolveGenericStructType(const types::TypeSyntax& syntax, std::string_view written);
    // Binds the type parameters that `pattern` (a written type) names to the
    // parts of `actual`; a parameter already bound to another type is an error.
    // `fixed`, for a literal argument, holds what the other arguments fixed:
    // a literal never contradicts them (`Max(B, 1)` with B Byte is Max<UInt8>).
    void unifyTypeSyntax(const types::TypeSyntax& pattern, TypeRef actual, const std::vector<std::string>& parameters,
                         std::vector<TypeRef>& bound, const std::string& owner, const std::string& from,
                         const std::vector<TypeRef>* fixed);
    const FunctionSignature& instantiateRoutine(const ast::FunctionDeclaration& generic,
                                                const std::vector<TypeRef>& arguments);
    TypeRef instantiateStruct(const ast::TypeDeclaration& generic, const std::vector<TypeRef>& arguments);
    void analyzePendingInstances();
    static std::string instanceName(std::string_view name, const std::vector<TypeRef>& arguments);
    std::size_t instanceNesting(TypeRef type) const;
    void requireBoundedNesting(std::string_view generic, const std::vector<TypeRef>& arguments) const;
    void requireBoolCondition(const ast::Expression& expression);
    TypeRef inferSectionDeclarationType(const ast::Expression* value) const;
    // The struct type named `name`, or null.
    TypeRef resolveStruct(std::string_view name) const;
    static const types::StructField* resolveStructField(TypeRef structType, std::string_view fieldName);

    static bool looksLikeIdentifier(std::string_view text);
    static bool isInternalSyntheticName(std::string_view name);
    static bool isNumericType(TypeRef type);
    static bool isNumericLiteral(const ast::Expression& expression);
    static void checkDecimalLiteral(std::string_view text, TypeRef type);
    void checkDecimalConversion(const ast::CallExpression& call, TypeRef target);
    bool isRoundingCall(const ast::CallExpression& call) const;
    bool staticDecimalValue(const ast::Expression& expression, std::string& scaled) const;
    TypeRef analyzeRounding(const ast::CallExpression& call);
    static bool isIntegerType(TypeRef type);
    static bool isPreludeCall(std::string_view name);
    static bool isExceptionType(std::string_view name);
    static bool isMemberCall(const ast::CallExpression& expression);
    static std::string discardedResultMessage(const ast::Expression& expression);
    void analyzeDiscard(const ast::Expression& expression);
    static std::string normalizeName(std::string_view name);
    bool canAssign(TypeRef targetType, TypeRef valueType) const;
    static bool typesMatch(TypeRef left, TypeRef right);
    bool canAssignValue(TypeRef targetType, TypeRef valueType, const ast::Expression& value);

    types::TypeContext& types() { return result_.types(); }
    const types::TypeContext& types() const { return result_.types(); }

    // CANON-19: constant integer expressions are evaluated at compile time and
    // any overflow, division by zero or out-of-range shift is a compile error.
    void foldConstantExpression(const ast::Expression& expression);
    bool constantIntegerValue(const ast::Expression& expression, std::int64_t& value) const;
    // CANON-13 (range-types): the Range type a value belongs to: its own type
    // when that is a Range, else the Range its arithmetic was computed in
    // (`M + 1` belongs to TMeters); nullptr for any other value.
    TypeRef rangeFamilyOf(const ast::Expression& expression, TypeRef type) const;
    // A constant Integer that belongs to no Range: it mixes with any Range.
    bool isUniversalInteger(const ast::Expression& expression, TypeRef type) const;
    // The Range two operands share, nullptr when neither has one; mixing two
    // Ranges, or a Range with an Integer value, is an error (as in Ada).
    TypeRef requireOneRangeFamily(const ast::Expression& left, TypeRef leftType, const ast::Expression& right,
                                  TypeRef rightType, const std::string& context);
    // The constant with its domain: unsigned when the expression is a UInt64.
    bool integerConstant(const ast::Expression& expression, detail::IntegerConstant& value) const;
    void foldUnsignedBinary(const ast::BinaryExpression& binary, std::uint64_t a, std::uint64_t b);
    // Analyzes `expression` where a value of `context` is expected: a UInt64
    // context makes integer literals UInt64 (change uint64).
    TypeRef analyzeWithIntegerContext(const ast::Expression& expression, TypeRef context);
    void requireStatementExpression(const ast::Expression& expression);

    // Std.Files / Std.IOUtils (OpenSpec change file-io; SemanticFiles.cpp).
    // The File type and the file functions exist only in a program that uses
    // the module; a File is never copied (F6).
    void declareStandardModuleNames(const ast::ModuleNode& module);
    static bool isFileType(TypeRef type);
    // Whether `name` called with `argumentCount` arguments is a file function
    // (ReadLn with no argument is the console prelude function).
    static bool isFileFunction(std::string_view name, std::size_t argumentCount);
    TypeRef analyzeFileCall(std::string_view name, const ast::CallExpression& call);
    // Std.Terminal (SemanticIntrinsics.cpp, change console-abi-terminal).
    void declareModuleIntrinsics(const ast::ModuleNode& module);
    const ModuleIntrinsic* moduleIntrinsic(std::string_view name) const;
    TypeRef intrinsicType(const ModuleIntrinsic& function, std::string_view name);
    TypeRef intrinsicResultType(const ModuleIntrinsic& function);
    TypeRef analyzeIntrinsicIdentifier(const ast::IdentifierExpression& identifier, const ModuleIntrinsic& function);
    TypeRef analyzeIntrinsicCall(const ast::CallExpression& call, const ModuleIntrinsic& function);
    // Whether the module that declares this file function is used.
    bool fileFunctionVisible(std::string_view name) const;
    // Std.Strings / Std.StrUtils (change std-strings; SemanticStrings.cpp).
    void declareStringFunctionNames(const ast::ModuleNode& module);
    // A routine of the program (also a generic one) has this name: a standard
    // function of a module the program does not use leaves the name to it.
    bool userRoutineNamed(std::string_view name) const;
    bool stringFunctionVisible(std::string_view name) const;
    // `argumentTypes`, when given, holds the types of the arguments, already
    // analyzed (Length's argument is analyzed first to tell an array).
    TypeRef analyzeStringCall(const ast::CallExpression& call, const std::vector<TypeRef>* argumentTypes);
    // A value stored into a File must be a file just opened (OpenRead,
    // Create, OpenAppend): anything else would copy a File.
    void requireFreshFile(const ast::Expression& value) const;
    // A File may only be a local or a `ref mut` parameter.
    static void rejectFileIn(TypeRef type, const std::string& what);

    // Std.Debug (OpenSpec change debug-assert; SemanticDebug.cpp):
    // `Assert(Condition)` and `Assert(Condition, Message)`, visible only in a
    // program that uses the module.
    TypeRef analyzeAssertCall(const ast::CallExpression& call);

    // Safe pointers (OpenSpec change safe-pointers; SemanticPointers.cpp).
    // `Own<T>` is the single owner of a heap object, never copied (only
    // `Move`d); `Option<Own<T>>` may be `None`; `Link<T>` does not own and is
    // checked by a generation on each use. A move-only type is an owner or
    // an aggregate that holds one.
    TypeRef resolvePointerType(const types::TypeSyntax& syntax, std::string_view written);
    static bool isMoveOnlyType(TypeRef type);
    // A struct that holds an `Own<T>` field (directly, in a part or a struct
    // field) has no default value: it is built with its owners given.
    static std::string ownerFieldWithoutDefault(TypeRef type);
    static std::string requiredFieldWithoutDefault(TypeRef type);
    static std::string requiredFieldMessage(const std::string& name, TypeRef type, const std::string& field);
    void validateFillDefault(const std::string& fieldName, TypeRef fieldType, const std::string& literal) const;
    bool isFreshOwnerValue(const ast::Expression& value) const;
    // Assignability of pointer values: nullopt when no pointer is involved.
    std::optional<bool> canAssignPointer(TypeRef targetType, TypeRef valueType,
                                         const ast::Expression& value) const;
    bool isPointerBuiltinCall(std::string_view name, std::size_t argumentCount) const;
    TypeRef analyzePointerBuiltin(std::string_view name, const ast::CallExpression& call);
    // The type reached through a pointer for `.Field`, `[I]` and methods.
    static TypeRef typeThroughPointer(TypeRef type);
    void noteOwnerGivenAway(const ast::Expression& owner);
    void noteOwnerAssigned(const Symbol& symbol);
    void requireOwnerUsable(const Symbol& symbol) const;
    void forgetMovedOwnersOf(const Scope* scope);
    void requireOwnersBackAfterLoop(const std::unordered_map<const Symbol*, const Scope*>& start);
    // Owner variables given away by Move or Free on the paths analyzed so
    // far, with the scope that declared them.
    std::unordered_map<const Symbol*, const Scope*> movedOwners_;
    std::unordered_set<const Symbol*> currentParameters_;
    // The root of a place reached through a pointer: writable, owned by no
    // variable of the routine.
    Symbol pointerTargetRoot_;

    SymbolTable symbols_;
    SemanticResult result_;
    std::unordered_map<std::string, FunctionSignature> functions_;
    // The written field types of each struct, by struct, in field order: a
    // field's type is resolved once every Type and Const is declared.
    std::unordered_map<TypeRef, std::vector<std::string>> writtenFieldTypes_;
    // Const and State variables declared with a type name not declared yet
    // (a struct of a later Type section), by variable name.
    std::vector<std::pair<std::string, std::string>> pendingSectionTypes_;
    // State declarations written `Name Type` without ":=", checked once all
    // types are known: only structs may omit the initializer (CANON-5).
    std::vector<std::pair<std::string, std::string>> stateDeclarationsWithoutInitializer_;
    TypeRef currentFunctionReturnType_ = nullptr;
    bool currentFunctionSawReturn_ = false;
    std::size_t loopDepth_ = 0;
    std::size_t repeatDepth_ = 0;
    std::size_t exceptionHandlerDepth_ = 0;
    std::size_t retryHandlerDepth_ = 0;
    std::size_t ensureDepth_ = 0;
    bool hasMain_ = false;
    // r3.99: the program and the module whose declaration is analyzed.
    const ast::ModuleNode* program_ = nullptr;
    // Where the analysis is (change source-positions): the start of the
    // statement or declaration being checked, and its module.
    void notePosition(const ast::SourcePosition& position);
    void notePosition(const ast::AstNode& item);
    const SemanticResult& analyzeProgram(const ast::ModuleNode& module);
    ast::SourcePosition errorPosition_;
    std::string errorModule_;
    std::string currentModule_;
    void enterModule(const std::string& module);
    std::unordered_map<std::string, std::string> typeModules_;
    std::string moduleOfType(TypeRef type) const;
    bool outsideModuleOf(TypeRef type) const;
    void requireFieldVisible(TypeRef structType, const types::StructField& field) const;
    void requireMethodVisible(const FunctionSignature& signature) const;
    void requireBuildableHere(TypeRef type) const;
    std::string unknownNameText(std::string_view name) const;
    bool filesVisible_ = false;    // Std.Files: the File type and the basic operations
    bool ioUtilsVisible_ = false;  // Std.IOUtils: the conveniences (it uses Std.Files)
    // The modules whose intrinsics the program sees (Std.Terminal, ...).
    std::unordered_set<std::string> intrinsicModules_;
    bool mathVisible_ = false;      // the program uses Std.Math (Round, Trunc)
    bool debugVisible_ = false;    // the program uses Std.Debug (Assert)
    bool stringsVisible_ = false;  // Std.Strings: Length, Substr, Pos, case
    bool strUtilsVisible_ = false; // Std.StrUtils: the conveniences (it uses Std.Strings)
    std::unordered_map<const ast::Expression*, std::int64_t> constants_;
    // CANON-13 (range-types): expressions computed in a Range (`M + 1`),
    // typed in its base for code generation.
    std::unordered_map<const ast::Expression*, TypeRef> rangeFamilies_;
    // Char Consts (`Const Letter := 'A'`) by normalized name, as Unicode scalar
    // values; used for static case choices (ADR-0011).
    std::unordered_map<std::string, std::int64_t> charConstants_;
    // OPEN-4: locals declared in open scopes, in declaration order, and the
    // symbols read so far.
    struct TrackedLocal {
        const Symbol* symbol;
        const Scope* scope;
    };
    std::vector<TrackedLocal> localsToRead_;
    std::unordered_set<const Symbol*> readSymbols_;
    // ADR-0012: the copy of the target inside a desugared `X += R`; analyzing
    // it is not a read of X.
    std::unordered_set<const ast::IdentifierExpression*> updateOnlyReads_;

    // Generics: the templates by normalized name, the struct instances and
    // what each was made from, and the routine instances still to check.
    std::unordered_map<std::string, const ast::FunctionDeclaration*> genericRoutines_;
    std::unordered_map<std::string, const ast::TypeDeclaration*> genericStructs_;
    struct StructInstance {
        const ast::TypeDeclaration* generic = nullptr;
        std::vector<TypeRef> arguments;
    };
    std::unordered_map<TypeRef, StructInstance> structInstanceOf_;
    struct PendingInstance {
        const ast::FunctionDeclaration* declaration = nullptr;
        std::string origin;  // the routine whose call made it
    };
    std::vector<PendingInstance> pendingInstances_;
    // The expression analyzed for a known target type (analyzeExpressionFor).
    const ast::Expression* targetExpression_ = nullptr;
    TypeRef targetType_ = nullptr;
    // UInt64 where the expression being analyzed is expected to be one
    // (change uint64): a literal there is a UInt64, up to 2^64-1. It reaches
    // a literal through operators and IIf branches, never into a call.
    TypeRef integerContext_ = nullptr;
};

} // namespace inox::compiler::semantic
