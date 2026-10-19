## MODIFIED Requirements

### Requirement: Fields without an implicit value are required
A field of an enum, Char, array or type-parameter type without a default, and a
field of a struct without complete type defaults, SHALL be given in every
construction; a struct with such a field (directly, nested or in a part)
SHALL NOT take its type defaults, in a local, in State or by `New(TName)`
(r3.82).

#### Scenario: Declared without a value
- **WHEN** `Q TPacket` with `FData Array[1..4] Byte`, `C TCard` with `FSuit TSuit`, `R TRoute` whose `FFrame TFrame` has such a field, a part with one, a State variable, or `New(TPacket)`
- **THEN** the compiler rejects each, naming the field (tests/diagnostics/struct-array-field-required.inox, tests/diagnostics/enum-field-without-default.inox, tests/diagnostics/char-field-without-default.inox, tests/diagnostics/nested-required-field.inox, tests/diagnostics/part-required-field.inox, tests/diagnostics/state-required-field.inox, tests/diagnostics/new-required-field.inox)

#### Scenario: Omitted in the construction
- **WHEN** a construction omits such a field
- **THEN** the compiler rejects it (tests/diagnostics/struct-array-field-omitted.inox, tests/diagnostics/enum-field-omitted.inox, tests/diagnostics/nested-field-omitted.inox)

#### Scenario: Given or defaulted
- **WHEN** they have defaults (`:= Approved`, `:= Fill(7)`) or are given in the construction, also nested
- **THEN** the values hold them (tests/runtime/required-fields.inox, tests/runtime/struct-required-fields.inox)
