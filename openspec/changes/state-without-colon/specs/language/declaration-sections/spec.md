## MODIFIED Requirements

### Requirement: Section entries
A `Type` section SHALL hold Struct, Range and Enum declarations; a `Const`
line SHALL hold `Name := Expression`; a `State` section SHALL hold variable
declarations, one per line. `Type` and `State` are sections, not blocks: no
`:` after the keyword and no closing `;`; each ends where the next module item
starts. A field or State type SHALL be any type the language accepts in a
local declaration, including `Array[...] T`.

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

#### Scenario: Not a State or Const form
- **WHEN** a State section is written `State :` or closed with `;`, or a State or Const entry is written `Name : Type`
- **THEN** the compiler rejects it naming the right form (tests/diagnostics/state-with-colon.inox, tests/diagnostics/state-closing-semicolon.inox, tests/diagnostics/state-legacy-colon.inox, tests/diagnostics/const-legacy-colon.inox)
