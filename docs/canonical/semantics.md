# Inox 0.1 pre-alpha - Semantics

This document is the canonical semantics specification for Inox 0.1 pre-alpha.

Only semantics already decided by `AGENTS.md` and
`docs/canonical/syntax.md` are documented here. Topics listed here but not yet
decided are explicitly marked as unspecified.

## Canonical Rules

- Inox is case-insensitive.
- Inox is strongly typed.
- Inox is Ada/ObjectPascal-like.
- Shadowing is forbidden.
- `;` closes blocks and is equivalent to `End`.
- `;` is not a statement terminator.
- `:` opens named blocks.
- Parentheses in conditions are optional.
- The official backend is LLVM.
- Inox 0.1 pre-alpha must remain small and compilable.
- `Integer` is `Int64`.
- `Float` is `Float64`.
- `Currency` and `Crypto` exist.
- `Decimal` does not exist.
- `State` is used for mutable global state.
- Arrays have explicit ranges.
- Vectors are dynamic and 0-based.
- Sets follow Pascal/Ada style with a finite ordinal base.
- Exceptions exist since 0.1.
- Exceptions are lightweight, without heavy RTTI.
- Casts use `TypeName(expr)`.
- Implicit conversions are allowed only for safe widening.
- `Module` closes at EOF.
- The automatic prelude includes `Sys.IO`, `Sys.Math`, and `Sys.Std`.
- `case` follows Ada/SPARK style, without fallthrough or `break`.
- `otherwise` is mandatory when a `case` is not exhaustive.
- Assignment `:=` is right-associative.
- Chained assignment is allowed.
- Assignment inside a boolean expression is forbidden.

## Case Insensitivity

Inox programs are case-insensitive.

Names that differ only by letter case denote the same language-level name.

Examples:

```inox
Value
value
VALUE
```

These spellings are semantically the same name.

The same rule applies to reserved words and language-defined names.

Because the language is case-insensitive, a program must not declare distinct
entities whose names differ only by case.

## Scope

Inox has scoped named entities.

Inline `var` declares in the current block.

`Var` blocks declare in the current block.

Names declared inside `if`, `while`, `for`, `repeat`, `case`, or `try` only
exist inside that block.

Global `Const` is allowed.

`State` is the only mechanism for mutable global state.

The canonical semantic restriction already decided is that shadowing is
forbidden.

This document does not yet define declaration order rules, visibility between
sibling blocks, visibility between nested blocks except for the block-local
rule above, or import and export visibility.

## Shadowing

Shadowing is forbidden.

A declaration must not introduce a name that hides, replaces, or competes with
another visible declaration of the same name.

Because Inox is case-insensitive, shadowing checks are also case-insensitive.

For example, if `Value` is already visible, then `value` and `VALUE` are the
same name for semantic purposes and must not be accepted as separate
declarations.

The precise diagnostic wording is not specified yet.

## State

`State` is used for mutable global state.

`State` is the only mechanism for mutable global state.

The detailed declaration syntax, initialization rules, visibility rules, and
mutation rules for `State` are not specified yet.

## Blocks

Blocks are explicit semantic regions corresponding to the syntax opened by `:`
and closed by either `;` or `End`.

The following two block endings are semantically equivalent:

```inox
Name :
    ...
;
```

```inox
Name :
    ...
End
```

The semicolon closes the current block. It has no independent statement
termination semantics.

The semantic meaning of each specific block kind is not specified yet. This
document does not define the semantics of modules, functions, procedures,
control-flow blocks, exception blocks, or state blocks.

`Module` closes at EOF.

For module blocks, EOF acts as the module boundary.

## Statements

Statements are not terminated by `;`.

The full statement semantics for Inox 0.1 pre-alpha are not specified yet.

## Assignment

Inox is strongly typed, so assignment must respect the type system.

Assignment uses `:=`.

Assignment `:=` is right-associative.

Chained assignment is allowed.

Assignment inside a boolean expression is forbidden.

Implicit conversions during assignment are allowed only for safe widening.

Casts use `TypeName(expr)`.

The concrete assignment syntax and full assignment semantics are not specified
yet. This document does not yet define:

- assignment operators
- assignable entities
- initialization versus reassignment
- mutability rules
- copy, move, or reference behavior
- assignment to array elements, vector elements, record fields, or other
  subobjects

No assignment behavior beyond strong typing, safe widening, and explicit casts
may be invented before the type system and assignment syntax are specified
canonically.

## Precedence

