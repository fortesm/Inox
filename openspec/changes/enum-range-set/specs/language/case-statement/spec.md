## ADDED Requirements

### Requirement: Enum and Range coverage
A `case` on an Enum or Range selector SHALL compile without `else` exactly
when its choices cover every value of the type. A missing value SHALL be a
compile error that names it.

#### Scenario: Complete enum case
- **WHEN** a case on a TSuit names Club, Diamond, Heart and Spade without else
- **THEN** it compiles and runs (tests/runtime/enum-case.inox)

#### Scenario: Incomplete enum case
- **WHEN** a case on a TSuit omits Spade and has no else
- **THEN** the compiler rejects it naming Spade (tests/diagnostics/enum-case-incomplete.inox); a Range case missing 7..12 is rejected too (tests/diagnostics/range-case-incomplete.inox)
