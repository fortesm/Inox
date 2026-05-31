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
- `if`, `elif`, and `else` branches are opened by line breaks, without `then`
  or `:`.
- A single `;` closes a complete `if`/`elif`/`else` structure.
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
- `Type` is a section/declarator, not a block; it has no `:` and no closing `;`.
- `TName Struct ... ;` declares a nominal struct type; the `;` closes the Struct.
- Structs declare fields only. Associated methods are declared outside structs.
- The canonical boolean type is `Bool`.
- Integer bitwise operators are `bitand`, `bitor`, `bitxor`, `bitnot`, `shr`,
  and `shl`.
- `break` and `continue` apply to the innermost loop.
- `Return Expression` and expressionless `Exit` are distinct.
- `repeat` is a general loop. `until Condition` conditionally exits the nearest
  enclosing `repeat`.

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

Blocks are explicit semantic regions. Named blocks are opened by `:` and closed
by either `;` or `End`. Conditional branches are opened by a line break after
`if`, `elif`, or `else`, and one `;` closes the complete conditional structure.

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

`break` exits the innermost loop.

`continue` skips to the next iteration of the innermost loop.

Both are invalid outside loops. Full semantic validation may be implemented
after parser support.

`repeat` is a general loop. It does not require an `until` statement.

`until Condition` is valid only inside `repeat`. Its condition must have type
`Bool`. When its condition is true, execution exits the nearest enclosing
`repeat`; otherwise execution continues with the following statement in the
same `repeat` body. This is equivalent to a conditional loop exit.

An `until` statement may occur at the beginning, middle, or end of a `repeat`
body. A single `repeat` may contain multiple `until` statements.

A `repeat` body without `until` is an explicit infinite loop. It may still be
left by `break`, `Exit`, or `Return Expression`.

`Return Expression` exits the current subroutine and returns a value. It is the
canonical function-return form.

`Exit` exits the current subroutine immediately. It takes no expression and
returns no value. It does not replace `Return`. There is no implicit `Result`
and no `Return := Expression` form.

A subroutine without an explicit return type returns no value. It may be called
as a statement. It must not be used where a value-producing expression is
required.

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
- assignment to array elements, vector elements, struct fields, or other
  subobjects

No assignment behavior beyond strong typing, safe widening, and explicit casts
may be invented before the type system and assignment syntax are specified
canonically.

## Precedence

Operator precedence, from highest to lowest, is:

1. Parentheses
2. Calls, indexing, and member access
3. `^`
4. Unary `+`, unary `-`, `not`, and `bitnot`
5. `*`, `/`, `div`, and `mod`
6. `+` and `-`
7. `shl` and `shr`
8. `bitand`
9. `bitxor`
10. `bitor`
11. `..`
12. `in`
13. `=`, `#`, `<`, `>`, `<=`, and `>=`
14. `and`
15. `xor`
16. `or`
17. `:=`

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

The logical boolean operators are `and`, `or`, `xor`, and `not`.

The integer bitwise operators are `bitand`, `bitor`, `bitxor`, `bitnot`,
`shr`, and `shl`. The `^` operator is exponentiation and is never XOR.

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

Inox 0.1 pre-alpha has simple enums in Pascal/Ada style.

Enums are nominal types.

Enum values are finite and ordinal.

This document does not define:

- enum declaration syntax
- enumerator ordering
- underlying representation
- enum assignment
- enum comparison
- conversion to or from integers
- enum ranges

Compilers and tools must not invent enum behavior beyond the simple, nominal,
finite, ordinal model until it is specified in a canonical document.

## Conditions

Parentheses around conditions are optional.

The following two syntactic forms have the same semantic condition:

```inox
if Ready
    Return 1
;
```

```inox
if (Ready)
    Return 1
;
```

For `if`, `elif`, and `else`, the line break opens the branch. These constructs
do not use `then` or `:`. A single `;` closes the complete conditional
structure; it does not appear between branches. Indentation is visual only.

Conditions must have type `Bool`.

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
- `Ord`

The automatic prelude exposes these names without explicit `Use`.

## Out of Scope

This document does not define future language features. It also does not define
semantics for topics not yet specified by canonical syntax or type-system
documents.

## Struct Semantics

Structs are nominal value types. In the 0.1 backend subset, a struct declaration has the form:

```inox
Type
    TPoint Struct
        FX Integer
        FY Integer
    ;
```

A `Struct` declares fields only. It does not declare or repeat method signatures. Associated methods and protocol/behavior reuse are future layers outside the struct declaration itself.

A local struct variable can be declared in a `Var ... ;` block. `Var` does not use `:`:

```inox
Var
    P TPoint
;
```

The current LLVM prototype initializes local struct storage with a zero/default value, then applies any supported field default values. Field access is nominal and case-insensitive:

```inox
P.FX := 10
Return P.FX
```

The initial implemented field types are `Integer` and `Bool`. Field default values are supported for literal `Integer` and `Bool` defaults:

```inox
Type
    TConfig Struct
        FPort Integer := 8080
        FEnabled Bool := true
    ;
```

Variant structs, tags, embedding, general struct parameters, struct returns, and struct equality remain future work.


## Associated Method Semantics

Associated methods are functions or subroutines qualified by a nominal type name. The initial canonical form is:

```inox
TType.Method(Self TType, Args...) ReturnType :
    ...
;
```

The receiver is explicit in the declaration and is passed implicitly at the call site:

```inox
Value.Method(args)
```

This is syntactic and semantic association, not inheritance and not classical object orientation. A `Struct` declares data fields only. Method declarations remain outside the `Struct`, preserving the DRY rule that method signatures are not duplicated inside aggregate declarations.

The 0.1 backend subset supports associated methods on local struct variables, with field reads and field assignments through the explicit receiver parameter.

## Consolidated Semantic Reference

`docs/canonical/language-reference.md` and `docs/site/index.html` consolidate the 0.1 semantic decisions for humans and agents. The current semantic model is nominal, strongly typed, case-insensitive, post-object-oriented, and composition-oriented. Structs are value types; associated methods are statically resolved behavior associated with nominal struct types. Future behavior reuse must be explicit and statically checked, not duck typing.
