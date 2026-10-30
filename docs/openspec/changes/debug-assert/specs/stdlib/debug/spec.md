## Purpose
`Std.Debug`: `Assert`, a check that stops the program when its condition is
false. Part of the Inox law, detailing the constitution (CANON-18,
CANON-19).

## ADDED Requirements

### Requirement: Assert is visible with Use Std.Debug
`Assert` SHALL exist only in a program that writes `Use Std.Debug`; without
it the name is free for the program's own routines.

#### Scenario: Without the module
- **WHEN** a program calls Assert without `Use Std.Debug`
- **THEN** it is rejected naming the module (tests/diagnostics/assert-without-use.inox)

#### Scenario: Own routine
- **WHEN** a program without the module declares and calls its own Assert
- **THEN** its routine runs (tests/runtime/assert-name-free.inox)

### Requirement: Assert checks a Bool condition
`Assert(Condition)` and `Assert(Condition, Message)` SHALL take a Bool and an
optional String, evaluated in order, and do nothing when the condition is
true.

#### Scenario: Condition holds
- **WHEN** both forms run with true conditions
- **THEN** the program continues (tests/runtime/debug-assert-holds.inox)

#### Scenario: Not a Bool
- **WHEN** the condition is an Integer
- **THEN** it is rejected (tests/diagnostics/assert-condition-bool.inox)

### Requirement: A failed Assert is a trap
When the condition is false the program SHALL stop at once with `Inox runtime
error: assertion failed` (followed by `: Message` when given) and exit status
70, like any runtime fault: no handler, `Else` or `ensure` runs. Assert is
never compiled out.

#### Scenario: Not catchable
- **WHEN** an Assert fails inside try with a plain except and ensure
- **THEN** only the output before it and the trap appear (tests/runtime/debug-assert-fails.inox)

#### Scenario: With a message
- **WHEN** `Assert(Total > 0, "Total must be positive")` fails
- **THEN** the program prints `Inox runtime error: assertion failed: Total must be positive` and stops (tests/runtime/debug-assert-message.inox)
