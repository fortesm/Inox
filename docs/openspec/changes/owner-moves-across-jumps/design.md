## Where the edges are lost (main, r3.133)

The analysis keeps `movedOwners_` (symbol -> scope of its declaration), the
owners given away on some path that reaches the current statement.

- `LeaveStatement` and `ContinueStatement` (SemanticStatements.cpp, the
  `loopDepth_` checks) only check that a loop encloses them; they neither
  record the current set nor end the path.
- `IfStatement`, `UnlessStatement`, `CaseStatement`, the try regions: each
  branch's final set is merged into the set after the statement, so a
  branch that ended in `leave` or `continue` feeds the fall-through instead
  of its jump target.
- `requireOwnersBackAfterLoop` compares only the set at the end of the body
  with the set before the loop. The set at a `continue` (the next round)
  and the sets at the `leave`s (after the loop) never reach it, so:

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
   `leave`, `continue` (and with Q1 `Return`, `Raise`, `Exit`) end the path:
   the state after them is unreachable. A merge (if/unless/case, the end of
   a try) unites the reachable incoming sets; when none is reachable, the
   state after the statement is unreachable. Statements after a jump in the
   same block are analyzed with the unreachable state (nothing is moved on
   it; the rest of the analysis is unchanged).
2. **Loop frames.** Each loop pushes a frame holding the union of the sets at
   its `leave`s and at its `continue`s. A jump adds the current set to the
   innermost loop's frame (Inox has no labels: `leave` and `continue` act on
   the innermost loop, so nested loops are frames on a stack).
3. **End of a loop body.** The set at the end of the body (if reachable)
   united with the frame's `continue` set must add no owner declared before
   the loop to the set before it (rule 3, `requireOwnersBackAfterLoop`, its
   message unchanged: "assign it again before the next round"). For
   `repeat`, a `continue` goes to `until`, whose condition then sees that
   set.
4. **After a loop.** The set after it is the set before it united with the
   frame's `leave` set; an owner given away before a `leave` and used after
   the loop is the existing error ("was given away (Move or Free) on a path
   that reaches here").
5. **Owners of inner scopes.** A set recorded at a jump keeps only the
   owners declared in a scope that encloses the loop; those declared inside
   the body die with it (`forgetMovedOwnersOf`, unchanged).
6. **`ensure`.** A `leave` or `continue` inside a try runs its `ensure`
   first: the owners `ensure` gives away are added to the sets recorded
   inside that try's body and handlers. A jump inside the `ensure` body
   itself (accepted today) records the set it sees there, like any jump.
7. **Run time.** The trap-17 check stays; no code generation change.
8. **No new syntax, no change of CANON-25.** Rule 3 is the rule; the change
   makes the analysis follow every path it names.

## Questions

- **Q1 (exits).** `Return`, `Raise` and `Exit` end the path like a jump, so
  `if E: Free(P); Return; ;` followed by a use of P is accepted (today it is
  rejected: the branch's set is merged into the fall-through).
  Recommendation: yes; it uses the same reachability, makes no unsound
  program pass, and only accepts programs whose rejection was a false
  positive.
- **Q2 (Retry).** `Retry` runs the try body again; the owners given away
  when it runs reach the body's start. Same class of gap (trap 17 at run
  time). Recommendation: follow it in this change, with the loop rule: the
  set at a `Retry` must add no owner declared before the try to the set at
  the try's start ("assign it again before Retry"). Alternative: a later
  item.
- **Q3 (the runtime test).** `tests/runtime/pointers-moved-owner.inox`
  exists to show the run-time backstop; once the analysis follows `leave`
  it is a compile error. Recommendation: move it to `tests/diagnostics/`
  with that error; the check stays in the IR (the snapshots show it), and no
  accepted program can reach it any more.

## Acceptance tests

Diagnostics (each with its line and column):
- `pointer-moved-before-leave`: given away, `leave`, used after the loop.
- `pointer-moved-before-continue`: `continue` skips the assignment.
- `pointer-moved-before-leave-nested`: inner loop's `leave`, used in the
  outer loop after the inner one.
- `pointer-moved-before-leave-branch`: `leave` in one `elif` arm.
- `pointer-moved-in-repeat-continue`: `repeat` with `continue`.
- `pointer-moved-before-leave-ensure`: `ensure` frees, the try `leave`s.
- `pointer-moved-owner` (the moved runtime test, Q3).
- With Q2: `pointer-moved-before-retry`.

Runtime (accepted, output checked, leak-free):
- `pointers-moves-across-jumps`: assigned again before `leave` and before
  `continue`; an owner declared in the body moved before `leave`; nested
  loops whose inner `leave` carries nothing out; a `leave` branch that
  moves and the fall-through that uses (the use is on another path).
- With Q1: `pointers-moved-then-return` (Free then Return in a branch, use
  after it).

Gates (E19): suite Debug, Release, ASan+UBSan (1 MiB), `string_leak_check`,
`owner_safety_check`, `stack_frames`, fuzz 1 and 7, differential,
backend_gaps, LLVM IR and TIR snapshots against main (only the added and
moved tests), openspec, constitution_check, version_check.

## Approval

(To be recorded: who approved, the date, and the answers to Q1-Q3.)
