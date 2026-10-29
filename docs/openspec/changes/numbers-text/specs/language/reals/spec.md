## MODIFIED Requirements

### Requirement: Conversions take numbers
A conversion into a number SHALL take a number (an integer, a real, or for
an integer type a Char's code point) or a String (r3.90). A Bool or an enum
SHALL be rejected, with the way to write it (`IIf(B, 1, 0)`,
`Float(Ord(E))`); text is never an enum value.

#### Scenario: Values that are not numbers
- **WHEN** a program writes `Float(True)`, `Float(Heart)` or `TSuit("Club")`
- **THEN** the compiler rejects each (tests/diagnostics/conversion-of-bool.inox, tests/diagnostics/float-enum-conversion.inox, tests/diagnostics/text-to-enum.inox)

## ADDED Requirements

### Requirement: Numbers as text
`String(N)` and `String(F)` SHALL write the number as Put does (a real as
its shortest round-trip text); `Integer(S)`, every integer type, `Float(S)`
and `Float32(S)` SHALL read the whole String: an optional `-` and digits,
for a real an optional fraction and exponent, within the type. Other text
(spaces, `+`, `_`, a value outside) raises ConversionError (r3.90).

#### Scenario: Both ways
- **WHEN** a program converts numbers of several types to text and back, and invalid or out-of-range text
- **THEN** valid text gives the value, a Float survives the round trip, and the rest raises ConversionError (tests/runtime/number-text.inox)

#### Scenario: Printing
- **WHEN** a program prints Floats and Float32s
- **THEN** each prints its shortest round-trip text (tests/runtime/float-printing.inox)
