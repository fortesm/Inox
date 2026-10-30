## ADDED Requirements

### Requirement: Classes and case of a Char
`IsLetter(C)` (general category L*), `IsDigit(C)` (Nd), `IsSpace(C)`
(White_Space), `IsUpper(C)` (Lu), `IsLower(C)` (Ll) and `IsPunctuation(C)`
(P*) SHALL classify a Char by Unicode, never by a locale; `UpperCase(C)` and
`LowerCase(C)` of a Char map it to one Char, as the String functions map
each character (r3.89).

#### Scenario: Classes
- **WHEN** letters, digits of two scripts, spaces, punctuation and ideographs are classified and mapped
- **THEN** each has its Unicode class and case (tests/runtime/char-classes.inox); a String argument is rejected (tests/diagnostics/char-class-argument.inox), and without `Use Std.Strings` the name is unknown (tests/diagnostics/char-class-without-use.inox)
