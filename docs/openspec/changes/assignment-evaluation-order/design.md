# Design — order of evaluation of assignments

Status: approved with amendments (Marcelo Fortes, 2026-11-04), recorded
below; the implementation follows this text.

## What happens today

Code generation (`FunctionEmitter::emitLocalAssignment`,
`codegen/FunctionEmitterAggregates.cpp`) computes the address of the
target first and evaluates the right-hand side after it, on every path
but a plain variable: an element (`emitElementAddress`, then R), a field
(`emitMemberAddress`, then R), `Value(P)` (`emitPlace`, then R), a String
append (`emitStringAppend`: `emitPlace`, then the parts). An address that
crosses an owner (`H.FChild.FV`) is the old object's once R replaces the
owner. Measured (2026-11-04, Debug, `repro/`):

| Program | Prints | Under the rule below |
|---|---|---|
| `assignment-target-owner-replaced` (`H.FChild.FV := Drop`) | `1 10`: the store lands in K's object | `10 1` |
| `compound-assignment-target-owner-replaced` (`H.FChild.FV += Drop`) | `1 11` | `11 1` |
| `compound-assignment-value-before-target` (`V[I] += Shift`, Shift sets I to 2) | `110 200 2` | `100 210 2` |
| `compound-assignment-reference-index` (the same through `ref mut I`) | `110 200 2` | `100 210 2` |
| `assignment-target-invalidated-by-index` (`H.FChild.FV[ChangeOwner] := 10`) | `1 10`: K's object written | stops: "use of a freed object or an empty Link" |
| `read-place-invalidated-by-index` (`X := H.FChild.FV[ChangeOwner]`) | `7 1 7`: X read from K's object | stops, as above |
| `assignment-target-trap-after-value` (`V[I] := Shift`, Shift prints and sets I out of range) | `value evaluated`, `not reached` | `value evaluated`, then "array index out of bounds" |

The last three show that the order between the two sides is not enough:
`emitElementAddress` reaches the base (through `H.FChild`) before it
evaluates the index, so a call in the index invalidates the address being
built, on the target's side and on a read alike.

`L op= R` is turned by phase 2b into `L := L op R`, the target cloned
(`AstNormalizer::normalizeCompound`, marked compound); that the two copies
name one element holds only because codegen evaluates both before R. The
index of a compound target is evaluated twice (two range checks).

The same pattern exists outside assignments (Q1, a change of its own): a reference
argument's address is computed before a later argument;
`Store(H.FChild.FV, Drop)`, with `Store(ref mut X Integer, V Integer)`,
prints `1 10` (`repro/reference-argument-owner-replaced.inox`).

## The rule (CANON-5, approved with an amendment)

New requirement "CANON-5 Order of an assignment" (OpenSpec allows 500
characters per requirement):

> `L := R` SHALL evaluate R once, then L once as a place, then store R's
> value there; R's value lives until the store. `L op= R` SHALL evaluate
> R once, then L once, read L, apply op and store into that same place.
> A call in R that replaces or frees what L reaches therefore never
> leaves the store a stale place: L is reached after it.

Compound assignment, revised: "`L += R`, `-=`, `*=`, `/=` and `^=` SHALL
update L with `L op R`, in the order of an assignment" (the rest
unchanged: an existing target, side-effect-free indices, not chainable,
not a read of L). A chain (ADR-0009) keeps its order: X once, stores right
to left; each store is an assignment under the rule.

**Amendment (the invariant, approved).** New requirement "CANON-25 No
access through an invalidated object", beside the `with` rule that
already applies it (CANON-11 with CANON-25, `with-freed-target`):

> No place SHALL be read or written after the object that holds its
> storage was freed or replaced, in every build, including when that
> happens while the place itself is evaluated (a call in an index). Such
> an access stops the program ("use of a freed object or an empty Link")
> before it happens; it never reaches the old address, nor another object
> that reuses its memory.

The failure is the controlled stop the `with` rule already has; no place
is rebuilt after the fact.

**Q3 (approved).** R's effects happen before any failure of the target:
an index out of range, a dead Link or an object freed while L is
evaluated stops the program after them, with no implicit rollback; the
tests show it, on the exception paths too, and the temporaries of R are
released as on any other exit.

## Implementation

1. **R first, every form.** Each path of `emitLocalAssignment` and
   `emitStringAppend` evaluates R, then the place, then stores
   (`emitOwningStore`, the invariant checks after): variables, State,
   fields, elements, nested structs, slices, `Value(P)`, paths through
   `Own` and `Option<Own>` (by `Link`) and through a `Link`.
2. **R's value outlives L.** When evaluating L may run code (a call in an
   index of a plain assignment), R's value is made independent of storage
   L can change before L is evaluated: an owned result is already held
   to the end of the statement (`holdOwnedResult`); a String read from a
   place is retained (`protectBorrowed`); an aggregate holding Strings has
   each of them retained (`forEachLeafAt`); an owner taken by `Move(P)` is
   a value, not a place. When L cannot run code nothing changes.
