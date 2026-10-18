# case-statement Specification

## Purpose
The Ada-style `case` statement (ADR-0011, CANON-12) and `unless`. Part of the
Inox law, detailing the constitution (`openspec/specs/constitution`).

## Requirements

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

### Requirement: Arm syntax
A case header SHALL be `case Expression` on its own line, and an arm SHALL be
its choices followed by its body, on the same line or the next. `of`, `when`,
`=>`, `:` and `do` SHALL be compile errors. `case` SHALL NOT be an expression
(reserved for a future version).

#### Scenario: Single-line and next-line arms
- **WHEN** arms put their body on the choice line or on the next line
- **THEN** both compile (tests/semantic/valid/case-arm-on-next-line.inox, tests/runtime/case-lowering.inox)

#### Scenario: Words and symbols from other languages
- **WHEN** a case writes `case X of`, an arm `1 => ...` or an arm `when 1 ...`
- **THEN** the compiler rejects it (tests/diagnostics/case-of-keyword.inox, tests/diagnostics/case-arrow-arm.inox, tests/diagnostics/case-when-arm.inox)

#### Scenario: Case as an expression
- **WHEN** a program writes `Y := case X ...`
- **THEN** the compiler rejects it (tests/diagnostics/case-as-expression.inox)

### Requirement: Choices are static and of the selector's type
A choice SHALL be a static value (literal, Const or constant expression) or a
static range `A..B` with A <= B. A Const keeps its own type, which SHALL match
the selector's; a literal-only expression SHALL be read in the selector's type
and SHALL fit it. A non-discrete selector such as String SHALL be an error.

#### Scenario: Static expressions and Const choices
- **WHEN** choices use constant expressions, Consts and Char literals
- **THEN** the case compiles (tests/semantic/valid/case-static-expressions.inox, tests/semantic/valid/case-const-expression-choice.inox, tests/semantic/valid/case-char-choices.inox)

#### Scenario: Wrong choices and selectors
- **WHEN** a choice is a variable, an inverted range, a literal that does not fit a Byte selector, a Const or Bool of another type, or the selector is a String
- **THEN** the compiler rejects it (tests/diagnostics/case-nonstatic-choice.inox, tests/diagnostics/case-inverted-range.inox, tests/diagnostics/case-literal-out-of-range.inox, tests/diagnostics/case-typed-const-mismatch.inox, tests/diagnostics/case-choice-type-mismatch.inox, tests/diagnostics/case-string-selector.inox)

### Requirement: Unless runs its body when the condition is false
`unless C` SHALL run its body exactly when C is false.

#### Scenario: Unless inside a loop
- **WHEN** `unless I mod 2 = 0` guards a body inside a for loop
- **THEN** the body runs for odd I only (tests/runtime/unless-lowering.inox)

### Requirement: Enum and Range coverage
A `case` on an Enum or Range selector SHALL compile without `else` exactly
when its choices cover every value of the type. A missing value SHALL be a
compile error that names it.

#### Scenario: Complete enum case
- **WHEN** a case on a TSuit names Club, Diamond, Heart and Spade without else
- **THEN** it compiles and runs (tests/runtime/enum-case.inox)

#### Scenario: Incomplete enum case
- **WHEN** a case on a TSuit omits Spade and has no else
- **THEN** the compiler rejects it naming Spade (tests/diagnostics/enum-case-incomplete.inox); a Range case missing 7..12 is rejected too (tests/diagnostics/range-case-incomplete.inox)
