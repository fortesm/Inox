## Purpose
The `Type`, `Const` and `State` declaration sections (CANON-5): which entries
each section holds and how malformed entries are reported.

## ADDED Requirements

### Requirement: Section entries
A `Type` section SHALL hold Struct, Range and Enum declarations; a `Const`
section or line SHALL hold `Name := Expression`; a `State :` section SHALL hold
variable declarations closed by `;`. A field or State type SHALL be any type
the language accepts in a local declaration, including `Array[...] T`.

#### Scenario: Multi-token field types
- **WHEN** a struct declares `FData Array[1..4] Byte`
- **THEN** the program compiles (test to add: tests/runtime/struct-array-field.inox)

#### Scenario: Existing section programs
- **WHEN** the existing State and Const tests run
- **THEN** their results are unchanged (tests/runtime/state-lowering.inox)

### Requirement: Section diagnostics name the line
A malformed section entry SHALL produce a diagnostic that names the entry and
its source line.

#### Scenario: Field without a type
- **WHEN** a struct field is written `FX` with no type on line 4
- **THEN** the compiler reports the field and line 4 (test to add: tests/diagnostics/struct-field-without-type.inox)
