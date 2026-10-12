## ADDED Requirements

### Requirement: State of struct and array type
A State variable SHALL be allowed to have a struct or array type. Without an
initializer it SHALL start at its type defaults; with one, the initializer SHALL
run in source order with the scalar initializers.

#### Scenario: Struct and array State
- **WHEN** a module declares `Origin TPoint := TPoint(FX := 1, FY := 2)` and `Counts Array[1..3] Integer` in State, and routines update both
- **THEN** every routine sees the updated values (test to add: tests/runtime/state-aggregates.inox)
