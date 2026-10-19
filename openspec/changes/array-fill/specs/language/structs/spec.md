## ADDED Requirements

### Requirement: Fields without an implicit value are required
A field of an enum, array or type-parameter type without a default, and a
field of a struct without complete type defaults, SHALL be given in every
construction; a struct with such a field (directly, nested or in a part)
SHALL NOT take its type defaults, in a local, in State or by `New(TName)`
(r3.82).

#### Scenario: Declared without a value
- **WHEN** `Q TPacket` with `FData Array[1..4] Byte`, `C TCard` with `FSuit TSuit`, `R TRoute` whose `FFrame TFrame` has such a field, a part with one, a State variable, or `New(TPacket)`
- **THEN** the compiler rejects each, naming the field (tests/diagnostics/struct-array-field-required.inox, tests/diagnostics/enum-field-without-default.inox, tests/diagnostics/nested-required-field.inox, tests/diagnostics/part-required-field.inox, tests/diagnostics/state-required-field.inox, tests/diagnostics/new-required-field.inox)

#### Scenario: Omitted in the construction
- **WHEN** a construction omits such a field
- **THEN** the compiler rejects it (tests/diagnostics/struct-array-field-omitted.inox, tests/diagnostics/enum-field-omitted.inox, tests/diagnostics/nested-field-omitted.inox)

#### Scenario: Given or defaulted
- **WHEN** they have defaults (`:= Approved`, `:= Fill(7)`) or are given in the construction, also nested
- **THEN** the values hold them (tests/runtime/required-fields.inox, tests/runtime/struct-required-fields.inox)

## MODIFIED Requirements

### Requirement: Type-default initialization
A struct variable declared without `:=` SHALL start with each field at its
declared default, or at its type's zero when it is a scalar, String, Set or
pointer without one. A nested struct field SHALL get its own type defaults.
A struct whose type defaults are not complete (CANON-5, r3.82) SHALL NOT be
declared without `:=`.

#### Scenario: Defaults and zeros
- **WHEN** `P TPoint` is declared where `FX Integer := 1` and `FY Integer` has no default
- **THEN** `P.FX` is 1 and `P.FY` is 0 (tests/runtime/struct-defaults.inox)

#### Scenario: Defaults of every scalar type
- **WHEN** a struct has Integer, Int8, Byte, Natural, Float, Float32 and Bool fields with literal defaults, negative and hexadecimal ones included
- **THEN** a type-default variable holds those defaults (tests/runtime/struct-defaults.inox)

#### Scenario: Defaults that do not fit
- **WHEN** a default is `Byte := 300`, `Natural := -1` or `Float := 1`
- **THEN** the compiler rejects it (tests/diagnostics/struct-default-out-of-range.inox, tests/diagnostics/struct-default-negative-natural.inox, tests/diagnostics/struct-float-default-integer.inox)
