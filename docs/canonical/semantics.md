# Inox Semantics Canonical Reference

## Core model

Inox is strongly typed, nominal where it matters, case-insensitive, and designed for mission-critical software. Unsafe defaults are rejected.

## Modules and Use

One `.inox` file declares one logical `Module`. EOF closes the module. `Use`
declares a semantic dependency and is not textual inclusion. The minimum 0.1
driver loads local dependencies recursively, imports their declarations for
analysis and emission, and rejects cyclic `Use` graphs.

Module resolution checks the entry-file directory first and then `stdlib/` at
the project root. `Std.Core` is the conceptual implicit prelude/core module.
`Std.IO`, `Std.Math`, and `Std.Debug` are explicit standard-library modules.

## Mutability

- Parameters are immutable by default.
- Local variables declared in `Var` are mutable.
- Mutating associated methods require `Self mut`.
- Ordinary mutable parameters such as `mut X Integer` are reserved and must be rejected in 0.1.

## Return and Exit

- `Return Expression` is valid only in functions with return types.
- `Exit` is valid only in subroutines without return types and in `Main`.
- Falling through the end of a subroutine is allowed.
- Falling through a function without return is invalid.

## Structs

Structs are nominal value types. Ordinary assignment, ordinary parameters, and ordinary returns copy the struct value. Associated-method receivers may be lowered by pointer internally, but the language does not become class-based.

## Control flow

`if` has no `then` or `:`. `repeat` is a general loop. `until` is an internal conditional exit. `for I in A..B (S)` has inclusive bounds, direction determined by the range, and positive step.

`case` has no fall-through. Enum cases without `otherwise` must be exhaustive.

## Numeric safety

Integer overflow is invalid. Constant overflow is a compile-time error. Division by zero for `div` and `mod` is always an error/trap. Integer `/` is invalid; use `div`.

## Strings and Char

`String` is UTF-8, immutable, non-null, with `""` as default. `Char` is Unicode scalar value.

## Null and unsafe

There is no universal `null`/`nil`. Unsafe pointers and C interop are not in the 0.1 safe core.

## Zero-argument calls and declarations

A function or subroutine with no parameters is declared without parentheses. A
call with no arguments is also written without parentheses. Empty parentheses are
rejected so that the language does not inherit C/Java-style syntactic noise.
