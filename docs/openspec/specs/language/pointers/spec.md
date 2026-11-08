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

#### Scenario: Jumps, handlers, ensure and Retry
- **WHEN** an owner is given away on a path through `leave`, `continue`, `until`, a handler, an `ensure` or `Retry`
- **THEN** a use it reaches is rejected (tests/diagnostics/pointer-moved-before-leave.inox, tests/diagnostics/pointer-moved-before-continue.inox, tests/diagnostics/pointer-moved-before-leave-nested.inox, tests/diagnostics/pointer-moved-before-leave-branch.inox, tests/diagnostics/pointer-moved-owner.inox, tests/diagnostics/pointer-moved-before-until.inox, tests/diagnostics/pointer-moved-before-until-in-while.inox, tests/diagnostics/pointer-moved-before-until-round.inox, tests/diagnostics/pointer-freed-before-exception.inox, tests/diagnostics/pointer-moved-before-raise.inox, tests/diagnostics/pointer-freed-in-inner-handler.inox, tests/diagnostics/pointer-freed-in-ensure-before-leave.inox, tests/diagnostics/pointer-freed-in-ensure-before-continue.inox, tests/diagnostics/pointer-freed-in-ensure-before-until.inox, tests/diagnostics/pointer-freed-in-ensure-one-branch.inox, tests/diagnostics/pointer-freed-in-nested-ensures.inox, tests/diagnostics/pointer-moved-before-retry.inox, tests/diagnostics/pointer-freed-in-ensure-before-retry.inox, tests/diagnostics/pointer-moved-before-retry-zero.inox), and a path that assigns it again is accepted (tests/runtime/pointers-moves-across-jumps.inox, tests/runtime/pointers-moves-across-until.inox, tests/runtime/pointers-moves-across-handlers.inox, tests/runtime/pointers-moves-across-ensure.inox, tests/runtime/pointers-moves-across-nested-ensures.inox, tests/runtime/pointers-moves-before-retry.inox, tests/runtime/pointers-moved-then-exit.inox)

#### Scenario: Owners received as ref mut
- **WHEN** a routine gives an owner received as `ref mut` away and returns, or a handler uses an owner passed as `ref mut` in its try
- **THEN** it is rejected (tests/diagnostics/pointer-refmut-freed-by-callee.inox, tests/diagnostics/pointer-refmut-moved-by-callee.inox, tests/diagnostics/pointer-refmut-freed-on-a-branch.inox, tests/diagnostics/pointer-refmut-freed-before-return.inox, tests/diagnostics/pointer-refmut-freed-before-exit.inox, tests/diagnostics/pointer-refmut-freed-in-ensure.inox, tests/diagnostics/pointer-refmut-nested-return.inox, tests/diagnostics/pointer-refmut-nested-exit.inox, tests/diagnostics/pointer-refmut-freed-through-intermediate.inox, tests/diagnostics/pointer-refmut-generic-consume.inox, tests/diagnostics/pointer-refmut-field-freed-by-callee.inox, tests/modules/refmut-callee-gives-away/Main.inox, tests/diagnostics/pointer-refmut-used-in-handler.inox, tests/diagnostics/pointer-refmut-used-in-ensure.inox, tests/diagnostics/pointer-refmut-transitive-exception.inox, tests/diagnostics/pointer-refmut-transitive-exception-inner.inox), and a routine that gives it back is accepted (tests/runtime/pointers-refmut-give-back.inox, tests/runtime/pointers-refmut-across-calls.inox, tests/runtime/pointers-refmut-nested-exits.inox, tests/modules/refmut-callee-gives-back/Main.inox)

#### Scenario: Owners received as ref mut and exceptions
- **WHEN** a routine gives away an owner it received as `ref mut` where an exception may leave it
- **THEN** it is rejected (tests/diagnostics/pointer-owner-field-lost-by-exception.inox, tests/diagnostics/pointer-refmut-freed-before-raise.inox, tests/diagnostics/pointer-refmut-freed-before-call.inox, tests/diagnostics/pointer-refmut-freed-before-method.inox, tests/diagnostics/pointer-refmut-freed-before-conversion.inox, tests/diagnostics/pointer-refmut-freed-before-file-write.inox, tests/diagnostics/pointer-refmut-raise-in-new-value.inox, tests/diagnostics/pointer-refmut-freed-before-invariant.inox, tests/diagnostics/pointer-refmut-freed-before-retry.inox, tests/diagnostics/pointer-refmut-freed-before-rethrow.inox, tests/diagnostics/pointer-refmut-typed-handler-not-proven.inox, tests/diagnostics/pointer-refmut-raise-in-handler.inox, tests/diagnostics/pointer-refmut-ensure-one-branch.inox, tests/diagnostics/pointer-refmut-nested-ensures-order.inox, tests/diagnostics/pointer-refmut-raise-in-ensure.inox, tests/diagnostics/pointer-refmut-generic-raise.inox, tests/diagnostics/pointer-refmut-recursive-raise.inox, tests/modules/refmut-raise-gives-away/Main.inox), and a routine that gives it back first is accepted (tests/runtime/pointers-refmut-exception-exits.inox, tests/modules/refmut-raise-gives-back/Main.inox)

#### Scenario: Run-time backstop
- **WHEN** a routine reads an owner that is empty (null, as Move and Free leave it), called from a native host
- **THEN** the program stops with "use of an owner after Move or Free", exit code 70, and the check is in the generated code (tests/native/owner-check.inox, tests/codegen/llvm-owner-check-smoke.inox)

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

### Requirement: Links are made only from writable places
`Link(X)` SHALL be an error when X, or the owner X names, is reached from a
read-only place (a `ref` parameter, a `Self` receiver that is not
`Self mut`, or `with` on one of them) through fields, elements and owners:
a Link is a capability to write and cannot be made from a read-only path.
A place reached through a Link, a mutable variable, State, a `ref mut`
parameter, `Self mut` or a value owner parameter may be linked.

#### Scenario: Links from read-only and writable places
- **WHEN** Links are taken from `ref` and `Self` paths and from writable ones
- **THEN** the first are rejected and the others run (tests/diagnostics/link-from-ref-owner.inox, tests/runtime/read-only-through-owners.inox)
