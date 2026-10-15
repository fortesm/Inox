## The rule

Every heap object has exactly one owner (`Own<T>`). Everyone else only
points to it (`Handle<T>`), and every use of a Handle checks that the object
still exists. No reference counting, no garbage collector, no arena, no
borrow checker: the non-owning references that Rust checks at compile time
with lifetimes are checked here at run time with one comparison.

| Goal of the maintainer | How it holds |
|---|---|
| Freed memory is never used by mistake | a Handle keeps the generation it saw; after a free (or a reuse of the block) the generations differ: `Alive` is false, or the use traps |
| No reference to an invalid address | no arithmetic, no integer-to-pointer conversion; only `New` makes pointers |
| No variable without a value | no null; an owner's absence is explicit and must be tested; an empty Handle is not Alive |
| Nothing freed twice | only the owner frees, and an owner is never copied (`Move` is explicit) |
| Nothing leaks | an owner that leaves its block frees its object; a Handle keeps nothing alive, so there are no ownership cycles |

## Syntax

| Operation | Pascal | C | Inox |
|---|---|---|---|
| Pointer type | `PNo = ^TNo` | `struct No *` | `Own<TNo>`, `Handle<TNo>` |
| Create | `New(P)` | `malloc(...)` | `P := New(TNo(FValor := 1))` |
| Field | `P^.FValor` | `p->valor` | `P.FValor` |
| Whole value | `P^` | `*p` | `Value(P)` |
| Address of a variable | `@X` | `&x` | lent only: a `ref` parameter |
| Non-owning reference | — | — | `Handle(P)` |
| Free | `Dispose(P)` (unsafe) | `free(p)` (unsafe) | `Free(P)` (safe) |
| Still there? | — | — | `Alive(H)` |
| Give ownership away | — | — | `Q := Move(P)` |
| Nothing | `nil` | `NULL` | an empty Handle; for an owner, open question 1 |

Types follow `Slice<T>` (CANON-8); operations follow the built-in routines
(`Length(A)`, `Ord(E)`). `Own`, `Handle`, `New`, `Free`, `Alive`, `Move` and
`Value` are built in, so the change does not wait for user generics.

## Examples

```
== A routine returns a new object, as in Go, without a collector:
== the caller becomes its owner.
NovoInimigo(Vida Integer) Own<TInimigo> :
    Return New(TInimigo(FVida := Vida))
;

Type
    TInimigo Struct
        FVida Integer := 100
    ;
    TSoldado Struct
        FAlvo Handle<TInimigo>              == born empty: Alive(FAlvo) = false
    ;
    TPasta Struct
        FNome String := ""
        FPai Handle<TPasta>                 == points to the parent, does not own it
        FFilhos Vector<Own<TPasta>>         == the parent owns its children (with Vector)
    ;

Main :
    Orc := NovoInimigo(100)                 == Main owns the orc
    S TSoldado
    S.FAlvo := Handle(Orc)
    S.FAlvo.FVida -= 30                     == checked: alive
    Free(Orc)                               == the orc dies now
    if Alive(S.FAlvo)
        S.FAlvo.FVida -= 30
    else
        PutLn("procurar outro alvo")
    ;
    S.FAlvo.FVida -= 30                     == trap: "use of a freed object"
;                                           == anything still owned is freed here
```

Moves are explicit; the compiler never guesses:

```
Q := Move(P)        == Q owns now; using P afterwards is a compile error
Q := P              == error: "an owner is never copied: use Move(P)"
```

## Rules

1. `New(V)` copies the value `V` to the heap and returns `Own<T>`.
2. An `Own<T>` is never copied (CANON-17, as a File): a local, a field, an
   array element or a result; it is given away only with `Move`. A struct
   that holds an `Own` is never copied either.
3. Using a moved-from owner is a compile error (a definite-move analysis,
   like the definite-assignment rules of CANON-5).
4. An owner frees its object when its block ends, when it is overwritten
   (`P := New(...)` frees the old object first), or by `Free(P)`; after
   `Free(P)`, P may only be assigned again.
5. Freeing an object frees the objects it owns (its `Own` fields), without
   recursion, so a long list cannot overflow the stack.
6. `Handle(P)` makes a Handle of an owned object. A Handle is copied
   freely, keeps nothing alive, and is born empty.
7. Every access through a Handle checks its generation: a dead or empty
   Handle traps (CANON-19, "use of a freed object"); `Alive(H)` asks without
   trapping. The check is removed when the compiler proves no `Free` can
   happen in between; a future static proof (FUTURE, SPARK style) can
   remove the rest.
8. `=` between two pointers is a compile error; `Same(A, B)` asks whether
   two references reach the same object, and `Value(A) = Value(B)` compares
   contents.
9. A pointer cannot be printed, converted to a number, or built from one.

## Run time

- Each object has a header with a 64-bit generation (8 bytes); it never
  wraps in practice. `Free` increments it and returns the block to a
  per-size free list in libinoxrt; a reused block keeps counting, so an old
  Handle never matches it.
- `Own<T>` is one address: owner access costs nothing. `Handle<T>` is an
  address plus a generation: one load and one compare per access.
- Allocation and release go through libinoxrt only (the Windows rule
  already used for Strings: memory never crosses C runtimes). The owner's
  release uses the cleanup paths of Strings and Files, so an exception
  never leaks an object; `tools/string_leak_check.py` extends to pointer
  programs.
- Generations are not atomic while Inox has no threads; concurrency (`do`)
  decides how pointers cross threads.

## Safety compared

| | Pascal | Go | Ada 2005 | Inox |
|---|---|---|---|---|
| Typed pointers, no arithmetic | ✓ | ✓ (outside `unsafe`) | ✓ | ✓ |
| No null | — | — (`nil` panics) | `not null` optional | ✓ |
| Pointer cannot outlive its target | — | ✓ (collector) | ✓ accessibility checks | ✓ owner frees, Handles checked |
| No use after free | — (`Dispose`) | ✓ (collector) | — (`Unchecked_Deallocation`) | ✓ detected, never a wrong object |
| No double free | — | ✓ | — | ✓ single owner |
| Deterministic release, no pauses | ✓ | — | ✓ | ✓ |

## Open question

1. **An owner that may be absent** (the end of a list, an optional child):
   a) `Option<Own<TNo>>` (the form FUTURE already names for absence);
   b) the short form `Own<TNo>?`; c) a word, `optional Own<TNo>`. In every
   form the compiler requires a test before use.

## Later, each its own change

- Arenas as an optional optimization for many small objects that die
  together.
- Destructors (change `lifecycle`): code that runs when an owner frees its
  object.
- `Pointer<T>`, address arithmetic and C interop behind an `Unsafe`
  boundary, never in the safe core.

## Order of work after approval

1. `Own<T>`, `New`, `Free`, `Move`, `Value`, the move analysis, owner
   absence (question 1); leak-checked tests, also with exceptions.
2. `Handle<T>`, `Alive`, `Same`, the generation check and its trap.
3. Constitution (FUTURE "Ownership, absence and failure" becomes law),
   grammar, manual, examples (list, tree with parent, game target).
