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
- **THEN** it prints 2 and true (tests/runtime/enums.inox)

#### Scenario: Both declaration forms
- **WHEN** a program declares one short and one multi-line enum and uses both
- **THEN** it compiles and runs (tests/runtime/enums.inox)

### Requirement: No implicit Integer conversion
An enum value SHALL NOT be used as an Integer, nor an Integer as an enum.
`TEnum(I)` SHALL convert explicitly; a run-time I outside 0..N-1 SHALL stop the
program and a constant one SHALL be a compile error.

#### Scenario: Implicit use rejected
- **WHEN** a program writes `X Integer := Heart` or `S TSuit := 2`
- **THEN** the compiler rejects it (tests/diagnostics/enum-implicit-integer.inox, tests/diagnostics/enum-from-integer.inox)

#### Scenario: Checked conversion
- **WHEN** `TSuit(I)` runs with I = 4
- **THEN** the program stops with a conversion fault (tests/runtime/enum-conversion-range.inox); a constant one is rejected (tests/diagnostics/enum-conversion-constant.inox)

### Requirement: Enum variables need an initializer
An enum variable SHALL require an explicit initializer (strict Ada rule).

#### Scenario: Missing initializer
- **WHEN** a program declares `Suit TSuit`
- **THEN** the compiler rejects it (tests/diagnostics/enum-without-initializer.inox)

### Requirement: Enum ranges in for
`for S in Club..Heart` SHALL iterate the literals in ordinal order.

#### Scenario: Iterate a sub-range
- **WHEN** a program prints `Ord(S)` for each S in `Diamond..Spade`
- **THEN** it prints 1, 2, 3 (tests/runtime/enums.inox)

### Requirement: A literal belongs to one enum
The same literal name SHALL NOT appear in two enum types of a program, nor
clash with any other visible name (case-insensitive).

#### Scenario: Literal in two enums
- **WHEN** `TSuit` and `TColor` both declare `Heart`
- **THEN** the compiler rejects it (tests/diagnostics/enum-literal-in-two-enums.inox)

### Requirement: Low, High, Succ and Pred
`Low(T)` and `High(T)` SHALL give the first and last literal of an enum type
T. `Succ(E)` and `Pred(E)` SHALL give the next and previous literal; past the
last or before the first they SHALL stop the program (a constant case SHALL be
a compile error).

#### Scenario: Walking an enum
- **WHEN** a program prints `Ord(Low(TSuit))`, `Ord(High(TSuit))` and `Ord(Succ(Club))`
- **THEN** it prints 0, 3 and 1 (tests/runtime/enums.inox)

#### Scenario: Past the last literal
- **WHEN** `Succ(S)` runs with S = Spade
- **THEN** the program stops with a range fault (tests/runtime/enum-succ-past-end.inox); written as a constant it is rejected (tests/diagnostics/enum-succ-constant.inox)

### Requirement: Printing an enum prints its name
`Put(E)` and `PutLn(E)` SHALL print the literal's name as spelled in the enum
declaration; `Ord(E)` SHALL give its ordinal.

#### Scenario: Name and ordinal
- **WHEN** `Naipe TNaipe := Copas` with `TNaipe (Paus, Ouros, Copas, Espadas)` and the program runs `PutLn(Naipe)` and `PutLn(Ord(Naipe))`
- **THEN** it prints `Copas` and `2` (tests/runtime/enums.inox)

### Requirement: Enums in aggregates
An enum struct field MAY have a literal default; an enum field without a
default and an enum array element SHALL start at the first literal, as
every aggregate starts at its type's defaults (CANON-5).

#### Scenario: Field default and element
- **WHEN** a struct field `FSuit TSuit := Heart` and an array of TSuit are declared without values
- **THEN** the field holds Heart and the element Club (tests/runtime/enums.inox)

### Requirement: An enum has no arithmetic
`+`, `-` and the other arithmetic operators SHALL NOT apply to enum values,
and `Integer(E)` SHALL be rejected in favor of `Ord(E)`.

#### Scenario: Arithmetic and Integer(E)
- **WHEN** a program writes `Heart + 1` or `Integer(Heart)`
- **THEN** the compiler rejects both, pointing to Succ and Ord (tests/diagnostics/enum-arithmetic.inox, tests/diagnostics/enum-integer-conversion.inox)
