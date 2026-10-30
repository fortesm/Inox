# ADR-0016 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1903-1934) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Rationale and consequences

Rationale: arrays are values (CANON-13), so passing a large array by value
copies it; `ref` removes the copy without aliasing, and `ref mut` replaces
output parameters without pointers. Rule 4 is the "many readers or one
writer" discipline in its simplest checkable form; it is also the basis for
data-race checks in future parallel constructs.
Consequence: `ref` is a reserved word.
