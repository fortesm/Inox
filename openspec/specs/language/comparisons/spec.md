# language/comparisons Specification

## Purpose
Which operand types each comparison operator (`=` `#` `<` `>` `<=` `>=`)
accepts, what it means for each type, and how the Bool results combine
(CANON-14, CANON-20; canon v3.56).

## Requirements

### Requirement: Strings are ordered byte by byte
`<` `>` `<=` `>=` SHALL order two Strings by their UTF-8 bytes compared as
unsigned values, the shorter first when one is a prefix of the other. This is
Unicode code point order: case-sensitive, not a locale collation.

#### Scenario: Ordering
- **WHEN** a program compares "Ana" with "ana", "Ana" with "Ana Maria", "" with "Ana", "ana" with "água", "Z" with "a" and "z" with "á"
- **THEN** in each pair the first is smaller (tests/runtime/strings-ordering.inox)

#### Scenario: Runtime order
- **WHEN** the runtime compares equal strings, a prefix, upper and lower case, and multi-byte letters
- **THEN** it returns 0, -1 or 1 as above (tests/runtime-unit/StringRuntimeTests.cpp)

### Requirement: Comparison results combine like any Bool
The result of any comparison SHALL be a Bool usable with `not`, `and`,
`xor`, `or`, `if`/`elif`, `unless`, `while`, `until`, the conditional
expression, and as a value stored or compared with `=`/`#`.

#### Scenario: Combinations
- **WHEN** a program uses String comparisons under `not`, `and`, `or`, `xor`, `unless`, `elif`, `while`, `until`, `if ... then ... else` and stores one in a Bool variable
- **THEN** each gives the expected result (tests/runtime/strings-ordering.inox)

### Requirement: Bool values are ordered false < true
Two Bool values SHALL compare with every comparison operator, `false` being
smaller than `true`, as in Pascal and Ada (maintainer decision, v3.57).

#### Scenario: Bool equality
- **WHEN** a program prints `A = B` and `A # B` for two Bool variables
- **THEN** it prints their equality (tests/runtime/strings-ordering.inox; backend_gaps probe bool-comparison)

#### Scenario: Bool ordering
- **WHEN** a program compares `false` and `true` with `<` `>` `<=` `>=`, also through Bool expressions such as `(1 > 2) < (2 > 1)`
- **THEN** `false` is smaller than `true` (tests/runtime/bool-ordering.inox)

### Requirement: Structs are not compared
`=` `#` `<` `>` `<=` `>=` on struct operands SHALL be a compile error that
points to comparing their fields, as in Pascal (maintainer decision, v3.57).

#### Scenario: Struct comparison
- **WHEN** a program writes `P = Q` or `P < Q` with struct operands
- **THEN** the compiler rejects it (tests/diagnostics/struct-equality.inox, tests/diagnostics/struct-ordering.inox)
