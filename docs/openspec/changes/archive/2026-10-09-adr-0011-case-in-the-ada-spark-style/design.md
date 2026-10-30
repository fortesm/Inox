# ADR-0011 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2021-2053) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Rationale and consequences

Rationale: Ada and SPARK make a case total. A case on an error code that
handles 404 and silently does nothing for 500 is the bug this rule prevents
(ChatGPT's example). Static, non-overlapping choices make the arm that runs
unambiguous and let the compiler check coverage.
Consequence: `|` becomes a token; the comma form is rejected with a migration
message. Lowering is unchanged by this ADR (the case lowering is Phase 2).
