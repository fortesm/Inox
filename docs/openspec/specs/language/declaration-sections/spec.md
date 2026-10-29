# declaration-sections Specification

## Purpose
The `Type`, `Const` and `State` declaration sections (CANON-5): which entries
each section holds and how malformed entries are reported. Part of the Inox
law, detailing the constitution (`docs/openspec/specs/constitution`). There is no
type alias form `TName := Type` (Marcelo Fortes, 2026-10-12: "TNome := Integer
deve ser erro").

## Requirements

### Requirement: Section entries
A `Type` section SHALL hold Struct, Range and Enum declarations; a `Const`
section or line SHALL hold `Name := Expression`; a `State :` section SHALL hold
variable declarations closed by `;`. A field or State type SHALL be any type
the language accepts in a local declaration, including `Array[...] T`.

#### Scenario: Multi-token field types
- **WHEN** a struct declares `FData Array[1..4] Byte`
- **THEN** the program compiles (tests/runtime/struct-array-field.inox)

#### Scenario: Every Type form
- **WHEN** a Type section declares a Struct, two Ranges, a short Enum and a multi-line Enum
- **THEN** the program compiles (tests/semantic/valid/type-section-forms.inox)

#### Scenario: Existing section programs
- **WHEN** the existing State and Const tests run
- **THEN** their results are unchanged (tests/runtime/state-lowering.inox)

#### Scenario: Not a Type form
- **WHEN** a Type section contains `TCount := Integer`, or is written `Type :`
- **THEN** the compiler rejects it naming the canonical forms (tests/diagnostics/type-assignment-form.inox, tests/parser/invalid/type-colon.inox)

### Requirement: Section diagnostics name the line
A malformed section entry SHALL produce a diagnostic that names the entry and
its source line.

#### Scenario: Field without a type
- **WHEN** a struct field is written `FX` with no type
- **THEN** the compiler reports the field and its line (tests/diagnostics/struct-field-without-type.inox)

#### Scenario: Malformed defaults, ranges and enums
- **WHEN** a field default is an expression, a Range has no `Low..High`, or a multi-line Enum puts two literals on one line
- **THEN** the compiler reports the line (tests/diagnostics/struct-field-default-expression.inox, tests/diagnostics/range-without-bounds.inox, tests/diagnostics/enum-literals-per-line.inox)

### Requirement: Slices are not stored in sections
A struct field and a State variable SHALL NOT have a slice type (ADR-0017).

#### Scenario: Slice field and slice State
- **WHEN** a struct declares `FItems Slice<Integer>` or State declares `View Slice<Integer>`
- **THEN** the compiler rejects it (tests/diagnostics/struct-field-slice.inox, tests/diagnostics/state-slice.inox)

### Requirement: One State declaration per line
A State section SHALL declare one name per line; a grouped declaration SHALL be
reported as not supported yet.

#### Scenario: Grouped State declaration
- **WHEN** State contains `A, B Integer := 1`
- **THEN** the compiler reports that grouped declarations in State are not supported yet (tests/diagnostics/state-grouped.inox)
