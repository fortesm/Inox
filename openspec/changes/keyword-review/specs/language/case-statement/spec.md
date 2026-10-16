## RENAMED Requirements

- FROM: `### Requirement: Choices never overlap and otherwise is last`
- TO: `### Requirement: Choices never overlap and else is last`

- FROM: `### Requirement: Otherwise is required unless coverage is proven`
- TO: `### Requirement: Else is required unless coverage is proven`

## MODIFIED Requirements

### Requirement: Case selects one arm by static choices
A `case Expression` statement SHALL evaluate its selector exactly once and run
the first arm whose choices contain the value. A choice SHALL be a static value
or a static range `A..B`; alternatives SHALL be separated by `|` (a comma is a
compile error). The selector SHALL be discrete: an integer type, Bool, Char,
an enum or a range.

#### Scenario: Values, ranges, alternatives and negative ranges
- **WHEN** a program runs `case N` with arms `0`, `1..Limit | 20`, `-5..-1` and `else`
- **THEN** each N selects exactly the matching arm (tests/runtime/case-lowering.inox)

#### Scenario: The selector is evaluated once
- **WHEN** the selector is a call with a visible side effect
- **THEN** the side effect happens once (tests/runtime/case-lowering.inox)

#### Scenario: Comma between alternatives
- **WHEN** a choice list is written `1, 2`
- **THEN** the compiler rejects it (tests/diagnostics/case-comma-separator.inox)

### Requirement: Choices never overlap and else is last
Two choices of one case SHALL NOT overlap, in one arm or across arms;
`else` SHALL appear at most once and SHALL be the last arm (r3.74; formerly
`otherwise`, which is now rejected with a message naming `else`). There is no
fall-through.

#### Scenario: Overlapping choices
- **WHEN** a range choice overlaps another choice
- **THEN** the compiler rejects the case (tests/diagnostics/case-overlap-range.inox, tests/diagnostics/case-duplicate-across-arms.inox)

#### Scenario: Misplaced else
- **WHEN** `else` is not the last arm or appears twice, or an arm is written `otherwise`
- **THEN** the compiler rejects the case (tests/diagnostics/case-else-not-last.inox, tests/diagnostics/case-two-else.inox, tests/diagnostics/case-otherwise-removed.inox)

### Requirement: Else is required unless coverage is proven
Without `else`, the choices SHALL cover every value of the selector type, or
the case SHALL be rejected naming the first missing value. Bool needs both
values; an integer type needs its whole range; Natural is 0..Int64.Max;
UInt64 always needs `else`. A total case cannot fall through.

#### Scenario: Incomplete Integer case
- **WHEN** a case on an Integer handles only some values and has no else
- **THEN** the compiler rejects it (tests/diagnostics/case-integer-incomplete.inox)

#### Scenario: Total Bool case and function ending in a total case
- **WHEN** a case handles True and False, or a function ends in a total case whose arms all Return
- **THEN** it compiles and runs (tests/runtime/case-lowering.inox, tests/semantic/valid/case-exhaustive-returns.inox)

#### Scenario: Incomplete Bool case and UInt64 without else
- **WHEN** a Bool case handles only True, or a UInt64 case has no else
- **THEN** the compiler rejects it (tests/diagnostics/case-bool-incomplete.inox, tests/diagnostics/case-uint64-needs-else.inox)

#### Scenario: Natural coverage
- **WHEN** a case on Natural covers 0..9223372036854775807, or leaves 0 out
- **THEN** the first compiles, the second is rejected (tests/semantic/valid/case-natural-total.inox, tests/diagnostics/case-natural-missing-zero.inox)
