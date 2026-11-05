## Why

CANON-25 rule 3 (`safe-pointers` design): using an owner after `Move` or
`Free` is a compile error. The move analysis of phase 4 does not follow the
edges of `leave` and `continue` (item 2.1 of `safe-pointers-followups`), so
an owner given away on such a path is caught only at run time (trap 17, "use
of an owner after Move or Free", `tests/runtime/pointers-moved-owner.inox`).

Status: **design awaiting the maintainer's approval** (G7: memory ownership).
Risk: high (memory ownership); the law, the syntax and the generated code do
not change.

## What Changes

- The analysis carries the owners given away at each `leave` to the
  statement after its loop, and at each `continue` to the loop's next round;
  a path that jumps or exits no longer flows into the statements after the
  branch that holds it.
- Programs it rejects: those that use, after a loop, an owner given away
  before a `leave`; those whose `continue` reaches the next round with an
  owner given away and not assigned again. Both are breaches of rule 3
  that today trap at run time.
- Programs it accepts that it rejects today: none by `leave`/`continue`
  alone; with Q1, a branch that ends in `Return`, `Raise` or `Exit` no
  longer counts as a path to the statements after it.
- The run-time check (trap 17) stays in the generated code (item 2.1's
  second half, its removal, waits until the coverage is demonstrated).

## Impact

- Phase 4 only: `semantic/SemanticStatements.cpp` (if, unless, case, loops,
  try, jumps), `semantic/SemanticPointers.cpp`, `SemanticAnalyzer.h`.
- No change to the parser, contract lowering, the TIR, code generation or
  libinoxrt: the LLVM IR and the typed IR of every program that compiles
  are identical.
- `tests/runtime/pointers-moved-owner.inox` (accepted today, trap 17) is
  rejected at compile time: it moves to `tests/diagnostics/`.
- Specs: the scenario of CANON-25 rule 3 gains the jump paths; STATUS and
  the "As implemented" note of `safe-pointers` lose the gap.
