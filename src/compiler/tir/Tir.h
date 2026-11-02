// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// The typed intermediate representation (F6, change typed-ir; phase 5b in
// docs/ARCHITECTURE.md). A tree, as the AST after phases 2b and 4, in
// which every expression has a type and says what it is, never how it was
// written: a builtin is an operation, a variable a place with its origin, a
// call a routine of the program. tir/Lower builds it from the analyzed AST
// and the SemanticResult and decides nothing the analysis did not.
//
// Ownership (condition 3 of the design): every node lives in the arena of
// its Program and is trivially destructible; nodes refer to each other by
// pointer. The types (`const types::Type*`) belong to the type context of
// the SemanticResult, the symbols and signatures to the analyzer: both
// outlive the program during code generation. `origin` pointers back to
// the AST are tolerated during the migration (steps 1-4) and leave at step 5.

#include "../ast/Ast.h"
#include "../semantic/CallMeaning.h"
#include "../semantic/SemanticResult.h"
#include "../semantic/StdIntrinsics.h"
#include "../semantic/StdStrings.h"
#include "../semantic/Symbol.h"
#include "../types/Type.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <string_view>
#include <type_traits>
#include <vector>

namespace inox::compiler::tir {

using TypeRef = const types::Type*;

// A run of nodes or values in the arena.
template <typename T>
struct Span {
    T* items = nullptr;
    std::size_t count = 0;

    T* begin() const { return items; }
    T* end() const { return items + count; }
    std::size_t size() const { return count; }
    bool empty() const { return count == 0; }
    T& operator[](std::size_t index) const { return items[index]; }
};

// The memory of one Program: blocks that are freed together, nodes that
// need no destructor.
class Arena {
public:
    Arena() = default;
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    template <typename T>
    T* make()
    {
        static_assert(std::is_trivially_destructible_v<T>, "an arena node needs no destructor");
        return new (allocate(sizeof(T), alignof(T))) T();
    }

    template <typename T>
    Span<T> array(std::size_t count)
    {
        static_assert(std::is_trivially_destructible_v<T>, "an arena node needs no destructor");
        Span<T> span;
        span.count = count;
        if (count != 0) {
            span.items = static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
            for (std::size_t index = 0; index < count; ++index) {
                new (&span.items[index]) T();
            }
        }
        return span;
    }

    std::string_view text(std::string_view value);

    // Bytes taken from the system, and bytes given to nodes.
    std::size_t reserved() const { return reserved_; }
    std::size_t used() const { return used_; }

private:
    void* allocate(std::size_t size, std::size_t alignment);

