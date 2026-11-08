## Why

CANON-25 (r3.135): "A routine gives back an owner it received as `ref mut`
before it returns." The rule covers normal exits only. A routine that
frees a `ref mut` owner and then raises leaves it given away; when the
argument was an `Own<T>` field, the field is empty for whoever catches the
exception, and its next use stops with trap 17 (item 2.5 of
`safe-pointers-followups`, accepted provisionally on 2026-11-06;
`tests/runtime/pointers-owner-field-lost-by-exception.inox`). Verified on
main 7401b1a: the field may be reached through a local struct, through an
owner (`H.FBox.FNode`), through `Self` in a method, and through a
`ref mut` struct parameter; the caller's handler, its `ensure`, or any
outer handler then sees the empty field.

Status: **proposal awaiting the maintainer's decision** (G7: memory
ownership; the provisional acceptance of Q3 in r3.135 does not choose the
rule). Risk: high. No semantic change before approval.

## What Changes (recommended alternative C, see design.md)

- A routine gives back every owner it received as `ref mut` on every exit,
  normal or by an exception: at each point where an exception may leave
  it, the owner must not be given away (after the `ensure`s crossed).
- The points where an exception may leave a routine are named in phase 5
  from the generated code's ground truth (design.md, section 1): `Raise`,
  rethrow, a spent `Retry`, calls of routines, methods, generic instances
  and implicit invariant checks, String-to-number conversions and the
  Std.Files operations; a guard keeps the list exhaustive.
- Then no caller ever sees a `ref mut` owner given away, whatever the
  argument: variable, field, field through an owner or `Self`.

## Impact

- Phase 5 only (`SemanticPointers.cpp`, `SemanticStatements.cpp`,
  `SemanticCalls.cpp`, `SemanticAnalyzer.h`); the generated code of every
  accepted program is unchanged; trap 17 stays as the run-time guard.
- CANON-25 text: "before it returns" becomes "before it returns or an
  exception leaves it" (constitution, maintainer's decision).
- Programs it rejects: a routine that gives a `ref mut` owner away and
  then reaches a raising point before assigning it again.
- `pointers-owner-field-lost-by-exception` becomes a diagnostic; trap 17
  keeps an emission test and an executable native fixture (Q3).
- r3.135 Q2 stays unchanged (maintainer, 2026-11-08).
