## ADR-0017 — Read-only slices  (Status: Accepted, 2026-10-11)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1877-1902) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Decision

Decision (Marcelo Fortes, 2026-10-11, approving the order "Array, then ref,
then a read-only Slice (A[5..10])": do everything, in the proposed order). The
idea comes from the design discussion with ChatGPT on Go slices, adapted to
Inox safety: a slice is a bounded view, read-only, that never outlives its
array.
1. `A[L..H]`, `A[..H]`, `A[L..]` and `A[..]` give a view of elements L..H
   of the array (or slice) A, without copying. An omitted bound is A's own
   Low or High. The type is `Slice<T>` (generic arguments use `<...>`).
2. A slice keeps the indices of its array (as in Ada): `A[3..6]` is indexed
   3..6; `Low`, `High` and `Length` give its bounds at run time.
3. Bounds: Low(A) <= L <= H <= High(A); a slice is never empty, like an
   array (so `for I in Low(S)..High(S)` never runs backwards). Constant
   bounds are checked at compile time, the others at run time ("slice bounds
   out of range"). Every index is checked against the slice's bounds.
4. Read-only: an element of a slice cannot be assigned.
5. Lifetime: only a named array, parameter or a field/element of one is
   sliced (never a temporary); a slice variable is bound once (no
   reassignment); a slice cannot be returned, and slice fields and State do
   not exist. So a slice never outlives its array.
6. A slice is passed by value (it is already a view); `ref` on a slice is an
   error. Arrays and slices are not compared.
