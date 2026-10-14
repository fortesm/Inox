<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Roadmap to 1.0

A plan, not law: the order in which the law of
`openspec/specs/constitution/` is implemented. Status is in
`docs/STATUS.md`. Renumbered on 2026-10-12 for the version scheme of r3.60
(Marcelo Fortes: "Sim esse roadmap é antigo pode renumerar"); the old
0.1.x–0.9.x plan of 2026-06 is in `docs/history/canon-changelog.md`. A
MINOR version closes when its list is done; the contents of a later MINOR
may still move.

```text
0.1     done   foundation: spec, driver, tests, release packages, LLVM
               subset, checked arithmetic, exceptions on Linux and Windows
0.2     now    the imperative core: structs, fixed arrays, slices, ref
               parameters, case, Strings, text files (done); UInt64; Enum,
               Range and Set; Char values; Float conversions and Float Const;
               Std.Strings / Std.StrUtils with concatenation
0.3            exact decimals (Currency, Crypto); Std.DateTime /
               Std.DateUtils; Std.SystemUtils; Std.Debug.Assert; console I/O
               moved into libinoxrt (the final I/O ABI)
0.4            ownership: Vector<T> with move semantics and Clone,
               Option<T> and Result<T, E>, Self owned
0.5            modules: Export, interface and body, package search
0.6            backend: object files and linker integration (E22);
               portability hardening; clang-format, clang-tidy, cppcheck,
               coverage
0.7            contracts (Pre, Pos, Invariant); generic constraints if
               real programs ask for them
0.8            concurrency (`do`) with data-race safety
0.9            alpha: the 1.0 scope is complete; then beta: frozen, fixes only
1.0            stable: the core language, reproducible releases, coherent
               documentation and examples
```

Stages (constitution VERSIONING): pre-alpha until the 1.0 scope is complete,
then alpha, beta and stable. What belongs to the 1.0 scope (for example
whether concurrency or contracts are in it) is the maintainer's decision.

## Next work (OpenSpec changes, in the maintainer's order)

- `uint64` — UInt64/UInteger values.
- `enum-range-set` — Enum, Range and Set with `Ord`, `Low`/`High`,
  `Succ`/`Pred`, enum `for` and `case` coverage.
- `std-strings` — Std.Strings / Std.StrUtils, String concatenation (design approved, r3.65).
- `method-reuse` — `Struct (TPart)`, Override, methods of built-in types
  (done, r3.66).
- `design-by-contract` — Eiffel's contracts with `pre`/`pos`/`invariant`/
  `variant` (done, r3.67). Checks always on through 1.0 (r3.68). After 1.0:
  Eiffel-style monitoring levels per module, and static proof of contracts
  (SPARK style), FUTURE.
- Designs awaiting decisions: `generics` (simplest form), `safe-pointers`,
  `binary-streams` (binary files, positions; streams after generics).
- Then the rest of 0.2: Char values and output; Float conversions and Float
  `Const`.
