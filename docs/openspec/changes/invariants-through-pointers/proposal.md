## Why

Item 3.1 of `safe-pointers-followups`: CANON-24 requires a struct's
invariant after a write from outside its own methods; a write through an
`Own` or a `Link` (`P.FValue := -1`) was not checked (verified on main
8f300f6), and `Value(L).FValue := X` was rejected as a place with side
effects.

Authorization: Marcelo Fortes, 2026-11-08 (conformance with CANON-24; its
rules are unchanged). Risk: medium.

## What Changes

- `recordInvariantPlaces`: a pointer (Own, Option<Own>, Link) on the path of
  a checked place stands for the struct it points at; the written place
  itself when it is a pointer is checked as that struct for field and
  element writes and method receivers, not when the pointer is replaced or
  passed whole (the new object is checked when made; a callee checks its
  own writes).
- `isSideEffectFree`: `Value(P)` of a side-effect-free place is free of
  side effects.
- `SemanticResult::InvariantCheck::through`; the TIR builder wraps such a
  check's place in a `through` place; code generation is unchanged.

## Impact

Phase 5 (`SemanticContracts.cpp`, `SemanticExpressions.cpp`,
`SemanticCalls.cpp`, `SemanticAnalyzer.h`, `SemanticResult.h`) and the TIR
builder (`BuildExpressions.cpp`). Programs that write through pointers get
the checks; programs that broke an invariant that way now trap.