3. **Places guarded by the generation of their object (Q2, every
   build).** A place reached through a pointer (the nearest pointer on
   its path, `lastPointerOnPath`: an `Own`, an `Option<Own>` by `Link`, a
   `Link`, `Value(P)`) keeps, from the moment its object is reached, the
   object's header and the generation read then (`LinkParts`, as `with`
   does). Before each access through it that follows the evaluation of
   code that may run a routine (not side-effect free, the analysis's
   `isSideEffectFree`), the generation is read again and compared; a
   different one stops the program (fault 16). Freeing an owner frees
   what it owns, each with its generation raised, so the nearest pointer
   on the path covers the owners above it. The check stands before every
   access, a load as well as a store (a read of a place, the read of a
   compound target, the store). Where no such code runs between reaching
   the object and the access, there is no check: a local's store, and
   every place whose indices are side-effect free, cost what they cost
   today.
4. **Identity under reuse.** The check compares generations, not
   addresses: libinoxrt never returns an object's block to the system
   while the program runs, raises its generation when it frees it, and
   keeps that generation when it hands the block out again (its free
   lists, `OwnRuntime.cpp`). A block reused by another live object
   therefore has another generation, and the check sees it. This property
   of the runtime, on which Links already rely, is stated in
   `docs/ARCHITECTURE.md` as part of libinoxrt's contract; the runtime's
   code does not change.
5. **Compound assignment as one operation.** Phase 2b keeps
   `CompoundAssignmentStatement`'s meaning without relying on a second
   evaluation of the clone: the normalized form stays `L := L op R`,
   marked compound, and code generation reads the marked form as "R, then
   the place of L once, its guarded load, op, its guarded store" (the
   clone is never evaluated as a place of its own; the duplicate index
   evaluation and range check go). The operator is applied to two values:
   the arithmetic now emitted from two AST operands is reached through
   one function that takes the operator, the type and the two values. In
   the typed IR, the compound `Assign` gets `op` and R as its value (the
   target is its place); the printer shows it, and the TIR snapshot
   difference is declared. AST node types do not change.
6. **Detection of stale accesses (tests only).** A checking build of
   libinoxrt (`INOX_OWN_QUARANTINE`) never reuses a freed block and
   poisons its payload (`ASAN_POISON_MEMORY_REGION`); the programs of
   `tests/runtime` built with clang's `-fsanitize=address` run against it
   (`tools/owner_safety_check.py`, beside `string_leak_check.py`), so an
   access to a freed object is an ASan report, not a changed output. The
   release runtime is unchanged. Every reproduction is checked both ways:
   its output or trap, and no report.

Outside this change (approved, Q1): reference arguments and receivers
(`emitArguments`, `emitReferenceArgument`) get a change of their own,
right after this one and before F6 step 4. It defines how a reference
keeps its validity from binding to the end of its use by the callee;
evaluating value arguments before reference arguments is not the
solution. `repro/reference-argument-owner-replaced.inox` moves there. A
structural change this design does not foresee (a representation of
references with lifetimes in the typed IR, a change of the routines'
ABI) gets a design of its own before it is implemented.

## Gates

Suite; the new runtime tests; the owner safety check; ASan+UBSan suite (1
MiB stack); fuzz 1 and 7; differential; backend_gaps; Windows;
`tools/stack_frames.py` (no protected frame grows); memory. The IR
snapshot changes on purpose, and identity is not required; every
difference is declared (`ir-snapshot-expect.json`) and classified per
function by a script that compares the instructions after numbering
temporaries and labels, into:

- a reordering of evaluation (the same instructions, R's before the
  target's);
- a redundant evaluation removed (the second evaluation of a compound
  target and its range check);
- a validity check added (the generation load, comparison and stop of
  item 3);
- the authorized representation of compound assignment (one place, its
  load, the operation on two values, its store);
- anything else: unexpected, and the gate fails.

A program may fall in several of the first four; it is listed with each.
The classification shows where the code changed, not that it is right:
the behavioral tests and the owner safety check do that. The typed IR
snapshot changes only for compound assignments.

## Decisions (Marcelo Fortes, 2026-11-04)

- CANON-5 text: approved, with the amendment of the invariant above.
- Q1, reference arguments: a separate change, right after this one and
  before F6 step 4; not by evaluating value arguments first.
- Q2, generation check: in every build, on the accesses that can be
  invalidated; the quarantine runtime is a test tool, the checks are part
  of the program.
- Q3, a failure of the target after R: approved, stated in the tests.
- R preserved beyond fresh results (Strings, aggregates with references,
  values borrowed from mutable storage).
- The IR classifier separates the five kinds above.
- Done means: no path that evaluates a place reaches an invalidated
  object, even when its memory was reused by another live object; the
  output of `H.FChild.FV := Drop` alone is not the criterion.
