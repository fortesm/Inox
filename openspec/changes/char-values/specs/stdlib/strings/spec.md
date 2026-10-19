## ADDED Requirements

### Requirement: The character at a position
`CharAt(S, I)` (Std.Strings) SHALL be the Char at position I, counted from 1
in characters; a position outside 1..Length(S) SHALL stop the program with
"string index out of range" (change char-values). `S[I]` stays reserved.

#### Scenario: CharAt
- **WHEN** a program reads `CharAt("águaé", 1)` and `CharAt("águaé", 5)`
- **THEN** it gets á and é (tests/runtime/chars.inox)

#### Scenario: CharAt outside the String
- **WHEN** `CharAt(Word, I)` runs with I = Length(Word) + 1
- **THEN** the program stops with string index out of range (tests/runtime/char-at-out-of-range.inox)

