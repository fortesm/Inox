## Why

CANON-25 rule 3 (`safe-pointers` design): using an owner after `Move` or
`Free` is a compile error. The move analysis of phase 5 (semantic analysis)
does not follow every path the rule names (item 2.1 of
`safe-pointers-followups`): the edges of `leave`, `continue` and `until`,
the paths an exception takes from inside a try to its handlers, the effects
of `ensure` on every departure, and the back edge of `Retry`. An owner
given away on such a path is caught only at run time (trap 17, "use of an
owner after Move or Free", `tests/runtime/pointers-moved-owner.inox`).

Approval: Marcelo Fortes, 2026-11-05 (G7: memory ownership; design at
commit dede102). Risk: high (memory ownership); the law, the syntax and the
generated code do not change.

## What Changes

- The analysis carries the owners given away at each `leave` and at each
  true `until` to the statement after its loop, and at each `continue` and
  false `until` to the loop's next round.
- `Return`, `Raise`, `Exit`, `leave`, `continue` and `Retry` end the path:
  it no longer flows into the statements after the branch that holds it
  (Q1). A path that ends in `Raise` keeps its state for the handlers that
  can catch it.
- A handler starts from every owner given away anywhere in its try body
  (an exception can come from any operation there), not only from the set
  at the end of the body.
- `ensure` applies its effects to every path that leaves its try:
  normal end, handler end, `leave`, `continue`, `until`, `Return`, `Exit`,
  `Raise`, a propagating exception and `Retry` (CANON-15).
- `Retry` is checked like the end of a loop round, on the set after
  `ensure`: it may not reach the try's start with an owner given away (Q2).
- Programs it rejects: breaches of rule 3 on those paths, which trap at run
  time today. Programs it accepts that it rejects today: a branch that ends
  in `Return`, `Raise` or `Exit` after giving an owner away no longer
  counts as a path to the statements after it.
- The run-time check (trap 17) stays in the generated code, and an
  independent runtime test keeps it reachable and checked (Q3).

## Impact

- Phase 5 (semantic analysis) only: `semantic/SemanticStatements.cpp` (if,
  unless, case, loops, until, try, jumps, exits),
  `semantic/SemanticPointers.cpp`, `SemanticAnalyzer.h`.
- No change to the parser, normalization, contract lowering, the TIR, code
  generation or libinoxrt: the LLVM IR and the typed IR of every program
  that compiles are identical.
- `tests/runtime/pointers-moved-owner.inox` (accepted today, trap 17) is
  rejected at compile time: it moves to `tests/diagnostics/`.
  `tests/runtime/pointers-owner-freed-by-callee.inox` takes its place as
  the runtime test of trap 17.
- Specs: the scenarios of CANON-25 rule 3 gain the jump, exception, ensure
  and Retry paths; STATUS and the "As implemented" note of `safe-pointers`
  lose the gap.
