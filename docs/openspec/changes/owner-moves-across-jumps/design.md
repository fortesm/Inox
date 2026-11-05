## Where the edges are lost (main, r3.133)

The analysis of phase 5 (semantic analysis) keeps `movedOwners_` (symbol ->
scope of its declaration): the owners given away on some path that reaches
the current statement.

- `LeaveStatement` and `ContinueStatement` (SemanticStatements.cpp, the
  `loopDepth_` checks) only check that a loop encloses them; they neither
  record the current set nor end the path.
- `UntilStatement` only checks `repeatDepth_` and its condition; its true
  edge (out of the innermost `repeat`, across any inner `while` or `for`)
  is not followed.
- `IfStatement`, `UnlessStatement`, `CaseStatement`: each branch's final
  set is merged into the set after the statement, so a branch that ended in
  a jump or an exit feeds the fall-through instead of its target.
- `requireOwnersBackAfterLoop` compares only the set at the end of the body
  with the set before the loop; the sets at `continue` and at `leave` never
  reach it or the statement after the loop.
- Try: a handler starts from the set at the start of the try united with
  the set at the end of the body. An owner freed and assigned again inside
  the body, with an operation that raises in between, is missed:

      try
          Free(P)
          X := A[I]            -- may raise IndexError
          P := New(TNode)
      except
          On IndexError
              PutLn(P.FValue)  -- trap 17 today
      ;

- `ensure` is analyzed from the end of the body and the handlers only; the
  paths that leave the try by a jump, an exit, a `Raise` or a `Retry` do
  not see its effects, and the set at `Retry` is not checked at all.

Examples of the loop gaps:

      for I in 1..3                    while C
          Q := Move(P)                     Use(P)
          leave                            Free(P)
          P := New(TNode)                  if D
      ;                                        continue
      PutLn(P.FValue)   -- trap 17         ;
                                           P := New(TNode)
                                       ;    -- next round: trap 17

## Decisions

1. **Reachability.** The analysis state is the moved set plus "reachable".
   `leave`, `continue`, `Return`, `Raise`, `Exit` and `Retry` end the path:
   the state after them is unreachable (Q1). A merge (if, unless, case, the
   end of a try) unites the reachable incoming sets; when none is
   reachable, the state after the statement is unreachable. Statements after
   an ending in the same block are analyzed with the unreachable state.
   Ending a path never discards its set: before the state becomes
   unreachable, the set is recorded in the frame of its target (a loop, the
   handlers of a try, the `ensure` of a try).
2. **Loop frames.** Each loop pushes a frame holding the union of the sets
   at its exits (`leave`, true `until`) and at its next-round edges
   (`continue`, false `until`). Inox has no labels: `leave` and `continue`
   act on the innermost loop, `until` on the innermost `repeat`, across any
   inner `while` or `for` (verified on main), so a frame is found on the
   stack by kind.
3. **`until`.** The set at `until C` is added to the exit set of the
   innermost `repeat` frame (true) and the path continues with the same set
   (false: the rest of the round, then the next round). An `until` inside an
   inner loop also crosses that loop's scope (decision 6).
4. **End of a loop round.** The set at the end of the body (if reachable),
   united with the frame's next-round set, must add no owner declared
   before the loop to the set before it (rule 3, `requireOwnersBackAfterLoop`,
   its message unchanged: "assign it again before the next round").
5. **After a loop.** The set after it is the set before it united with the
   end-of-body set (for `while` and `for`, whose condition may end the loop)
   and the frame's exit set; an owner given away before an exit and used
   after the loop is the existing error ("was given away (Move or Free) on a
   path that reaches here").
6. **Owners of inner scopes.** A set recorded for a target keeps only the
   owners declared in a scope that encloses the target; those declared in
   the scopes the edge leaves die with them (`forgetMovedOwnersOf`,
   unchanged).
7. **Exceptions inside a try (conservative).** Every try pushes a frame
   that accumulates every owner given away anywhere in its body, in nested
   statements, nested trys, their handlers and their `ensure`s, whatever the
   path. A handler starts from the set at the start of the try united with
   that accumulation: any operation of the body may raise, and the set at
   any raising point is contained in it. A `Raise` (or a propagating
   exception) is therefore covered by the frames it may reach; the path then
   ends (decision 1). An exception raised in a handler or in `ensure` goes
   outward and is covered by the enclosing frames.
8. **`ensure` on every departure.** The paths that leave a try (normal end,
   end of a handler, `leave`, `continue`, `until`, `Return`, `Exit`,
   `Raise`, a propagating exception, `Retry`) are recorded in its frame with
   their target. `ensure` is analyzed once, from entry E = the set at the
   start of the try united with the accumulation of decision 7 (it contains
   every incoming set). With X the set at its end and M the owners it gives
   away, each recorded path with set S leaves with
   `out(S) = (S ∩ X) ∪ (X ∩ M)`: what S had and `ensure` did not assign
   again on every path, plus what `ensure` gave away. The result goes to the
   target (the frame of an enclosing loop or try, or the statement after
   the try); for `Return` and `Exit` it only feeds the enclosing frames.
   A jump inside the `ensure` body itself (accepted today) records the set
   it sees there, like any jump.
9. **`Retry`.** CANON-15: `Retry` runs `ensure`, then the whole try body.
   `out(S)` at a `Retry` must add no owner declared before the try to the
   set at the try's start; otherwise the error is "X is given away (Move or
   Free) before Retry: assign it again before Retry" (Q2).
