## Why

Two defects of `with` (CANON-11), found on 2026-10-16 while running the
maintainer's example (`With TCustomer` ... `With TCustomer.Comments`):

1. A target other than a variable (`with Customer.Comments`, `with A[I]`,
   a field through `Own`/`Link`, `Value(L)`) passed the analyzer and was
   "not yet implemented" in the backend.
2. `with TCustomer` (a type) was rejected only at the first `.Member`, with
   a message that did not name the problem ("unknown field or
   zero-argument method Name in").

Marcelo Fortes, 2026-10-16, on the proposed fixes (evaluate the target once
and act on its storage, as Visual Basic; check it at every use through a
pointer; reject a type at the `with` line with a hint): agreed, to be
implemented.

Status: implemented. Risk: low (lowering of an existing construct and a
diagnostic).

## What Changes

- CANON-11 states what the code now does: the target, a variable or a
  value (never a type), is evaluated once on entry (indices and calls
  included); `.Member` acts on its storage, never on a copy; through a
  pointer, the object holding it is checked at each use, so a `Free` inside
  the body stops the program at the next `.Member` (CANON-25).
- A type, a module or a routine without a result as target is a compile
  error at the `with` line; for a type the message shows the named
  construction (`X := TClient(Field := Value, ...)`).
- A value that is not a place (a function result) stays read-only, as
  before.
