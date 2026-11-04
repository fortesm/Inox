# Design — order of evaluation of assignments

Status: awaiting approval (Marcelo Fortes). No code before it.

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

`L op= R` is turned by phase 2b into `L := L op R`, the target cloned
(`AstNormalizer::normalizeCompound`, marked compound); that the two copies
name one element holds only because codegen evaluates both before R. The
index of a compound target is evaluated twice (two range checks).

The same pattern exists outside assignments (question Q1): a reference
argument's address is computed before a later argument;
`Store(H.FChild.FV, Drop)`, with `Store(ref mut X Integer, V Integer)`,
prints `1 10` (`repro/reference-argument-owner-replaced.inox`).

## The rule (CANON-5, text for approval)

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
not a read of L). Scenarios name the tests of `repro/`. A chain
(ADR-0009) keeps its order: X once, stores right to left; each store is
an assignment under the rule.

Observable change: the element `V[I] += F()` updates is chosen after F; a
trap of the target (an index out of range, a dead Link) now happens
after R's effects. No program of the suite is expected to depend on the
old order; any one that does is shown, not adjusted.

## Implementation

1. **Code generation, every form.** Each path of `emitLocalAssignment`
   and `emitStringAppend` evaluates R first, then the place, then stores
   (`emitOwningStore`, the invariant checks after). Variables, State,
   fields, elements, nested structs, `Value(P)`, paths through `Own` and
   through a `Link` (whose generation check then runs after R, and traps
   on an object R freed).
2. **R's value lives until the store.** An owned result is already held
   to the end of the statement (`holdOwnedResult`); a String borrowed from
   a place is retained (`protectBorrowed`) whenever evaluating L may run
   code (a call in an index of a plain assignment). An owner moved by R
   (`Move(P)`) is a value, not a place.
3. **Compound assignment as one operation.** Phase 2b keeps
   `CompoundAssignmentStatement`'s meaning without relying on a second
   evaluation of the clone: the normalized form stays
   `L := L op R`, marked compound, and code generation reads the marked
   form as "R, then the place of L once, a load from that place, op, a
   store to it" (the clone is never evaluated as a place of its own).
   The operator is applied to two values: the arithmetic now emitted from
   two AST operands is reached through one function that takes the
   operator, the type and the two values (no new abstraction beyond it).
   In the typed IR, the compound `Assign` gets `op` and R as its value
   (the target is its place), the printer shows it; the TIR snapshot
   difference is declared. AST node types do not change.
4. **Detection of stale writes (tests).** A checking build of libinoxrt
   (`INOX_OWN_QUARANTINE`, test only) never reuses a freed block and
   poisons it (`ASAN_POISON_MEMORY_REGION`); the programs of
   `tests/runtime` built with clang's `-fsanitize=address` run against it
   (`tools/owner_safety_check.py`, beside `string_leak_check.py`), so a
   store into a freed object is an ASan report, not a changed output. The
   reproductions are checked both ways: their output, and no report.
   The release runtime is unchanged.

## Gates

Suite; the new runtime tests; the owner safety check; ASan+UBSan suite (1
MiB stack); fuzz 1 and 7; differential; backend_gaps; Windows;
`tools/stack_frames.py` (no protected frame grows); memory. The IR
snapshot changes on purpose: every program with an assignment whose R is
not a constant and whose target is not a plain variable reorders. Each
difference is declared (`ir-snapshot-expect.json`), classified by a
script that shows it is only a reordering (same instructions per
function, R's before the target's) or names it, and reviewed; the typed
IR snapshot changes only for compound assignments.

## Questions for the maintainer

- **Q1** Reference arguments (`F(ref mut H.FChild.FV, Drop)`, a `Self
  mut` receiver reached through an owner) have the same defect. In this
  change, under a rule of their own (for example: every value argument
  first, then the places, in order), or a change of their own right after
  this one?
- **Q2** The complementary defence: a generation check of the target
  block before the store, in Debug only, in every build, or not now?
- **Q3** The order of a trap of the target after R's effects (above):
  accepted as stated?
