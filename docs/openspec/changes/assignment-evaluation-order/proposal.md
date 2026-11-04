## Why

An assignment through an owner can write into an object that its own
right-hand side freed. With

    State
        H THolder := THolder(FChild := New(TNode))
        K THolder := THolder(FChild := New(TNode))
    ;
    Drop Integer :
        H.FChild := New(TNode)
        K.FChild := New(TNode)
        Return 10
    ;
    Main :
        H.FChild.FV := Drop
        PutLn(H.FChild.FV, " ", K.FChild.FV)
    ;

the program prints `1 10`: code generation computes the address of
`H.FChild.FV` from the old child, `Drop` frees that child and a new owner
of K takes its block from the free list, and the store lands in K's
object. `H.FChild.FV += Drop` prints `1 11`. Found while investigating the
compound-assignment finding of the parser audit (2026-11-04); the audit's
own case, `V[I] += F()` with F changing I, does not misbehave today, but
only because code generation happens to compute the target before the
right-hand side. The constitution states no order of evaluation between
the target and the value of an assignment; ADR-0012 calls the long form
`L := L op R` "exact because the allowed targets have no side effects",
which does not hold when R has them.

The suite under ASan does not see it: the compiler is instrumented, the
programs it builds are not, and libinoxrt keeps freed blocks on free
lists (for the generation checks of Links), so a stale write is never a
write to memory the allocator returned.

Approval: Marcelo Fortes, 2026-11-04: a memory-safety defect of high
priority, blocking F6 step 4; the change is opened before step 4.
Direction chosen: the right-hand side is evaluated before the target. An
assignment evaluates R once, keeps its value alive, then evaluates L once
as a place, checks it and stores; `L op= R` is its own operation (R, then
L once, the read of L, the operator, the store into the same place), not
the textual long form. The rule is general, not a fix of the program that
revealed it; checking the target's validity before the store is a
complementary defence, rejecting calls that may change an owner is not
the solution. The design is approved before any code.

Touches: CANON-5 (assignment, compound assignment) and CANON-25 (no
access through an invalidated object; constitution relocked); ADR-0012's consequence. Compiler phases: normalization (2b,
the compound form), the typed IR (5b, the compound `Assign`), code
generation (6, the order of every assignment form); libinoxrt (a checking
mode for the tests, design). `docs/ARCHITECTURE.md`, `docs/STATUS.md`.

Risk: high (memory ownership; changes observable behavior).

Status: **implemented** (canon r3.124); design approved with amendments
(Marcelo Fortes, 2026-11-04; `design.md`, "Decisions").

## What Changes

1. CANON-5: the order of evaluation of `L := R` and `L op= R` (text in
   `design.md`, for approval).
2. Code generation: every assignment form evaluates R before the target
   (local, State, field, element, nested struct, `Value(P)`, through
   `Own`, `Option<Own>` reached by `Link`, String append); compound
   assignment reads and writes one place, resolved once, after R.
3. Every place reached through a pointer is checked against its object's
   generation before an access that follows code able to free it, in
   every build (the program stops, as a `with` target does).
4. The reproductions in `repro/` become permanent runtime tests, with a
   check that detects an access to a freed object, not only the output.
5. Revision line, kCanonRevision, CHANGELOG (**changed behavior**).

## Out of scope

Reference arguments and receivers: a change of their own, right after
this one and before F6 step 4 (decision Q1).

F6 step 4 (places and encodings in the typed IR): this change does not
move codegen to the typed IR; it changes the order the backend already
emits, and the typed IR's compound `Assign` only as far as the rule needs.