Operator precedence, from highest to lowest, is:

1. Parentheses
2. Calls, indexing, and member access
3. `^`
4. Unary `+`, unary `-`, and `not`
5. `*`, `/`, `div`, and `mod`
6. `+` and `-`
7. `..`
8. `in`
9. `=`, `#`, `<`, `>`, `<=`, and `>=`
10. `and`, `xor`, and `or`
11. `:=`

The `^` operator is exponentiation and associates to the right.

Exponentiation has higher precedence than unary operators. Therefore:

```inox
-x^2
```

means:

```inox
-(x^2)
```

Assignment `:=` is right-associative.

Chained assignment is allowed.

Assignment inside a boolean expression is forbidden.

## Conversion Rules

Inox is strongly typed.

Implicit conversions are allowed only for safe widening.

Casts use `TypeName(expr)`.

This document does not define:

- numeric narrowing
- boolean conversions
- string conversions
- enum conversions
- array conversions
- set conversions
- user-defined conversions

No conversion other than safe implicit widening or explicit `TypeName(expr)`
casts must be accepted unless it is defined by a canonical specification.

## Exceptions

Exceptions exist in Inox 0.1 pre-alpha.

Exception syntax includes:

```inox
try
except
finally
raise
```

Exceptions are lightweight, without heavy RTTI.

This document does not define:

- exception declaration syntax
- unwinding
- cleanup semantics
- checked or unchecked exception rules

## Case

`case` follows Ada/SPARK style.

`case` has no fallthrough.

`case` does not use `break`.

`case` accepts individual values and ranges.

`otherwise` is mandatory when the `case` is not exhaustive.

For enums, if all values are covered, `otherwise` is not mandatory.

## Arrays

Arrays have explicit ranges.

This document does not define:

- array declaration syntax
- index types
- indexing semantics
- mutability
- allocation
- value versus reference behavior
- array assignment
- array comparison

No array behavior beyond explicit ranges may be invented until it is specified
in a canonical document.

## Vectors

Vectors are dynamic and 0-based.

This document does not define:

- vector declaration syntax
- element types
- allocation
- resizing rules
- indexing semantics beyond the 0-based rule
- mutability
- value versus reference behavior
- vector assignment
- vector comparison

No vector behavior beyond dynamic size and 0-based indexing may be invented
until it is specified in a canonical document.

## Sets

Sets follow Pascal/Ada style with a finite ordinal base.

This document does not define:

- set declaration syntax
- set literals
- membership
- set operations
- mutability
- set assignment
- set comparison

No set behavior beyond the finite ordinal base rule may be invented until it is
specified in a canonical document.

## Enums

Enum semantics are not specified yet for Inox 0.1 pre-alpha.

This document does not define:

- enum declaration syntax
- enumerator ordering
- underlying representation
- enum assignment
- enum comparison
- conversion to or from integers
- enum ranges

Compilers and tools must not invent enum behavior until it is specified in a
canonical document.

## Conditions

Parentheses around conditions are optional.

The following two syntactic forms have the same semantic condition:

```inox
Construct condition :
    ...
;
```

```inox
Construct (condition) :
    ...
;
```

The semantics of truth values, conditional constructs, and condition expression
typing are not specified yet.

## Primitive Numeric Types

`Integer` is `Int64`.

`Float` is `Float64`.

`Currency` and `Crypto` exist.

`Decimal` does not exist in Inox 0.1 pre-alpha.

`Currency` is exact monetary decimal.

`Currency` is never `Float64`.

`Currency` is intended for international fiat money.

Detailed `Currency` rounding semantics belong in `runtime.md`.

`Crypto` is exact high-precision decimal.

`Crypto` is never `Float64`.

`Crypto` is intended for cryptoassets.

Detailed `Crypto` support for scales and networks belongs in `runtime.md`.

## Generics

Generics use `[]`.

The detailed semantics of generic declaration, instantiation, constraints, and
monomorphization are not specified yet.

## Prelude

The automatic prelude includes:

- `Sys.IO`
- `Sys.Math`
- `Sys.Std`

`Sys.IO` provides:

- `Put`
- `PutLn`
- `ReadLn`

`Sys.Math` provides:

- `Sin`
- `Cos`
- `Sqrt`
- `Abs`

`Sys.Std` provides:

- `Length`

The automatic prelude exposes these names without explicit `Use`.

## Out of Scope

This document does not define future language features. It also does not define
semantics for topics not yet specified by canonical syntax or type-system
documents.
