## Purpose
`Std.Strings` (the String functions every program needs) and `Std.StrUtils`
(Delphi-style conveniences): by Unicode rules, independent of any one human
language. Part of the Inox law, detailing the constitution (CANON-14,
CANON-18).

## ADDED Requirements

### Requirement: Concatenation with +
`A + B` SHALL join two Strings into a new String; a chain is joined with one
allocation, and `S := S + X` / `S += X` SHALL cost amortized O(length of X)
without changing any other String. A String and a value of another type
SHALL NOT be joined implicitly.

#### Scenario: Join
- **WHEN** a program joins literals, variables and a chain of four Strings
- **THEN** it prints the joined text (test to add: strings-concat)

#### Scenario: Append in a loop
- **WHEN** a program appends one character 100 000 times
- **THEN** it finishes in linear time and frees every block (test to add: strings-append-loop, leak-checked)

#### Scenario: Shared value unchanged
- **WHEN** `T := S` and then `S += "x"`
- **THEN** T keeps the old text (test to add: strings-append-shared)

#### Scenario: No implicit conversion
- **WHEN** a program writes `"N=" + 5`
- **THEN** it is rejected (test to add: string-concat-integer)

### Requirement: Lengths and positions count characters
`Length(S)` SHALL return the number of code points, `ByteLength(S)` the
number of UTF-8 bytes; positions SHALL start at 1 and count code points.
`Substr(S, Start, Count)` SHALL return Count characters from Start, and a
range outside S SHALL stop the program with "string index out of range".

#### Scenario: Multi-byte text
- **WHEN** a program measures and slices "água😀"
- **THEN** Length is 5, ByteLength is 9 and `Substr(S, 2, 3)` is "gua" (test to add: strings-length-substr)

#### Scenario: Out of range
- **WHEN** `Substr("abc", 3, 2)` runs
- **THEN** the program stops with "string index out of range" (test to add: strings-substr-range)

### Requirement: Search, case and conveniences
`Pos`, `UpperCase`, `LowerCase` (Std.Strings) and `Trim`, `LTrim`, `RTrim`,
`Left`, `Right`, `PadLeft`, `PadRight`, `Replicate`, `Replace`, `Contains`,
`StartsWith`, `EndsWith`, `SameText`, `CompareText` (Std.StrUtils) SHALL
behave as listed in the change `std-strings`, by Unicode rules that belong to
no single human language.

#### Scenario: Case-insensitive comparison
- **WHEN** a program compares "ÁGUA" and "água" with `SameText`
- **THEN** they are equal, with no locale set (test to add: strutils-case)

#### Scenario: Trim
- **WHEN** a program trims spaces, tabs and a no-break space
- **THEN** Trim, LTrim and RTrim remove them at both ends, the start and the end (test to add: strutils-trim)

### Requirement: The names exist only with Use
The functions of Std.Strings SHALL be visible only with `Use Std.Strings` or
`Use Std.StrUtils`, and those of Std.StrUtils only with `Use Std.StrUtils`;
elsewhere their names SHALL be free. `+` on Strings needs no `Use`.

#### Scenario: Without Use
- **WHEN** a program calls Trim without `Use Std.StrUtils`
- **THEN** the compiler asks for it (test to add: strutils-without-use)
