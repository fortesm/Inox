# module-state Specification

## Purpose
Module `State` variables (CANON-5 "Const and State"): explicit, visible global
mutable storage. Part of the Inox law, detailing the constitution
(`docs/openspec/specs/constitution`).

## Requirements

### Requirement: State variables are module-level mutable storage
A `State :` section SHALL declare variables that every routine of the module
can read and write. Scalar State variables SHALL require an initializer, and
initializers SHALL run in source order before Main (an initializer may call a
function or read an earlier State variable).

#### Scenario: Read, write and initialize
- **WHEN** routines increment and read State variables initialized by a call and by an earlier State
- **THEN** the program prints the expected values (tests/runtime/state-lowering.inox)

#### Scenario: Scalar without initializer
- **WHEN** a scalar State variable has no initializer
- **THEN** the compiler rejects it (tests/diagnostics/state-scalar-without-initializer.inox)

#### Scenario: Range checks apply
- **WHEN** a negative value is stored into a Natural State variable
- **THEN** the program stops with value out of range for Natural (tests/runtime/state-natural-range.inox)

### Requirement: State of struct and array type
A State variable SHALL be allowed to have a struct or array type. A struct
without an initializer SHALL start at its type defaults; an array SHALL have
one (`Fill(Value)`, r3.81). Initializers SHALL run in source order with the
scalar initializers.

#### Scenario: Struct and array State
- **WHEN** a module declares `Origin TPoint := TPoint(FX := 1, FY := 2)`, `Counts Array[1..3] Integer := Fill(0)` and `Grid Array[1..2, 1..2] TPoint := Fill(Home)` in State, and routines update them
- **THEN** every routine sees the updated values (tests/runtime/state-aggregates.inox)
