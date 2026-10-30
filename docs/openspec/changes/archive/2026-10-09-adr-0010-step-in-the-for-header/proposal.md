## ADR-0010 — `step` in the for header  (Status: Accepted, 2026-10-09)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2054-2065) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Decision

Decision (approved by Marcelo Fortes, 2026-10-09): the step of a counted loop is
written with the reserved word `step`, as in Kotlin:
`for I in A..B step S`. The form `for I in A..B (S)` is removed, with a
migration diagnostic. The bounds are range-level expressions (CANON-20).
