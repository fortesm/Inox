# language/ranges Specification

## Purpose
Range types (CANON-13 Range): a named, constrained set of Integer values,
a type of its own as in Ada, or a constrained subset of Char, declared on
one line without `;`.

## Requirements

### Requirement: A Range constrains its base type
`TName Range A..B` SHALL declare a type over the base type of A and B
(Integer family or Char) holding A..B, with A <= B. A Range variable SHALL
never hold a value outside A..B.

#### Scenario: Declaration and use
- **WHEN** a program declares `TMonth Range 1..12`, stores 12 in a TMonth and stores `Integer(M) + 1` into an Integer
- **THEN** it prints 13 for the Integer (tests/runtime/ranges.inox)

#### Scenario: Constant outside the range
- **WHEN** a program writes `M TMonth := 13`
- **THEN** the compiler rejects it (tests/diagnostics/range-constant-out-of-range.inox)

#### Scenario: Run-time value outside the range
- **WHEN** `M + 1`, with M a TMonth holding 12, is stored into a TMonth, or declares one (`N := M + 1`)
- **THEN** the program stops with a range fault (tests/runtime/range-runtime-out-of-range.inox, tests/runtime/range-inferred-out-of-range.inox)

#### Scenario: Empty range
- **WHEN** a program declares `TBad Range 5..1`
- **THEN** the compiler rejects it (tests/diagnostics/range-empty.inox)

### Requirement: An Integer Range is a type of its own
An Integer Range SHALL be a distinct type, as Ada's `type T is range A..B`
(r3.106, change range-types). Its values SHALL combine in arithmetic and
comparisons only with values of the same Range and with constants; the result
of its arithmetic SHALL belong to the Range and SHALL be range-checked when
it is stored; a declaration `N := M + 1` SHALL declare a value of the Range.

#### Scenario: Same Range and constants
- **WHEN** a program adds, multiplies, negates and compares TMeters values and constants, and declares `N := D * 2`
- **THEN** the results are TMeters values (tests/runtime/range-types.inox)

#### Scenario: Two Ranges, or a Range and an Integer variable
- **WHEN** an expression mixes a TMeters and a TSeconds, or a TMeters and an Integer variable, in arithmetic, a comparison or the branches of IIf
- **THEN** the compiler rejects it and names the conversion (tests/diagnostics/range-mix-two-ranges.inox, tests/diagnostics/range-mix-integer-variable.inox, tests/diagnostics/range-compare-integer-variable.inox, tests/diagnostics/range-conditional-mix.inox)

### Requirement: Range values cross types only by conversion
A Range value SHALL be stored into, or passed as, an Integer (or another
Range) only through a written conversion, `Integer(X)` or `T(X)`; an Integer
value SHALL enter a Range only through `T(X)`, which checks the bounds; a
constant inside the bounds SHALL enter directly. A Range value SHALL index an
array, be printed by Put and PutLn, and be converted by `String(X)`, as it
is; a Set of a Range SHALL test values of that Range and constants.

#### Scenario: Conversions
- **WHEN** a program passes `Integer(D)` to an Integer parameter, stores `TMeters(I)` and `TMeters(S)`, indexes an array with a TDigit and tests it in a `Set<TDigit>`
- **THEN** it runs (tests/runtime/range-types.inox)

#### Scenario: Missing conversions
- **WHEN** a program passes a TMeters to an Integer parameter, stores a TSeconds or an Integer variable into a TMeters, or tests a TDigit in a `Set<TPercent>`
- **THEN** the compiler rejects it (tests/diagnostics/range-into-integer-parameter.inox, tests/diagnostics/range-from-other-range.inox, tests/diagnostics/range-from-integer-variable.inox, tests/diagnostics/range-in-other-set.inox)

### Requirement: Range bounds, Succ and Pred
The bounds of a Range SHALL be static values (literals or Consts). `Low(T)`
and `High(T)` SHALL give them; `Succ` and `Pred` past them SHALL stop the
program; a Char Range has no `Succ` or `Pred` (a Char is not an ordinal).

#### Scenario: Bounds and steps
- **WHEN** a Range's bound is a State variable, or `Succ(M)` runs with M at the high bound
- **THEN** the first is rejected (tests/diagnostics/range-bounds-not-static.inox) and the second stops the program (tests/runtime/range-succ-past-end.inox)

### Requirement: Char Ranges hold Chars
A Range over Char (`TLetter Range 'A'..'Z'`) SHALL hold Chars: a Char
stored into it SHALL be checked against its bounds, a constant outside them
being a compile error and any other value stopping the program with "value
out of range"; a value of it SHALL be a Char; `Low` and `High` SHALL give its
bounds (change char-values).

#### Scenario: Char Range values
- **WHEN** a program stores 'M' in a TLetter, iterates `Low(TLetter)..High(TLetter)` and prints them
- **THEN** it holds and prints the Chars (tests/runtime/chars.inox)

#### Scenario: Char outside the Range
- **WHEN** a program writes `Upper TLetter := 'a'`, or stores a Char variable holding 'a'
- **THEN** the first is rejected (tests/diagnostics/char-range-constant.inox) and the second stops the program (tests/runtime/char-range-store.inox)
