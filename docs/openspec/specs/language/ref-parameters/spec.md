# ref-parameters Specification

## Purpose
`ref X T` and `ref mut X T` parameters (ADR-0016). Part of the Inox law,
detailing the constitution (`docs/openspec/specs/constitution`).

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

### Requirement: A routine header is checked as it is written
A routine header SHALL be `Name(P, ...) R`, each parameter `X T`, `ref X T`
or `ref mut X T`, and a method's first parameter may be `Self` or
`Self mut` without a type. A mistake in a header SHALL be a parse error at
the token where it is found: a missing parameter name or type, a missing
`,` or `)`, a trailing comma, a reserved word as a parameter name, or more
than one result type.

#### Scenario: Header mistakes are parse errors with a position
- **WHEN** a header has a trailing comma, a reserved word as a parameter name, a parameter without a type, two result types, or `mut X T`
- **THEN** each is a parse error at its line and column (tests/diagnostics/routine-header-trailing-comma.inox, routine-header-reserved-word, routine-header-missing-type, routine-header-two-results, mut-parameter-reserved)

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

### Requirement: A ref parameter is read-only through its owners
What a `ref` parameter owns SHALL be read-only too: a field, an element or
an object reached from it through `Own<T>` or `Option<Own<T>>`, at any
depth, SHALL NOT be assigned, passed as `ref mut`, receive a `Self mut`
call, be written with `Value(P) :=`, be freed or moved, also inside
`with`; and `Link(...)` of such a place or owner SHALL be an error. A path
through a `Link<T>` is not read-only. A value parameter of an owner type
keeps its objects writable: it belongs to the routine.

#### Scenario: Writing what a ref parameter owns
- **WHEN** a routine with `ref H THolder` writes `H.FChild.FValue`, at one or two owner levels, by assignment, compound assignment, ref mut argument, Self mut call, Value, Free or with
- **THEN** each is rejected naming H and `ref mut` (tests/diagnostics/ref-owner-write.inox, ref-nested-owner-write, ref-owner-deep, ref-owner-compound, ref-owner-element, ref-owner-ref-mut-argument, ref-owner-self-mut-call, ref-owner-value-write, ref-owner-with, ref-owner-free-option, ref-owner-move-option, ref-option-parameter-free)

#### Scenario: A Link from a ref parameter
- **WHEN** a routine takes `Link(H.FChild)` with `ref H`, or `Link(P)` with `ref P Own<TNode>`
- **THEN** it is rejected (tests/diagnostics/link-from-ref-field.inox, link-from-ref-owner, link-from-ref-parameter)

#### Scenario: What stays writable
- **WHEN** a routine writes through owners of a `ref mut` parameter and of a value owner parameter, links the latter, and writes through a Link field of a `ref` parameter
- **THEN** each runs (tests/runtime/read-only-through-owners.inox)
