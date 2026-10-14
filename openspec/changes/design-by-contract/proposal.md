## Why

Marcelo Fortes, 2026-10-14: "agora implemente design by contract
exatamente igual eiffel a diferença é que as palavras reservadas pre para
precondições e pos para poscondições ensure deve ser outra palavra que não
conflite com o ensure de exceções varra a documentação de eiffel na
internet se houver implementações mais moderna e melhores me avise talvez
de java [...] se assegura de checar se llvm oferece primitivas que auxiliam
este desenho. Façao resto usual em documentar, testar e gerar codigo e
atualizar as especificações".

Inox already reserved `Pre`, `Pos` and `Invariant` (FUTURE) and has
`Std.Debug.Assert` (Eiffel's `check`). Eiffel's `ensure` is Inox's `pos`, so
it never meets `ensure` of try.

Status: **implemented**, canon r3.67. Risk: high (a lowering pass, new traps,
checks at call sites); every rule has a test.

## What Changes

- `pre Condition[, "message"]` and `pos Condition[, "message"]` on the first
  lines of a routine; `old X` and `Result` in a `pos`.
- `invariant Condition[, "message"]` lines in a Struct; a struct's invariant
  includes the invariants of the structs it reuses (CANON-9).
- An Override inherits the contracts it replaces: `pre else` (or else) and
  `pos then` (and then), Eiffel's `require else` and `ensure then`.
- `invariant` and `variant` on the first lines of a loop.
- A broken contract is a trap (CANON-19 categories), never an exception.
- New contextual words: `pre`, `pos`, `old`, `Result`, `invariant`,
  `variant` (a line that starts with one of them, not followed by `:=`).

## Where Inox differs from Eiffel, and why

1. **A violation is a trap, not an exception.** Eiffel raises an exception a
   `rescue`/`retry` may handle. Inox's law separates bugs (traps, CANON-19)
   from expected failures (exceptions, CANON-15: "contract violations stay a
   distinct concept"), as for Assert (decided 2026-10-13, option A).
2. **Always checked.** Eiffel lets each class choose a monitoring level (by
   default only `require`). Inox checks every contract, like Assert, which is
   never compiled out. A switch for production builds is a later decision.
3. **Invariants after outside writes.** Eiffel forbids writing another
   object's attributes; Inox has no visibility yet, so a field write or a ref
   mut argument from outside a struct's methods is followed by its invariant.
4. **Messages instead of tags.** Eiffel writes `positive: x > 0`; `:` only
   declares a routine in Inox, so a clause takes an optional message after a
   comma: `pre X > 0, "positive"`.
5. **Contracts only read, checked by the compiler.** Eiffel asks for
   side-effect-free assertions; Inox rejects a Self mut method, a ref mut
   argument, a file function or input inside a contract.
6. **Loop checks** run at the start of every iteration and when the loop ends,
   which observes the same states as Eiffel's "after initialization and after
   every iteration"; `for` has no variant (it always ends).
