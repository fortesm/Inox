## ADDED Requirements

### Requirement: Case on a String
A `case` SHALL accept a String selector (Marcelo Fortes, 2026-10-19). Each
choice SHALL be a String literal or a String Const, compared byte for byte
as `=` compares; ranges SHALL be rejected; the same String twice SHALL be an
overlap; `else` SHALL be required, since no choices cover every String.

#### Scenario: Commands
- **WHEN** a program selects on `Trim(Line)` with `"start" | "go"`, `"stop" | "halt" | Quit`, `"" | "\t"`, `"café"` and `else`, and on a concatenation
- **THEN** each line runs its arm, case and accents count, and the selector is evaluated once (tests/runtime/case-strings.inox)

#### Scenario: String case errors
- **WHEN** a String case has no else, a range, the same String twice (also through a Const), a variable or an integer as a choice
- **THEN** the compiler rejects each (tests/diagnostics/case-string-without-else.inox, tests/diagnostics/case-string-range.inox, tests/diagnostics/case-string-duplicate.inox, tests/diagnostics/case-string-not-static.inox, tests/diagnostics/case-string-integer-choice.inox)

## MODIFIED Requirements

### Requirement: Case selects one arm by static choices
A `case Expression` statement SHALL evaluate its selector exactly once and run
the first arm whose choices contain the value. A choice SHALL be a static value
or a static range `A..B`; alternatives SHALL be separated by `|` (a comma is a
compile error). The selector SHALL be discrete (an integer type, Bool, Char,
an enum or a range) or a String (change case-strings).

#### Scenario: Values, ranges, alternatives and negative ranges
- **WHEN** a program runs `case N` with arms `0`, `1..Limit | 20`, `-5..-1` and `else`
- **THEN** each N selects exactly the matching arm (tests/runtime/case-lowering.inox)

#### Scenario: The selector is evaluated once
- **WHEN** the selector is a call with a visible side effect
- **THEN** the side effect happens once (tests/runtime/case-lowering.inox)

#### Scenario: Comma between alternatives
- **WHEN** a choice list is written `1, 2`
- **THEN** the compiler rejects it (tests/diagnostics/case-comma-separator.inox)

### Requirement: Choices are static and of the selector's type
A choice SHALL be a static value (literal, Const or constant expression) or a
static range `A..B` with A <= B. A Const keeps its own type, which SHALL match
the selector's; a literal-only expression SHALL be read in the selector's type
and SHALL fit it. A selector that is neither discrete nor a String, such as
a Float, SHALL be an error.

#### Scenario: Static expressions and Const choices
- **WHEN** choices use constant expressions, Consts and Char literals
- **THEN** the case compiles (tests/semantic/valid/case-static-expressions.inox, tests/semantic/valid/case-const-expression-choice.inox, tests/semantic/valid/case-char-choices.inox)

#### Scenario: Wrong choices and selectors
- **WHEN** a choice is a variable, an inverted range, a literal that does not fit a Byte selector, a Const or Bool of another type, or the selector is a Float
- **THEN** the compiler rejects it (tests/diagnostics/case-nonstatic-choice.inox, tests/diagnostics/case-inverted-range.inox, tests/diagnostics/case-literal-out-of-range.inox, tests/diagnostics/case-typed-const-mismatch.inox, tests/diagnostics/case-choice-type-mismatch.inox, tests/diagnostics/case-float-selector.inox)
