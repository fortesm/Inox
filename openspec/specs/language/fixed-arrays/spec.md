# fixed-arrays Specification

## Purpose
Fixed arrays `Array[Low..High] T` (CANON-13 "Array"). Part of the Inox law,
detailing the constitution (`openspec/specs/constitution`).

## Requirements

### Requirement: Arrays are bounded value types
`Array[Low..High] T` SHALL be a fixed-size value type whose bounds are integer
literals or Const names, with Low <= High. Assignment and by-value passing SHALL
copy the array. An array without initializer SHALL start with every element at
its type's zero. Arrays SHALL NOT be compared.

#### Scenario: Declaration, copy and parameters
- **WHEN** a program fills an array, copies it, changes the copy and passes arrays to routines
- **THEN** the original is unchanged and the results are correct (tests/runtime/arrays.inox)

#### Scenario: Bounds are part of the type
- **WHEN** an `Array[1..3] Integer` is assigned to an `Array[0..2] Integer`
- **THEN** the compiler rejects it (tests/diagnostics/array-bounds-part-of-type.inox)

#### Scenario: Array literals are reserved
- **WHEN** a program writes `A Array[1..3] Integer := [1, 2, 3]`
- **THEN** the compiler rejects it (tests/diagnostics/array-literal-reserved.inox)

#### Scenario: Empty bounds and comparison
- **WHEN** a program declares `Array[3..1] Integer` or compares two arrays
- **THEN** the compiler rejects it (tests/diagnostics/array-empty-bounds.inox, tests/diagnostics/array-compare.inox)

### Requirement: Every index is checked
An index SHALL be an integer. A constant index outside Low..High SHALL be a
compile-time error; any other index outside it SHALL stop the program with
"array index out of bounds".

#### Scenario: Run-time index out of bounds
- **WHEN** `A[I]` runs with I = 4 on `Array[1..3]`
- **THEN** the program stops with array index out of bounds (tests/runtime/array-index-out-of-bounds.inox)

#### Scenario: Constant index and non-integer index
- **WHEN** a program writes `A[4]` on `Array[1..3]` or `A[1.5]`
- **THEN** the compiler rejects it (tests/diagnostics/array-constant-index-out-of-bounds.inox, tests/diagnostics/array-index-not-integer.inox)

### Requirement: Multi-dimensional arrays, Low, High and Length
`Array[L1..H1, L2..H2] T` SHALL be an array of arrays and `M[I, J]` SHALL mean
`M[I][J]`. `Low(A)`, `High(A)` and `Length(A)` SHALL be compile-time constants.
`A[I] += X` SHALL be accepted when the indices have no side effects.

#### Scenario: Matrix, bounds functions and compound assignment
- **WHEN** a program uses a 2-D Byte array, Low/High/Length and `A[2] += 100`
- **THEN** it prints the expected values (tests/runtime/arrays.inox)

#### Scenario: Index with a call in a compound assignment
- **WHEN** a program writes `V[Next] += 1` where Next is a function
- **THEN** the compiler rejects it (tests/diagnostics/compound-index-target.inox)

### Requirement: Element writes need a mutable array
`A[I] := X` SHALL require a mutable array variable; a by-value parameter is
immutable.

#### Scenario: Writing a parameter's element
- **WHEN** a routine assigns an element of its array parameter
- **THEN** the compiler rejects it (tests/diagnostics/array-parameter-immutable.inox)

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
