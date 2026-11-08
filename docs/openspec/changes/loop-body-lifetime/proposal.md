## Why

Item 2.2 of `safe-pointers-followups`: an owner, or a variable some Link
points to (a linked local), declared in a loop body lives until its
declaration runs again or the routine returns, not until the end of the
round that declared it. Nothing leaks (three probes under AddressSanitizer
on main 7401b1a), but the lifetime is longer than the scope, and it shows:
on main, after

    for I in 1..2
        N TNode := TNode(FValue := I)
        P := New(TNode(FValue := I * 10))
        L := Link(N)
        M := Link(P)
    ;
    PutLn(Alive(L), " ", Alive(M))          -- prints: true true
    PutLn(Value(L).FValue, Value(M).FValue) -- prints: 2 20

both Links still reach variables whose scope has ended. CANON-25 gives a
Link no meaning past its target's life; the lifecycle work of 0.5
(destructors) will also need each scope's end to be a defined point.

Approval: Marcelo Fortes, 2026-11-08 (G7; Q1-Q5 decided). Risk: high
(memory ownership). Depends on PR #149 (r3.136), its base.

## What Changes (recommended)

- The owners and linked locals a loop body declares end at the end of each
  round and on every path out of the body: `continue`, `leave`, a true
  `until`, `Return`, `Exit`, `Raise` and an exception; in the reverse order
  of their declarations, after the `ensure`s the path leaves.
- An owner still held is freed (a moved one is not, its slot is empty); a
  linked local's tombstone is freed, so its Links die (`Alive` false, a
  read traps as for any dead Link).
- Strings and values that only hold Strings keep their current release
  (Q4).

## Impact

- Code generation (the loop lowering) and possibly the TIR, decided in
  design.md (Q5); phase 5 unchanged; libinoxrt unchanged.
- Programs whose behavior changes: a Link to a loop-local read after its
  round (it now traps or reads `Alive` false); memory of loop-local owners
  is given back one round earlier.
- LLVM IR changes for every loop that declares an owner or a linked local
  in its body; each such program is listed in the PR, all others stay
  identical.
