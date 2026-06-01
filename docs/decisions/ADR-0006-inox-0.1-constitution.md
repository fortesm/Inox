# ADR-0006: Inox 0.1 Language Constitution

Status: Accepted

## Context

During 0.1 pre-alpha design, Inox resolved a series of foundational syntax and semantic questions. This ADR records those decisions so humans, ChatGPT, Codex, and future agents do not re-open settled matters or import defaults from other languages accidentally.

## Decision

Inox 0.1 safe core is defined by the following decisions.

1. Associated receivers use `Self`, `Self mut`; `Self owned` is future. Do not repeat the receiver type in `Self` because the method prefix already supplies it.
2. Ordinary parameters are immutable by default. Local variables in `Var` are mutable. `mut X Integer` is reserved and must be rejected in 0.1.
3. `Exit` is allowed only in subroutines without return values and in `Main`. Functions use `Return Expression`.
4. Integer overflow is invalid behavior. Constant overflow is always a compile-time error. Runtime overflow should trap in checking mode. Do not promise wraparound.
5. Integer `/` is invalid. Use `div` and `mod`. Future `/` is real division for `Float`.
6. Narrowing conversions are never implicit. Explicit conversions use `TypeName(Expression)`.
7. `String` is UTF-8, immutable, non-null, and defaults to `""`.
8. `Char` is Unicode scalar value, not byte, not integer, not grapheme cluster.
9. Fixed arrays use `Array[Low..High] Type`; ranges are part of the type; arrays are bounds-checked value types.
10. `Vector[T]` is future dynamic 0-based owning/move type; assignment moves; `Clone()` copies.
11. `for I in A..B (S)` is inclusive; direction comes from `A..B`; step is positive.
12. `Range` declarations in `Type` are simple line declarations and do not use `;`.
13. Enums have short and multi-line forms, are nominal and ordinal, and do not implicitly convert to/from `Integer`.
14. `Set[T]` requires finite ordinal base (`Enum` or finite `Range`) and is not a generic hash set.
15. `case Expression` has no fall-through; enum cases without `otherwise` must be exhaustive.
16. `Module` is first declaration, no `;`, EOF closes module. `Use` is semantic dependency, not textual inclusion. Multi-file support belongs in 0.1.
17. No `public`, `private`, `protected`, or `published` in 0.1. Future visibility should prefer `Export`.
18. Future contracts/protocols/behaviors are static capability checks, not Java interfaces, OO subtyping, duck typing, or mixins.
19. No universal `null`/`nil`. Future absence is `Option[T]`; recoverable failure is `Result[T, E]`.
20. No raw pointers, `Pointer[T]`, `unsafe`, or C interop in the 0.1 safe core.

## Consequences

Agents must not alter these decisions without a new ADR. If implementation lags the spec, document the gap explicitly rather than changing the language silently.

## Deferred architectural work

The following are 0.2+ design areas: borrowing, arenas, unsafe boundaries, contracts, Chapel-style parallelism, vector runtime, string/Unicode runtime, module export, variant structs, metadata tags, and full package/build tooling.

## Block closing token

`End`/`end` is not a keyword in Inox 0.1 and is not accepted as a synonym for `;`. Only `;` closes blocks. This avoids retaining Pascal/Ruby-style legacy alternatives in the parser.
