## ADDED Requirements

### Requirement: Enum and Range coverage
A `case` on an Enum or Range selector SHALL compile without `otherwise` exactly
when its choices cover every value of the type. A missing value SHALL be a
compile error that names it.

#### Scenario: Complete enum case
- **WHEN** a case on a TSuit names Club, Diamond, Heart and Spade without otherwise
- **THEN** it compiles and runs (test to add: tests/runtime/enum-case.inox)

#### Scenario: Incomplete enum case
- **WHEN** a case on a TSuit omits Spade and has no otherwise
- **THEN** the compiler rejects it naming Spade (test to add: tests/diagnostics/enum-case-incomplete.inox)
