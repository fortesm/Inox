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

1. **Raising points.** Phase 5 marks the points where an exception may
   leave the current statement: `Raise` and bare `Raise`; `Retry` (a spent
   budget rethrows); a call of a user routine, a method or a generic
   instance (conservatively: any of them may raise); the built-ins whose
   runtime calls `__inox_raise` (file I/O raising `IOError` and its
   subtypes, the String conversions raising `ConversionError`), taken from
   the built-in table, not from names. Contract failures, overflow, bounds
   and allocation failures trap and are not raising points.
2. **Departure.** At a raising point reached with a `ref mut` owner given
   away, the set departs as an exception exit through the enclosing trys:
   a try with a catch-all handler (plain `except` or `Else`) takes the path
   to its handlers (already checked from their start set); otherwise the
   path crosses the try's `ensure` (`out(S)`) and goes on outward; at the
   routine's edge the r3.135 check applies. Only routines with `ref mut`
   parameters do this work.
3. **Error.** "P is received as ref mut and given away (Move or Free) where
   an exception may leave the routine: assign it again first", at the
   raising point.
4. **What stays.** r3.134 handler and `ensure` analysis, r3.135 Q1 and Q2
   unchanged. With C, Q2 (the caller treats a `ref mut` variable as given
   away for its handlers) becomes redundant; relaxing it is a separate
   decision (Q2 below), not part of this change.
5. **Run time.** Trap 17 stays in the generated code.

Precision. Accepted: free and assign again with no raising point between
(`Free(P)` then `P := New(TNode(...))`); a raising call before the `Free`
or after the assignment; a give-back in an `ensure` around the raising
part. Rejected: a call of any user routine, or a raising built-in, between
`Free(P)` and the assignment, unless an `ensure` or a catch-all handler
gives `P` back. The repository has no other routine with a `ref mut` owner
parameter than the tests of r3.135; their expected results are listed in
the tests below.

## Questions for the maintainer

- **Q1.** Adopt C, with CANON-25 reading "A routine gives back an owner it
  received as `ref mut` before it returns or an exception leaves it"?
  Recommendation: yes.
- **Q2.** Keep the r3.135 caller-side rule (Q2) unchanged in this change,
  and decide its relaxation separately? Recommendation: yes, keep it.
- **Q3.** Trap 17: with C no known accepted program reaches it. Keep the
  check, and verify it independently by an LLVM IR check of its emission
  (the expected null test and the call with kind 17 in a program that
  reads an owner), instead of a runtime trap test? Recommendation: yes,
  with the residual-path search repeated whenever a new path appears.

## Acceptance tests

Negative (`tests/diagnostics/`):
- `pointer-refmut-freed-before-raise`: `Free(P)`, `Raise`, assignment.
- `pointer-refmut-freed-before-call`: `Free(P)`, a call of a user routine,
  assignment.
- `pointer-refmut-freed-before-conversion`: `Free(P)`, a raising String
  conversion, assignment.
- `pointer-refmut-freed-before-retry`: a handler frees, then `Retry`.
- `pointer-refmut-freed-in-typed-handler-try`: the raising point is in a
  try whose typed handler gives `P` back (the type may not match).
- `pointer-refmut-generic-raise`: a generic instance frees its `ref mut T`
  and raises.
- `pointer-owner-field-lost-by-exception` (the runtime test moved).

Positive (`tests/runtime/`, leak-checked):
- `pointers-refmut-exception-exits`: free and assign with no raising point
  between; the raising call before the free; an `ensure` that gives back
  around a raising call; a catch-all handler that gives back; recursion;
  field arguments through a local struct, an owner, `Self` and a `ref mut`
  struct, all caught by handlers that then use the field.

Run-time guard (Q3): `tests/codegen/llvm-owner-check-smoke.inox`, an
emission test of the existing `run_llvm_emission_test` form, with the
expected IR fragments of the owner check and its kind 17.

Gates (E19), with the LLVM IR (Debug, Release) and TIR snapshots of main
recorded before the change: only added or moved tests may differ.

## Approval

Pending: the maintainer's decision on Q1-Q3 and dated approval (G7).
