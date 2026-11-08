## Reproduction (main 7401b1a)

    Renew(ref mut P Own<TNode>) :          -- accepted since r3.135: no
        Free(P)                            -- normal exit leaves P given away
        if True
            Raise RangeError
        ;
        P := New(TNode)
    ;

    try                                    -- all accepted today:
        Renew(B.FNode)                     -- a local struct's field
        Renew(H.FBox.FNode)                -- a field through an owner
        B.Refresh                          -- Self.FNode inside a method
        Outer(B)                           -- B.FNode of a ref mut struct
    except
        On RangeError
            PutLn("caught")
    ;
    PutLn(H.FBox.FNode.FValue)             -- trap 17

The callee's normal exits give `P` back (r3.135), so the obligation holds;
the exception exit does not. For an owner variable argument the caller is
protected by r3.135 Q2 (its handlers and `ensure` see it as given away);
for a field no variable exists to record, and the empty field outlives the
call. Dropping the struct later frees nothing twice (null owner, verified
under AddressSanitizer), so the residual risk is trap 17, not memory
corruption.

## Alternatives

- **A. Restrict field arguments at the call.** Reject an `Own<T>` field as a
  `ref mut` argument where an exception from the call can reach code that
  sees the field: inside a try of the caller with a handler or `ensure`,
  and always when the field's root outlives the caller (a parameter,
  `Self`, a field reached through an owner or a Link, State). Simple and
  sound, but it rejects the common forms `Renew(Self.FNode)` and
  `Renew(P.FNext)` (list and tree code) everywhere, try or not.
- **B. Field paths in the caller's moved set.** Extend r3.135 Q2 to place
  paths of local roots (`B.FNode` given away for the handlers), plus A for
  roots that outlive the caller. The moved set becomes keyed by paths, a
  second form of ownership state; outliving roots stay restricted as in A.
- **C. The callee gives the owner back on every exit (recommended).**
  Extend the r3.135 obligation to the exception exits of the callee. Each
  raising point (below) is a departure of the r3.134 frames, like `Return`:
  it crosses the enclosing `ensure`s (`out(S)`), and the `ref mut` owners
  must not be given away after them. Sound for every argument form at
  once, with no caller-side tracking: by induction no call ever returns,
  normally or not, with a `ref mut` owner given away. Local to the callee:
  modules, generic instances (checked per instance), recursion and
  aliasing (ADR-0016 exclusivity) need nothing more.
- **D. Interprocedural effect summaries.** "Routine may leave parameter k
  given away on an exception", applied by the caller to its argument; for
  fields it still needs the path tracking of B, plus a fixpoint for
  recursion and one summary per instance and module. Not recommended.
- **E. Run-time repair** (give the field a value when unwinding): there is
  no value to give without changing CANON-25 (`Own<T>` is never empty).
  Rejected.

## Design of C

### 1. Where an exception can leave a statement (Q1)

The ground truth is the generated code: an exception unwinds only out of
(a) `__inox_raise` and `__inox_exception_rethrow`, (b) a call of a routine
the compiler emitted from Inox source, and (c) a libinoxrt function
declared without `nounwind` (RuntimeAbi.cpp; every other runtime function
is `nounwind` and reports errors by trapping). Read on main a484d46, that
gives the complete list of raising points, each with the phase 5 fact
that identifies it:

