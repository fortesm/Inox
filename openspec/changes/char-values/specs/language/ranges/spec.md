## ADDED Requirements

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

## MODIFIED Requirements

### Requirement: Range bounds, Succ and Pred
The bounds of a Range SHALL be static values (literals or Consts). `Low(T)`
and `High(T)` SHALL give them; `Succ` and `Pred` past them SHALL stop the
program; a Char Range has no `Succ` or `Pred` (a Char is not an ordinal).

#### Scenario: Bounds and steps
- **WHEN** a Range's bound is a State variable, or `Succ(M)` runs with M at the high bound
- **THEN** the first is rejected (tests/diagnostics/range-bounds-not-static.inox) and the second stops the program (tests/runtime/range-succ-past-end.inox)
