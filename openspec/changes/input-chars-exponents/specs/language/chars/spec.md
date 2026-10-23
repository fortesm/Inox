## ADDED Requirements

### Requirement: Reading a Char
`Get(C)` SHALL read the next character of the input exactly, white space and
line ends included, into a Char or a Char Range (a value outside the Range
traps, "value out of range"); `EndOfInput` tells when nothing is left
(CANON-17, r3.89).

#### Scenario: Characters and a Range
- **WHEN** a program reads characters until EndOfInput, and a capital into a Range of lower-case letters
- **THEN** every character arrives (tests/runtime/get-char.inox) and the capital traps (tests/runtime/get-char-range.inox)
