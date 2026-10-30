# ADR-0010 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2054-2065) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Rationale and consequences

Rationale: the parenthesized step could not be told apart from a call
(`A..F(2)`), so the old grammar limited the upper bound to a primary
expression; `1..N + 1` needed parentheses. A keyword removes the ambiguity and
reads as English. Direction, inclusivity, the positive-step rule and the
evaluate-once rule (DECISION P-B) are unchanged.
Consequence: `step` is reserved. Lowering is unchanged.
