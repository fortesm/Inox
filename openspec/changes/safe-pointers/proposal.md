## Why

Linked lists, trees, graphs and objects that refer to each other need values
that point to other values. Inox forbids unsafe pointers in its safe core
(CANON-1) and records the direction "ownership beyond Self/Self mut and
ref/ref mut: Self owned, moves, arenas and regions; unsafe boundaries,
Pointer<T> and C interop outside the safe core" (FUTURE). Marcelo Fortes,
2026-10-13: "Ponteiros"; 2026-10-15: "Que tal se começarmos por Safe
Pointers?".

The maintainer's goals (2026-10-15): "Não deixar um endereço de memória que
foi liberado ser reutilizado no mesmo contexto. Não deixar fazer
referências a um endereço inválido, ou a uma variável sem valor", with no
pointer arithmetic, keeping the safety of Pascal, Go (without its garbage
collector) and Ada 2005, and a syntax neither "feia e desajeitada" (Pascal)
nor "críptica" (C).

Status: **design rewritten 2026-10-15 from the maintainer's decisions;
awaiting approval of design.md** (one open question). Risk: high (memory
ownership, runtime ABI). No code before the design is approved.

## Decisions taken (Marcelo Fortes, 2026-10-15)

- **No reference counting:** "bom se usarmos contagem por referência,
  ficamos igual a Delphi e isso tem problemas. então essas estariam
  descartadas". `Ref<T>` and `Weak<T>` of the first draft are dropped.
  (String and File keep their internal counts: invisible, immutable or
  never copied, no cycles; an implementation detail, not a pointer.)
- **No garbage collection:** "não quero ter de forma alguma, garbage
  collection".
- **No arena now:** "E se não houvesse nem Arena, nem GC, nem Contagem de
  referência [...]?"; "depois podemos ter arena no futuro opcionalmente".
- **One model: a single owner and checked references** (`Own<T>`,
  `Handle<T>`), with the recommendations accepted: "Aceito todas as suas
  recomendações" (explicit `Move`, `Value(P)`, types written like
  `Slice<T>`, built-in types without waiting for user generics, the owner
  frees at the end of its block, a dead Handle traps and `Alive` tests it,
  a Handle points only to an owned object, the unsafe layer much later,
  destructors decided in change `lifecycle`).
- **An empty Handle:** "Acatado: um `Handle` pode simplesmente nascer vazio
  (`Alive` = false), e então ele mesmo serve como 'sem pai' sem precisar
  de `Option`".

## What Changes (proposed)

- `Own<T>`: the single owner of a heap object. `New(Value)` creates it; it
  is freed when its owner's block ends or by `Free(P)`; it is never copied,
  only moved with `Move(P)` (the rule of File, CANON-17).
- `Handle<T>`: a reference that does not own. Each use checks a generation
  number, so it never reaches a freed or reused object: `Alive(H)` tests
  it, and the use of a dead Handle is a trap. A Handle is born empty.
- Field access with a dot (`P.FValor`, `H.FValor`), never `^` or `->`;
  `Value(P)` is the whole value. No arithmetic, no integer-to-pointer
  conversion, no null.
- Arenas: a possible optional optimization later. `Pointer<T>`, address
  arithmetic and C interop only inside a future `Unsafe` boundary, never in
  the safe core.
