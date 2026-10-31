## Why

Today `ref` and a plain `Self` protect the variable and its plain fields
but not the objects its owners own: with `ref H THolder` and
`FChild Own<TNode>`, `H.FChild.FValue := 1` is accepted, a `Self` method
writes `Self.FChild.FValue`, and `Link(H.FChild)`, `Link(Self.FChild)` and
`Link(P)` with `ref P Own<TNode>` give a writable Link to the caller's
object (probes 8-10 of the exploration `concurrency`). A read-only
reference that lets its callee change what it owns contradicts ADR-0016
("`ref X T` gives read-only access") and CANON-9 ("`Self` only reads its
receiver"). The exploration `concurrency` needs the fix (its decision D8),
but the fix stands on its own, in sequential programs.

Approval: Marcelo Fortes approved D8 as the semantic direction on
2026-10-31 (exploration `concurrency`, section 26), then approved this
design, classified it as high risk and authorized its implementation on
2026-10-31, within this change's scope only.

Touches: ADR-0016 and CANON-10 Parameters and mutability
(`language/ref-parameters`), CANON-9 receivers (`language/structs`),
CANON-25 access through pointers and Links (`language/pointers`).

Risk: high (it changes when the compiler lets a program write memory
owned through `Own<T>` and when it lets a program make a `Link<T>`: the
memory-ownership clause of the proposal rules). No lowering, no runtime,
no ABI change.

## What Changes

1. **Read-only is deep through owners.** A place reached from a `ref`
   parameter or from the plain `Self` of a method, through fields,
   elements and owners (`Own<T>`, `Option<Own<T>>`) at any depth, is
   read-only: no assignment (also compound), no `ref mut` argument, no
   `Self mut` call, no `Value(P) := ...`, no `Free`/`Move` of an
   `Option<Own>` place, and inside `with` on such a place the same. Found
   while implementing, and in scope: `Free(P)` and `Move(P)` of a
   `ref P Option<Own<T>>` parameter itself were accepted and emptied the
   caller's variable; they are rejected.
2. **No Link from a read-only place.** `Link(X)` of such a place, or of
   such an owner (`Link(P)` with `ref P Own<T>`), is an error.
3. **Unchanged:**
   - `ref mut` parameters and `Self mut` receivers write through owners as
     today;
   - a value parameter of an owner type (or of a struct holding one)
     belongs to the routine: its objects stay writable and linkable, as
     today (`CountUp(List Own<TNode>)` keeps `Link(List)`);
   - a path through a `Link` is not read-only: a Link is a write
     capability by design (CANON-25), wherever it was reached from;
   - mutable locals and State.
4. **Diagnostics** name the read-only root and the fix, e.g.
   `H.FChild.FValue is read-only: H is a ref parameter and H.FChild is
   owned through it; declare ref mut H to change what it owns (ADR-0016)`.

## Out of scope

`do`, `forall`, threads, scheduling, runtime changes, frozen State,
transferability, re-keying: everything else of the exploration
`concurrency`. No new reserved word, type or construct. The constitution
text (CANON-9, CANON-25 and the ADR-0016 summary) changes only after the
implementation confirms the semantics, in this change's last task.

## Behavior that must not change

Every existing test and example: by the inspection of section 24 of the
exploration, none writes through an owner from `ref` or `Self`, or links
from them. The prototype confirms it on the whole suite and the fuzz
corpus.
