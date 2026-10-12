## Purpose
Finite sets (CANON-13 Set): `Set<T>` over an Enum or finite Range, a value
type with mathematical set operations. Not a hash set.

## ADDED Requirements

### Requirement: Set base types
`Set<T>` SHALL require T to be an Enum or a finite Range. `Set<Integer>`,
`Set<Float>` and `Set<String>` SHALL be compile errors, and so SHALL a base
with more than 256 values.

#### Scenario: Invalid base
- **WHEN** a program declares `S Set<Integer>`
- **THEN** the compiler rejects it (test to add: tests/diagnostics/set-invalid-base.inox)

### Requirement: Sets are values, empty by default
A Set variable without initializer SHALL be empty. Assignment and by-value
passing SHALL copy the set.

#### Scenario: Default and copy
- **WHEN** a program declares `S Set<TSuit>`, copies it to T, adds Club to T
- **THEN** `Club in S` is false and `Club in T` is true (test to add: tests/runtime/sets.inox)

### Requirement: Membership, comparison and operations
`E in S` SHALL test membership; `=` and `#` SHALL compare sets; `<=` and `>=`
SHALL test subset and superset. `Union`, `Intersection`, `Difference`,
`SymmetricDifference`, `With` and `Without` SHALL return new sets.

#### Scenario: Operations
- **WHEN** a program builds sets with With and Union and tests them with in, =, <= and Difference
- **THEN** each result is the mathematical one (test to add: tests/runtime/sets.inox)

### Requirement: Set literal in a Set-typed context
`[A, B, C]` SHALL denote the set of those elements where a `Set<T>` is
expected (initializer, assignment, argument, return).

#### Scenario: Literal initializer
- **WHEN** a program writes `S Set<TSuit> := [Club, Heart]`
- **THEN** `Club in S` and `Heart in S` are true and `Spade in S` is false (test to add: tests/runtime/sets.inox)
