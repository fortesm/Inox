## ADDED Requirements

### Requirement: Sets over a Char Range
`Set<TLetter>` SHALL be a set over a Char Range of at most 256 values, with
Char literals as elements, `in`, the comparisons and the six functions
(change char-values).

#### Scenario: A set of letters
- **WHEN** a program builds `Vowels Set<TLetter> := ['A', 'E', 'I', 'O', 'U']`, includes 'Y' and tests membership
- **THEN** each test follows the set (tests/runtime/chars.inox)

