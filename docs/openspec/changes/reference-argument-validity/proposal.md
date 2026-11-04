## Why

A reference argument (`ref`, `ref mut`, ADR-0016) and a method's receiver
are places whose address code generation computes before the arguments
that follow. When one of those arguments replaces the owner the place
crosses, the routine wrote the freed object, or another object that took
its block: `Store(H.FChild.FV, Drop)` and `H.FChild.Put(Drop)` printed
`1 10`, K's object written. This breaks CANON-25 "No access through an
invalidated object" (r3.124), which already states the rule.

Approval: Marcelo Fortes, 2026-11-04: a restricted change for the
reproduced defect of reference arguments and receivers, with the smallest
sufficient mechanism; CANON-5 is not changed again.

Touches: code generation only (`emitArguments`, `emitMethodCall`): the
CANON-25 check of r3.124 (the generation of the object holding the place,
captured when it is reached, compared before the call) applied to a
reference argument followed by an argument that may run a routine, and to
a receiver passed by address followed by one. No rule changes; CANON-25's
scenario names the new tests (constitution relocked).

Risk: normal (the mechanism and the rule are those of r3.124).

Status: **implemented** (canon r3.125).

## What Changes

1. The check before the call, where a later argument may run a routine.
2. Tests `reference-argument-invalidated-by-argument`,
   `receiver-invalidated-by-argument`.
3. Revision line, kCanonRevision, CHANGELOG.

## Out of scope

A callee that itself frees the object a reference argument reaches, then
uses the reference (`Replace(ref mut X)` replacing `H.FChild`, then `X :=
10`): reproduced, not fixed by a check in the caller; a decision is asked
of the maintainer.
