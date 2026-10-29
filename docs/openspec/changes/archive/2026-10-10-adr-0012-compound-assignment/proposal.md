## ADR-0012 — Compound assignment  (Status: Accepted, 2026-10-10)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2007-2020) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Decision

Decision (Marcelo Fortes, in the design chat with ChatGPT): Inox adds `+=`,
`-=`, `*=`, `/=` and `^=` next to the plain form `I := I + 1`, which stays
valid. `L op= R` means `L := L op R` with L evaluated exactly once.
- `/=` is valid only where `/` is valid: `I /= 2` with an Integer is an error,
  because Inox has no integer `/` (use `div`). `^=` is power.
- Statement level only and not chainable (ADR-0009). The target is an existing
  variable or field path; index targets come with arrays.
- Updating a variable is not reading it (CANON-5 rule 7, OPEN-4).
