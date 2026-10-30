## Syntax

```
Type
    TStack Struct
        FItems Array[1..5] Integer
        FCount Integer := 0
        invariant Self.FCount >= 0 and Self.FCount <= 5
    ;

TStack.Push(Self mut, X Integer) :
    pre Self.FCount < 5, "the stack is not full"
    pos Self.FCount = old Self.FCount + 1
    Self.FCount += 1
    Self.FItems[Self.FCount] := X
;

TCountedStack.Push(Self mut, X Integer) Override :
    pos then Self.FOperations = old Self.FOperations + 1
    ...
;

while I < N
    invariant Total = I * (I + 1) div 2
    variant N - I
    ...
;
```

## Lowering (compile time)

- The parser keeps the clauses of routines and structs, and turns loop
  clauses into checks at the start of the body (plus one after while and
  repeat); a variant keeps its previous value in a hidden local.
- `semantic/ContractLowering.cpp`, run on the whole program before the
  analyzer: a routine with contracts (its own or inherited) becomes a
  wrapper with the routine's name and signature that checks `pre`, saves
  each `old` value, calls the body (moved to a hidden `__Name_body`), checks
  `pos` with `Result`, and returns. A Struct's clauses become a hidden
  method `TName.__invariant(Self)`. Inherited clauses are copied with `Self`
  replaced by the path to the part.
- The analyzer checks the intrinsics `__check` and `__pure`, keeps contracts
  read-only, and records where invariants must hold.

## Run time

- `__check` is `br i1 (llvm.expect.i1 cond, true)` to a cold failure block
  that calls `@__inox_contract_fault` (`cold noreturn nounwind`), which
  prints "Inox runtime error: <text>" and exits with 70.
- LLVM has no contract primitive. `llvm.expect` and the `cold`/`noreturn`
  attributes keep the failure path off the hot path; `llvm.assume` is not
  used, because it would let the optimizer delete the check it describes;
  `llvm.trap` would lose the message.

## Other languages (checked 2026-10-14)

- **Eiffel**: the model followed here (eiffel.org, archive.eiffel.com
  tutorial chapters 9 and 10).
- **Ada 2012 / SPARK**: `Pre`, `Post`, `Type_Invariant`, `'Old`, `'Result`,
  `Pre'Class` for inheritance; SPARK proves them at compile time. The best
  modern step beyond Eiffel; a later change (FUTURE).
- **C++26** (P2900): `pre`, `post`, `contract_assert`, with evaluation
  semantics ignore / observe / enforce / quick-enforce chosen at build time;
  no `old`, no invariants, contracts on virtual functions only from C++29.
- **Java**: no contracts in the language (only `assert`, off by default);
  JML is an external notation. Nothing better to take from it.
