# ADR-0006 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1829-1876) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Rationale and consequences

Consequence: agents must not alter these without a new ADR; if implementation
lags spec, document the gap rather than changing the language silently.
Deferred 0.2+: borrowing, arenas, unsafe boundaries, contracts, Chapel-style
parallelism, vector runtime, string/Unicode runtime, module export, variant
structs, metadata tags, full package/build tooling.

Historical supersession note (v3.19; the locked ADR-0006 text above is unchanged):
ADR-0006 decision 4 used the phrase “Runtime overflow should trap in checking
mode.” That policy is superseded by DECISION P-A, approved 2026-10-09: runtime
arithmetic faults are deterministic Inox traps in every conforming build. This
note records the later approved decision without rewriting the locked ADR.
