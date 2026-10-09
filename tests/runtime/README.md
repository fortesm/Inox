# Runtime tests

Each `NAME.inox` is compiled and executed with `inox --run`.

| Sidecar | Meaning |
| --- | --- |
| `NAME.out` | the program must exit 0 and print exactly this output |
| `NAME.trap` | the program must compile, run and stop with a runtime fault: non-zero exit status and the diagnostic in this file (for example `Inox runtime error: division by zero`) |
| `NAME.out` with `NAME.trap` | additionally, the complete output (program output, runtime diagnostic, driver note) must match exactly |
| `NAME.in` | optional standard input for the program |

These tests pin the checked-arithmetic rules of CANON-21: integer overflow, division
by zero, out-of-range shift counts and invalid `for` steps trap and are never
wraparound or undefined behavior.

A runtime fault is a deterministic Inox trap (CANON-19): the program flushes its
output, prints `Inox runtime error: <category>` on standard error and exits with
status 70. It is not an exception: `try`/`except`/`ensure` cannot intercept it
(`fault-not-catchable.inox`).
