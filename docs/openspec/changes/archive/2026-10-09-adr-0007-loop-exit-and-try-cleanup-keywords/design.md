# ADR-0007 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2100-2112) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Rationale and consequences

Rationale: Inox spells control flow with its own vocabulary instead of carrying
C/Java defaults (ADR-0005/ADR-0006, GOVERNANCE G4). `ensure` states what the
clause guarantees (its body runs on every departure) and does not collide with
design-by-contract, which keeps `Pre`/`Pos`/`Invariant`.
Consequence: lexical change only. Semantics, lowering and runtime behavior are
unchanged. No compatibility or migration diagnostic is kept for the old words.
