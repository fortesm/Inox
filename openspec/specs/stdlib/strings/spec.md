# stdlib/strings Specification

## Purpose
`Std.Strings` (the String functions every program needs) and `Std.StrUtils`
(Delphi-style conveniences), with `+` on Strings: by Unicode rules,
independent of any one human language. Part of the Inox law, detailing the
constitution (CANON-14, CANON-18).

## Requirements

### Requirement: Concatenation with +
`A + B` SHALL join two Strings into a new String, and `S += X` SHALL append
X to S; a chain SHALL be joined with one allocation. A String and a value of
another type SHALL NOT be joined implicitly. `+` on Strings needs no `Use`.

#### Scenario: Join
- **WHEN** a program joins literals, variables, empty Strings and a chain of five parts
- **THEN** it prints the joined text (tests/runtime/strings-concat.inox)

#### Scenario: No implicit conversion
- **WHEN** a program writes `"N=" + N` with N an Integer
- **THEN** it is rejected: "there is no implicit conversion to String" (tests/diagnostics/string-plus-integer.inox)

### Requirement: Appending in place
`S := S + X` and `S += X` SHALL cost amortized O(length of X): when S alone
holds its block and the block has room, X is copied into it; otherwise a
block with room to grow replaces it. No other String SHALL change, and S may
be a local, a State variable, a field, an element or a `ref mut` parameter.

#### Scenario: Append in a loop
- **WHEN** a program appends "ab" 100 000 times
- **THEN** it prints 200000 for Length and ByteLength and frees every block (tests/runtime/strings-concat.inox, leak-checked)

#### Scenario: Shared value unchanged
- **WHEN** `Copy := S` and then `S += "y"`
- **THEN** Copy keeps the old text (tests/runtime/strings-concat.inox)

#### Scenario: Every place
- **WHEN** a program appends to a field, an element, a State variable and a `ref mut` parameter, and writes `S := S + S + "z"`
- **THEN** each holds the joined text (tests/runtime/strings-concat.inox)

### Requirement: Lengths and positions count characters
`Length(S)` SHALL return the number of code points and `ByteLength(S)` the
number of UTF-8 bytes. Positions SHALL start at 1 and count code points.
`Substr(S, Start, Count)` SHALL return Count characters from Start, with
`1 <= Start <= Length(S) + 1`, `Count >= 0` and `Start + Count - 1 <=
Length(S)`.

#### Scenario: Multi-byte text
- **WHEN** a program measures and slices "água😀"
- **THEN** Length is 5, ByteLength is 9 and `Substr(W, 2, 3)` is "gua" (tests/runtime/strings-functions.inox)

#### Scenario: The end of a String
- **WHEN** a program takes `Substr(S, Length(S) + 1, 0)` and `Substr(S, 1, Length(S))`
- **THEN** it gets "" and the whole String (tests/runtime/strings-functions.inox)

### Requirement: A position outside a String traps
A position, count or width that a function does not accept SHALL stop the
program with "string index out of range" (CANON-19): Substr outside the
String, `Left`/`Right` with N outside `0..Length(S)`, `PadLeft`/`PadRight`
with a negative width and `Replicate` with a negative count.

#### Scenario: Out of range
- **WHEN** `Substr("héllo", 4, 3)`, `Left("héllo", 6)`, `PadLeft(S, -1)` or `Replicate(S, -1)` runs
- **THEN** the program stops with "Inox runtime error: string index out of range" (tests/runtime/string-substr-out-of-range.inox, tests/runtime/string-left-too-long.inox, tests/runtime/string-pad-negative-width.inox, tests/runtime/string-replicate-negative.inox)

### Requirement: Search and case
`Position(Sub, S)` SHALL return the position of the first Sub in S, 0 when
Sub is absent or empty. `UpperCase` and `LowerCase` SHALL apply the Unicode
simple case mappings, one code point to one, so the length never changes
(`ß` stays `ß`). They belong to Std.Strings.

#### Scenario: Search and case
- **WHEN** a program searches "wö" in "héllo wörld" and changes the case of it and of "ÀÉÎ ΩMEGA"
- **THEN** Position is 7, and the results are "HÉLLO WÖRLD" and "àéî ωmega" (tests/runtime/strings-functions.inox)

### Requirement: Trimming, edges and padding
`Trim`, `LTrim` and `RTrim` SHALL remove Unicode White_Space at both ends,
the start and the end. `Left(S, N)` and `Right(S, N)` SHALL return the first
and the last N characters. `PadLeft(S, Width)` and `PadRight(S, Width)` SHALL
add spaces up to Width characters and return a longer String unchanged.

#### Scenario: Trim
- **WHEN** a program trims spaces, a tab and a no-break space
- **THEN** Trim, LTrim and RTrim remove them at both ends, the start and the end (tests/runtime/strings-functions.inox)

#### Scenario: Edges and padding
- **WHEN** a program takes Left, Right and pads multi-byte text
- **THEN** counts are in characters: `PadLeft("é", 4)` is three spaces and "é" (tests/runtime/strings-functions.inox)

### Requirement: Repeating, replacing and testing
`Replicate(S, N)` SHALL repeat S N times. `Replace(S, Old, New)` SHALL replace
every Old, left to right and without overlaps; an empty Old changes nothing.
`Contains`, `StartsWith` and `EndsWith` SHALL return a Bool, and an empty
String is found in any String.

#### Scenario: Replace and find
- **WHEN** a program replicates, replaces and tests Strings, empty ones included
- **THEN** the results follow these rules (tests/runtime/strings-functions.inox; runtime unit tests in tests/runtime-unit/StringFunctionsTests.cpp)

### Requirement: Comparison ignoring case
`SameText(A, B)` SHALL be true when A and B are equal after Unicode simple
case folding; `CompareText(A, B)` SHALL return -1, 0 or 1 comparing the
folded code points, a prefix first. No locale is involved.

#### Scenario: Case-insensitive comparison
- **WHEN** a program compares "ÁGUA" and "água" with SameText, and "a" with "B" with CompareText
- **THEN** they are equal, and "a" comes first (tests/runtime/strings-functions.inox)

### Requirement: The names exist only with Use
The functions of Std.Strings SHALL be visible only with `Use Std.Strings` or
`Use Std.StrUtils`, and those of Std.StrUtils only with `Use Std.StrUtils`.
Elsewhere their names SHALL be free, and a call of one SHALL name the
module to use.

#### Scenario: Without Use
- **WHEN** a program calls Trim without a Use, Left with only `Use Std.Strings`, or Substr without a Use
- **THEN** the compiler names the module to add (tests/diagnostics/string-function-without-use.inox, tests/diagnostics/string-strutils-with-strings-only.inox, tests/diagnostics/string-substr-without-use.inox)

#### Scenario: Free names
- **WHEN** a program without `Use Std.StrUtils` declares its own Trim and Left and a variable named Contains
- **THEN** they are used as declared (tests/runtime/string-names-free.inox)

#### Scenario: Arguments
- **WHEN** Left gets a String count or Replace gets two arguments
- **THEN** the compiler names the argument or the count expected (tests/diagnostics/string-function-argument-type.inox, tests/diagnostics/string-function-argument-count.inox)

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
