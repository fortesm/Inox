## MODIFIED Requirements

### Requirement: Membership, comparison and operations
`E in S` SHALL test membership; `=` and `#` SHALL compare sets; `<=` and `>=`
SHALL test subset and superset. `Union`, `Intersection`, `Difference`,
`SymmetricDifference`, `Include` and `Exclude` SHALL return new sets and never
change their arguments (r3.81: they replaced `With` and `Without`, whose use
is rejected with the new names). A call of one of them as a statement drops
the new set and SHALL be rejected, so a Pascal habit (`Include(S, X)`) is
caught.

#### Scenario: Operations
- **WHEN** a program builds sets with Include, Exclude and Union and tests them with in, =, <= and Difference
- **THEN** each result is the mathematical one (tests/runtime/sets.inox)

#### Scenario: A set function as a statement, and the old names
- **WHEN** a program writes `Include(Seen, Club)` as a statement, `With(S, X)` or `Without(S, X)`
- **THEN** the compiler rejects each, showing `Seen := Include(Seen, ...)` or the new name (tests/diagnostics/set-function-statement.inox, tests/diagnostics/set-with-renamed.inox, tests/diagnostics/set-without-renamed.inox)
