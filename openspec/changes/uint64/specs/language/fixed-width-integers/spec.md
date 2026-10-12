## ADDED Requirements

### Requirement: UInt64 covers 0 to 2^64-1
`UInt64` (alias `UInteger`) SHALL hold every value 0..18446744073709551615 and
SHALL work in every position other integer types do. Comparisons, `div`, `mod`
and `shr` SHALL use unsigned meaning.

#### Scenario: Values above Int64.Max
- **WHEN** a UInt64 starts at 9223372036854775807, is incremented, compared and printed
- **THEN** it prints 9223372036854775808 and compares greater than its start (test to add: tests/runtime/uint64.inox)

#### Scenario: Unsigned div, mod and shift
- **WHEN** a UInt64 above Int64.Max is divided, reduced with mod and shifted right
- **THEN** the results are the unsigned ones (test to add: tests/runtime/uint64.inox)

### Requirement: UInt64 never wraps
A UInt64 arithmetic result outside 0..2^64-1 SHALL stop the program with
"integer overflow".

#### Scenario: Overflow at the top and below zero
- **WHEN** a UInt64 holding the maximum is incremented, or one holding 0 is decremented
- **THEN** the program stops with integer overflow (test to add: tests/runtime/uint64-overflow.inox, tests/runtime/uint64-underflow.inox)

### Requirement: UInt64 conversions are checked
`UInt64(X)` from a negative value and `Integer(U)` or `Natural(U)` from a value
above Int64.Max SHALL stop the program with "integer conversion out of range";
a constant outside the target SHALL be a compile-time error. An Integer
variable SHALL NOT mix with a UInt64 without a conversion.

#### Scenario: Run-time conversions
- **WHEN** `UInt64(X)` runs with X = -1, or `Integer(U)` with U above Int64.Max
- **THEN** the program stops with integer conversion out of range (test to add: tests/runtime/uint64-conversion-range.inox)

#### Scenario: Negative constant and mixed operands
- **WHEN** a program writes `U UInt64 := -1` or `U + I` with an Integer variable I
- **THEN** the compiler rejects it (test to add: tests/diagnostics/uint64-negative-literal.inox, tests/diagnostics/uint64-mixed-integer.inox)

### Requirement: UInt64 literals and negation
In a UInt64 context only, an integer literal (decimal or hexadecimal) SHALL be
allowed up to 18446744073709551615; elsewhere a literal SHALL still fit Int64.
Unary minus on a UInt64 SHALL be a compile error.

#### Scenario: The largest UInt64 literal
- **WHEN** a program writes `U UInt64 := 18446744073709551615` and `H UInt64 := $FFFFFFFFFFFFFFFF`, and prints both
- **THEN** both print 18446744073709551615 (test to add: tests/runtime/uint64.inox)

#### Scenario: Large literal outside a UInt64 context, and negation
- **WHEN** a program writes `X := 18446744073709551615` or `-U` with U a UInt64
- **THEN** the compiler rejects it (test to add: tests/diagnostics/uint64-literal-needs-context.inox, tests/diagnostics/uint64-unary-minus.inox)
