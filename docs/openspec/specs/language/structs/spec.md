# structs Specification

## Purpose
Struct values (CANON-9): nominal value types with fields, defaults, named
construction, initializers and nesting, method reuse with `Override`, and
methods of built-in value types. Associated methods are specified in the
constitution (CANON-9). Part of the Inox law, detailing the constitution
(`docs/openspec/specs/constitution`).

## Requirements

### Requirement: Type-default initialization
A struct variable declared without `:=` SHALL start with each field at its
declared default, or at its type's zero when it is a scalar, String, Set or
pointer without one. A nested struct field SHALL get its own type defaults.
A struct whose type defaults are not complete (CANON-5, r3.82) SHALL NOT be
declared without `:=`.

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

### Requirement: Fields without an implicit value are required
A field of an enum, Char, array or type-parameter type without a default, and a
field of a struct without complete type defaults, SHALL be given in every
construction; a struct with such a field (directly, nested or in a part)
SHALL NOT take its type defaults, in a local, in State or by `New(TName)`
(r3.82).

#### Scenario: Declared without a value
- **WHEN** `Q TPacket` with `FData Array[1..4] Byte`, `C TCard` with `FSuit TSuit`, `R TRoute` whose `FFrame TFrame` has such a field, a part with one, a State variable, or `New(TPacket)`
- **THEN** the compiler rejects each, naming the field (tests/diagnostics/struct-array-field-required.inox, tests/diagnostics/enum-field-without-default.inox, tests/diagnostics/char-field-without-default.inox, tests/diagnostics/nested-required-field.inox, tests/diagnostics/part-required-field.inox, tests/diagnostics/state-required-field.inox, tests/diagnostics/new-required-field.inox)

#### Scenario: Omitted in the construction
- **WHEN** a construction omits such a field
- **THEN** the compiler rejects it (tests/diagnostics/struct-array-field-omitted.inox, tests/diagnostics/enum-field-omitted.inox, tests/diagnostics/nested-field-omitted.inox)

#### Scenario: Given or defaulted
- **WHEN** they have defaults (`:= Approved`, `:= Fill(7)`) or are given in the construction, also nested
- **THEN** the values hold them (tests/runtime/required-fields.inox, tests/runtime/struct-required-fields.inox)

### Requirement: Method reuse
`TName Struct (TPart, ...)` SHALL reuse each struct listed: the part is a
field named after its type (`A.TPerson`), written first, and the struct also
offers the part's fields and methods directly, at every level. A reused
method runs on its part: `A.UpdateName(X)` is `A.TPerson.UpdateName(X)`.
A struct is never its part: there is no subtyping. Construction names the
part: `TSubscription(TPerson := P)`.

#### Scenario: Fields and methods through the parts
- **WHEN** a struct reuses one that reuses a third, and the program calls and assigns their members directly, through `with` and by the full path
- **THEN** every member is reached and a copy stays independent (tests/runtime/method-reuse.inox, examples/method-reuse.inox)

#### Scenario: Parts hold Strings
- **WHEN** reused String fields go through arrays, State, parameters, results and an exception
- **THEN** the output is right and no String leaks (tests/runtime/strings-method-reuse.inox)

#### Scenario: Construction names the part
- **WHEN** a construction names a field of a part
- **THEN** it is rejected with advice to construct the part (tests/diagnostics/reuse-construct-part-field.inox)

### Requirement: Reused names have one path
Every member a struct offers SHALL be reachable by exactly one path. It is
an error at the struct's declaration when a part is not a struct, a name is
reached through two parts, a field of the struct has a reused name, or a
method has the name of a reused field; a struct that reuses itself contains
itself.

#### Scenario: Ambiguous or hidden names
- **WHEN** two parts bring the same field, one struct is reached twice, a field hides a reused one, a method takes a reused field's name, a part is Integer, or two structs reuse each other
- **THEN** each is rejected (tests/diagnostics/reuse-name-two-paths.inox, tests/diagnostics/reuse-diamond.inox, tests/diagnostics/reuse-field-hides-part.inox, tests/diagnostics/reuse-method-named-like-field.inox, tests/diagnostics/reuse-part-not-struct.inox, tests/diagnostics/reuse-recursive.inox)

#### Scenario: No empty parentheses
- **WHEN** a struct is declared `TName Struct ()`
- **THEN** it is rejected (tests/diagnostics/reuse-empty-parentheses.inox)

### Requirement: Override replaces a reused method
A method with the name of a reused method SHALL be marked `Override`
(`TSubscription.Title(Self) String Override :`) and keep its signature: the
parameters after `Self`, their modes, the result and `Self` or `Self mut`.
It replaces the reused methods for callers of the struct; they stay
reachable by their path. `Override` on anything else is an error.

#### Scenario: Replace one or two reused methods
- **WHEN** a struct overrides a method of one part, and another overrides a method that two parts bring
- **THEN** callers of the struct get the override and the paths reach the parts' methods (tests/runtime/method-reuse.inox)

#### Scenario: Override misused
- **WHEN** a reused name is redefined without Override, Override replaces nothing, changes the signature, marks a plain routine or a built-in type's method
- **THEN** each is rejected (tests/diagnostics/reuse-method-without-override.inox, tests/diagnostics/reuse-override-nothing.inox, tests/diagnostics/reuse-override-signature.inox, tests/diagnostics/reuse-override-not-method.inox, tests/diagnostics/reuse-override-builtin.inox)

### Requirement: Methods of built-in types
A built-in value type SHALL accept methods declared like a struct's:
`String.UpdateStr(Self mut, Value String) :`, `Integer.Twice(Self) Integer
:`. `Self` is a copy of the value; `Self mut` is the caller's variable and
needs a writable receiver. A method belongs to its exact type. Arrays and
File have no methods.

#### Scenario: Helpers on fields and values
- **WHEN** methods of Integer, Bool, Float, Byte and String are called on variables, fields and expressions
- **THEN** they return and change the expected values (tests/runtime/builtin-type-methods.inox)

#### Scenario: Self mut on a literal
- **WHEN** a Self mut method of String is called on a literal
- **THEN** it is rejected (tests/diagnostics/self-mut-on-literal.inox)

### Requirement: Receivers are written only when writable
`Self` SHALL only read its receiver, and a field SHALL be assigned only
through a mutable variable, `Self mut` or a `ref mut` parameter, also inside
`with`. A `Self mut` method needs a writable receiver, a field of `Self mut`
may be passed as `ref mut`, and a `ref mut` argument may not reach the
receiver's variable (ADR-0016).

#### Scenario: Read-only receivers and parameters
- **WHEN** a method writes through `Self`, a routine writes a field of a value parameter or through `with` on it, calls a Self mut method on it, or passes the receiver's field as ref mut
- **THEN** each is rejected (tests/diagnostics/self-readonly-assignment.inox, tests/diagnostics/parameter-field-assignment.inox, tests/diagnostics/with-parameter-assignment.inox, tests/diagnostics/self-mut-on-parameter.inox, tests/diagnostics/ref-mut-aliases-receiver.inox)

#### Scenario: A field of Self mut by reference
- **WHEN** a Self mut method passes one of its fields to a ref mut parameter
- **THEN** the field changes (tests/runtime/strings-method-reuse.inox)
