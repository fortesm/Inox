## Why

CANON-25 "Moves are checked": using an owner after `Move` or `Free` on any
path is a compile error. A routine that receives an owner as `ref mut` may
give it away (`Free(P)`, `Q := Move(P)`) and return; the caller's later use
is accepted and stops at run time with trap 17 (`tests/runtime/
pointers-owner-freed-by-callee.inox`, item 2.4 of
`safe-pointers-followups`). The same call empties an owner field passed as
`ref mut` (`Release(B.FNode)`), which CANON-25 forbids for a field that is
not `Option<Own<T>>` (verified on main fa1a37c).

Status: **design awaiting the maintainer's approval** (G7: memory
ownership; the r3.134 approval does not cover it). Risk: high.

## What Changes

- Q1 (recommended): a routine gives back every owner it received as
  `ref mut` before it returns. At each normal exit (the end of the body,
  `Return`, `Exit`, after the `ensure`s they cross) a `ref mut` parameter
  given away on a path that reaches the exit is a compile error, in the
  callee. The caller then keeps its owner valid after the call, with no
  summary of the callee.
- Q2 (recommended): on the exception edge of a call, an owner variable
  passed as `ref mut` counts as given away for the handlers and the
  `ensure` of the enclosing trys (the r3.134 try frames), since the callee
  may raise between giving it away and assigning it again.
- Q3 (recommended): the run-time check stays; an owner field passed as
  `ref mut` to a callee that frees it and raises remains a path the
  analysis does not follow, and is the independent runtime test of trap 17.
- Programs it rejects: a callee that does not give a `ref mut` owner back
  (today trap 17 or an emptied field); a handler or `ensure` that uses,
  without assigning it first, an owner variable passed as `ref mut` inside
  its try.

## Impact

- Phase 5 (semantic analysis) only: `SemanticStatements.cpp` (exits),
  `SemanticPointers.cpp` (the check, the departures),
  `SemanticCalls.cpp` (the `ref mut` argument check), `SemanticAnalyzer.h`.
  No change to the parser, the TIR, code generation or libinoxrt: the LLVM
  IR and the typed IR of every accepted program are identical.
- CANON-25 "Moves are checked" gains one sentence (constitution text,
  maintainer's decision): "A routine gives back an owner it received as
  `ref mut` before it returns."
- `pointers-owner-freed-by-callee` becomes a diagnostic; a new runtime test
  keeps trap 17 reachable and checked.
