## ADDED Requirements

### Requirement: Arrays as struct fields
A struct field SHALL be allowed to have an array type. Its elements SHALL start
at zero (or at the element struct's defaults). `S.F[I]` SHALL read and write
one element with the index checked; copying the struct SHALL copy the array.
An array field SHALL NOT have a literal default.

#### Scenario: Field elements and copy
- **WHEN** a program writes `B.FData[2] := 7`, copies B into C and changes `C.FData[2]`
- **THEN** `B.FData[2]` is still 7 (tests/runtime/struct-array-field.inox)

#### Scenario: Default on an array field
- **WHEN** a struct declares `FData Array[1..2] Integer := 1`
- **THEN** the compiler rejects it (tests/diagnostics/struct-array-field-default.inox)

#### Scenario: Field index out of bounds
- **WHEN** `B.FData[I]` runs with I = 5 on `Array[1..4] Byte`
- **THEN** the program stops with array index out of bounds (tests/runtime/struct-array-field-out-of-bounds.inox)
