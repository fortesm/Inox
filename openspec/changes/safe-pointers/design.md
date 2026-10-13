## Ref<T>: shared, counted, never null

```
Type
    TNode Struct
        FValue Integer
        FNext Option<Ref<TNode>>       == the end of the list is None
    ;

Main :
    Last Ref<TNode> := New(TNode(FValue := 2))       == count 1
    Head Ref<TNode> := New(TNode(FValue := 1, FNext := Some(Last)))
    Head.FValue += 10                  == fields through the reference
    Other Ref<TNode> := Head           == a second reference: the same node, count 2
    PutLn(Other.FValue)                == 11
;                                      == counts reach 0: both nodes freed here
```

- `New(Value)` copies a value to the heap and returns `Ref<T>`.
- Field access goes through the reference (`Head.FValue`); a reference is
  never null, so there is no null check and no null fault.
- Copying a `Ref<T>` shares the object (explicit aliasing: the type says
  so). Releases use the String/File ownership paths already in the emitter
  (owned slots, cleanup pad, statement temporaries), so exceptions cannot
  leak a node; `tools/string_leak_check.py` extends to Ref programs.
- Cycles (a doubly linked list, a parent pointer) would never be freed;
  the back link is a `Weak<T>`, which does not keep the object alive and is
  read as `Option<Ref<T>>`.
- Counts are not atomic while Inox has no threads; concurrency (change to
  come, `do`) decides how a Ref crosses threads.

## Arena (phase 2): regions for many small objects

```
Build :
    Nodes Arena
    Root Ptr<TTree> := Nodes.New(TTree(FValue := 1))   == valid while Nodes lives
    ...
;                                              == Nodes and every object in it freed at once
```

Objects are bump-allocated and freed together; a `Ptr<T>` from an arena
cannot be returned, stored in State or in anything that outlives the arena
(checked like `ref` parameters and slices, ADR-0016/0017), so it can never
dangle. Fast, no counts, cycles allowed inside the arena.

## Decisions for the maintainer

1. **ARC `Ref<T>` as the general safe pointer** (recommended; it extends the
   String/File model) — or unique ownership with moves and borrow checking
   (Rust; more rules to learn and check).
2. **Mutation through a shared Ref**: allowed (recommended; aliasing is
   visible in the type) — or only through a unique Ref (safer, harder to
   use).
3. **Names**: `Ref<T>`, `Weak<T>`, `New(...)`, `Ptr<T>` and `Arena` — or
   others (`Shared<T>`, `Pointer<T>` reserved for the unsafe layer).
4. **Arenas in phase 2** (recommended) or together with Ref.
5. **Unsafe layer** (`Pointer<T>`, address arithmetic, C interop): a later
   change, never in the safe core.

## Order of work after approval

`Option<T>` (change `generics`) → `Ref<T>` with `New` and field access →
`Weak<T>` → `Arena` and `Ptr<T>`. Each with leak-checked tests.
