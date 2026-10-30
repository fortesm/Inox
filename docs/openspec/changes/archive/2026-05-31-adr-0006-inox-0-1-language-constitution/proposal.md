## ADR-0006 — Inox 0.1 Language Constitution  (Status: Accepted)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1829-1876) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Decision

Context: records foundational decisions so humans, ChatGPT, Codex, and future
agents do not re-open settled matters or import other languages' defaults.
Decisions 1–20 (safe core):
 1. Receivers `Self`, `Self mut`; `Self owned` future. Do not repeat receiver
    type in `Self` (the method prefix supplies it).
 2. Params immutable by default. Locals (inline or formerly Var) mutable.
    `mut X Integer` reserved and rejected in 0.1.
 3. `Exit` only in subroutines without return values and in `Main`. Functions use
    `Return Expression`.
 4. Integer overflow invalid. Constant overflow = compile-time error. Runtime
    overflow should trap in checking mode. No wraparound promise.
 5. Integer `/` invalid; use `div`/`mod`. Future `/` is real division for Float.
 6. Narrowing never implicit. Explicit conversion `TypeName(Expression)`.
 7. `String` UTF-8, immutable, non-null, defaults to `""`.
 8. `Char` Unicode scalar value, not byte/integer/grapheme cluster.
 9. Fixed arrays `Array[Low..High] Type`; ranges part of the type; bounds-checked
    value types.
10. `Vector[T]` future dynamic 0-based owning/move; assignment moves; `Clone` copies.
11. `for I in A..B (S)` inclusive; direction from `A..B`; positive step; iterator
    implicit, read-only, loop-scoped, cannot shadow any visible symbol.
12. `Range` declarations in `Type` are line declarations, no `;`.
13. Enums short and multi-line; nominal and ordinal; no implicit Integer conversion.
14. `Set[T]` requires finite ordinal base (Enum or finite Range); not a hash set.
15. `case Expression` no fall-through; enum cases without `otherwise` exhaustive.
16. `Module` first, no `;`, EOF closes. `Use` semantic dependency, not textual
    inclusion. Multi-file support in 0.1.
17. No public/private/protected/published in 0.1. Future visibility prefers `Export`.
18. Future contracts/protocols/behaviors are static capability checks, not Java
    interfaces, OO subtyping, duck typing, or mixins.
19. No universal `null`/`nil`. Future absence `Option[T]`; failure `Result[T,E]`.
20. No raw pointers, `Pointer[T]`, `unsafe`, or C interop in the 0.1 safe core.
Local scope and shadowing: `Name Type := Expr` declares; `Name := Expr` assigns;
shadowing forbidden in all local scopes incl. case-only differences; local
visible declaration-point -> end of block; use-before-declaration invalid.
Block closing token: `End`/`end` is not a keyword and not a synonym for `;`.
