# fixed-arrays Specification

## Purpose
Fixed arrays `Array[Low..High] T` (CANON-13). Arrays inside struct fields and
State, and String or struct elements, are specified but not implemented yet.

## Requirements

### Requirement: Arrays are bounded value types
`Array[Low..High] T` SHALL be a fixed-size value type whose bounds are integer
literals or Const names, with Low <= High. Assignment and by-value passing SHALL
copy the array. An array without initializer SHALL start with every element at
its type's zero. Arrays SHALL NOT be compared.

#### Scenario: Declaration, copy and parameters
- **WHEN** a program fills an array, copies it, changes the copy and passes arrays to routines
- **THEN** the original is unchanged and the results are correct (tests/runtime/arrays.inox)

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
