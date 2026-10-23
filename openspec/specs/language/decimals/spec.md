# language/decimals Specification

## Purpose
Currency (an Int64 scaled by 10^6) and Crypto (an Int128 scaled by 10^18),
the exact decimals of Inox: literals, arithmetic, rounding, conversions,
text. Part of the Inox law, detailing the constitution (CANON-8).

## Requirements

### Requirement: Exact literals
A literal where a Currency or a Crypto is expected (an initializer, an
assignment, an argument, a field default, a Const conversion, an operand
next to a decimal) SHALL be read exactly in that type: more places than it
holds (6 or 18) or a value beyond it is a compile error.

#### Scenario: Literals
- **WHEN** a program writes `19.99`, `3 * Price`, `Crypto(0.000000000000000001)`, `19.999_999_9` and `10_000_000_000_000` as Currency
- **THEN** the first three are exact (tests/runtime/decimals.inox) and the others are rejected (tests/diagnostics/decimal-too-many-places.inox, tests/diagnostics/decimal-literal-out-of-range.inox)

### Requirement: Rounding half to even
`*` and `/` SHALL compute the exact result and round it once to the type's
places, a half going to the even neighbour (A1); so do `Round(X)` (Std.Math),
`Currency(Cr)` and `Currency(F)`.

#### Scenario: Halves
- **WHEN** 0.0000025, 0.0000035, 2.5 and 3.5 are rounded
- **THEN** they give 0.000002, 0.000004, 2 and 4 (tests/runtime/decimals.inox)

### Requirement: Decimals do not mix implicitly
An Integer, a Float or the other decimal type SHALL NOT meet a decimal in
an operator: the conversion is written (`Price * Currency(Count)`), except
a literal, which takes the decimal's type (A2). `^`, `div` and `mod` do not
apply.

#### Scenario: Mixing
- **WHEN** a Currency meets an Integer variable, a Float, a Crypto, `^` or `div`
- **THEN** each is rejected with the conversion to write (tests/diagnostics/decimal-mix-integer.inox, tests/diagnostics/decimal-mix-float.inox, tests/diagnostics/decimal-mix-crypto.inox, tests/diagnostics/decimal-power.inox, tests/diagnostics/decimal-div.inox)

### Requirement: Conversions
`Currency(X)` and `Crypto(X)` SHALL take an integer (exact), a Float (rounded
half to even), the other decimal (`Crypto(C)` exact, `Currency(Cr)` rounded,
A4) or a String (exact, else ConversionError); `Integer(C)` truncates
toward zero (A3), `Float(C)` is the nearest real. Beyond the target is
"integer conversion out of range".

#### Scenario: Both ways
- **WHEN** a program converts between decimals, integers, reals and text, and a Crypto beyond Currency
- **THEN** each value is as stated (tests/runtime/decimals.inox) and the last traps (tests/runtime/decimal-conversion-range.inox)

### Requirement: Where decimals live
Currency and Crypto SHALL live in locals, parameters, results, struct fields
(with a literal default), arrays, State, Consts (`Const Fee :=
Currency(2.5)`) and IIf, and be compared with `=`, `#`, `<`, `<=`, `>`, `>=`.

#### Scenario: Everywhere
- **WHEN** a program keeps prices in structs, arrays, State and Consts and passes them to routines
- **THEN** the values arrive exactly (tests/runtime/decimals.inox)

### Requirement: Output and input
A decimal SHALL be written with at least two decimals and no trailing zeros
after them (A5); `Get` reads one strictly. Overflow is "decimal overflow",
division by zero "division by zero".

#### Scenario: Text
- **WHEN** a program prints, writes to a file and reads decimals, overflows and divides by zero
- **THEN** the text is as stated (tests/runtime/decimals.inox, tests/runtime/decimals-input.inox) and the faults trap (tests/runtime/decimal-overflow.inox, tests/runtime/decimal-division-by-zero.inox)
