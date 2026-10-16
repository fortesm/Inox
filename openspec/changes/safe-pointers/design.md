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
| Address of a variable | `@X` | `&x` | `Handle(X)`: any variable, field or array element |
| Non-owning reference to a heap object | — | — | `Handle(P)` |
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

A navigator over an array of records (Marcelo Fortes's example,
2026-10-16): a Handle to the array and a Handle to the current record,
whose fields are read and written in place, with no copy.

```
Const Tamanho := 5

Type
    TContato Struct
        FNome String := ""
        FTelefone String := ""
        FIdade Integer := 0
    ;
    TNavegador Struct
        FTabela Handle<Array[1..Tamanho] TContato>
        FIndice Integer := 1
    ;

TNavegador.Primeiro(Self mut) :
    Self.FIndice := 1
;
TNavegador.Ultimo(Self mut) :
    Self.FIndice := Tamanho
;
TNavegador.Proximo(Self mut) :
    pre not Self.NoFim, "não há próximo"
    Self.FIndice += 1
;
TNavegador.Anterior(Self mut) :
    pre not Self.NoInicio, "não há anterior"
    Self.FIndice -= 1
;
TNavegador.NoInicio(Self) Bool :
    Return Self.FIndice = 1
;
TNavegador.NoFim(Self) Bool :
    Return Self.FIndice = Tamanho
;
TNavegador.Atual(Self) Handle<TContato> :
    Return Handle(Self.FTabela[Self.FIndice])   == the record inside the array
;

Main :
    Contatos Array[1..Tamanho] TContato
    Contatos[1] := TContato(FNome := "Ana", FTelefone := "1111", FIdade := 30)
    ...
    Nav := TNavegador(FTabela := Handle(Contatos))  == Handle of a local: no cost
    Nav.Primeiro
    PutLn(Nav.Atual.FNome, " - ", Nav.Atual.FTelefone)
    Nav.Ultimo
    Nav.Atual.FIdade += 1                           == writes the record itself
    PutLn(Contatos[Tamanho].FIdade)
;
```

"Next" in an array is the index plus one, never pointer arithmetic; in a
doubly linked list it is a pointer (`FProximo Option<Own<TNo>>`,
`FAnterior Handle<TNo>`).

A Handle that outlives its variable compiles, and is detected:

```
Vazar Handle<Integer> :
    X Integer := 42
    Return Handle(X)          == compiles: X is placed in a generation block
;

Main :
    H := Vazar
    if Alive(H)               == false: X ended with Vazar
        PutLn(Value(H))
    ;
    PutLn(Value(H))           == trap: "use of a freed object", never garbage
;
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
6. `Handle(X)` makes a Handle of an owned object, of any variable (local,
   parameter, State), of a field or of an array element (Marcelo Fortes,
   2026-10-15/16: "Corretamente! É isso mesmo"; "1 - Com Handle"). A Handle
   is copied freely, keeps nothing alive, and is born empty.
7. Every access through a Handle checks its generation: a dead or empty
   Handle traps (CANON-19, "use of a freed object"); `Alive(H)` asks without
   trapping. The check is removed when the compiler proves no `Free` can
   happen in between; a future static proof (FUTURE, SPARK style) can
   remove the rest.
8. `=` between two pointers is a compile error; `Same(A, B)` asks whether
   two references reach the same object, and `Value(A) = Value(B)` compares
   contents.
9. A pointer cannot be printed, converted to a number, or built from one.
10. **Permissive with detection** (Marcelo Fortes, 2026-10-16: "2 -
    permissivo com detecção"): a Handle may go anywhere (returned, stored
    in a field, an array or State), as freely as a Pascal pointer. The
    compiler chooses the cost: when a Handle provably stays inside the block
    of the variable it points to, the variable stays on the stack and the
    Handle costs nothing; when it may escape, the variable is placed in a
    generation block (as Go moves an escaping variable to the heap, but with
    no collector) that is freed when the variable's scope ends, and a use
    after that finds `Alive` false or traps. Never a wrong value (CANON-1).
    Pascal's freedom without its dangling pointers; Ada's compile-time
    refusal is not needed.

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
