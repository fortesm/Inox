## Why

Linked lists, trees, graphs and objects that refer to each other need values
that point to other values. Inox forbids unsafe pointers in its safe core
(CANON-1) and records the direction "ownership beyond Self/Self mut and
ref/ref mut: Self owned, moves, arenas and regions; unsafe boundaries,
Pointer<T> and C interop outside the safe core" (FUTURE). Marcelo Fortes
raised pointers on 2026-10-13 and chose on 2026-10-15 to start with safe
pointers.

The maintainer's goals (2026-10-15): freed memory is never reused in the
same context, and nothing refers to an invalid address or to a variable
without a value; no pointer arithmetic; the safety of Pascal, Go (without
its garbage collector) and Ada 2005; a syntax neither as clumsy as Pascal's
nor as cryptic as C's.

Status: **design approved** (Marcelo Fortes, 2026-10-16); implementation follows tasks.md. Risk: high (memory
ownership, runtime ABI). No code before the design is approved.

## Decisions taken (Marcelo Fortes, 2026-10-15)

- **No reference counting:** it would repeat Delphi's problems. `Ref<T>` and `Weak<T>` of the first draft are dropped.
  (String and File keep their internal counts: invisible, immutable or
  never copied, no cycles; an implementation detail, not a pointer.)
- **No garbage collection,** in any form.
- **No arena now;** arenas may come later as an option.
- **One model: a single owner and checked references** (`Own<T>`,
  `Link<T>`), with every recommendation accepted (explicit `Move`, `Value(P)`, types written like
  `Slice<T>`, built-in types without waiting for user generics, the owner
  frees at the end of its block, a dead Link traps and `Alive` tests it,
  the unsafe layer much later,
  destructors decided in change `lifecycle`).
- **Links to any variable, field or array element** (2026-10-15/16),
  through a checked reference.
- **Permissive with detection** (2026-10-16): a Link may go anywhere; one that outlives its variable is
  detected (`Alive` false or a trap), never dangling.
- **Names and absence** (2026-10-16): `Link<T>`, not to be confused with
  Delphi's or Windows' handles, replaces
  `Handle<T>`; `Value`, `New` and `Free` stay; an absent owner is
  `Option<Own<TNo>>` (the word `Optional` as a second spelling is not
  adopted for now; see design.md).
- **An empty Link:** a Link may be born empty (`Alive` is false), so it
  serves as "no parent" without an `Option`.

## What Changes (proposed)

- `Own<T>`: the single owner of a heap object. `New(Value)` creates it; it
  is freed when its owner's block ends or by `Free(P)`; it is never copied,
  only moved with `Move(P)` (the rule of File, CANON-17).
- `Link<T>`: a reference that does not own, to an owned object or to any
  variable, field or array element (`Link(X)`). Each use checks a generation
  number, so it never reaches a freed or reused object: `Alive(H)` tests
  it, and the use of a dead Link is a trap. A Link is born empty.
- Field access with a dot (`P.FValor`, `H.FValor`), never `^` or `->`;
  `Value(P)` is the whole value. No arithmetic, no integer-to-pointer
  conversion, no null.
- Arenas: a possible optional optimization later. `Pointer<T>`, address
  arithmetic and C interop only inside a future `Unsafe` boundary, never in
  the safe core.
