## Purpose
The String value type (CANON-14): UTF-8, immutable, never null, `""` by
default, with value semantics wherever a value can live.

## ADDED Requirements

### Requirement: Strings are values with an empty default
A String struct field without default and a String array element SHALL hold
`""`. A String local or State variable SHALL require an initializer (CANON-5
rule 3, as today). Assignment and passing SHALL behave as copies, because
Strings are immutable.

#### Scenario: Defaults and copies
- **WHEN** a program prints a String field without default, then runs `S String := "abc"`, `T := S`, `S := "xyz"`
- **THEN** it prints an empty line, and T prints "abc" (test to add: tests/runtime/strings.inox)

#### Scenario: Local without initializer
- **WHEN** a program declares `S String` with no `:=`
- **THEN** the compiler rejects it (test to add: tests/diagnostics/string-without-initializer.inox)

#### Scenario: Every position
- **WHEN** Strings are stored in struct fields, array elements and State, and passed to and returned from routines
- **THEN** each prints the value stored (test to add: tests/runtime/strings.inox)

### Requirement: Equality is byte by byte
`=` and `#` SHALL compare the UTF-8 bytes of two Strings.

#### Scenario: Equal and different strings
- **WHEN** a program compares "ação" with "ação" and "a" with "A"
- **THEN** the first is equal and the second is different (test to add: tests/runtime/strings.inox)

### Requirement: No memory is leaked or freed twice
Every String created at run time SHALL be released exactly once when its last
owner goes away, including when a scope is left by `Return`, `Exit`, `leave`
or an exception.

#### Scenario: Scope exits under the leak checker
- **WHEN** the String test programs run in the AddressSanitizer build with leak detection
- **THEN** no leak, double free or use after free is reported (test to add: tests/runtime/strings-exception-cleanup.inox)
