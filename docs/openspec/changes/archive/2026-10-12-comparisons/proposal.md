## Why

Strings had only `=` and `#`. Marcelo Fortes, 2026-10-12: Strings are ordered as in Pascal (<, >, >=, <=, #, not, unless, combined with or and and), as a professional language and compiler require, and the specification says so

Checking this found a wider hole: the analyzer accepted `<` on Strings, `=`
and `<` on structs, `<` on Bool, and even `=` on two Bool variables, and the
backend then emitted invalid IR (clang rejected it). No probe covered these.

Risk: low (one runtime function, comparison lowering, analyzer rules); no
design.md needed.

## What Changes

- Layer A (canon v3.56, CANON-14): `<` `>` `<=` `>=` order Strings byte by
  byte over UTF-8, a prefix first: Unicode code point order, case-sensitive,
  not a locale collation.
- Bool `=` and `#` are lowered in their own width (they produced invalid IR).
- Bool is ordered false < true (Marcelo Fortes, 2026-10-12: Bool is ordered, false < true, as in Pascal and Ada; canon v3.57).
- Structs are never compared (Marcelo Fortes, 2026-10-12: structs are not compared with =/#; compare them field by field, as in Pascal; it stays an error that says so; canon v3.57). Both used to be accepted
  and miscompiled.
- Runtime: `__inox_str_compare(ptr, ptr) -> i32` in libinoxrt.

## Capabilities

### New Capabilities
- `language/comparisons`: which operand types each comparison operator
  accepts, and how the results combine.

### Modified Capabilities
(none; the String ordering requirement is part of `language/comparisons` so
that this change does not depend on the archive order of `string-runtime`)

## Impact

runtime/StringRuntime.cpp, the emitter's comparison lowering, the analyzer's
relational rule, tests, canon v3.56, manual, CHANGELOG, backend_gaps probes.
