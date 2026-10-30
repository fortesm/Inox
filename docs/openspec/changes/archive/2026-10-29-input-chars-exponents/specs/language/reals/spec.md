## ADDED Requirements

### Requirement: Exponents
A real literal MAY end with an exponent after its fraction: `e` or `E`, an
optional sign and digits (`1.5e3`, `2.0E-7`, `6.022_140_76e23`). `1e3`,
without a fraction, and `1.5e`, without digits, SHALL be errors; a literal
beyond Float64 SHALL be a compile error, and beyond Float32 where a
Float32 is expected (r3.89).

#### Scenario: Literals with exponents
- **WHEN** a program writes reals, a Float32 and Consts with exponents
- **THEN** they have their values (tests/runtime/real-exponent.inox)

#### Scenario: Malformed or too large
- **WHEN** a program writes `1e3`, `1.5e`, `1.0e400`, a Const `9.9e999` or a Float32 `1.0e39`
- **THEN** each is rejected (tests/diagnostics/real-exponent-without-fraction.inox, tests/diagnostics/real-exponent-without-digits.inox, tests/diagnostics/real-literal-out-of-range.inox, tests/diagnostics/real-const-out-of-range.inox, tests/diagnostics/real-exponent-float32-range.inox)

### Requirement: Reading reals
`Get(F)` of a Float or a Float32 SHALL read a real number strictly (CANON-17):
an optional `-`, digits, an optional fraction and exponent, then white space
or the end; a Float32 is rounded once from the text.

#### Scenario: Input
- **WHEN** a program reads `-12.5`, `3`, `1.5e3` and a Float32 just above a tie
- **THEN** the values arrive and the Float32 is above 1 (tests/runtime/get-real.inox)
