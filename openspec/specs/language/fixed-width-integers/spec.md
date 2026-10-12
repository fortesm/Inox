# fixed-width-integers Specification

## Purpose
Fixed-width integer types of CANON-8 (Int8, Int16, Int32, Byte = UInt8,
UInt16, UInt32) and `bitnot`. UInt64/UInteger is specified by the canon and not
yet implemented. Mirror of the canon (G7): the canon stays the law until Marcelo Fortes
approves this capability's migration.

## Requirements

### Requirement: Fixed-width values never wrap
Every arithmetic result of a fixed-width type SHALL stay in the type's range;
a result outside it SHALL stop the program with "Inox runtime error: integer
overflow". An unsigned result below zero is an overflow, never a large value.

#### Scenario: Byte overflow
- **WHEN** a Byte holding 255 is incremented
- **THEN** the program stops with integer overflow (tests/runtime/byte-overflow.inox)

#### Scenario: Unsigned underflow and Int8 negation
- **WHEN** a UInt8 holding 3 is decreased by 4, or an Int8 holding -128 is negated
- **THEN** the program stops with integer overflow (tests/runtime/unsigned-underflow.inox, tests/runtime/int8-negate-min.inox)

### Requirement: A constant is read in the fixed-width type when it fits
A constant Integer expression SHALL be read in a fixed-width type when its
value fits — as initializer, assigned value, argument, return or operand next
to a value of that type — and SHALL be a compile-time range error otherwise.
An Integer variable SHALL never narrow implicitly. (Confirmed by Marcelo
Fortes, 2026-10-12.)

#### Scenario: Constants that fit and constants that do not
- **WHEN** a program writes `B Byte := 200`, `B += 55`, then `B Byte := 256` or `B + 300`
- **THEN** the first two compile; the others are rejected (tests/runtime/fixed-width-integers.inox, tests/diagnostics/byte-literal-out-of-range.inox, tests/diagnostics/byte-operand-out-of-range.inox)

#### Scenario: No implicit narrowing of a variable
- **WHEN** an Integer variable initializes a Byte
- **THEN** the compiler rejects it (tests/diagnostics/byte-from-integer-variable.inox)

### Requirement: Conversions and shifts are checked
`T(X)` to a fixed-width type SHALL reject a constant outside T at compile time
and SHALL stop the program with "integer conversion out of range" for a
run-time value outside T. A shift count SHALL be 0..width-1.

#### Scenario: Run-time conversion out of range
- **WHEN** `Byte(X)` runs with X = 300
- **THEN** the program stops with integer conversion out of range (tests/runtime/byte-conversion-runtime-range.inox)

#### Scenario: Constant conversion out of range
- **WHEN** a program writes `Byte(300)` or `Int8(-129)`
- **THEN** the compiler rejects it (tests/diagnostics/byte-conversion-out-of-range.inox, tests/diagnostics/int8-conversion-out-of-range.inox)

#### Scenario: Shift count outside the width
- **WHEN** a Byte is shifted by 8
- **THEN** the program stops with invalid shift count (tests/runtime/byte-shift-count.inox)

### Requirement: Fixed-width values work everywhere a value does
Fixed-width values SHALL work in locals, parameters, results, struct fields,
State, case selectors, comparisons and printing; `bitnot` SHALL be the
complement within the type's width.

#### Scenario: All positions
- **WHEN** the fixed-width program of the suite runs
- **THEN** it prints the expected values (tests/runtime/fixed-width-integers.inox)
