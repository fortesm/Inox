## ADR-0008 — Precedence of `..` and `in`  (Status: Accepted, 2026-10-09)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2082-2099) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Decision

Decision (approved by Marcelo Fortes, 2026-10-09): CANON-20 places range
construction `..` at level 10 and membership `in` at level 11, between `bitor`
(9) and the relational operators (12). Both are non-associative. The table is
renumbered with integer levels; the relative order of the older levels does not
change.
