# case-statement Specification

## Purpose
The Ada-style `case` statement (ADR-0011, canon CANON-12 "case") and `unless`.
Migrated from the canon on 2026-10-12; the canon section points here.

## Requirements

### Requirement: Case selects one arm by static choices
A `case Expression` statement SHALL evaluate its selector exactly once and run
the first arm whose choices contain the value. A choice SHALL be a static value
or a static range `A..B`; alternatives SHALL be separated by `|` (a comma is a
compile error). The selector SHALL be discrete: an integer type, Bool, Char,
an enum or a range.

#### Scenario: Values, ranges, alternatives and negative ranges
- **WHEN** a program runs `case N` with arms `0`, `1..Limit | 20`, `-5..-1` and `otherwise`
- **THEN** each N selects exactly the matching arm (tests/runtime/case-lowering.inox)

#### Scenario: The selector is evaluated once
- **WHEN** the selector is a call with a visible side effect
- **THEN** the side effect happens once (tests/runtime/case-lowering.inox)

#### Scenario: Comma between alternatives
- **WHEN** a choice list is written `1, 2`
- **THEN** the compiler rejects it (tests/diagnostics/case-comma-separator.inox)

### Requirement: Choices never overlap and otherwise is last
Two choices of one case SHALL NOT overlap, in one arm or across arms;
`otherwise` SHALL appear at most once and SHALL be the last arm. There is no
fall-through.

#### Scenario: Overlapping choices
- **WHEN** a range choice overlaps another choice
- **THEN** the compiler rejects the case (tests/diagnostics/case-overlap-range.inox, tests/diagnostics/case-duplicate-across-arms.inox)

#### Scenario: Misplaced otherwise
- **WHEN** `otherwise` is not the last arm or appears twice
- **THEN** the compiler rejects the case (tests/diagnostics/case-otherwise-not-last.inox, tests/diagnostics/case-two-otherwise.inox)

### Requirement: Otherwise is required unless coverage is proven
Without `otherwise`, the choices SHALL cover every value of the selector type,
otherwise the case SHALL be rejected naming the first missing value. Bool needs
both values; an integer type needs its whole range; Natural is 0..Int64.Max;
UInt64 always needs `otherwise`. A total case cannot fall through.

#### Scenario: Incomplete Integer case
- **WHEN** a case on an Integer handles only some values and has no otherwise
- **THEN** the compiler rejects it (tests/diagnostics/case-integer-incomplete.inox)

#### Scenario: Total Bool case and function ending in a total case
- **WHEN** a case handles True and False, or a function ends in a total case whose arms all Return
- **THEN** it compiles and runs (tests/runtime/case-lowering.inox, tests/semantic/valid/case-exhaustive-returns.inox)

#### Scenario: Natural coverage
- **WHEN** a case on Natural covers 0..9223372036854775807, or leaves 0 out
- **THEN** the first compiles, the second is rejected (tests/semantic/valid/case-natural-total.inox, tests/diagnostics/case-natural-missing-zero.inox)

### Requirement: Unless runs its body when the condition is false
`unless C` SHALL run its body exactly when C is false.

#### Scenario: Unless inside a loop
- **WHEN** `unless I mod 2 = 0` guards a body inside a for loop
- **THEN** the body runs for odd I only (tests/runtime/unless-lowering.inox)
