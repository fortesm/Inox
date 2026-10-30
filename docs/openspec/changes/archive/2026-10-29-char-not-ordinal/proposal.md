## Why

Writing the constitution showed that the analyzer accepted `Length(S)` on a
String and `Ord(C)` on a Char (both then stopped in the backend), although
the law reserves String lengths for `Std.StrUtils` (CANON-14) and defines
`Ord` for enum values (CANON-13), with every Char conversion explicit. The
maintainer confirmed (Marcelo Fortes, 2026-10-12): "Excelente não pode ter
isso para char mesmo!"

Risk: normal (semantic analysis only).

## What Changes

- `Length(X)` is accepted only for a fixed array or a slice; on anything
  else it is "Length takes an array or a slice (CANON-13); String lengths are
  reserved for Std.StrUtils (CANON-14)".
- `Ord(X)` is rejected until enums exist, and never accepts a Char: "Ord
  takes an enum value (CANON-13); a Char is not an ordinal: its conversions
  are explicit (CANON-14)".
- A routine of the program named Length or Ord is still called normally.
- Constitution CANON-14 (Char, Reserved String operations) names the new
  tests; `Std.System` documentation corrected.

Out of scope: enums (change `enum-range-set` brings `Ord(E)`), String
functions (change `std-strings`).
