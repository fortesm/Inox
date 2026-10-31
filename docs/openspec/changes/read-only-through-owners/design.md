## Where the check lives today **[code]**

`SemanticAnalyzer::writableRoot` (`semantic/SemanticExpressions.cpp`)
walks a place from outside in (`__member`, `__index`). When the base of a
step has type `Own` or `Link`, it returns `pointerTargetRoot_`: "writable,
because it is reached through a pointer". It never looks at where the
owner itself came from, so an owner reached through `ref H` or `Self`
makes everything below it writable. The callers of `writableRoot`:

| Caller | Check |
|---|---|
| `SemanticExpressions.cpp` assignment to a field / an element | "assignment target is not mutable" |
| `SemanticCalls.cpp` `checkReferenceArguments`, receiver aliasing | `ref mut` argument is a writable place |
| `SemanticReuse.cpp` `Self mut` receiver | returns early when the receiver's type is `Own`/`Link` |
| `SemanticPointers.cpp` `Free`/`Move` of `Option<Own>` | writable place |
| `SemanticStatements.cpp` `with` | the binding is as writable as its target |

`Link(...)` (`SemanticPointers.cpp`) checks its own root: a variable of
the routine or State, not a value parameter; it returns early for an
argument that is already a pointer (`Link(P)`) and stops at the first
pointer on the path, with no writability check.

Parameters are declared in `SemanticStatements.cpp` with
`isMutable = (mode == RefMut)`; `Ref`, `Value` and the plain `Self` (mode
`Value` on a method) are all `isMutable = false`, so today a value
parameter and a `ref` parameter look the same to `writableRoot`.

## The change

1. **Symbol** (`Symbol.h`) gets one flag, `deepReadOnly`: true for a
   `ref` parameter, for the plain `Self` of a method (the first parameter
   of an associated method in mode `Value`), and for a `with` binding whose
   target is deep read-only. False everywhere else (value parameters keep
   owning their objects).
2. **`writableRoot`**: a step whose base is `Own` (or `Option<Own>`
   reached by `Link`) no longer returns `pointerTargetRoot_` at once; the
   walk continues to the root and remembers that it crossed an owner. At
   the root:
   - a mutable symbol (local, State, `ref mut`, `Self mut`) → that symbol,
     as today;
   - a symbol that is not mutable and not `deepReadOnly` (a value
     parameter) and the walk crossed an owner → `pointerTargetRoot_`, as
     today (its owned objects are writable);
   - a `deepReadOnly` symbol → null (read-only);
   - a step whose base is a `Link` still returns `pointerTargetRoot_` at
     once: a Link is a write capability.
   `Value(P)` as a target follows the same rule for P.
3. **`Self mut` receiver** (`SemanticReuse.cpp`): the early return stays
   for `Link`; for `Own` the receiver goes through `writableRoot`.
4. **`Link(...)`**: after the existing checks, the place (or, for
   `Link(P)` of an owner, P itself) must not be deep read-only: walk it
   as in 2; a deep read-only root rejects the Link. A path through a Link
   is accepted, as today.
5. **`with`**: the binding's `deepReadOnly` is the target's; its
   `isMutable` stays as today.
6. **Diagnostics**: a helper names the root and its kind ("a ref
   parameter", "the receiver of a method that is not Self mut", "the
   target of with on ..."), and the fix (`ref mut X`, `Self mut`).
   Messages cite ADR-0016 (ref), CANON-9 (Self), CANON-25 (Link).

Generic instances are analyzed as ordinary routines, so the rule applies
to each instance. Contract wrappers (`ContractLowering`) pass parameters
to `__Name_body` with their modes; `pre`/`pos` already only read.

No lowering, no IR, no runtime change: the generated code of every
accepted program is identical (to verify with `tools/ir_snapshot.py`
`record`/`compare` on the suite).

## Validation plan

- **IR snapshot**: `tools/ir_snapshot.py record` before, `compare` after:
  no difference in any accepted program.
- **Suite**: 1053 tests plus the new ones, unchanged results for the old.
- **ASan+UBSan** with a 1 MiB stack; **mutation fuzz** seeds 1 and 7 (new
  rejections are expected only for mutants that write through an owner
  from `ref`/`Self`; any crash is a failure).
- **backend_gaps**, eh_state_balance, string_leak_check,
  grammar_consistency (no grammar change), openspec validate,
  constitution_check after the relock.
- **Report**: every existing test or example whose result changed (the
  inspection predicts none), with the reason.

Design approval: Marcelo Fortes, 2026-10-31
