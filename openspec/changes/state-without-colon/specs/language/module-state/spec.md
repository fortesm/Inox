## MODIFIED Requirements

### Requirement: State variables are module-level mutable storage
A `State` section SHALL declare variables that every routine of the module
can read and write. The section is written `State`, its variables on the
next lines, and `;` (no `:`); a module may have several.
Scalar State variables SHALL require an initializer, and initializers SHALL
run in source order before Main (an initializer may call a function or read
an earlier State variable).

#### Scenario: Read, write and initialize
- **WHEN** routines increment and read State variables initialized by a call and by an earlier State
- **THEN** the program prints the expected values (tests/runtime/state-lowering.inox)

#### Scenario: Where a section ends
- **WHEN** several State sections stand between a Const, a Type section, a function, a method and Main
- **THEN** every variable is visible to every routine and the program prints the expected values (tests/runtime/state-section-boundaries.inox)

#### Scenario: Scalar without initializer
- **WHEN** a scalar State variable has no initializer
- **THEN** the compiler rejects it (tests/diagnostics/state-scalar-without-initializer.inox)

#### Scenario: Range checks apply
- **WHEN** a negative value is stored into a Natural State variable
- **THEN** the program stops with value out of range for Natural (tests/runtime/state-natural-range.inox)
