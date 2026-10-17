Approved by Marcelo Fortes, 2026-10-17 (quoted in proposal.md): "Tipos de
verdade, a regra é clara e simples mapear os tipos que o llvm já oferece."

## Principles

- **One source of truth per fact.** The range of Int8, what a Link looks like
  in memory, the fields of a struct: each is written once, in the module that
  owns it.
- **Low coupling, direction of dependencies.** `types` depends only on the
  C++ standard library; `ast` does not know `types`; `semantic` produces
  types; `codegen` consumes them and never re-derives them.
- **No behavior change.** Each step is checked with `tools/ir_snapshot.py`
  against the snapshot recorded on `main` before the change (IR, `--dump-types`,
  diagnostics and exit codes of all 633 programs). The suite, the sanitizers
  and the other gates run after each step.
- **Steps that compile.** The work lands in commits that each build and pass
  every gate, not as one switch-over.

## The type module (`src/compiler/types/`)

`Type` is immutable once complete and created only by `TypeContext`, which
interns it: two equal types are the same object, so `A == B` is a pointer
comparison and a type can be a map key.

| Kind | Inox types | LLVM |
|---|---|---|
| `Bool` | Bool | `i1` |
| `Integer` (bits, signed, range) | Int8..Int64, UInt8..UInt64, Natural | value `i64`; storage `i8`/`i16`/`i32`/`i64` (CANON-8: narrow integers are computed in i64 and range-checked) |
| `Float` (bits) | Float32, Float64 | `float`, `double` |
| `Char` | Char | `i32` (values not lowered yet: STATUS) |
| `Decimal` | Currency, Crypto | not lowered yet (STATUS) |
| `String`, `File` | String, File | `ptr` to a runtime block |
| `Struct` (nominal, fields) | `TName Struct` | `%tname` |
| `Array` (low, high, element) | `Array[L..H] E` | `[N x storage(E)]` |
| `Slice` (element) | `Slice<E>` | `{ ptr, i64, i64 }` |
| `Own` (target) | `Own<T>` | `ptr` |
| `Option` (Own) | `Option<Own<T>>` | `ptr` (null is None) |
| `Link` (target) | `Link<T>` | `{ ptr, ptr, i64 }` |
| `Exception` | IOError, RangeError, ... | runtime type identifiers |
| `None`, `Void` | the type of `None`; no value | none |

The LLVM spelling lives in codegen (`TypeLayout`), derived from the kind with
a `switch`; the type module only records what a kind is. Aliases (`Integer`,
`Byte`, `Float`, `UInteger`) are names of the same object. A type's `name()`
is its canonical spelling, exactly as the diagnostics print it today.

`TypeSyntax` reads the written form the parser produces (`Array[1..Size]
Byte`, `Link<Array[1..5] TContato>`) into a tree; the analyzer resolves the
tree into a `Type`, applying the language rules and their messages. It
replaces `support/ArrayType` and `support/PointerType`.

## Analyzer

- Every place that held a type name holds `const types::Type*` (null: not
  known, the empty string of today). `Symbol`, routine parameters and
  results, struct fields, the result of each `analyze...` function.
- Type predicates (`isIntegerType`, `integerTypeRange`, `isMoveOnlyType`,
  ...) ask the type object; the string lists go away.
- `SemanticResult` owns the `TypeContext` and records the type of each
  expression, of each declaration with a written type, the routine
  signatures and the struct types.

## Code generator

- `TypeLayout` maps a `Type` to its LLVM types by kind.
- Struct layouts and routine signatures come from the analysis
  (`parseFunctionSignature` from tokens and `collectStructDefinitions` from
  the AST are removed).
- Locals, parameters and expressions carry their `Type`; `expressionLlvmType`
  asks the analysis instead of recomputing.

## Statement cloning

`ast::cloneStatement(statement, rewriteExpression, rewriteTypeName)` copies a
statement tree; the type-name hook lets a generic instance replace `T`. Unit
tests check that a cloned routine body prints the same as the original.

## Not in this change

The parser keeps producing written type text (a typed AST for types is a
later step); generics themselves are stage 2.
