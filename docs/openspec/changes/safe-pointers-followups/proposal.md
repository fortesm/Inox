## Why

Change `safe-pointers` (canon r3.73, CANON-25, PR #58) implemented the
approved design with deviations and gaps, recorded in its design.md ("As
implemented") and in `docs/STATUS.md`. Marcelo Fortes, 2026-10-16:
"Excelente, registre onde a implementação difere do design e as lacunas
para corrigirmos futuramente". This change keeps them as work to do, one
task each, so none is lost. No language change is decided here: each item
that touches the law goes through its own approval.

Status: open, nothing implemented. Risk: medium to high per item (memory
ownership).

## What differs from the design, and the target

1. **Link to a parameter** (design rule 6 lists parameters). Today
   `Link(P)` of a parameter is a compile error, "not supported yet"; a
   local copy can be linked. Target: a Link to a value parameter (its
   storage placed in a generation block, as a linked local), and decide
   what a Link to a `ref`/`ref mut` parameter means (it reaches the
   caller's variable, whose generation the callee does not know).
2. **Zero-cost Links** (rule 10). Today every linked local lives in a
   generation block ("tombstone"). Target: an escape analysis that keeps
   the local on the stack, with no check, when every Link to it provably
   stays inside its block.
3. **Removing proven checks** (rule 7). Today every use of a Link checks
   its generation. Target: remove the check when no `Free`, owner
   overwrite or routine exit can happen between two uses.
4. **Moves across `leave` and `continue`** (rule 3). Today a use after
   `Move` or `Free` reached through `leave`/`continue` is caught at run
   time (trap "use of an owner after Move or Free"); every other path is a
   compile error. Target: the move analysis follows these edges too, so
   the error is always at compile time and the backstop check can go.
5. **Release in loop bodies** (rule 4, "when its block ends"). Today an
   owner or linked local declared in a loop body is freed when its
   declaration runs again or the routine returns. Target: freed at the end
   of each iteration, as the rule says.
6. **Invariants through pointers** (CANON-24 with CANON-25). Today a write
   through `Own`/`Link` to a struct with `invariant` lines is not checked.
   Target: check the invariant after such a write, as after a field write
   from outside.
7. **Discarding an owner result**. A function called as a statement is a
   backend gap (STATUS); an owner it returns must be freed at once when
   that gap is closed.
8. **Built-in spellings to fold into generics**. `Own<T>`, `Link<T>` and
   `Option<Own<T>>` are special type names (`support/PointerType`), and
   `Option` exists only around `Own`. Target: with change `generics`,
   they become ordinary generic types of the type representation, and
   `Option<T>` works for any T; `Vector<Own<T>>` (the design's
   `FFilhos`) comes with `Vector<T>`.

Not gaps (decided elsewhere): an array of owners is
`Array[...] Option<Own<T>>` by design; how pointers cross threads is
decided with concurrency (`do`); arenas, `Self owned` and destructors are
FUTURE (`lifecycle`).
