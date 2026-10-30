## MODIFIED Requirements

### Requirement: State of struct and array type
A State variable SHALL be allowed to have a struct or array type. A struct
without an initializer SHALL start at its type defaults; an array SHALL have
one (`Fill(Value)`, r3.81). Initializers SHALL run in source order with the
scalar initializers.

#### Scenario: Struct and array State
- **WHEN** a module declares `Origin TPoint := TPoint(FX := 1, FY := 2)`, `Counts Array[1..3] Integer := Fill(0)` and `Grid Array[1..2, 1..2] TPoint := Fill(Home)` in State, and routines update them
- **THEN** every routine sees the updated values (tests/runtime/state-aggregates.inox)
