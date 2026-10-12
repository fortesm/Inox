## ADR-0009 — Chained assignment  (Status: Accepted, 2026-10-09)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2066-2081) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Decision

Decision (approved by Marcelo Fortes, 2026-10-09): `A := B := C := X` is valid
Inox. X is evaluated exactly once; the value is stored in C, then B, then A.
Each target is a variable or a field path, and each follows CANON-5 rule 1
(the first appearance of a name declares it, with the inferred type). The chain
is a STATEMENT form: `:=` is never an expression, so `X + (A := 3)`,
`if (A := 3) = 3` and `A Integer := B := 3` are compile errors.
