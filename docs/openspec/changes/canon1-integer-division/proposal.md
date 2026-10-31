## Why

CANON-1 Safety defaults still said "no integer `/`". r3.95 (change
`integer-slash`) made `/` of two integers the integer division, truncated
toward zero, as in Ada, and updated CANON-2, 5, 8, 19, 20 and 23, but not
CANON-1, so the constitution contradicted itself. Correcting the sentence
was decided by Marcelo Fortes, 2026-10-30.

Touches: CANON-1 Safety defaults (text), consistent with CANON-8 and
CANON-19.

Risk: normal (one sentence of the constitution; the compiler, the runtime
and the language do not change).

Status: **implemented** (canon r3.108).

## What Changes

"no integer `/`" becomes "no unchecked integer division": a zero divisor
and `Int64.Min / -1` trap, and a constant zero divisor is a compile error,
as CANON-8 and CANON-19 already say. The constitution is relocked.

## Out of scope

No rule changes, no code changes.
