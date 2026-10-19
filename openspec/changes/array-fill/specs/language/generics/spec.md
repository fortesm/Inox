## ADDED Requirements

### Requirement: A field of a type parameter has no implicit value
A field of a generic struct whose type uses a type parameter (`FValue T`,
`FItems Array[1..3] T`) SHALL have no default and SHALL be given in every
construction, whatever the instance (r3.82): `D TBox<Integer>` and
`B TBox<TStatus>` are errors, `TBox(FValue := 0)` is a value.

#### Scenario: Instances without a value
- **WHEN** a program declares `D TBox<Integer>` or `B TBox<TStatus>`, omits FValue in a construction, or writes `FValue T := 0`
- **THEN** the compiler rejects each (tests/diagnostics/generic-field-required.inox, tests/diagnostics/generic-field-required-enum.inox, tests/diagnostics/generic-field-omitted.inox, tests/diagnostics/generic-field-default.inox)

#### Scenario: Instances built
- **WHEN** a program builds `TBox(FValue := Pending)`, `TBox(FValue := 0)` and `TBox(FItems := Fill(""))`
- **THEN** each holds its values (tests/runtime/required-fields.inox, tests/runtime/generics-structs.inox)

