## ADDED Requirements

### Requirement: Routine contracts
A routine SHALL accept `pre` and `pos` lines before its statements:
`pre Condition` or `pre Condition, "message"`. Every `pre` is checked, in
order, each time the routine is called; every `pos` each time it returns
normally (not when an exception ends it). A condition is Bool. Main has no
contracts.

#### Scenario: Pre and pos hold
- **WHEN** methods, functions, recursive functions and a method of String run within their contracts
- **THEN** the program runs normally (tests/runtime/contracts.inox)

#### Scenario: Pre and pos broken
- **WHEN** a caller breaks a pre, or a routine breaks its pos
- **THEN** the program traps with the routine, the condition and the message (tests/runtime/contract-pre-violated.inox, tests/runtime/contract-pos-violated.inox)

#### Scenario: Where and what
- **WHEN** a clause follows a statement, is not Bool, has a message that is not a literal, or is on Main
- **THEN** it is rejected (tests/diagnostics/contract-pre-after-statements.inox, tests/diagnostics/contract-not-bool.inox, tests/diagnostics/contract-message-not-literal.inox, tests/diagnostics/contract-on-main.inox)

### Requirement: Old and Result
In a `pos`, `old X` SHALL be X as evaluated when the routine started, after
its `pre`, and `Result` the value the function returns. `old` binds to one
operand (`old N + 1` is `(old N) + 1`).

#### Scenario: Result checked
- **WHEN** a function returns a value its pos refuses
- **THEN** the program traps (tests/runtime/contract-result-violated.inox)

#### Scenario: Misplaced
- **WHEN** old is in a pre, or Result in a pre or a subroutine
- **THEN** it is rejected (tests/diagnostics/contract-old-in-pre.inox, tests/diagnostics/contract-result-in-pre.inox, tests/diagnostics/contract-result-in-subroutine.inox)

### Requirement: Struct invariants
`invariant Condition` lines in a Struct SHALL hold, with the invariants of
the structs it reuses, on a value just constructed or declared with its
defaults, before and after each method called on it from outside (a call on
the bare `Self` is not checked), and after a field or a ref mut argument is
changed from outside its own methods, also through `with`.

#### Scenario: Broken invariant
- **WHEN** a method, a field write, a ref mut argument or a construction leaves the invariant false
- **THEN** the program traps (tests/runtime/contract-invariant-method.inox, tests/runtime/contract-invariant-field.inox, tests/runtime/contract-invariant-ref-mut.inox, tests/runtime/contract-invariant-construction.inox)

#### Scenario: Unknown member
- **WHEN** an invariant names a field the struct does not have
- **THEN** it is rejected (tests/diagnostics/contract-invariant-unknown-field.inox)

### Requirement: Override contracts
An Override SHALL inherit the contracts of the methods it replaces: its
precondition is the inherited one or else its `pre else` lines; its
postcondition is the inherited one and then its `pos then` lines. Inherited
clauses are evaluated on the part (`Self` is `Self.TPart`).

#### Scenario: Inherited and own
- **WHEN** an Override is called within the inherited pre, within its own, and outside both
- **THEN** the first two run and the last traps naming both alternatives (tests/runtime/contract-override-pre.inox, tests/runtime/contracts.inox)

#### Scenario: Keywords
- **WHEN** an Override writes pre, or a routine that is not one writes pre else
- **THEN** it is rejected (tests/diagnostics/contract-plain-pre-in-override.inox, tests/diagnostics/contract-pre-else-not-override.inox)

### Requirement: Loop contracts
The first lines of a loop body SHALL accept `invariant` lines, checked at the
start of every iteration and, for while and repeat, when the loop ends, and
one `variant` (while and repeat only), an integer that is >= 0 and smaller
than at the previous iteration.

#### Scenario: Loops
- **WHEN** loops keep their invariants and variants, and when they break them
- **THEN** the first run (tests/runtime/contracts.inox) and the others trap (tests/runtime/contract-loop-invariant.inox, tests/runtime/contract-loop-variant.inox, tests/runtime/contract-loop-variant-negative.inox)

#### Scenario: Misplaced loop clauses
- **WHEN** a for has a variant, a loop has two, or invariant appears among statements
- **THEN** it is rejected (tests/diagnostics/contract-variant-in-for.inox, tests/diagnostics/contract-two-variants.inox, tests/diagnostics/contract-invariant-misplaced.inox)

### Requirement: Contracts only read and always trap
A contract SHALL not change the program's state: a Self mut method, a ref mut
argument, a file function or input in a contract is an error. A broken
contract SHALL be a trap (CANON-19, exit status 70): try, except and ensure
never see it; contracts are always checked.

#### Scenario: Read-only
- **WHEN** a contract calls a Self mut method or a routine with a ref mut parameter
- **THEN** it is rejected (tests/diagnostics/contract-calls-self-mut.inox, tests/diagnostics/contract-calls-ref-mut.inox)

#### Scenario: Not an exception
- **WHEN** a pre fails inside try with a plain except
- **THEN** the handler does not run and the program traps (tests/runtime/contract-pre-violated.inox)