| Raising point | Generated code | Phase 5 fact |
|---|---|---|
| `Raise T` | `__inox_raise(type id)` | the `RaiseStatement`, its type (F7) |
| bare `Raise` in a handler | rethrow | the `RaiseStatement` without a type |
| `Retry(N)` with a spent budget | rethrow | the `RetryStatement` |
| call of a routine, method or generic instance (also those of library modules written in Inox, Std.DateTime among them) | `call`/`invoke` of an Inox function | `CallMeaning` kind `Routine`, `Method`, `MethodNoArguments` |
| implicit call of a struct invariant (CANON-24: after construction, a field write or a `ref mut` argument from outside, around methods); a contract may call a function that raises (verified: an invariant calling a routine that raises `RangeError` reaches the caller's handler) | call of `T.__invariant` | the sites `recordInvariantPlaces` records |
| `T(S)` with S a String and T an integer, unsigned, float or decimal type (Currency, Crypto) | `__inox_str_to_i64/u64/f64/f32/dec` | `CallMeaning` kind `Conversion`, source String |
| Std.Files: OpenRead, Create, OpenAppend, ReadLn and Eof of a file, Close, Write and WriteLn to a file, DeleteFile, RenameFile | `__inox_file_open/readln/eof/close/write_*/delete/rename` | `BuiltinOp` of those names |

Not raising points, because they trap or cannot fail: contracts (`pre`,
`pos`, `invariant`, loop contracts, Assert), overflow, bounds, ranges,
`New` (allocation failure traps), `Free`, `Move`, Link checks, field
defaults (constants only, `types::FieldDefault`), console I/O, String
functions, sets, math, Exists, the Terminal, SystemUtils and DateTime
intrinsics (all `nounwind`). The routine's own `pre` runs before any give
away, and its own `pos` runs on a normal exit, where the r3.135 check has
already required every `ref mut` owner back.

Nested expressions need no case of their own: phase 5 visits every
argument, construction value, initializer, conversion operand and
contract expression, so a raising call inside any of them is a raising
point at that position, evaluated with the set that holds there. The
order is the evaluation order: in `P := New(TNode(FValue := F(X)))`, `F`
is analyzed before the assignment gives `P` back (SemanticExpressions:
the right side first, then `noteOwnerAssigned`), so a raise from `F`
leaves with `P` still given away.

Keeping the list exhaustive (Q4): the classification is one switch over
`CallKind` and `BuiltinOp` without a default (a new enumerator does not
compile until it is classified), and a compiler unit test pins the list of
runtime functions declared without `nounwind` against the operations
classified as raising, so a new raising runtime function fails the test
until phase 5 knows it.

### 2. Where the exception goes

At a raising point, inside the routine:

- A try whose body holds the point and has a plain `except` (CANON-15:
  it catches every Inox exception) or an `Else` catches it: the path goes
  to that try's handlers, which start from the r3.134 set and are checked
  at their own exits and raising points. It does not leave the routine.
- A typed handler catches it only when that is proven: a `Raise T` whose
  type is `T` or a subtype of the handler's type (F7 `isExceptionOf`).
  Otherwise (a call, a conversion, a file operation, a rethrow, a `Raise`
  of another type) the match is not proven and the path is also taken as
  going on outward, crossing that try's `ensure`.
- Crossing a try with `ensure` transforms the set by `out(S)` (r3.134),
  inner `ensure` first; an owner the `ensure` assigns again on every path
  is given back for that exit; one it assigns on some paths only is not.
- A raising point inside a handler or inside an `ensure` leaves that
  try (CANON-15: a handler's exception is not caught by its own try) and
  goes on outward from it.
- At the routine's edge the r3.135 check applies: no `ref mut` owner may be
  given away.

This is a new departure kind of the r3.134 frames (an exception exit),
recorded at raising points only while a `ref mut` owner is given away, so
routines without `ref mut` owners do no extra work and the frames of
r3.134 and r3.135 keep their results.

### 3. Error

"P is received as ref mut and given away (Move or Free) where an exception
may leave the routine: assign it again first", at the raising point.

### 4. What stays (Q2)

The r3.134 handler and `ensure` analysis and both r3.135 rules stay as
they are, including r3.135 Q2: an owner variable passed as `ref mut`
inside a try counts as possibly given away for the handlers and `ensure`
of that try. Under C that rule becomes redundant; relaxing it is a
separate proposal, not part of this change.

### 5. Modules, generics, recursion, methods

The check is local to the routine whose body is analyzed: a routine of
another module is checked in its module; a generic instance is analyzed
per instance (since r3.134 with its own owner state), so a `ref mut T`
with a move-only `T` is checked where the instance is made; recursion
needs nothing (a recursive call is a raising point like any call); a
method's `Self` is never given away (CANON-25), so only its `ref mut`
parameters are checked. ADR-0016 keeps `ref mut` arguments exclusive, so
no alias reaches the same owner through two parameters.

### 6. Run time (Q3)

Trap 17 stays in the generated code. With C, no accepted program known
reaches it, so its runtime test cannot be an Inox program. Its evidence
becomes two checks: an LLVM emission test (`tests/codegen`, the existing
`run_llvm_emission_test` form: the null test and the call with kind 17)
and an executable fixture: an Inox routine that takes an owner and reads
it, compiled by `inox`, linked with a small host that calls it with a null
owner across the ABI, expected to stop with "use of an owner after Move or
Free" and exit code 70 (the form of `tests/native`). The fixture needs a
new step in `run-tests.sh` (Q3).

Precision. Accepted: free and assign again with no raising point between
(`Free(P)` then `P := New(TNode(FValue := 1))`); a raising call before the
`Free` or after the assignment; an `ensure` that gives `P` back around a
raising part; a plain `except` or an `Else` that gives it back; a typed
handler that provably catches a `Raise T` and gives it back. Rejected: any
other raising point between `Free(P)` and the assignment, including one
inside the assignment's right side. The repository has no routine with a
`ref mut` owner parameter outside the tests of r3.135, whose results stay
as recorded.

## Questions for the maintainer

- **Q1.** Adopt C with the raising points of section 1 and the routing of
  section 2, and CANON-25 reading "A routine gives back an owner it
  received as `ref mut` before it returns or an exception leaves it"?
  Recommendation: yes.
- **Q2.** r3.135 Q2 kept unchanged (your instruction of 2026-11-08); its
  relaxation, if any, in a separate proposal. Recorded, no decision left.
- **Q3.** Trap 17: the emission test plus the native fixture with a null
  owner, as its executable check (a new `run-tests.sh` step)?
  Alternatives: a compiler-unit test that builds a TIR program directly
  and runs the generated code (needs a clang call from the unit tests), or
  a test-only flag that skips the move analysis (a new compiler option;
  not recommended). Recommendation: the native fixture.
- **Q4.** The exhaustiveness guard of section 1 (switch without default,
  unit test of the runtime functions without `nounwind`)? Recommendation:
  yes.

## Acceptance tests

Negative (`tests/diagnostics/`, each at its raising point):
- `pointer-refmut-freed-before-raise`: `Free(P)`, `Raise`, assignment.
- `pointer-refmut-freed-before-call`: `Free(P)`, a call of a user
  routine, assignment.
- `pointer-refmut-freed-before-method`: the same with a method call.
- `pointer-refmut-freed-before-conversion`: `Free(P)`, `Integer(S)`,
  assignment.
- `pointer-refmut-freed-before-file-write`: `Free(P)`, `WriteLn(F, X)`,
  assignment.
- `pointer-refmut-raise-in-new-value`: `P := New(TNode(FValue := F(X)))`
  after `Free(P)`, `F` a routine.
- `pointer-refmut-freed-before-invariant`: `Free(P)`, a write to a field
  of a struct with an invariant, assignment.
- `pointer-refmut-freed-before-retry`: a handler frees, then `Retry`.
- `pointer-refmut-freed-before-rethrow`: a handler frees, then `Raise`.
- `pointer-refmut-typed-handler-not-proven`: the raising call is in a try
  whose typed handler gives `P` back (the type is not proven).
- `pointer-refmut-raise-in-handler`: the plain `except` handler frees and
  raises (its own try does not catch it).
- `pointer-refmut-ensure-one-branch`: an `ensure` gives `P` back on one
  arm only around a raising call.
- `pointer-refmut-nested-ensures-order`: inner `ensure` assigns again,
  outer `ensure` frees, a raising call inside both.
- `pointer-refmut-raise-in-ensure`: an `ensure` frees and then calls a
  routine.
- `pointer-refmut-generic-raise`: a generic instance frees its
  `ref mut T` and calls a routine.
- `pointer-refmut-recursive-raise`: a recursive routine frees, then
  recurses.
- `tests/modules/refmut-raise-gives-away`: the routine is in another
  module.
- `pointer-owner-field-lost-by-exception`: the runtime test moved.

Positive (`tests/runtime/`, leak-checked):
- `pointers-refmut-exception-exits`: free and assign with no raising point
  between; a raising call before the free and after the assignment; an
  `ensure` that gives back around a raising call (also nested, inner
  first); a plain `except` and an `Else` that give back; a typed handler
  that provably catches `Raise RangeError` and gives back; recursion that
  gives back before recursing; a method with a `ref mut` owner parameter;
  a generic instance; field arguments through a local struct, an owner,
  `Self` and a `ref mut` struct, with exceptions caught by handlers that
  then use the field.
- `tests/modules/refmut-raise-gives-back`.

Run-time guard (Q3): the emission test and the native fixture of section 6.

Snapshots: LLVM IR (Debug, Release) and TIR of every program compared with
the baseline of main 7401b1a (recorded 2026-11-08, local); programs that
stay valid must be identical, only the tests added or moved here may
differ, each listed in the PR as newly rejected or new.

## Approval

Pending: the maintainer's decision on Q1-Q3 and dated approval (G7).
