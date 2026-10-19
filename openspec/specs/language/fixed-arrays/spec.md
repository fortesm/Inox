# fixed-arrays Specification

## Purpose
Fixed arrays `Array[Low..High] T` (CANON-13 "Array"). Part of the Inox law,
detailing the constitution (`openspec/specs/constitution`).

## Requirements

### Requirement: Arrays are bounded value types
`Array[Low..High] T` SHALL be a fixed-size value type whose bounds are integer
literals or Const names, with Low <= High. Assignment and by-value passing SHALL
copy the array. Arrays SHALL NOT be compared.

#### Scenario: Declaration, copy and parameters
- **WHEN** a program fills an array, copies it, changes the copy and passes arrays to routines
- **THEN** the original is unchanged and the results are correct (tests/runtime/arrays.inox)

#### Scenario: Bounds are part of the type
- **WHEN** an `Array[1..3] Integer` is assigned to an `Array[0..2] Integer`
- **THEN** the compiler rejects it (tests/diagnostics/array-bounds-part-of-type.inox)

### Requirement: Arrays are initialized where they are declared
Every local and State array, whatever its element type, SHALL have an
initializer (r3.81); without one the compiler SHALL reject it and show the
`Fill` that would fill it. No element is ever an implicit zero, False, first
enum literal or default of a component.

#### Scenario: Array without initializer
- **WHEN** a program declares `A Array[1..3] Integer`, `Hand Array[1..3] TSuit` or a State array without `:=`
- **THEN** the compiler rejects it with `A Array[1..3] Integer := Fill(0)` in the message (tests/diagnostics/array-without-initializer.inox, tests/diagnostics/enum-array-without-initializer.inox, tests/diagnostics/state-array-without-initializer.inox)

### Requirement: Fill takes its array from its target
`Fill(Value)` SHALL be an array whose type and bounds are those of the array
its place expects (a declaration, a State initializer, an assignment, an
argument or a returned value); without one it SHALL be rejected.
`Fill` is a predefined name (CANON-2).

#### Scenario: Fill without an array target
- **WHEN** a program writes `X := Fill(1)`, or declares a routine named Fill
- **THEN** the compiler rejects both (tests/diagnostics/array-fill-without-target.inox, tests/diagnostics/fill-predefined.inox)

### Requirement: Fill copies one value into every element
Value SHALL be evaluated once, and every element SHALL be a copy of it,
following the rules of types (a Range element checks it), copy (each String
element holds its own reference) and construction (a struct value, with its
defaults or built by `TName(...)`). In a multi-dimensional array Value SHALL
be an element or a whole row. An array of owners SHALL be filled only with
None.

#### Scenario: Fill of every element type
- **WHEN** a program fills arrays of Integer, Int8, UInt64, Float, Bool, an enum, a Range, a Set, String, a struct, by element and by row in two dimensions, in State, an assignment, an argument and a result, with a value whose evaluation counts calls
- **THEN** every element holds the value, changing one changes no other, and the value was evaluated once (tests/runtime/array-fill.inox)

#### Scenario: Fill into a Range
- **WHEN** `Fill(I)` with I = 13 initializes an `Array[1..3] TMonth` where `TMonth Range 1..12`
- **THEN** the program stops with value out of range (tests/runtime/array-fill-range.inox)

#### Scenario: Values that do not fit
- **WHEN** a program writes `Fill(1.5)` for an Integer array, or `Fill(One)` for an array of owners
- **THEN** the compiler rejects both (tests/diagnostics/array-fill-type.inox, tests/diagnostics/array-fill-owner.inox)

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
An array element type SHALL be allowed to be a struct, filled with a struct
value (`Fill(Origin)`). `A[I].Field` SHALL read and write one field of one
element, with the index checked as for any array.

#### Scenario: Element fields
- **WHEN** a program declares `A Array[1..3] TPoint := Fill(Zero)` with `Zero TPoint`, writes `A[2].FX := 5`, calls `A[1].Move(11)` and prints the fields, also of a 2-D array of structs
- **THEN** it prints the defaults and the new values (tests/runtime/array-of-struct.inox)

#### Scenario: Element index out of bounds
- **WHEN** `A[I].FX := 1` runs with I = 4 on `Array[1..3] TPoint`
- **THEN** the program stops with array index out of bounds (tests/runtime/array-of-struct-out-of-bounds.inox)

### Requirement: Arrays as struct fields
A struct field SHALL be allowed to have an array type. It SHALL have no
implicit value (r3.82): its default is `Fill(Literal)`, a literal of its
innermost element type (`None` for owners, `[]` for sets), or it is given in
the construction. An array of structs has no literal, so it is always given. `S.F[I]` SHALL read and write
one element with the index checked; copying the struct SHALL copy the array.
An array field SHALL NOT have a literal default.

#### Scenario: Field elements and copy
- **WHEN** a program writes `B.FData[2] := 7`, copies B into C and changes `C.FData[2]`
- **THEN** `B.FData[2]` is still 7 (tests/runtime/struct-array-field.inox)

#### Scenario: Fill defaults of every element type
- **WHEN** array fields default to `Fill` of an enum literal, a Bool, a Float, a String, an Int8, an Integer in two dimensions, None and `[]`, in a local, in State and by `New`
- **THEN** every element holds its literal (tests/runtime/struct-required-fields.inox)

#### Scenario: Default on an array field
- **WHEN** a struct declares `FData Array[1..2] Integer := 1`, or `FData Array[1..4] Byte` and is declared without a value
- **THEN** the compiler rejects both, showing `:= Fill(Value)` (tests/diagnostics/struct-array-field-default.inox, tests/diagnostics/struct-array-field-required.inox)

#### Scenario: Field index out of bounds
- **WHEN** `B.FData[I]` runs with I = 5 on `Array[1..4] Byte`
- **THEN** the program stops with array index out of bounds (tests/runtime/struct-array-field-out-of-bounds.inox)
