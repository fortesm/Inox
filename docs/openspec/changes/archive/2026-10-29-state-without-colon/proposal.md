## Why

CANON-4 gives `:` one meaning — it declares a routine and opens its body —
but the State section was written `State :` … `;`, the only other use of
`:`. The maintainer decided (Marcelo Fortes, 2026-10-12): State takes no `:`, and explained the idea behind it: a character was needed to say where a routine's body begins, hence `:`; it is needless in other blocks such as if, for, while and with; Inox is about writing less and doing more; `:` both opens the block and tells the compiler that `Sum(...)` is a routine.
The section keeps its closing `;` (Marcelo Fortes, 2026-10-13; an earlier answer, the same as Type, came from
a truncated message and is withdrawn).

Risk: normal (parser of one section; no lowering, runtime or ownership
change).

## What Changes

- `State` opens its variables at the line break and closes them with `;`,
  like a control structure: `State`, one declaration per line, `;`. A
  module may have several State sections.
- Rejected with a migration message: `State :`, a State section that
  reaches the next declaration without its `;`, and the old entry form
  `Name : Type` in State and Const (`:` only declares a routine).
- The constitution records the idea of the syntax ("write less, do more")
  and the double role of `:` in the maintainer's words (CANON-4); CANON-5
  "State" changes; specs `language/module-state` and
  `language/declaration-sections` change.
- Every program, test, gap probe and manual example with `State :` is
  migrated; grammar `state_section` changes.

Constitution requirements touched: CANON-4 (Semicolon closes, colon
declares; Sections and removed forms), CANON-5 State, the section "The idea
of the syntax". Out of scope: Type and Const (already without `:`).
Behavior that must not change: the meaning of every State program.
