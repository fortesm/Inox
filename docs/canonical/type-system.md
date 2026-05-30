# Inox 0.1 pre-alpha - Type System

This document is the canonical type-system specification for Inox 0.1
pre-alpha.

It is based on `AGENTS.md`, `docs/canonical/syntax.md`, and
`docs/canonical/semantics.md`. It documents only decisions that are canonical
for Inox 0.1 pre-alpha.

## Core Model

Inox is strongly typed, statically typed, and nominally typed.

Types are checked at compile time wherever enough information is available.
Operations must be valid for the declared or inferred types of their operands.

Type identity is nominal. Two distinct named types are not the same type merely
because they have the same representation or structure.

Inox has no universal `Any` type in 0.1 pre-alpha.

## Case Insensitivity

Type names are case-insensitive.

For example, these spellings denote the same type name:

```inox
Integer
integer
INTEGER
```

A program must not declare distinct types whose names differ only by case.

## Boolean

`Bool` is the boolean type.

`Boolean` is not a built-in type.

The literals `true` and `false` have type `Bool`.

The detailed truth-value operations and conditional typing rules are not
specified in this document.

## Signed Integers

Inox 0.1 pre-alpha defines these signed integer types:

- `Int8`
- `Int16`
- `Int32`
- `Int64`

`Integer` is an alias for `Int64`.

## Unsigned Integers

Inox 0.1 pre-alpha defines these unsigned integer types:

- `UInt8`
- `UInt16`
- `UInt32`
- `UInt64`

`UInteger` is an alias for `UInt64`.

## Natural

`Natural` is a non-negative integer type.

Its exact range and representation are not specified in this document.

## Floating-Point Types

Inox 0.1 pre-alpha defines these floating-point types:

- `Float32`
- `Float64`

`Float` is an alias for `Float64`.

## Currency

`Currency` exists in Inox 0.1 pre-alpha.

`Currency` is an exact monetary decimal type.

`Currency` is never `Float64`.

`Currency` is intended for international fiat money.

Detailed rounding semantics for `Currency` belong in `runtime.md`.

## Crypto

`Crypto` exists in Inox 0.1 pre-alpha.

`Crypto` is an exact high-precision decimal type.

`Crypto` is never `Float64`.

`Crypto` is intended for cryptoassets.

Detailed support for scales and networks belongs in `runtime.md`.

## No Decimal Type

`Decimal` does not exist in Inox 0.1 pre-alpha.

Compilers and tools must not treat `Decimal` as a built-in type.

## Character and String Types

`Char` is the character type.

`String` is the string type.

The detailed encoding, indexing, slicing, and storage semantics of `Char` and
`String` are not specified in this document.

## Arrays

Arrays have explicit ranges.

An array type includes its range as part of its type information.

The detailed syntax for array declarations, literals, indexing, assignment, and
comparison is not specified in this document.

## Vectors

Vectors are dynamic and 0-based.

A vector is distinct from an array: arrays have explicit ranges, while vectors
are dynamic and use 0-based indexing.

The detailed syntax for vector declarations, literals, resizing, assignment,
and comparison is not specified in this document.

## Enums

Inox 0.1 pre-alpha has simple enums in Pascal/Ada style.

Enums are nominal types.

Enum values are finite and ordinal.

The detailed enum declaration syntax, ordering rules, representation, and
conversion rules are not specified in this document.

## Sets

Sets follow Pascal/Ada style with a finite ordinal base.

A set type is based on a finite ordinal type.

The detailed set declaration syntax, literals, operations, assignment, and
comparison rules are not specified in this document.

## Subranges

Inox 0.1 pre-alpha supports subranges.

A subrange denotes a constrained range of values from an ordinal base type.

The detailed subrange declaration syntax, bounds checking rules, and runtime
representation are not specified in this document.

## Generics

Generics use `[]`.

The detailed semantics of generic declaration, instantiation, constraints, and
code generation are not specified in this document.

## Mutability

Mutability is expressed with `mut`.

The detailed syntax and rules for mutable bindings, mutable parameters, and
mutable aggregate values are not specified in this document.

`State` is the only mechanism for mutable global state.

## Local Type Inference

Inox 0.1 pre-alpha allows safe local type inference.

Inference must remain local and must not weaken strong static typing.

The exact inference algorithm is not specified in this document.

## Casts

Casts use:

```inox
TypeName(expr)
```

Casts are explicit conversions requested by the programmer.

The set of valid casts is not fully specified in this document.

## Implicit Conversions

Implicit conversions are allowed only for safe widening.

No implicit narrowing conversion is allowed.

No implicit conversion to or from a universal `Any` exists, because Inox 0.1
pre-alpha has no universal `Any`.

## Overflow

In safe mode, arithmetic overflow raises `OverflowError`.

The detailed definition of safe mode and the runtime behavior of `OverflowError`
belong in the runtime specification.

## Assignment

Assignment must respect the type system.

Implicit conversions during assignment are allowed only for safe widening.

Explicit casts use `TypeName(expr)`.

The detailed assignment rules are not specified in this document.

## Out of Scope

This document does not define:

- a universal `Any` type
- `Decimal`
- structural typing
- advanced generic constraints
- user-defined conversion mechanisms
- detailed runtime representation
- detailed `Currency` rounding semantics
- detailed `Crypto` scale or network semantics
