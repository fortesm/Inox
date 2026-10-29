# natural Specification

## Purpose
`Natural`, the non-negative subtype of Integer (ADR-0014, ADR-0015). Part of
the Inox law, detailing the constitution (`docs/openspec/specs/constitution`).

## Requirements

### Requirement: Natural is the non-negative subtype of Integer
Natural SHALL hold 0..Int64.Max with the representation of Integer and SHALL
NOT be UInt64. A Natural SHALL be usable wherever an Integer is. Arithmetic on
Natural SHALL be done in the Integer family and yield Integer.

#### Scenario: Natural values and Integer arithmetic
- **WHEN** a program mixes Natural and Integer values
- **THEN** it compiles and prints the expected values (tests/runtime/natural-values.inox)

### Requirement: Storing into a Natural checks the range implicitly
Storing an Integer into a Natural (initialization, assignment, compound
assignment, argument, return, field, State) SHALL check 0..Int64.Max at run time
without a written conversion, stopping with "value out of range for Natural"
when negative. A negative constant SHALL be a compile-time error.

#### Scenario: Implicit check and N += 1
- **WHEN** a program writes `N Natural := X` and `N += 1`
- **THEN** it compiles and runs (tests/runtime/natural-implicit-check.inox)

#### Scenario: Negative value at run time
- **WHEN** `N := N - 5` runs with N = 3
- **THEN** the program stops with value out of range for Natural (tests/runtime/natural-assignment-negative.inox, tests/runtime/state-natural-range.inox)

#### Scenario: Negative constant
- **WHEN** a program writes `N Natural := -1`
- **THEN** the compiler rejects it (tests/diagnostics/natural-negative-constant.inox)
