# language/ranges Specification

## Purpose
Range subtypes (CANON-13 Range): a named, constrained subset of an Integer
family type or of Char, declared on one line without `;`.

## Requirements

### Requirement: A Range constrains its base type
`TName Range A..B` SHALL declare a subtype of the base type of A and B
(Integer family or Char) holding A..B, with A <= B. A Range variable SHALL
never hold a value outside A..B.

#### Scenario: Declaration and use
- **WHEN** a program declares `TMonth Range 1..12`, stores 12 in a TMonth and adds 1 into an Integer
- **THEN** it prints 13 for the Integer (tests/runtime/ranges.inox)

#### Scenario: Constant outside the range
- **WHEN** a program writes `M TMonth := 13`
- **THEN** the compiler rejects it (tests/diagnostics/range-constant-out-of-range.inox)

#### Scenario: Run-time value outside the range
- **WHEN** an Integer variable holding 13 is stored into a TMonth
- **THEN** the program stops with a range fault (tests/runtime/range-runtime-out-of-range.inox)

#### Scenario: Empty range
- **WHEN** a program declares `TBad Range 5..1`
- **THEN** the compiler rejects it (tests/diagnostics/range-empty.inox)

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
