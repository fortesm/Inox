## Current behavior (main 7401b1a)

Code generation keeps one slot per owned local for the whole routine
(`registerOwnedSlot`): it starts empty in the entry block, a declaration
that runs again frees the previous value first (`emitOwningStore`; for a
linked local, `declareTombstone` frees the previous tombstone), and every
exit and the routine's cleanup pad free all slots in reverse order of
registration (`emitReleaseOwnedSlots`, null-safe). So nothing leaks or is
freed twice, but a loop-local lives until its next declaration or the end
of the routine, and its Links stay alive after its scope (proposal.md).

## Decisions proposed

1. **Which locals (Q1).** Every local declared in the loop body, in nested
   blocks of the body (`if`, `case`, `try`, `with`) but not in an inner
   loop's body (that loop ends its own), whose slot is an owner, a value
   holding owners (struct or array) or a linked local's tombstone. The
   scope is the body, the unit item 2.2 names; a general end-of-block rule
   for every block belongs to `lifecycle` (M5).
2. **When (Q2).** The body ends:
   - at the end of a round (falling through the last statement) and at
     `continue`, before the next round;
   - at `leave` and at a true `until`, before the statement after the loop;
     an `until` inside an inner loop ends the inner body, then the repeat's
     body;
   - at `Return` and `Exit`, before the routine's own release;
   - when an exception leaves the body (a `Raise`, or one from a call),
     in the cleanup on its way out.
   A false `until` does not end the body.
3. **Order (Q3).** Reverse order of declaration within the body (the last
   declared first), inner bodies before outer ones, as the routine's
   release already does; `ensure` first: a path that leaves a try inside
   the body runs that try's `ensure` while the body's locals are alive,
   then the body ends (CANON-15: `ensure` runs when control leaves the
   try; the body encloses the try).
4. **How.** Ending a slot is what `Free` does: load, free if not empty,
   store empty. An owner given away (Move, Free, passed by value) has an
   empty slot, so it is not freed again; the routine's final release
   finds the slot empty. A tombstone freed bumps its block's generation,
   so every Link to the local dies. Struct and array values are released
   by the existing drop helpers (owners in fields and elements, Strings
   they hold).
5. **Strings (Q4).** Strings and values that hold only Strings are
   reference counted and their release is not observable; they keep the
   current rule (released at the next declaration or the routine's end).
   Extending the rule to them changes the IR of most loops with no
   observable gain; recommended: not now.
6. **Where the decision lives (Q5).** The set of locals each loop body
   ends is a scope fact. Option A (recommended): code generation derives it
   from the TIR structure (the declarations among the body's statements,
   outside inner loops), with no new TIR field; the decision of what a
   scope is stays in the TIR shape phase 5 produced. Option B: phase 5
   records the list on the loop statement of the TIR (a new field), and
   code generation only reads it. B is more explicit, A changes less; B is
   needed if a scope rule ever depends on semantic facts.
7. **Return of a loop-local owner.** `Return P` (or `Return Move(P)`) of an
   owner declared in the body hands it to the caller first (its temporary
   is disarmed, the slot emptied), then the body ends with an empty slot.
8. **Stack.** No new recursion; one release sequence per body exit, as
   `emitReleaseOwnedSlots` already emits per routine exit; checked by
   `stack_frames.py` and the Windows stack budget.

## Decisions (approved 2026-11-08)

- **Q1.** Owners and linked locals declared in loop bodies and their inner
  blocks (values holding owners included).
- **Q2.** Cleanup at the effective exits of the scope: a round's end,
  `continue`, `leave`, `until`, `Return`, `Exit`, exceptions that leave it.
- **Q3 (with adjustment).** The order follows the real lexical nesting: the
  `ensure`s actually crossed run in order, interleaved with the cleanups of
  the scopes left, not all `ensure`s before all cleanups.
- **Q4.** Strings and values holding only Strings keep their release;
  values holding owners are cleaned fully.
- **Q5.** Lifetime limits explicit in the TIR, minimal: each loop records
  the locals its body declares (`Stmt::scopeEnds`), a scope fact the TIR
  builder copies from the body's declarations; code generation decides only
  how each local's storage is released.

Implementation note: code generation emits a body whose `scopeEnds` hold a
releasable local as a protected region (the lowering of a try with
`ensure`), whose `ensure` ends those locals; the existing cleanup contexts
then give every transfer and the unwinding path the lexical order of Q3,
with no second dispatch mechanism.

## Acceptance tests

Runtime (`tests/runtime/`, under the string leak check with ASan):
- `pointers-loop-body-lifetime`: `for`, `while` and `repeat` bodies (a Link
  alive in the round, dead after it); a linked local; `continue` and
  `leave`, with an owner moved out of the body (not freed again); `until`
  inside a `while` inside a `repeat` (both bodies end); nested loops (the
  inner body ends each inner round, the outer local lives); owners in a
  struct and in an array declared in the body; an `ensure` inside the body
  that still sees the locals; an exception that leaves the body (the inner
  `ensure` sees the locals, the handler and the `ensure` outside the loop
  see them ended); exceptions caught inside the body; `Return` and `Exit`
  from inside a loop.
- `pointers-loop-link-after-round` (`.trap`): reading a Link to a loop
  local after its round traps.
- `pointers-loop-conditional-locals`: locals declared on conditional
  paths: an `if` arm that runs in one round only and one that never runs,
  alternative `case` arms (an owner, a struct of owners, a linked local), a
  `try` body and its handler, a nested loop whose body declares on one
  round only. Every such slot is allocated and emptied in the entry block
  (`inox.entry-init`), so the end of a round in which the declaration did
  not run releases an empty slot; checked by the LLVM verifier (`opt
  -passes=verify`, and clang on every run) and under ASan+UBSan.
- `tests/codegen/llvm-loop-conditional-owner-smoke.inox` (both harnesses):
  the slot's entry initialization and the body's release from the slot.
- `tests/tir/statements.tir`: the `ends` line of a loop.

Gates (E19), with the snapshots of main 7401b1a: every program whose IR
changes is a loop that declares an owner or linked local in its body, and
each is listed in the PR; all others identical.

## Approval

Design approval: Marcelo Fortes, 2026-11-08 (G7), Q1-Q5 as recorded above.
