# fixed-width-integers Specification

## Purpose
Fixed-width integer types of CANON-8 (Int8, Int16, Int32, Byte = UInt8,
UInt16, UInt32, UInt64 = UInteger) and `bitnot`. Part of the Inox law,
detailing the constitution (`openspec/specs/constitution`).

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

### Requirement: UInt64 covers 0 to 2^64-1
`UInt64` (alias `UInteger`) SHALL hold every value 0..18446744073709551615 and
SHALL work in every position other integer types do. Comparisons, `/`, `mod`
and `shr` SHALL use unsigned meaning.

#### Scenario: Values above Int64.Max
- **WHEN** a UInt64 starts at 9223372036854775807, is incremented, compared and printed
- **THEN** it prints 9223372036854775808 and compares greater than its start (tests/runtime/uint64.inox)

#### Scenario: Unsigned division, mod and shift
- **WHEN** a UInt64 above Int64.Max is divided, reduced with mod and shifted right
- **THEN** the results are the unsigned ones (tests/runtime/uint64.inox)

### Requirement: UInt64 never wraps
A UInt64 arithmetic result outside 0..2^64-1 SHALL stop the program with
"integer overflow".

#### Scenario: Overflow at the top and below zero
- **WHEN** a UInt64 holding the maximum is incremented, or one holding 0 is decremented
- **THEN** the program stops with integer overflow (tests/runtime/uint64-overflow.inox, tests/runtime/uint64-underflow.inox)

### Requirement: UInt64 conversions are checked
`UInt64(X)` from a negative value and `Integer(U)` or `Natural(U)` from a value
above Int64.Max SHALL stop the program with "integer conversion out of range";
a constant outside the target SHALL be a compile-time error. An Integer
variable SHALL NOT mix with a UInt64 without a conversion.

#### Scenario: Run-time conversions
- **WHEN** `UInt64(X)` runs with X = -1, or `Integer(U)` with U above Int64.Max
- **THEN** the program stops with integer conversion out of range (tests/runtime/uint64-from-negative.inox, tests/runtime/uint64-conversion-range.inox)

#### Scenario: Negative constant and mixed operands
- **WHEN** a program writes `U UInt64 := -1` or `U + I` with an Integer variable I
- **THEN** the compiler rejects it (tests/diagnostics/uint64-negative-literal.inox, tests/diagnostics/uint64-mixed-integer.inox)

### Requirement: UInt64 literals and negation
In a UInt64 context only, an integer literal (decimal or hexadecimal) SHALL be
allowed up to 18446744073709551615; elsewhere a literal SHALL still fit Int64.
Unary minus on a UInt64 SHALL be a compile error.

#### Scenario: The largest UInt64 literal
- **WHEN** a program writes `U UInt64 := 18446744073709551615` and `H UInt64 := $FFFFFFFFFFFFFFFF`, and prints both
- **THEN** both print 18446744073709551615 (tests/runtime/uint64.inox)

#### Scenario: Large literal outside a UInt64 context, and negation
- **WHEN** a program writes `X := 18446744073709551615` or `-U` with U a UInt64
- **THEN** the compiler rejects it (tests/diagnostics/uint64-literal-needs-context.inox, tests/diagnostics/uint64-unary-minus.inox)

### Requirement: UInt64 constants fold unsigned
A constant UInt64 expression SHALL be computed at compile time in the
unsigned domain, and a result below 0 or above 2^64-1 SHALL be a compile
error. A UInt64 Const SHALL be written as a conversion, `Const K :=
UInt64(18446744073709551615)`.

#### Scenario: Constant overflow and a UInt64 Const
- **WHEN** a program writes `U UInt64 := 18446744073709551615 + 1`, `U UInt64 := 3 - 5`, `Const Big := 18446744073709551615`, or uses `Const Big := UInt64(18446744073709551615)`
- **THEN** the first three are rejected (tests/diagnostics/uint64-constant-overflow.inox, tests/diagnostics/uint64-constant-underflow.inox, tests/diagnostics/uint64-const-literal.inox) and the last prints 18446744073709551615 (tests/runtime/uint64.inox)

### Requirement: UInt64 in case, for, input and files
A `case` on a UInt64 SHALL order its choices unsigned and SHALL need `else`;
a `for` over a range of UInt64 values SHALL have a UInt64 iterator and reach
2^64-1 without wrapping. `Get` SHALL read 0..18446744073709551615 into a
UInt64, and `PutLn` and `WriteLn` SHALL print it unsigned.

#### Scenario: case and for
- **WHEN** a case on 2^64-1 has the choices 0..9, 2^63..2^64-2 and 2^64-1, and loops run up to 2^64-1 and down from it with a step
- **THEN** the last choice runs and the loops run 6 and 4 times (tests/runtime/uint64.inox); a case without else is rejected (tests/diagnostics/uint64-case-without-else.inox)

#### Scenario: Input and files
- **WHEN** a program reads 18446744073709551615 and writes it to a file, or reads 18446744073709551616
- **THEN** it prints and reads back the value (tests/runtime/uint64-io.inox), and the second stops with invalid integer input (tests/runtime/uint64-input-range.inox)