    static constexpr std::size_t kBlockSize = 64 * 1024;
    std::vector<std::unique_ptr<std::byte[]>> blocks_;
    std::size_t offset_ = kBlockSize;
    std::size_t reserved_ = 0;
    std::size_t used_ = 0;
};

struct Expr;
struct Stmt;
struct Routine;

// The places (closed set, design of typed-ir): what can be read, written or
// passed by reference.
enum class PlaceKind : std::uint8_t {
    Local,           // a local variable of the routine
    Parameter,       // a parameter; `mode` says how it is passed
    Receiver,        // `Self` of a method; `isMutable` for `Self mut`
    LoopIterator,    // the iterator of a `for ... in`
    WithBinding,     // the binding of a `with`, identified by its statement
    HandlerBinding,  // `On E: X`
    State,           // a State variable of a module
    Field,           // field `field` of `base`
    Element,         // element `indices` of the array or slice `base`
    Through,         // the object `base` (an Own, an Option or a Link) points at
    Value            // a value that is not stored anywhere yet (`F(X).Field`)
};

struct Place {
    PlaceKind kind = PlaceKind::Local;
    TypeRef type = nullptr;
    // Can be written: a mutable variable, `ref mut`, `Self mut`, or a part
    // of one.
    bool isMutable = false;
    semantic::PassMode mode = semantic::PassMode::Value;  // Parameter, Receiver
    const semantic::Symbol* symbol = nullptr;  // Local .. State
    const Stmt* with = nullptr;                // WithBinding: its `with`
    const Place* base = nullptr;               // Field, Element, Through
    std::uint32_t field = 0;                   // Field
    Span<const Expr*> indices;                 // Element
    const Expr* value = nullptr;               // Value
};

enum class LiteralKind : std::uint8_t { Integer, Real, Decimal, String, Char, Bool, None };

enum class UnaryOp : std::uint8_t { Plus, Minus, Not, BitNot };

enum class BinaryOp : std::uint8_t {
    Add, Subtract, Multiply, Divide, Modulo, Remainder, ShiftLeft, ShiftRight, BitAnd, BitXor, BitOr, Power,
    Range, In, Equal, NotEqual, Less, Greater, LessEqual, GreaterEqual, And, Xor, Or
};

enum class ExprKind : std::uint8_t {
    Literal,      // `text` (`boolean` for a Bool)
    Read,         // the value of `place`
    Call,         // `routine` with `operands` (the receiver first for a method)
    Builtin,      // `op` with `operands`
    String,       // a function of Std.Strings / Std.StrUtils, `string`
    Intrinsic,    // a module intrinsic, `intrinsic`
    Unary,        // `unary` of operands[0]
    Binary,       // `binary` of operands[0] and operands[1]
    Convert,      // operands[0] converted to `type` (`T(X)`)
    Construct,    // a value of the struct `type`, `fields` in declaration order
    Conditional,  // operands: condition, then, else (ADR-0013)
    SetLiteral,   // operands: the elements (a Range is a Binary Range)
    Slice,        // operands: base, then the low and high bound when written (`hasLow`, `hasHigh`)
    Check,        // a contract check: operands[0] the condition, `checkKind`, `text` (CANON-24)
    Pure,         // operands[0], evaluated where it stands (`old X`, a loop variant)
    RangeCheck,   // operands[0] checked against the Range `type` on the way in (CANON-13)
    NaturalCheck, // operands[0] checked to be a Natural on the way in (ADR-0015)
    TypeValue     // a type written where a value goes (`for S in TSuit`, `Low(TSuit)`)
};

struct FieldValue {
    std::uint32_t field = 0;
    const Expr* value = nullptr;  // null: the field's default
};

// CANON-24: where a struct invariant must hold, as the analysis recorded it.
struct InvariantCheck {
    const Place* place = nullptr;
    TypeRef type = nullptr;  // a struct with an invariant
};
struct Invariants {
    Span<InvariantCheck> before;
    Span<InvariantCheck> after;
    TypeRef newValue = nullptr;  // a new value of this struct must satisfy its invariant
};

struct Expr {
    ExprKind kind = ExprKind::Literal;
    TypeRef type = nullptr;  // never null; Void for a call without a result
    ast::SourcePosition position;
    LiteralKind literal = LiteralKind::Integer;
    bool boolean = false;
    bool hasLow = false;
    bool hasHigh = false;
    std::int32_t checkKind = 0;
    std::string_view text;
    const Place* place = nullptr;
    const Routine* routine = nullptr;
    semantic::BuiltinOp op = semantic::BuiltinOp::None;
    semantic::StringFunction string = semantic::StringFunction::Length;
    const semantic::ModuleIntrinsic* intrinsic = nullptr;
    UnaryOp unary = UnaryOp::Plus;
    BinaryOp binary = BinaryOp::Add;
    Span<const Expr*> operands;
    Span<FieldValue> fields;
    const Invariants* invariants = nullptr;
};

enum class StmtKind : std::uint8_t {
    Evaluate,  // `value` for its effect (`discard`: `_ := F(X)`)
    Assign,    // `value` into `target` (`compound`: written `L op= R`)
    Declare,   // a local `symbol` of `type`, from `value` or its type's defaults
    If,        // `value` then `body`; `branches` are the elifs; `elseBody`
    Unless,    // `body` unless `value`
    While,     // while `value`: `body`
    Repeat,    // `body` again until an `Until` leaves it
    Until,     // leave the innermost repeat when `value` holds
    ForIn,     // `symbol` over `value` (`step` when written): `body`
    Case,      // `value` against `branches`; `elseBody` when `hasElse`
    Try,       // `body`, then `tryParts`
    Raise,     // the exception `text`; empty text re-raises the one being handled
    Retry,     // run the protected body again, `value` times at most
    Return,    // `value` (null for a subroutine)
    Exit,
    Leave,
    Continue,
    With,      // `symbol` bound to `value` (a Read of a place, or a value): `body`
    Block      // `body`
};

struct Branch {
    Span<const Expr*> conditions;  // If: one condition; Case: the choices
    Span<const Stmt*> body;
};

struct Handler {
    std::string_view typeName;  // as written (F7 gives the hierarchy its types)
    const semantic::Symbol* binding = nullptr;
    Span<const Stmt*> body;
};

struct TryParts {
    bool hasExcept = false;
    bool plainExcept = false;
    Span<const Stmt*> exceptBody;
    Span<Handler> handlers;
    Span<const Stmt*> elseBody;
    bool hasEnsure = false;
    Span<const Stmt*> ensureBody;
};

struct Stmt {
    StmtKind kind = StmtKind::Block;
    ast::SourcePosition position;
    bool compound = false;
    bool discard = false;
    bool hasElse = false;
    TypeRef type = nullptr;
    const semantic::Symbol* symbol = nullptr;
    const Place* target = nullptr;
    const Expr* value = nullptr;
    const Expr* step = nullptr;
    std::string_view text;
    Span<const Stmt*> body;
    Span<Branch> branches;
    Span<const Stmt*> elseBody;
    const TryParts* tryParts = nullptr;
    const Invariants* invariants = nullptr;
};

struct Parameter {
    const semantic::Symbol* symbol = nullptr;
    std::string_view name;
    TypeRef type = nullptr;
    semantic::PassMode mode = semantic::PassMode::Value;
    bool isReceiver = false;
};

struct Routine {
    std::string_view name;
    const ast::FunctionDeclaration* origin = nullptr;
    const semantic::FunctionSignature* signature = nullptr;
    bool isEntry = false;
    Span<Parameter> parameters;
    TypeRef result = nullptr;  // null for a subroutine
    Span<const Stmt*> body;
};

struct StateVariable {
    const semantic::Symbol* symbol = nullptr;
    std::string_view name;
    TypeRef type = nullptr;
    const Expr* initializer = nullptr;  // null: its type's defaults
};

struct Constant {
    const semantic::Symbol* symbol = nullptr;
    std::string_view name;
    TypeRef type = nullptr;
    const Expr* value = nullptr;  // a Literal
};

// One program: what code generation emits.
struct Program {
    Arena arena;
    Span<TypeRef> structTypes;       // declaration order, then the generic instances
    Span<StateVariable> states;      // source order
    Span<Constant> constants;        // source order
    Span<Routine*> routines;         // the program's, then the generic instances; the entry last
    const Routine* entry = nullptr;
};

std::string_view placeKindName(PlaceKind kind);
std::string_view exprKindName(ExprKind kind);
std::string_view stmtKindName(StmtKind kind);
std::string_view binaryOpText(BinaryOp op);
std::string_view unaryOpText(UnaryOp op);

} // namespace inox::compiler::tir
