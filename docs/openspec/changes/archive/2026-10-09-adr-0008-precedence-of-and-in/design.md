# ADR-0008 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2082-2099) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Rationale and consequences

Rationale: the operands of a range are arithmetic, so arithmetic and bitwise
operators must resolve first (`1..N + 1` is `1..(N + 1)`). Membership tests a
value against a range or set, so the range must be complete before `in` applies
(`X in 1..N + 1` is `X in (1..(N + 1))`). Membership yields a Bool that is
compared or combined like any relational result, so `in` binds tighter than the
relational and logical operators (`X in S = Flag` is `(X in S) = Flag`;
`X in S and Y in T` needs no parentheses). Chaining has no meaning for either
operator (a range has exactly two bounds; `X in A in B` would test a Bool for
membership), so Inox rejects the chain instead of picking a grouping
(CANON-20 refuses to guess).
Consequence: two parse diagnostics for the chains. No change to lowering.
