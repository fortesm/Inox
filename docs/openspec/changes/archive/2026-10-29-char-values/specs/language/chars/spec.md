## ADDED Requirements

### Requirement: Char literals
A Char literal SHALL be one Unicode scalar between single quotes, written in
UTF-8 or as an escape: `'\n'`, `'\r'`, `'\t'`, `'\0'`, `'\\'`, `'\''`. An
empty literal, two characters, any other escape and a surrogate
(U+D800..U+DFFF) SHALL be compile errors.

#### Scenario: Escapes and characters of every width
- **WHEN** a program prints `'\''`, `'\\'`, `'\t'`, `'é'` and `Char(128512)`
- **THEN** it prints the characters in UTF-8 (tests/runtime/chars.inox)

#### Scenario: Literals that are not one Char
- **WHEN** a program writes `'ab'`, `''`, `'\q'` or the UTF-8 bytes of a surrogate between quotes
- **THEN** the compiler rejects each (tests/diagnostics/char-literal-two.inox, tests/diagnostics/char-literal-empty.inox, tests/diagnostics/char-unknown-escape.inox, tests/diagnostics/char-surrogate-literal.inox)

### Requirement: Explicit conversions
`Char(I)` SHALL convert an integer code point, checked: outside 0..$10FFFF or
a surrogate, a constant is a compile error and any other value stops the
program with "integer conversion out of range". `Integer(C)` SHALL give the
code point. A Char and an integer SHALL never stand for each other without
these conversions; `Ord`, `Succ`, `Pred`, arithmetic and conversions to
other types SHALL be rejected.

#### Scenario: Converting both ways
- **WHEN** a program prints `Integer('é')`, `Char(65)` and `Char(Integer(C) + 1)`
- **THEN** it prints 233, A and the next character (tests/runtime/chars.inox)

#### Scenario: Out of range at run time
- **WHEN** `Char(I)` runs with I = 55296 or I = 1114112
- **THEN** the program stops with integer conversion out of range (tests/runtime/char-conversion-surrogate.inox, tests/runtime/char-conversion-range.inox)

#### Scenario: Conversions that are not written, or not Char conversions
- **WHEN** a program writes `C Char := 65`, `I Integer := 'A'`, `Char(55296)`, `'A' + 1`, `Succ('a')` or `Float('a')`
- **THEN** the compiler rejects each (tests/diagnostics/char-from-integer.inox, tests/diagnostics/char-to-integer.inox, tests/diagnostics/char-conversion-constant.inox, tests/diagnostics/char-arithmetic.inox, tests/diagnostics/char-succ.inox, tests/diagnostics/char-float-conversion.inox); `Ord('A')` stays rejected (tests/diagnostics/ord-of-char.inox)

### Requirement: Chars and Strings
`String(C)` SHALL be the one-character String of C; there is no implicit
conversion between Char and String (`S + String(C)`). `String(X)` of
anything but a Char or a String SHALL be rejected.

#### Scenario: String of a Char
- **WHEN** a program writes `"água" + String(Letter)` and `String('z') = "z"`
- **THEN** it gets the joined String and true (tests/runtime/chars.inox); `String(5)` is rejected (tests/diagnostics/string-of-integer.inox)

### Requirement: Comparison by code point
`=`, `#`, `<`, `<=`, `>` and `>=` SHALL compare Chars by code point, as
Strings are ordered.

#### Scenario: Ordering
- **WHEN** a program compares `'a' < 'b'`, `'Z' >= 'a'` and passes Chars to a generic `Largest`
- **THEN** the results follow the code points (tests/runtime/chars.inox)

### Requirement: Char ranges
`'a'..'z'` SHALL be a range of Chars for `for`, `case` and `in`; a `for`
over it SHALL give a Char iterator.

#### Scenario: Iterating and testing
- **WHEN** a program runs `for Code in 'a'..'e'`, a `case` with `'a'..'z' | 'A'..'Z'` and `'q' in 'a'..'z'`
- **THEN** it visits, picks and tests by code point (tests/runtime/chars.inox, tests/semantic/valid/case-char-choices.inox)

### Requirement: Where a Char lives
A Char SHALL be a value in locals, parameters (`ref` and `ref mut`
included), results, struct fields, State, array elements (stored in 32
bits), Consts, generic instances and `IIf`; `Put`, `PutLn`, `Write` and `WriteLn` SHALL write its
character in UTF-8. Reading one Char (`Get(C)`) is not yet in the language.

#### Scenario: Places and output
- **WHEN** a program uses a Char Const, State variable, field, array element, `ref mut` parameter and `IIf`, and writes Chars to a file
- **THEN** each holds and writes its character (tests/runtime/chars.inox)

