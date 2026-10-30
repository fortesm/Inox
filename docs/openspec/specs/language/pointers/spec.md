# language/pointers Specification

## Purpose
Safe pointers without reference counting or a collector (CANON-25): one
owner `Own<T>` per heap object, moves checked at compile time, and `Link<T>`
references checked by generation on every use. Part of the Inox law,
detailing the constitution (`docs/openspec/specs/constitution`).

## Requirements

### Requirement: Owners and New
A heap object SHALL be made by `New(Value)`, which copies the value into a
new block and returns its owner `Own<T>`, or by `New(TStruct)`, with the
struct's defaults. `New` of a pointer, a slice or a File is an error.

#### Scenario: New
- **WHEN** a program makes objects of structs, arrays, Strings and Integers with New
- **THEN** they hold the values given (tests/runtime/pointers.inox, tests/runtime/pointers-more.inox)

### Requirement: An owner is never copied
An `Own<T>`, an `Option<Own<T>>`, and a struct or array that holds one SHALL
be moved, never copied: a store, an argument, a construction or a Return
takes a fresh value (`New`, `Move(X)`, a routine's result, a construction,
or `None`). A conditional expression of owners, `old` of an owner and a
pointer comparison with `=` are errors.

#### Scenario: Copies rejected
- **WHEN** `Q := P` copies an owner, or `old` copies one
- **THEN** it is rejected (tests/diagnostics/pointer-owner-copied.inox, tests/diagnostics/pointer-old-of-owner.inox)

### Requirement: Freeing
An owner SHALL free its object when `Free(P)` runs, when it is overwritten,
when its routine returns (also through an exception), or when its
declaration runs again; the objects it owns are freed with it, in a loop,
never by recursion. `None` assigned to an `Option<Own<T>>` frees its object.

#### Scenario: Freed once
- **WHEN** lists, nested owners, Strings inside objects and an exception leave routines
- **THEN** every object is freed exactly once and nothing leaks (tests/runtime/pointers.inox, tests/runtime/pointers-strings.inox)

### Requirement: Move analysis
A variable that gave its owner away with `Move` or `Free` SHALL not be used
on any path until it is assigned again: branches join, a loop must give the
owner back before its next round, a handler sees what the try body gave
away. Only a variable gives an owner away; an `Option<Own<T>>` place becomes
`None`. A path the analysis does not follow traps with `use of an owner after
Move or Free`.

#### Scenario: Analysis
- **WHEN** an owner is used after Move, freed twice, freed on a branch, moved in a loop, or a field owner is freed
- **THEN** it is rejected (tests/diagnostics/pointer-use-after-move.inox, tests/diagnostics/pointer-double-free.inox, tests/diagnostics/pointer-moved-on-a-branch.inox, tests/diagnostics/pointer-moved-in-loop.inox, tests/diagnostics/pointer-free-of-field.inox)

#### Scenario: Run-time backstop
- **WHEN** an owner given away before a `leave` is used after the loop
- **THEN** the program traps (tests/runtime/pointers-moved-owner.inox)

### Requirement: Links
`Link(X)` SHALL give a `Link<T>` to the object of an owner, to a variable of
the routine or of State, or to a field or element of one (its indices
without side effects). A Link is copied freely and born empty. Each use
compares the generation it saw with its storage's: a freed object, a reused
block, or a variable whose routine returned is never reached. `Alive(L)`
answers; a use traps with `use of a freed object or an empty Link`.

#### Scenario: Reaching storage
- **WHEN** Links reach variables, fields, elements, State and objects, and write through them
- **THEN** the writes change that storage (tests/runtime/pointers.inox, tests/runtime/pointers-more.inox)

#### Scenario: Dead Links
- **WHEN** a Link is used after Free, after its block is reused, after its variable's routine returned, or empty
- **THEN** the program traps (tests/runtime/pointers-freed-link.inox, tests/runtime/pointers-reused-block.inox, tests/runtime/pointers-escaped-link.inox, tests/runtime/pointers-empty-link.inox)

#### Scenario: Not supported yet
- **WHEN** a Link reaches a parameter, or a place whose index calls a routine
- **THEN** it is rejected (tests/diagnostics/pointer-link-to-parameter.inox, tests/diagnostics/pointer-link-side-effects.inox)

### Requirement: Access through pointers
`P.Field`, `P[I]`, `P.Method` and `with P` SHALL reach the object of an
`Own` or a `Link`; a `Self mut` method changes it. `Value(P)` is the whole
object, read (a copy) or written (the object stays, its Links stay alive).
`Same(A, B)` is true when both reach one live object. An `Option<Own<T>>` is
reached through `Link(X)`; reading its fields directly is an error.

#### Scenario: Access
- **WHEN** methods, fields, elements and Value are used through owners and Links
- **THEN** they act on the object (tests/runtime/pointers.inox, tests/runtime/pointers-more.inox)

#### Scenario: Rejected access
- **WHEN** pointers are compared with `=` or an Option's field is read directly
- **THEN** it is rejected (tests/diagnostics/pointer-compared.inox, tests/diagnostics/pointer-option-direct.inox)

### Requirement: Pointer types
`Own<T>`, `Link<T>` and `Option<Own<T>>` SHALL be types of variables,
fields, parameters, results, array elements (`Link` and `Option` only:
every element is born empty) and State. `Option<X>` for any X other than
an owner is an error until `Option<T>`, after methods of generic
structs (ROADMAP); a pointer to a pointer or to a slice is an
error. An owner variable is born with New or Move; a struct holding an
`Own` field has no default value; `None` needs a declared type.

#### Scenario: Types
- **WHEN** arrays of owners, Option of a value, pointers to pointers, owners without New, structs without their owner, or untyped None are written
- **THEN** each is rejected (tests/diagnostics/pointer-array-of-owners.inox, tests/diagnostics/pointer-option-of-value.inox, tests/diagnostics/pointer-to-pointer.inox, tests/diagnostics/pointer-owner-without-new.inox, tests/diagnostics/pointer-struct-owner-default.inox, tests/diagnostics/pointer-none-untyped.inox)

### Requirement: Owners in routines and contracts
A value parameter of an owner type SHALL belong to the routine, which frees
it at every exit unless it gives it away. A contract's routine gives it on
to its body, so a `pos` reads it only through `old X.Field`.

#### Scenario: Contracts
- **WHEN** a routine with pre and pos takes an owner and a Link, and a pos reads the owner directly
- **THEN** the first runs and frees the owner (tests/runtime/pointers-contracts.inox); the second is rejected (tests/diagnostics/pointer-pos-reads-owner.inox)
