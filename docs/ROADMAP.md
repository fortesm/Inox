<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Roadmap to 1.0

A plan, not law: the order in which the law of
`docs/openspec/specs/constitution/` is implemented. Status is in
`docs/STATUS.md`. Renumbered on 2026-10-12 for the version scheme of r3.60
(decided by Marcelo Fortes); the old
0.1.x–0.9.x plan of 2026-06 is in `docs/history/canon-changelog.md`. A
MINOR version closes when its list is done; the contents of a later MINOR
may still move.

```text
0.1     done   foundation: spec, driver, tests, release packages, LLVM
               subset, checked arithmetic, exceptions on Linux and Windows
0.2     done   the imperative core: structs and method reuse, fixed arrays
               (Fill), slices, ref parameters, case (also on Strings),
               Strings and Std.Strings/StrUtils, text files, UInt64, Enum,
               Range and Set, Char, Float conversions and real Consts,
               generics (simplest form), design by contract, safe pointers,
               Std.Debug.Assert; transitive initialization; != (r3.61-r3.87)
0.3            numbers, text and the basic library: Currency and Crypto
               (exact decimals, done, r3.91); conversions between numbers and text,
               FieldCount/Field (done, r3.90); real literals with
               exponents, Get(C),
               Char classification and case mapping (done, r3.89);
               Std.DateTime/
               DateUtils, Std.SystemUtils (done, r3.92); console I/O in
               libinoxrt (done, r3.88); a function is not a statement (done,
               r3.88); Std.Terminal (done, r3.88) and a micro-editor
               (examples/edit.inox, done, r3.92) that accepts 0.3;
               never released apart: it ships in 0.4 (r3.101)
0.4            modules and encapsulation (done, r3.99): private by default
               with Public, Export Use, qualified names, aliases, selective
               imports, the search path (--path, INOX_PATH), inox
               interface, struct encapsulation per module; read-only
               fields later
0.5            ownership and collections: Vector<T> with move semantics and
               Clone, Option<T> and Result<T, E> (TryInteger), Split and
               Join, Self owned; methods of
               generic structs; the safe-pointers follow-ups; `lifecycle`
               (constructors and destructors); arenas as an option
0.6            expressiveness: routine types (named routines, then
               closures), variant structs, binary files and streams; the
               `| |`, `{ }` and `@` brainstorm settled; generic constraints
               if real programs ask for them
0.7            backend and quality: object files and linker integration
               (E22, no external clang driver), BSD and other Unices,
               clang-format, clang-tidy, cppcheck, coverage; typed AST for
               types, exception hierarchy in the types, line numbers in
               every diagnostic
0.8            alpha: the 1.0 scope is complete; documentation, examples
               and the site cover all of it
0.9            beta: the language is frozen, fixes only
1.0            stable: the core language, reproducible releases, coherent
               documentation and examples
after 1.0      concurrency with data-race safety (Chapel, Go, Rust, Delphi's
               forall; atomic String and File counts); contract monitoring
               levels per module and static proof (SPARK); BigCurrency;
               interface with C libraries (FFI: declaring external C
               routines, loading .so/.dll; C++ through C wrappers); a
               separate repository of Inox libraries, such as an Inox DBO
               (database objects)
```

Stages (constitution VERSIONING): pre-alpha until the 1.0 scope is complete,
then alpha, beta and stable. What belongs to the 1.0 scope is the
maintainer's decision. Marcelo Fortes decided on 2026-10-21: concurrency
comes after 1.0; 1.0 has no fixed date and may come after Christmas 2026;
visibility (0.4) comes before collections (0.5); the interface with C
libraries comes after 1.0.

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
- `modules-encapsulation` — 0.4: modules and encapsulation (done, r3.99;
  it answers the `encapsulation` direction of r3.70).
- Directions recorded in r3.70, designs to write: `routine-types` (Pascal procedural types: named
  routines first, closures after `safe-pointers`), `lifecycle`
  (constructors and destructors: the maintainer's analysis).
  `docs/OOP_COMPARISON.md` lists what object orientation gives and Inox's
  answer to each point.
- `char-values` — Char values and output (done, r3.83). Later: `Get(C)`,
  Char classification and case mapping.
- `float-conversions` — Float conversions and real Consts (done, r3.84).
- `case-strings` — `case` on a String (done, r3.85).
- `float-policy` and `not-equal-operator` — the floating-point policy and
  `!=` (done, r3.86 and r3.87).
- Releases: 0.2 was never tagged; 0.4.0 is the first version published
  by the release workflow, which publishes every new version merged into
  main (r3.101).
- 0.3, decided by Marcelo Fortes on 2026-10-21 (A1-F3) and delivered in
  five separate pull requests, one per change, to be tested apart: `console-abi-terminal` (the console in libinoxrt, a function is
  not a statement, Std.Terminal; done, r3.88); `input-chars-exponents`
  (`Get(C)`, `EndOfInput`, Get of reals, Char classification, real literals
  with exponents; done, r3.89); `numbers-text` (conversions between
  numbers and text, the shortest real text, FieldCount/Field; Split/Join
  move to 0.5, with collections; done, r3.90); `decimals` (Currency and
  Crypto; done, r3.91); then Std.DateTime/DateUtils,
  Std.SystemUtils and the micro-editor `examples/edit.inox`.
