# ref-parameters Specification

## Purpose
`ref X T` and `ref mut X T` parameters (ADR-0016).

## Requirements

### Requirement: References to the caller's storage
`ref X T` SHALL give the routine read-only access to the caller's value without
copying it; `ref mut X T` SHALL let the routine write the caller's variable,
field or array element. Writes SHALL keep the type's checks.

#### Scenario: Read-only array, swap, field and element
- **WHEN** a program sums an array by ref and swaps variables, a field and an element by ref mut
- **THEN** the caller sees the updated values (tests/runtime/ref-parameters.inox)

#### Scenario: Range check through a reference
- **WHEN** a ref mut Byte holding 255 is incremented
- **THEN** the program stops with integer overflow (tests/runtime/ref-mut-range.inox)

#### Scenario: A ref parameter is read-only
- **WHEN** a routine assigns its `ref` parameter
- **THEN** the compiler rejects it (tests/diagnostics/ref-readonly-assignment.inox)

### Requirement: Reference arguments are exact and writable
A reference argument SHALL have exactly the parameter's type. A `ref mut`
argument SHALL be a writable place: a mutable local, a State variable, a
`ref mut` parameter, or a field or element of one.

#### Scenario: Wrong type, a constant, a value parameter
- **WHEN** an Integer is passed to `ref mut B Byte`, a literal to `ref mut`, or a value parameter to `ref mut`
- **THEN** the compiler rejects each call (tests/diagnostics/ref-type-exact.inox, tests/diagnostics/ref-mut-not-writable.inox, tests/diagnostics/ref-mut-from-value-parameter.inox)

### Requirement: Exclusivity of ref mut
The variable a `ref mut` argument is rooted at SHALL appear in no other argument
of the same call, so a routine never sees one variable through two names
(`Swap(X, X)`, `Swap(A[1], A[2])` and `Swap(P.X, P.Y)` are errors). References
SHALL exist only during the call. (Confirmed by Marcelo Fortes, 2026-10-12.)

#### Scenario: The same variable twice
- **WHEN** a program calls `Swap(X, X)`
- **THEN** the compiler rejects it (tests/diagnostics/ref-mut-aliasing.inox)
