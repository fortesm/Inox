## MODIFIED Requirements

### Requirement: Enums in aggregates
An enum struct field SHALL have a literal default or be given in the
construction (r3.82), and an array of an enum SHALL be filled (`Fill(Club)`),
as an enum variable needs an initializer (CANON-5, r3.81): the first literal
is never an implicit value.

#### Scenario: Field default and element
- **WHEN** a struct field `FSuit TSuit := Heart` and `Hand Array[1..3] TSuit := Fill(Club)` are declared
- **THEN** the field holds Heart and the element Club (tests/runtime/enums.inox)

#### Scenario: Field without default and array without Fill
- **WHEN** a struct with `FSuit TSuit` is declared without a value or built without FSuit, or a program declares `Hand Array[1..3] TSuit`
- **THEN** the compiler rejects each (tests/diagnostics/enum-field-without-default.inox, tests/diagnostics/enum-field-omitted.inox, tests/diagnostics/enum-array-without-initializer.inox)
