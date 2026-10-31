## Why

While implementing F2 (change `source-positions`), Windows Debug overflowed
the default 1 MiB stack in the boundary tests of CANON-19. Measured on
Windows Debug with `editbin /STACK`, the whole compilation of
`limits-boundary-ok` needs 961 KiB on main: about 6% of margin, not the
margin of about two that CANON-19 states (measured on Linux Debug+ASan).
The coming foundation steps (F3-F6) change the very functions on that
stack.

Approval: Marcelo Fortes, 2026-10-31: alternative (a), an 8 MiB stack
reserve for the compiler on Windows, and a permanent measuring job on the
pull requests that change the compiler; the depth limits do not change,
and the larger reserve must not hide a regression.

Touches: E19 (`engineering.md`, relocked). Compiler phases: 7 driver (its
link options); none of the phases' behaviour.

Risk: normal (a link option and a CI job; no code path changes).

Status: **implemented** (canon r3.113).

## What Changes

1. `CMakeLists.txt`: `inox` links with `/STACK:8388608` on Windows (a
   virtual reserve, not committed memory). The programs Inox builds, the
   runtime and the test executables keep the default.
2. Gates job `windows-stack` (pull requests that change `src/compiler`,
   `src/runtime`, the CMake files or the workflow): builds the head and
   the base in Windows Debug and finds, with `editbin`, the smallest
   reserve with which the boundary tests pass (whole compilation, and
   parsing only); it fails above 4096 KiB (half the reserve) or when the
   head needs more than 5% (at least 32 KiB) over the base, and reports
   both numbers.
3. The place of the last semantic error (`semanticErrorPlace()`) and the
   place the analyzer keeps are `thread_local`. Every handler of a
   `SemanticError` rethrows it or a new error made from it; none swallows
   one, so the place printed is the place of the error caught. A test
   checks the place through nested generic instances.
4. E19 and `docs/ARCHITECTURE.md` describe the budget.

## Out of scope

The depth limits of CANON-19; the stack of the programs Inox builds; Linux
(its main thread already has 8 MiB, and the 1 MiB sanitized run keeps the
CANON-19 margin there).
