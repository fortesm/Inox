## ADR-0007 — Loop exit and try cleanup keywords  (Status: Accepted, 2026-10-09)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2100-2112) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Decision

Decision (approved by Marcelo Fortes, 2026-10-09): the loop exit statement is
spelled `leave` and the try cleanup clause is spelled `ensure`. They replace
`break` and `finally`, which stop being reserved words. `continue` is unchanged.
