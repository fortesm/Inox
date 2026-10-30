# language/strings Specification

## Purpose
The String value type (CANON-14): UTF-8, immutable, never null, `""` by
default, with value semantics wherever a value can live. Part of the Inox law,
detailing the constitution (`docs/openspec/specs/constitution`).

## Requirements

### Requirement: Strings are values with an empty default
A String struct field without default and a String array element SHALL hold
`""`. A String struct field MAY have a String literal default. A String local
or State variable SHALL require an initializer (CANON-5 rule 3, as today).
Assignment and passing SHALL behave as copies, because Strings are immutable.

#### Scenario: Defaults and copies
- **WHEN** a program prints a String field without default, then runs `S String := "abc"`, `T := S`, `S := "xyz"`
- **THEN** it prints an empty field, and T prints "abc" (tests/runtime/strings.inox)

#### Scenario: Literal field default
- **WHEN** a struct declares `FName String := "no name"` and a program prints that field of a declared value and of an array element
- **THEN** each prints "no name" (tests/runtime/strings-aggregates.inox)

#### Scenario: Default of the wrong type
- **WHEN** an Integer field has the default `"12"`, or a String field has the default `12`
- **THEN** the compiler rejects both (tests/diagnostics/struct-default-string-on-integer.inox, tests/diagnostics/struct-default-number-on-string.inox)

#### Scenario: Local without initializer
- **WHEN** a program declares `S String` with no `:=`
- **THEN** the compiler rejects it (tests/diagnostics/string-without-initializer.inox)

#### Scenario: Every position
- **WHEN** Strings are stored in locals, struct fields, array elements of one and two dimensions, arrays of structs and State, passed by value and by `ref mut`, and returned from routines
- **THEN** each prints the value stored (tests/runtime/strings.inox, tests/runtime/strings-aggregates.inox)

### Requirement: Equality is byte by byte
`=` and `#` SHALL compare the UTF-8 bytes of two Strings.

#### Scenario: Equal and different strings
- **WHEN** a program compares "ação" with "ação" and "a" with "A"
- **THEN** the first is equal and the second is different (tests/runtime/strings.inox)

#### Scenario: Comparison without owned Strings
- **WHEN** a program only compares String literals and parameters, owns no String and raises nothing
- **THEN** it builds and runs: the driver links libinoxrt whenever the String runtime is used (tests/runtime/strings-literal-compare.inox)

### Requirement: GetLn reads a line into a String
`GetLn(S)` with a String variable SHALL read the rest of the input line into
`S` without the line break, dropping a carriage return before it; at end of
input `S` SHALL be `""`. The line SHALL be well-formed UTF-8 (RFC 3629),
otherwise the program SHALL stop with the runtime fault "invalid UTF-8 input"
(kind 13). `Get(S)` with a String SHALL be a compile error that points to
`GetLn`.

#### Scenario: Lines, CR LF, empty line, last line, end of input
- **WHEN** a program reads lines that include a CR LF line, an empty line, a last line without newline and then reads past the end
- **THEN** it gets each line without its break, `""` for the empty line and `""` past the end (tests/runtime/strings-getln.inox with .in)

#### Scenario: Invalid UTF-8
- **WHEN** an input line holds the bytes C3 28
- **THEN** the program stops with "Inox runtime error: invalid UTF-8 input" after printing the lines before it (tests/runtime/strings-invalid-utf8.inox, .trap)

#### Scenario: Get on a String
- **WHEN** a program calls `Get(S)` with a String S
- **THEN** the compiler says "Get reads Integer values; use GetLn(S) to read a line of text into a String" (tests/diagnostics/get-string.inox)

### Requirement: No memory is leaked or freed twice
Every String created at run time SHALL be released exactly once when its last
owner goes away, including when a routine is left by `Return`, `Exit`,
`leave`, `continue`, `Retry` or an exception, and when a statement that holds
a call result is abandoned by an exception. A String a statement has read
SHALL stay valid until the statement ends, even when a routine it calls
replaces the place it was read from.

#### Scenario: Scope exits under the leak checker
- **WHEN** the String test programs run under AddressSanitizer with leak detection
- **THEN** no leak, double free or use after free is reported (tools/string_leak_check.py over tests/runtime/string*.inox; Linux CI)

#### Scenario: A value read is held for its statement
- **WHEN** a statement reads a String from State, a field of State or a `ref mut` parameter and then calls a routine that replaces it
- **THEN** the value read is used as it was, with no use after free (tests/runtime/string-borrowed-read.inox, leak-checked with the generated code instrumented)

#### Scenario: Cleanup pads balance exception states
- **WHEN** exceptions unwind through routines holding Strings
- **THEN** every exception state a cleanup pad captures is consumed by its rethrow (tests/eh-lifetime/string-cleanup.inox, tools/eh_state_balance.py)

### Requirement: The runtime is unit tested
The String runtime SHALL have unit tests for immortal blocks, retain and
release, equality, line input and UTF-8 validation, run by the test suite on
Linux and Windows.

#### Scenario: Runtime unit tests
- **WHEN** the suite runs
- **THEN** `inoxrt-tests` passes (tests/runtime-unit/StringRuntimeTests.cpp)
