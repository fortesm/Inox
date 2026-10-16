## MODIFIED Requirements

### Requirement: Comparison results combine like any Bool
The result of any comparison SHALL be a Bool usable with `not`, `and`,
`xor`, `or`, `if`/`elif`, `unless`, `while`, `until`, the conditional
expression, and as a value stored or compared with `=`/`#`.

#### Scenario: Combinations
- **WHEN** a program uses String comparisons under `not`, `and`, `or`, `xor`, `unless`, `elif`, `while`, `until`, `IIf(C, A, B)` and stores one in a Bool variable
- **THEN** each gives the expected result (tests/runtime/strings-ordering.inox)
