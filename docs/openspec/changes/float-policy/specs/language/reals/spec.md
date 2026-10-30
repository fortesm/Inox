## ADDED Requirements

### Requirement: A typed real never changes precision implicitly
A typed real value (a variable, a result, a computed Const) SHALL never
change precision implicitly: Float32 and Float64 never mix without a written
conversion, nor an integer and a real.

#### Scenario: Literals and mixes that are errors
- **WHEN** a program writes a Float32 literal beyond its range, `Ratio Float := Count` with an Integer Count, `Small + Large` of a Float32 and a Float, a Float64 variable or a computed Const where a Float32 is expected
- **THEN** the compiler rejects each (tests/diagnostics/float32-constant-range.inox, tests/diagnostics/float-implicit-integer.inox, tests/diagnostics/float32-float64-mix.inox, tests/diagnostics/float32-from-float64-variable.inox, tests/diagnostics/float32-typed-const.inox)

### Requirement: IEEE special values and Float32 precision
NaN and the infinities SHALL keep their kind through a real conversion; a
value too small for Float32 SHALL become a subnormal or a zero of its sign,
without error; Float32 expressions SHALL be computed in Float32, never
widened (r3.86).

#### Scenario: Special values and underflow
- **WHEN** `Float32(F)` runs with F an infinity, NaN, or 10^-51 and its negation
- **THEN** it gives the infinity, NaN and zeros of each sign (tests/runtime/float-conversions.inox)

#### Scenario: Float32 overflow
- **WHEN** `Float32(F)` runs with F = 3.0 times 10^39
- **THEN** the program stops with value out of range (tests/runtime/float32-overflow.inox)

### Requirement: Universal real Consts
A Const whose value is a real literal, or its negation, SHALL be universal,
as Ada's named numbers and Go's untyped constants: a Float where a Float is
expected and a Float32 where a Float32 is, rounded once from its digits
(r3.86). Std.Math's `Pi`, `Tau` and `E` are universal Consts written with
more digits than either type holds.

#### Scenario: Universal Consts
- **WHEN** a program writes `Pi32 Float32 := Pi` and `Low32 Float32 := Low` with `Const Low := -2.5`, and uses Pi as a Float
- **THEN** Pi32 is the Float32 nearest to pi, distinct from the Float pi, and Low32 is -2.5 (tests/runtime/float-conversions.inox)

## MODIFIED Requirements

### Requirement: Real literals take their context
A real literal (`1.5`, `1_000.25`) SHALL be a Float (Float64), and a Float32
where a Float32 is expected: an initializer, an assigned value, an argument,
a returned value, an operand next to a Float32. A Float32 literal SHALL be
rounded once, from its decimal text, never through a Float64 (r3.86); beyond
Float32's range it SHALL be a compile error.

#### Scenario: Float32 literals
- **WHEN** a program declares `Small Float32 := 0.1`, computes `Small * 2.0 < 0.3`, and declares `Tie Float32 := 1.000000059604644775390626`, just above a tie between two Float32 values
- **THEN** the literals are Float32, and Tie is above 1 (rounded once; through a Float64 it would be 1) (tests/runtime/float-conversions.inox)

### Requirement: Reals between widths
`Float(G)` of a Float32 SHALL widen exactly; `Float32(F)` SHALL round to the
nearest Float32 (IEEE 754, ties to even), a loss of precision being no error.
A finite F whose rounding overflows SHALL stop the program with "value out of
range", never becoming an infinity or saturating (r3.86); a constant beyond
it is a compile error.

#### Scenario: Rounding and widening
- **WHEN** `Wide := Float(Small)` with `Small Float32 := 0.1`
- **THEN** `Wide = 0.1` is false and `Float32(Wide) = Small` is true (tests/runtime/float-conversions.inox)

### Requirement: Real Consts
A `Const` SHALL hold a Float or a Float32 given by a constant real
expression: literals, other real Consts, unary minus, `+ - * /` and
`Float`/`Float32` conversions of constants, folded at compile time (a
Float32 one rounded at each step); like every Const it may follow the
routine that uses it.

#### Scenario: Consts
- **WHEN** a program declares `Const Half := 1.0 / 2.0`, `Const Shift := -Half * 4.0 + Rate`, `Const Third := Float32(1.0 / 3.0)`, `Const Seven := Float(7)` and a Const after its use
- **THEN** each has its value (tests/runtime/float-conversions.inox)
