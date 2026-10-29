# slices Specification

## Purpose
Read-only slices `A[L..H]` of type `Slice<T>` (ADR-0017). Part of the Inox
law, detailing the constitution (`docs/openspec/specs/constitution`).

## Requirements

### Requirement: A slice is a read-only view that keeps the array's indices
`A[L..H]`, `A[..H]`, `A[L..]` and `A[..]` SHALL give a view of elements L..H of
an array or slice without copying; an omitted bound is the base's own. The view
SHALL keep the base's indices, and `Low`, `High` and `Length` SHALL give its
bounds. Elements of a slice SHALL NOT be assigned. (Confirmed by Marcelo
Fortes, 2026-10-12.)

#### Scenario: Forms, bounds, nesting and later writes
- **WHEN** a program sums several slices, slices a slice, writes the array afterwards and slices Byte arrays and matrix rows
- **THEN** it prints the expected values (tests/runtime/slices.inox)

#### Scenario: Writing through a slice
- **WHEN** a program assigns an element of a slice
- **THEN** the compiler rejects it (tests/diagnostics/slice-readonly.inox)

### Requirement: Slice bounds are checked and a slice is never empty
Low(base) <= L <= H <= High(base) SHALL hold. Constant bounds outside it SHALL
be compile-time errors and run-time bounds outside it SHALL stop the program
with "slice bounds out of range". An index outside the slice SHALL stop the
program with "array index out of bounds".

#### Scenario: Bounds outside the array
- **WHEN** a slice is taken with L = 0 on `Array[1..3]`, with L > H, or with a run-time H past the end
- **THEN** the first two are rejected and the third stops the program (tests/diagnostics/slice-constant-bounds.inox, tests/diagnostics/slice-empty.inox, tests/runtime/slice-bounds-runtime.inox)

#### Scenario: Index outside the slice
- **WHEN** `S[4]` runs on `A[2..3]` of a 5-element array
- **THEN** the program stops with array index out of bounds (tests/runtime/slice-index-out-of-bounds.inox)

### Requirement: A slice never outlives its array
Only a named array, parameter, or a field or element of one SHALL be sliced; a
slice variable SHALL be bound once; a slice SHALL NOT be returned; a slice
SHALL be passed by value, never by ref.

#### Scenario: Escapes are rejected
- **WHEN** a program slices a function result, reassigns a slice, returns a slice or declares a `ref` slice parameter
- **THEN** the compiler rejects it (tests/diagnostics/slice-of-temporary.inox, tests/diagnostics/slice-reassign.inox, tests/diagnostics/slice-return.inox, tests/diagnostics/slice-by-ref.inox)
