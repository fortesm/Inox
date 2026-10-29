# language/sets Specification

## Purpose
Finite sets (CANON-13 Set): `Set<T>` over an Enum or finite Range, a value
type with mathematical set operations. Not a hash set.

## Requirements

### Requirement: Set base types
`Set<T>` SHALL require T to be an Enum or a finite Range. `Set<Integer>`,
`Set<Float>` and `Set<String>` SHALL be compile errors, and so SHALL a base
with more than 256 values.

#### Scenario: Invalid base
- **WHEN** a program declares `S Set<Integer>`
- **THEN** the compiler rejects it (tests/diagnostics/set-invalid-base.inox); a base of 300 values is rejected (tests/diagnostics/set-too-large.inox)

### Requirement: Sets are values, empty by default
A Set variable without initializer SHALL be empty. Assignment and by-value
passing SHALL copy the set.

#### Scenario: Default and copy
- **WHEN** a program declares `S Set<TSuit>`, copies it to T, adds Club to T
- **THEN** `Club in S` is false and `Club in T` is true (tests/runtime/sets.inox)

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

### Requirement: Set literal in a Set-typed context
`[A, B, C]` SHALL denote the set of those elements where a `Set<T>` is
expected (initializer, assignment, argument, return).

#### Scenario: Literal initializer
- **WHEN** a program writes `S Set<TSuit> := [Club, Heart]`
- **THEN** `Club in S` and `Heart in S` are true and `Spade in S` is false (tests/runtime/sets.inox)

### Requirement: Sets compare without order
`<` and `>` SHALL NOT apply to sets, and the elements of a set literal SHALL
be values of its base.

#### Scenario: Order and elements
- **WHEN** a program writes `S < S`, or `[Club, 3]` for a Set<TSuit>
- **THEN** the compiler rejects both (tests/diagnostics/set-order-compare.inox, tests/diagnostics/set-element-type.inox)

### Requirement: Sets over a Char Range
`Set<TLetter>` SHALL be a set over a Char Range of at most 256 values, with
Char literals as elements, `in`, the comparisons and the six functions
(change char-values).

#### Scenario: A set of letters
- **WHEN** a program builds `Vowels Set<TLetter> := ['A', 'E', 'I', 'O', 'U']`, includes 'Y' and tests membership
- **THEN** each test follows the set (tests/runtime/chars.inox)
