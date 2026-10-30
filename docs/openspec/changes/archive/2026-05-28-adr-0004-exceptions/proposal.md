## ADR-0004 — Exceptions  [SOURCE FILE WAS EMPTY]

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1796-1804) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Decision

Topic intended: exception model. Its substance is now in CANON-15. `try`/`except`/
`finally`/`raise` exist in 0.1 syntax; lowering incremental; future Option/Result
complement, not replace.

Historical supersession note (v3.20; the locked ADR-0004 text above is unchanged):
the try cleanup clause spelled `finally` above is spelled `ensure` since v3.20
(ADR-0007). The operative rules are in CANON-15.

This record has no separate rationale section.
