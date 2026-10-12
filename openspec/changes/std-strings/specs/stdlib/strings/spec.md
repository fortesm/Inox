## Purpose
`Std.Strings`: String functions beyond the comparison operators, by Unicode
rules and independent of any one human language (PLANNED, canon v3.57).

## ADDED Requirements

### Requirement: Language-neutral by default
Every `Std.Strings` function SHALL follow Unicode rules that belong to no
single human language (default case mapping and folding, White_Space,
normalization, root collation order). A language-specific behavior SHALL
only be available through an explicit, separate call.

#### Scenario: Case-insensitive comparison
- **WHEN** a program compares "ÁGUA" and "água" with `SameText`
- **THEN** they are equal, with no locale set (test to add with the implementation)

#### Scenario: Neutral alphabetical order
- **WHEN** a program collates "água", "agua" and "azul"
- **THEN** "agua" and "água" sort together, before "azul" (test to add with the implementation)
