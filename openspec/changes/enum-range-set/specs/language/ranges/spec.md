## Purpose
Range subtypes (CANON-13 Range): a named, constrained subset of an Integer
family type or of Char, declared on one line without `;`.

## ADDED Requirements

### Requirement: A Range constrains its base type
`TName Range A..B` SHALL declare a subtype of the base type of A and B
(Integer family or Char) holding A..B, with A <= B. A Range variable SHALL
never hold a value outside A..B.

#### Scenario: Declaration and use
- **WHEN** a program declares `TMonth Range 1..12`, stores 12 in a TMonth and adds 1 into an Integer
- **THEN** it prints 13 for the Integer (test to add: tests/runtime/ranges.inox)

#### Scenario: Constant outside the range
- **WHEN** a program writes `M TMonth := 13`
- **THEN** the compiler rejects it (test to add: tests/diagnostics/range-constant-out-of-range.inox)

#### Scenario: Run-time value outside the range
- **WHEN** an Integer variable holding 13 is stored into a TMonth
- **THEN** the program stops with a range fault (test to add: tests/runtime/range-runtime-out-of-range.inox)

#### Scenario: Empty range
- **WHEN** a program declares `TBad Range 5..1`
- **THEN** the compiler rejects it (test to add: tests/diagnostics/range-empty.inox)
