## Why

Asked how Inox compares with C, Go, Rust and Python, a measurement showed
that the driver called clang without any `-O`: every program was built at
-O0. The same IR built at -O2 runs an arithmetic loop as fast as C -O2.
CANON-23 mentioned an optimization policy without defining one.

| 300 million steps of `(I * I) mod 7 + I / 3` | time |
|---|---|
| Inox as built before (-O0) | 1.99 s |
| Inox -O2 | 0.41 s |
| C gcc -O2 | 0.41 s |
| Go | 0.48 s |
| Rust -O, with checked overflow | 0.60 s |
| Python | about 47 s |

Approval (Marcelo Fortes, 2026-10-23), to the proposal of an optimization
policy: yes, as the default; optimizations set in the source for LLVM come later.

Risk: normal (every built program changes its machine code; the generated
IR does not change).

Status: **implemented** (canon r3.98).

## What Changes

1. **Driver.** `inox run`, `inox build` and `inox file.inox` call clang++
   with `-O2`; `--debug` right before the file (`inox --debug f.inox`,
   `inox run --debug f.inox`, `inox build --debug f.inox`) uses `-O0`.
   After the file, `--debug` is the program's argument. Help text updated.
2. **Law.** CANON-23 Optimization (new): optimized by default, `--debug`,
   optimization never changes behavior. CANON-23 Lowering rules: no
   nsw/nuw, checks are explicit traps (the "until the policy allows them"
   wording is gone). Optimization chosen in the source for LLVM is a later
   design.
3. **Tests.** The whole suite now runs optimized (988 passed). Both runners
   add three `--debug` driver tests: arguments, the 157 expressions of
   math-expressions at -O0, and a trap at -O0.
4. **Docs.** Site (commands and a paragraph on optimization), CHANGELOG,
   STATUS.

## Later (recorded, not designed)

Optimizations chosen in the source and passed to LLVM (per routine or
module), as the maintainer announced.