10. **Run time.** The trap-17 check stays in the generated code; no code
   generation change (Q3).
11. **No new syntax, no change of CANON-25.** Rule 3 is the rule; the
   change makes the analysis follow every path it names.

Soundness of decision 8: an owner absent from `out(S)` is either absent
from X (the analysis from E, a superset of S, never ends with it given
away) or absent from both S and M (it is not given away on entry, and
`ensure` never gives it away). The precision lost against analyzing
`ensure` once per path is confined to owners `ensure` assigns on one path
only.

## Questions (answered)

- **Q1 (exits).** `Return`, `Raise` and `Exit` end the path like a jump.
  Answer: yes.
- **Q2 (Retry).** `Retry` is followed in this change, on the set after
  `ensure` (CANON-15). Answer: yes.
- **Q3 (the runtime test).** `pointers-moved-owner` moves to
  `tests/diagnostics/`; the runtime protection stays and is checked by an
  independent test. Answer: yes, with the protection preserved.

## Observation outside this change (for a separate decision)

Trap 17 stays reachable by accepted programs: a routine with a `ref mut`
owner parameter may `Free` it, and the caller's use after the call is not
rejected (verified on main: run-time error 17, exit 70). This is a call
edge, not a jump: it is not part of item 2.1 and is not changed here. It is
what the independent runtime test of trap 17 uses.

## Acceptance tests

Negative (`tests/diagnostics/`, each with its line and column):
- `pointer-moved-before-leave`: given away, `leave`, used after the loop.
- `pointer-moved-before-continue`: `continue` skips the assignment.
- `pointer-moved-before-leave-nested`: inner loop's `leave`, used in the
  outer loop after the inner one.
- `pointer-moved-before-leave-branch`: `leave` in one `elif` arm.
- `pointer-moved-before-until`: given away, true `until`, used after the
  `repeat`.
- `pointer-moved-before-until-in-while`: `until` inside a `while` inside the
  `repeat`.
- `pointer-moved-before-until-round`: the false `until` reaches the next
  round with the owner given away.
- `pointer-freed-before-exception`: the example above (Free, a raising
  index, the assignment, a handler that uses the owner).
- `pointer-moved-before-raise`: given away, `Raise`, the handler uses it.
- `pointer-freed-in-inner-handler`: an inner handler frees and raises
  again, the outer handler uses the owner.
- `pointer-freed-in-ensure-before-leave`: `ensure` frees, the try `leave`s,
  used after the loop.
- `pointer-freed-in-ensure-before-continue`: `ensure` frees, the try
  `continue`s.
- `pointer-freed-in-ensure-before-until`: `ensure` frees, the try's true
  `until`, used after the `repeat`.
- `pointer-moved-before-retry`: a handler frees, then `Retry`.
- `pointer-freed-in-ensure-before-retry`: `ensure` frees, a handler
  `Retry`s.
- `pointer-moved-owner` (the runtime test moved, Q3).

Positive (`tests/runtime/`, accepted, output checked, leak-free):
- `pointers-moves-across-jumps`: assigned again before `leave` and before
  `continue`; an owner declared in the body moved before `leave`; nested
  loops whose inner `leave` carries nothing out; a `leave` branch that
  moves and a fall-through that uses (the use is on another path).
- `pointers-moves-across-until`: assigned again before `until`; `until`
  inside a `while`; an owner declared in the body moved before `until`.
- `pointers-moves-across-handlers`: Free and assignment in the body with a
  handler that assigns before using; a `Raise` after a move whose handler
  assigns again; a handler that does not use the owner.
- `pointers-moves-across-ensure`: `ensure` assigns again on the `leave`,
  `continue` and `until` paths; `ensure` that does not touch the owner.
- `pointers-moves-before-retry`: a handler that assigns again before
  `Retry`; an `ensure` that assigns again before `Retry`.
- `pointers-moved-then-exit` (Q1): Free then `Return`, `Raise` or `Exit` in
  a branch, use after it.
- `pointers-owner-freed-by-callee` (`.trap`, Q3): the independent check of
  trap 17 (the observation above): the program compiles, reaches the check
  and stops with "use of an owner after Move or Free", never a null access.
  Its LLVM IR snapshot shows the check.

Gates (E19): suite Debug, Release, ASan+UBSan (1 MiB), `string_leak_check`,
`owner_safety_check`, `stack_frames`, fuzz 1 and 7, differential,
backend_gaps, LLVM IR and TIR snapshots against main (only the added and
moved tests change), openspec, constitution_check, version_check.

## Approval

Q1-Q3: answered yes by the maintainer, Marcelo Fortes, on 2026-11-05 (Q3
with the run-time protection preserved and independently tested). The
revised design waits for his approval (G7) before implementation.
