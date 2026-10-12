# ADR-0017 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1877-1902) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Rationale and consequences

Rationale: a slice is "a reference plus a length": the read-only half of
safe array views without pointer arithmetic. A writable view (`Slice mut`),
concatenation and Vector come later with ownership.
