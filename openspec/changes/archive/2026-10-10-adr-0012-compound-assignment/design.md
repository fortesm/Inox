# ADR-0012 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2007-2020) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Rationale and consequences

Rationale: a concise, widely known convenience that keeps Inox meaning: `^` is
power, not XOR, and integer division keeps its own spelling.
Consequence: five new tokens; no new lowering (the parser stores the long form,
exact because the allowed targets have no side effects).
