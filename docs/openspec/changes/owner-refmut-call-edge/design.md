## Where the call edge is lost (main fa1a37c)

- `analyzeFunction` (SemanticStatements.cpp) declares a `ref mut`
  parameter as a variable of the routine; `Free(P)` and `Move(P)`
  (SemanticPointers.cpp, `analyzePointerBuiltin`) accept it like any owner
  variable and `noteOwnerGivenAway` records it. At the routine's exits the
  set is discarded (`leaveScope` forgets the routine's owners): nothing
  says that this owner is the caller's storage.
- The call (`SemanticCalls.cpp`, the reference-argument check of
  ADR-0016, shared by routines, methods and generic instances) checks the
  argument's type, writability and exclusivity, and nothing about owners:
  after the call the caller's set is unchanged, so its use is accepted.
- The same call reaches a field: `Release(B.FNode)` frees an owner field,
  which CANON-25 only allows for `Option<Own<T>>`. Later uses of
  `B.FNode` trap 17; dropping `B` frees nothing twice (verified under
  AddressSanitizer).
- A generic instance does the same: `Consume<T>(ref mut X T)` that frees
  `X`, called with an owner, traps 17 at the caller's use (verified).
- Not affected, verified on main: `Free` and `Move` of a State owner are
  rejected ("only a variable gives its owner away"); `Self` and its fields
  cannot be given away; `Option<Own<T>>` is emptied by `Move`, not tracked,
  and its uses already test `Alive`; the same variable as two `ref mut`
  arguments, or `Move(P)` and `P` in one call, are rejected (ADR-0016, the
  argument order of the move analysis).

## Alternatives

- **A. The callee gives the owner back (recommended, Q1).** The obligation
  is local to the callee, like the loop rule of CANON-25 ("a loop gives an
  owner back before its next round"): no summary, no fixpoint, so calls
  across modules, transitive calls, recursion and generic instances need
  nothing more; an owner field passed as `ref mut` is never left empty on
  a normal return. A routine that means to consume an owner takes it by
  value (`Drop(P Own<T>)`, called with `Move(P)`), which the analysis
  already follows. Cost: a callee that gives away a `ref mut` owner without
  assigning it again is rejected; no program of the repository does this
  except the trap test.
- **B. Effect summaries.** Each routine records which `ref mut` owners it
  may leave given away; the caller adds them to its set. Accepts more
  programs (the caller may assign the owner again), but needs summaries for
  routines analyzed after their callers, a fixpoint for recursion, one
  summary per generic instance and per module, and cannot follow a field
  argument (only a variable gives an owner away): the emptied field stays.
  Not recommended.

## Decisions (proposed)

1. **Obligation at normal exits (Q1).** While a routine is analyzed, its
   `ref mut` parameters of a move-only type are known. At the end of the
   body (when reachable), at `Return` and at `Exit`, a `ref mut` parameter
   in the moved set is an error in the callee: "P is received as ref mut
   and given away (Move or Free) on a path that returns: assign it again
   before the routine returns".
2. **Exits through `ensure`.** `Return` and `Exit` inside a try with
   `ensure` become a departure (a new kind, routine exit) of the r3.134
   try frames: the check applies to `out(S)` after every `ensure` the exit
   crosses, inner first. Before this change those departures needed no
   record (the routine's owners died); nothing else in the r3.134 analysis
   changes.
3. **Paths.** The moved set at an exit is the r3.134 set: a callee that
   frees and assigns again on every path is accepted; a free on one branch
   that reaches the exit is rejected; loops, `leave`, `continue`, `until`,
   handlers and `Retry` keep their r3.134 behavior.
4. **Exception edge (Q2).** An exception that leaves a callee has no
   normal exit, so the callee may have given the owner away (between
   `Free(P)` and `P := New(...)` any operation may raise). At a call, an
   owner variable passed as `ref mut` is added to the accumulation of
   every enclosing try frame (as `Free` already is), not to the current
   set: the statements after the call see a valid owner (decision 1), the
   handlers and the `ensure` of the enclosing trys treat it as given away.
   By induction this covers transitive calls: a routine whose `ref mut`
   owner is lost on an exception is protected at each caller that catches.
   Cost: a handler or `ensure` that uses such an owner without assigning
   it first is rejected, the same conservative choice as r3.134.
5. **Field arguments on the exception edge (Q3).** An owner field passed
   as `ref mut` cannot enter the moved set (it is not a variable); if the
   callee frees it and raises, a caller that catches and uses the field
   reaches trap 17. This stays a path the analysis does not follow (the
   CANON-25 text allows it), documented, and becomes the independent
   runtime test of trap 17 (`pointers-owner-field-lost-by-exception`).
   Making it static would need a rule on field arguments inside a try
   (out of this change).
6. **Generic routines.** A generic body is analyzed per instance
   (`analyzeFunction`, with its own owner state since r3.134): the
   obligation applies to the instance whose `T` is move-only, so
   `Swap<T>(ref mut A T, ref mut B T)` with owners is accepted and a
   `Consume<T>(ref mut X T)` that frees `X` is rejected at the instance,
   with the instance named in the diagnostic place already used for
   generic instances.
7. **Unknown effects.** Every routine is analyzed by this compiler (no
   foreign or external routine takes a `ref mut` owner); a routine whose
   body is not analyzed cannot be called with one, so no effect is unknown.
   Recursion needs nothing: the obligation is checked in the body alone.
8. **Run time.** Trap 17 stays in the generated code; no change outside
   phase 5.
9. **Constitution (maintainer's decision).** CANON-25 "Moves are checked"
   gains: "A routine gives back an owner it received as `ref mut` before
   it returns." (Q1). The rest of CANON-25 and CANON-10 is unchanged.

## Questions (answered 2026-11-06)

- **Q1.** Alternative A and its sentence in CANON-25: approved. A routine
  gives back every owner it received as `ref mut` before each reachable
  normal exit, after the `ensure`s that exit crosses (G2/G7/G8).
- **Q2.** Inside a try, an owner variable passed as `ref mut` counts as
  possibly given away on the exception edge, for the handlers and the
  `ensure` of the enclosing trys; r3.134 is otherwise unchanged: approved.
- **Q3.** Accepted provisionally: trap 17 stays for an `Own<T>` field
  emptied by an exception inside a routine called with it as `ref mut`;
  the static gap is recorded as a later task (item 2.5 of
  `safe-pointers-followups`) with an independent runtime test. No record
  states that every ownership path is proven safe at compile time.

These answers authorize only the scope of this change.

## Acceptance tests

Negative (`tests/diagnostics/`):
- `pointer-refmut-freed-by-callee`: the callee frees and returns (from
  `pointers-owner-freed-by-callee`, moved).
- `pointer-refmut-moved-by-callee`: the callee moves it into a local.
- `pointer-refmut-freed-on-a-branch`: freed on one `if` arm only.
- `pointer-refmut-freed-before-return`: freed, then `Return` in a loop.
- `pointer-refmut-freed-before-exit`: freed, then `Exit`.
- `pointer-refmut-freed-in-ensure`: `ensure` frees on the `Return` path.
- `pointer-refmut-freed-through-intermediate`: an intermediate routine
  passes its `ref mut` owner on, then frees it.
- `pointer-refmut-generic-consume`: a generic `Consume<T>` frees its
  `ref mut T`, instantiated with an owner.
- `pointer-refmut-module-callee`: the callee is in another module.
- `pointer-refmut-used-in-handler`: an owner passed as `ref mut` in a try
  is used by the handler (Q2).
- `pointer-refmut-used-in-ensure`: the same in `ensure`.
- `pointer-refmut-field-freed-by-callee`: the callee frees a field
  argument and returns (an emptied field, now rejected in the callee).
- `pointer-refmut-nested-return`: `Return` from inside a loop inside two
  nested trys whose inner `ensure` frees the owner and outer `ensure` does
  not assign it again.
- `pointer-refmut-nested-exit`: `Exit` from inside a `case` arm inside a
  loop, after a `Free` on that path.
- `pointer-refmut-transitive-exception`: the caller's try calls an
  intermediate routine that passes the owner on as `ref mut` to a callee
  that may raise; the caller's handler uses the owner (Q2 at the caller).
- `pointer-refmut-transitive-exception-inner`: the intermediate routine's
  own handler uses the owner after the inner call raised (Q2 at the
  intermediate level).

Positive (`tests/runtime/`, output checked, leak-free):
- `pointers-refmut-give-back`: the callee frees and assigns again; moves
  out and back; assigns again on both branches; assigns again in `ensure`
  on the `Return` path; the caller assigns its own owner after the call;
  recursion that frees and assigns again.
- `pointers-refmut-across-calls`: an intermediate routine passes its owner
  to another; a callee in another module; `Swap<T>` with owners; a field
  argument; a handler that assigns the owner again before using it.
- `pointers-refmut-nested-exits`: `Return` and `Exit` from nested loops and
  trys whose `ensure`s free and assign again, inner first; an exception
  that crosses an intermediate routine and is caught by a handler that
  assigns the owner again.
- `pointers-owner-field-lost-by-exception` (`.trap`, Q3): the callee frees
  a field argument and raises, the caller catches and uses the field: trap
  17, never a null access (the independent runtime test of trap 17).

Preservation of the generated code: before the first code change the LLVM
IR (Debug and Release) and TIR snapshots are recorded with the compiler of
main over the programs of this branch; after it, the comparison must show
no difference for any program that compiles on both, and only the added or
moved tests may differ (by being new, or rejected now). The comparison is
local evidence: the CI snapshot job runs only for PRs labelled `refactor`.

Gates (E19): Debug and Release suites, ASan+UBSan (1 MiB), string leak and
owner safety checks, stack_frames, fuzz 1 and 7, differential,
backend_gaps, grammar, the snapshot comparison above, openspec,
constitution_check, version_check, then CI on Linux and Windows.

## Approval

Design approval: Marcelo Fortes, 2026-11-06 (G7), with Q1 and Q2 approved
and Q3 accepted provisionally on condition of item 2.5; it covers this
change only.
