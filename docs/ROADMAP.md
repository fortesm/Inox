<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Roadmap to 1.0

A plan, not law: the order in which the law of
`openspec/specs/constitution/` is being implemented. Status is in
`docs/STATUS.md`. The milestone numbers below were written in 2026-06,
before the version scheme of r3.60; whether to renumber them is open
question 2 in `openspec/specs/constitution/revisions.md`.

```text
0.1.x   foundation: canonical spec, CLI, examples, tests, release validation, backend subset
0.2.x   imperative core completion and selected deferred features
0.3.x   aggregates, arrays, enums, ranges, sets, case exhaustiveness
0.4.x   standard library/runtime baseline
0.5.x   portability hardening
0.6.x   quality gates and CI maturity
0.7.x   safety/runtime-fault maturity
0.8.x   advanced composition/protocol direction
0.9.x   beta/freeze
1.0.0   stable core language, reproducible releases, coherent docs and examples
```

The backend sequence (clang as the external driver until the criteria of
E22 hold) is in `openspec/specs/constitution/engineering.md`.

## Next work (OpenSpec changes, in the maintainer's order)

- `uint64` — UInt64/UInteger values.
- `enum-range-set` — Enum, Range and Set with `Ord`, `Low`/`High`,
  `Succ`/`Pred`, enum `for` and `case` coverage.
- `std-strings` — Std.Strings / Std.StrUtils, String concatenation.
- Then: Currency and Crypto; Char values and output; Float conversions and
  Float `Const`; `Std.Debug.Assert`; module exports; Vector with move
  semantics.
