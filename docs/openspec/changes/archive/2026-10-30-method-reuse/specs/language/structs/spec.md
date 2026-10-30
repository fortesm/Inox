## ADDED Requirements

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
