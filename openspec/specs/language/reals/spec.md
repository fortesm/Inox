# language/reals Specification

## Purpose
Real numbers (CANON-8 Float and Float32, change float-conversions): their
literals, Consts and conversions. Part of the Inox law, detailing the
constitution (`openspec/specs/constitution`).

## Requirements

### Requirement: Real literals take their context
A real literal (`1.5`, `1_000.25`) SHALL be a Float (Float64), and a Float32
where a Float32 is expected: an initializer, an assigned value, an argument,
a returned value, an operand next to a Float32. A literal beyond Float32's
range there SHALL be a compile error. Float32 and Float64 SHALL never mix
without a written conversion, nor an integer and a real.

#### Scenario: Float32 literals
- **WHEN** a program declares `Small Float32 := 0.1` and computes `Small * 2.0 < 0.3`
- **THEN** both literals are Float32 and the program prints the values (tests/runtime/float-conversions.inox)

#### Scenario: Literals and mixes that are errors
- **WHEN** a program writes a Float32 literal beyond its range, `Ratio Float := Count` with an Integer Count, or `Small + Large` of a Float32 and a Float
- **THEN** the compiler rejects each (tests/diagnostics/float32-constant-range.inox, tests/diagnostics/float-implicit-integer.inox, tests/diagnostics/float32-float64-mix.inox)

### Requirement: Integers into reals
`Float(I)` and `Float32(I)` SHALL round an integer to the nearest real; a
UInt64 is read unsigned.

#### Scenario: Converting integers
- **WHEN** a program computes `Float(Total) / Float(Count)` and `Float(Large)` of the largest UInt64
- **THEN** it gets the quotient and a positive real (tests/runtime/float-conversions.inox)

### Requirement: Reals between widths
`Float(G)` of a Float32 SHALL widen exactly; `Float32(F)` SHALL round to the
nearest Float32, and a finite F beyond Float32's range SHALL stop the program
with "value out of range" instead of becoming an infinity; a constant beyond
it is a compile error.

#### Scenario: Rounding and widening
- **WHEN** `Wide := Float(Small)` with `Small Float32 := 0.1`
- **THEN** `Wide = 0.1` is false and `Float32(Wide) = Small` is true (tests/runtime/float-conversions.inox)

#### Scenario: Float32 overflow
- **WHEN** `Float32(F)` runs with F = 3.0 times 10^39
- **THEN** the program stops with value out of range (tests/runtime/float32-overflow.inox)

### Requirement: Reals into integers truncate and are checked
`T(F)` of a real into an integer type SHALL truncate toward zero. NaN, an
infinity, or a value whose truncation is outside T SHALL stop the program
with "integer conversion out of range"; a constant outside T is a compile
error.

#### Scenario: Truncation
- **WHEN** a program converts `Integer(7.9)`, `Integer(-7.9)`, `Byte(255.9)`, `UInt64(1000.75)`, `Natural(3.2)` and `Int8(-128.5)`
- **THEN** it gets 7, -7, 255, 1000, 3 and -128 (tests/runtime/float-conversions.inox)

#### Scenario: Out of range at run time
- **WHEN** `Byte(F)` runs with F = 256.9, `Integer(F)` with F = NaN or 2^63
- **THEN** the program stops with integer conversion out of range (tests/runtime/float-to-integer-range.inox, tests/runtime/float-to-integer-nan.inox, tests/runtime/float-to-int64-range.inox)

#### Scenario: Constant out of range
- **WHEN** a program writes `Byte(300.0)`
- **THEN** the compiler rejects it (tests/diagnostics/float-conversion-constant.inox)

### Requirement: Conversions take numbers
A conversion into a number SHALL take a number: an integer, a real, or for
an integer type a Char's code point. A Bool, a String or an enum SHALL be
rejected, with the way to write it (`IIf(B, 1, 0)`, `Float(Ord(E))`).

#### Scenario: Values that are not numbers
- **WHEN** a program writes `Float(True)`, `Integer("7")` or `Float(Heart)`
- **THEN** the compiler rejects each (tests/diagnostics/conversion-of-bool.inox, tests/diagnostics/conversion-of-string.inox, tests/diagnostics/float-enum-conversion.inox)

### Requirement: Real Consts
A `Const` SHALL hold a Float or a Float32 given by a constant real
expression: literals, other real Consts, unary minus, `+ - * /` and
`Float`/`Float32` conversions of constants, folded at compile time (a
Float32 one rounded at each step); like every Const it may follow the
routine that uses it. Std.Math's `Pi`, `Tau` and `E` are Consts.

#### Scenario: Consts
- **WHEN** a program declares `Const Half := 1.0 / 2.0`, `Const Shift := -Half * 4.0 + Rate`, `Const Third := Float32(1.0 / 3.0)`, `Const Seven := Float(7)` and a Const after its use
- **THEN** each has its value (tests/runtime/float-conversions.inox)
