## MODIFIED Requirements

### Requirement: Chars and Strings
`String(C)` SHALL be the one-character String of C; there is no implicit
conversion between Char and String (`S + String(C)`). `String(X)` of
anything but a Char, a String or a number (r3.90) SHALL be rejected.

#### Scenario: String of a Char
- **WHEN** a program writes `"água" + String(Letter)` and `String('z') = "z"`
- **THEN** it gets the joined String and true (tests/runtime/chars.inox); `String(True)` and `String(Heart)` are rejected (tests/diagnostics/string-of-bool.inox, tests/diagnostics/string-of-enum.inox)
