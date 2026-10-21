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
               parameters, case, Strings, text files, Std.Strings /
               Std.StrUtils with concatenation, UInt64, Enum, Range and Set
               (done); Char values (done, r3.83); Float conversions
               and Float Const (done, r3.84); `case` on Strings
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

- `uint64` — UInt64/UInteger values (done, r3.79).
- `enum-range-set` — Enum, Range and Set with `Ord`, `Low`/`High`,
  `Succ`/`Pred`, enum `for` and `case` coverage (done, r3.80).
- `std-strings` — Std.Strings / Std.StrUtils, String concatenation (done,
  r3.78). Later: grapheme lengths, collation, normalization, Split/Join,
  conversions between numbers and text.
- `method-reuse` — `Struct (TPart)`, Override, methods of built-in types
  (done, r3.66).
- `design-by-contract` — Eiffel's contracts with `pre`/`pos`/`invariant`/
  `variant` (done, r3.67). Checks always on through 1.0 (r3.68). After 1.0:
  Eiffel-style monitoring levels per module, and static proof of contracts
  (SPARK style), FUTURE.
- `safe-pointers` — one owner `Own<T>`, checked `Link<T>`, `Option<Own<T>>`,
  no counting, no collector (done, r3.73, CANON-25). Later: Links to
  parameters, the escape analysis that keeps linked locals on the stack,
  arenas as an option, destructors (`lifecycle`).
- `safe-pointers-followups` — where the implementation differs from the
  `safe-pointers` design and the gaps to close, one task each (Links to
  parameters, zero-cost Links, moves over `leave`/`continue`, release per
  loop iteration, invariants through pointers, owner results discarded,
  `Option<T>` with generics).
- `generics` — generic routines and structs in their simplest form: no
  constraints, inference only, any type as T (done, r3.77). Next: methods
  of generic structs, then `Option<T>`, `Result<T, E>` and `Vector<T>`.
- Designs awaiting decisions: `binary-streams` (binary files, positions;
  streams after generics),
  `variant-structs` (one collection of several kinds, examined with `case`;
  direction recorded in r3.69, design after `enum-range-set`).
- Directions recorded in r3.70, designs to write: `encapsulation` (close to
  Object Pascal and Rust), `routine-types` (Pascal procedural types: named
  routines first, closures after `safe-pointers`), `lifecycle`
  (constructors and destructors: the maintainer's analysis).
  `docs/OOP_COMPARISON.md` lists what object orientation gives and Inox's
  answer to each point.
- `char-values` — Char values and output (done, r3.83). Later: `Get(C)`,
  Char classification and case mapping.
- `float-conversions` — Float conversions and real Consts (done, r3.84).
- Then the rest of 0.2: `case` on Strings (approved 2026-10-19), then the
  v0.2.0 release.
