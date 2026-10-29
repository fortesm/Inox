## ADDED Requirements

### Requirement: FormatFloat
`FormatFloat(F, Decimals)` (Std.Strings) SHALL write a Float or a Float32
with exactly Decimals digits after the point, 0..20, rounded to nearest
from the exact binary value, without a point for 0; other Decimals stop
the program with "string index out of range" (r3.90).

#### Scenario: Decimals
- **WHEN** a program formats prices and fractions with 0 to 10 decimals, then 21
- **THEN** each has its digits and the last traps (tests/runtime/format-float.inox); an integer argument is rejected (tests/diagnostics/format-float-argument.inox)

### Requirement: Fields
`FieldCount(S, Sep)` (Std.StrUtils) SHALL be one more than the
non-overlapping occurrences of Sep in S, so `""` has one field and an empty
Sep leaves S whole; `Field(S, Sep, I)` SHALL be the I-th field, from 1,
empty fields included; outside 1..FieldCount it stops the program with
"string index out of range" (r3.90).

#### Scenario: Splitting a line
- **WHEN** a program walks the fields of `"ana;bia;;carla"` and other Strings, then asks for a fifth
- **THEN** it gets each field and the last traps (tests/runtime/fields.inox)
