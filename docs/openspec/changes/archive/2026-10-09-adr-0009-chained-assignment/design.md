# ADR-0009 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2066-2081) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Rationale and consequences

Rationale: the chain is a common, readable way to give several variables one
value, and keeping it at statement level avoids the C hazard of assignment
inside conditions (`if (A = B)` typed for `if (A == B)`).
Index targets (`V[I]`) are excluded: storing right to left means the next
target reads the previous one back, and an index expression could change in
between.
Consequence: the parser desugars the chain into one assignment per target
(`C := X`, `B := C`, `A := B`); no new AST node and no lowering change.
