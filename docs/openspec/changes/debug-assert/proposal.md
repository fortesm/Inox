## Why

`Std.Debug.Assert` was reserved "until canonical trap/abort behavior
exists" (open question 3 of `revisions.md`). The maintainer chose the trap
(Marcelo Fortes, 2026-10-13, alternative A of open question 3): a failed Assert
stops the program like an overflow.

Risk: normal (a branch to the existing runtime-fault function; no exception
lowering, runtime ABI or ownership change).

## What Changes

- `Use Std.Debug` makes `Assert(Condition Bool)` and `Assert(Condition Bool,
  Message String)` visible; without the `Use` the name is free.
- The arguments are evaluated in order; when the condition is false the
  program flushes its output, prints `Inox runtime error: assertion failed`
  (and `: Message`), and exits with 70 (fault kind 14). No handler or
  `ensure` runs. Assert is never compiled out.
- Constitution CANON-18 (Library modules), CANON-19 (Trap categories); new
  spec `stdlib/debug`; `stdlib/Std.Debug.inox`, manual, STATUS, CHANGELOG.

Out of scope: contracts (`Pre`, `Pos`, `Invariant`), which may reuse this
trap later.
