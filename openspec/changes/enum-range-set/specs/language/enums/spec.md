## Purpose
Enumeration types (CANON-13 Enum, CANON-5 rule 5): nominal, ordinal sets of
named values with no implicit conversion to or from Integer.

## ADDED Requirements

### Requirement: Enums are nominal and ordinal
An Enum SHALL be declared short (`TSuit (Club, Diamond, Heart, Spade)`) or
multi-line (`TDay Enum ... ;`). Its literals SHALL have ordinals 0..N-1 in
order. Values of one enum type SHALL compare with `=`, `#`, `<`, `<=`, `>`
and `>=` by ordinal.

#### Scenario: Ordinals and comparison
- **WHEN** a program prints `Ord(Heart)` and compares `Club < Spade`
- **THEN** it prints 2 and true (test to add: tests/runtime/enums.inox)

#### Scenario: Both declaration forms
- **WHEN** a program declares one short and one multi-line enum and uses both
- **THEN** it compiles and runs (test to add: tests/runtime/enums.inox)

### Requirement: No implicit Integer conversion
An enum value SHALL NOT be used as an Integer, nor an Integer as an enum.
`TEnum(I)` SHALL convert explicitly; a run-time I outside 0..N-1 SHALL stop the
program and a constant one SHALL be a compile error.

#### Scenario: Implicit use rejected
- **WHEN** a program writes `X Integer := Heart` or `S TSuit := 2`
- **THEN** the compiler rejects it (test to add: tests/diagnostics/enum-implicit-integer.inox)

#### Scenario: Checked conversion
- **WHEN** `TSuit(I)` runs with I = 4
- **THEN** the program stops with a conversion fault (test to add: tests/runtime/enum-conversion-range.inox)

### Requirement: Enum variables need an initializer
An enum variable SHALL require an explicit initializer (strict Ada rule).

#### Scenario: Missing initializer
- **WHEN** a program declares `Suit TSuit`
- **THEN** the compiler rejects it (test to add: tests/diagnostics/enum-without-initializer.inox)

### Requirement: Enum ranges in for
`for S in Club..Heart` SHALL iterate the literals in ordinal order.

#### Scenario: Iterate a sub-range
- **WHEN** a program prints `Ord(S)` for each S in `Diamond..Spade`
- **THEN** it prints 1, 2, 3 (test to add: tests/runtime/enums.inox)
