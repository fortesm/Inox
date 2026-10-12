## ADDED Requirements

### Requirement: Arrays of struct
An array element type SHALL be allowed to be a struct. Each element SHALL start
with the struct's type defaults. `A[I].Field` SHALL read and write one field of
one element, with the index checked as for any array.

#### Scenario: Element fields
- **WHEN** a program declares `A Array[1..3] TPoint`, writes `A[2].FX := 5`, calls `A[1].Move(11)` and prints the fields, also of a 2-D array of structs
- **THEN** it prints the defaults and the new values (tests/runtime/array-of-struct.inox)

#### Scenario: Element index out of bounds
- **WHEN** `A[I].FX := 1` runs with I = 4 on `Array[1..3] TPoint`
- **THEN** the program stops with array index out of bounds (tests/runtime/array-of-struct-out-of-bounds.inox)
