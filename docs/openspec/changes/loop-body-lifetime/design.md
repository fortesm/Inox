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

## Questions for the maintainer

- **Q1.** Scope: loop bodies only (nested blocks of the body included,
  inner loop bodies excluded)? Recommendation: yes.
- **Q2.** The exits of decision 2? Recommendation: yes.
- **Q3.** Order: reverse declaration, inner first, after `ensure`?
  Recommendation: yes.
- **Q4.** Strings keep their current release? Recommendation: yes.
- **Q5.** Option A (no TIR change) or B (a TIR field)? Recommendation: A.

## Acceptance tests

Runtime (`tests/runtime/`, leak-checked under ASan; trap tests with
`.trap`):
- `pointers-loop-owner-per-round`: an owner declared in `for`, `while`
  and `repeat` bodies, a Link to it checked with `Alive` after each round
  (false) and inside the round (true).
- `pointers-loop-linked-local-per-round`: the same for a linked local.
- `pointers-loop-exits-release`: `continue`, `leave`, true `until`,
  `until` inside a `while` inside a `repeat`, `Return`, `Exit`; nested
  loops; owners in a struct and in an array declared in the body.
- `pointers-loop-release-order`: the order observed through Links to
  several locals (each Link dead exactly when its local ends) and through
  an `ensure` that reads the locals before the body ends.
- `pointers-loop-moved-owner`: an owner moved out of the body (into an
  outer variable, returned, passed by value) is not freed at the round's
  end; double free excluded under ASan.
- `pointers-loop-exception-release`: `Raise` and an exception from a
  call inside the body, caught outside the loop and inside it.
- `pointers-loop-link-after-round` (`.trap`): reading a Link to a loop
  local after its round traps (the main program above).

Gates (E19), with the snapshots of main 7401b1a: every program whose IR
changes is a loop that declares an owner or linked local in its body, and
each is listed in the PR; all others identical.

## Approval

Pending: the maintainer's decisions on Q1-Q5 and dated approval (G7).
