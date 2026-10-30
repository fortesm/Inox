# ADR-0013 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1981-2006) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Rationale and consequences

Rationale: `?:` and `->` would reuse `:` (which declares routines) or spend a
symbol; `if ... then ... else` reads as English and keeps statement and
expression forms visibly apart (the statement has no `then`).
Consequence: `then` is reserved; the backend lowers it with a branch and a
phi (pending).
