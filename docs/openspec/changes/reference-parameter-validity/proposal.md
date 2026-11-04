## Why

A routine that frees the object its own reference parameter reaches, then
uses the parameter, wrote the freed object or another object that took its
block: `Replace(ref mut X Integer)` replacing `H.FChild` then `X := 10`,
called as `Replace(H.FChild.FV)`, printed `1 10` (K's object written); a
`Self mut` method did the same. The caller cannot see it, so the check of
r3.125 does not apply. This breaks CANON-25 "No access through an
invalidated object" (r3.124).

Approval: Marcelo Fortes, 2026-11-04: alternative (a), the reference
carries the generation of the object holding it, and the routine checks
it; the language's semantics do not change.

Touches: the routines' ABI (code generation): a parameter that is the
caller's storage (`ref`, `ref mut`, a struct receiver) is followed by the
header and the generation of the object holding that storage, or of the
immortal header for storage no owner frees (a local, State, a temporary).
The routine checks the generation at every use of the parameter, of a
`with` binding of it, and passes it on when it passes the parameter by
reference. No rule changes; CANON-25's scenario names the tests
(constitution relocked). `docs/ARCHITECTURE.md`.

Risk: high (routine ABI); the decision is the maintainer's of the same
date, recorded here.

Status: **implemented** (canon r3.126).

## What Changes

1. `FunctionParameter::callerStorage`; the two hidden parameters in every
   definition and call.
2. The check at every use of such a parameter (`emitStorageGuard`, the
   mechanism of `with` targets, renamed from `emitWithGuard` as it now
   serves both).
3. Tests: `reference-parameter-invalidated-by-callee`,
   `receiver-invalidated-by-method`, `reference-parameter-passed-on-invalidated`,
   `reference-parameter-with-invalidated`.
4. Revision line, kCanonRevision, CHANGELOG.

## Out of scope

A `ref mut` argument written `Value(P)` is passed as a copy (existing
behavior, unrelated to memory safety): backlog.
