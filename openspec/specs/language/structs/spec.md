# structs Specification

## Purpose
Struct values (CANON-9): nominal value types with fields, defaults, named
construction, initializers and nesting. Associated methods are covered by the
canon until they migrate. Mirror of the canon (G7): the canon stays the law
until Marcelo Fortes approves this capability's migration.

## Requirements

### Requirement: Type-default initialization
A struct variable declared without `:=` SHALL start with each field at its
declared default, or at its type's zero when it has none. A nested struct
field SHALL get its own type defaults.

#### Scenario: Defaults and zeros
- **WHEN** `P TPoint` is declared where `FX Integer := 1` and `FY Integer` has no default
- **THEN** `P.FX` is 1 and `P.FY` is 0 (tests/runtime/struct-defaults.inox)

#### Scenario: Defaults of every scalar type
- **WHEN** a struct has Integer, Int8, Byte, Natural, Float, Float32 and Bool fields with literal defaults, negative and hexadecimal ones included
- **THEN** a type-default variable holds those defaults (tests/runtime/struct-defaults.inox)

#### Scenario: Defaults that do not fit
- **WHEN** a default is `Byte := 300`, `Natural := -1` or `Float := 1`
- **THEN** the compiler rejects it (tests/diagnostics/struct-default-out-of-range.inox, tests/diagnostics/struct-default-negative-natural.inox, tests/diagnostics/struct-float-default-integer.inox)

### Requirement: Named construction is an expression
`TName(Field := Expr, ...)` SHALL build a struct value from named fields in any
order, using defaults for omitted fields. It SHALL be usable wherever a value of
the struct type is expected. Unknown, duplicate, positional and omitted scalar
fields without default SHALL stay compile errors (CANON-9).

#### Scenario: Construction in several positions
- **WHEN** a program writes `S := TPoint(FY := 4, FX := 3)`, passes `TPoint(FX := 1, FY := 2)` as an argument and returns a constructed value
- **THEN** each value has the named fields (tests/runtime/struct-construction.inox)

#### Scenario: Construction errors
- **WHEN** a construction names an unknown field, repeats a field or omits a scalar field without default
- **THEN** the compiler rejects it (tests/diagnostics/construction-unknown-field.inox, tests/diagnostics/construction-duplicate-field.inox, tests/diagnostics/construction-omitted-scalar.inox)

### Requirement: Struct initializers copy the value
`Q TPoint := Expr` and `P, Q TPoint := Expr` SHALL evaluate Expr once and give
each name its own copy. Later changes to one variable SHALL NOT affect another.

#### Scenario: Initializer from a call
- **WHEN** `Q TPoint := Shift(P)` runs and then `Q.FX` changes
- **THEN** P is unchanged (tests/runtime/struct-initializers.inox)

#### Scenario: Grouped initializer runs once
- **WHEN** `P, Q TPoint := Make` runs where Make prints a line
- **THEN** the line prints once and P and Q are independent copies (tests/runtime/struct-initializers.inox)

### Requirement: Nested struct fields
A struct field SHALL be allowed to have a struct type declared anywhere in the
module's Type sections. A struct that contains itself, directly or through
other structs, SHALL be a compile error.

#### Scenario: Field paths through nested structs
- **WHEN** `L.FB := Q` and `L.FB.FX := 7` run on a `TLine` with `FA, FB TPoint` fields
- **THEN** `L.FB.FX` is 7 and `L.FA` keeps its defaults (tests/runtime/struct-nested.inox)

#### Scenario: Methods and ref mut on nested fields
- **WHEN** a program calls `L.FA.Move(2)` and `Bump(L.FB)` with a `ref mut` parameter
- **THEN** the nested fields change (tests/runtime/struct-nested.inox)

#### Scenario: Recursive struct
- **WHEN** `TNode` has a field of type `TNode`
- **THEN** the compiler rejects it with "struct contains itself: TNode" (tests/diagnostics/struct-recursive.inox, tests/diagnostics/struct-recursive-indirect.inox)
