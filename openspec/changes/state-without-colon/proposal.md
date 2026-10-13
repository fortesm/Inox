## Why

CANON-4 gives `:` one meaning — it declares a routine and opens its body —
but the State section was written `State :` … `;`, the only other use of
`:`. The maintainer decided (Marcelo Fortes, 2026-10-12): "O State não deve
usar :", and explained the idea behind it: "Mas eu precisava de um caractere
que dissesse que ali começaria o corpo da sub-rotina dai veio o : … : é
desnecessário em outros blocos como if, for, while, with, etc... A ideia de
Inox é escrever menos. Fazer mais."; "`:` tanto serve para abrir o bloco
quanto serve para dizer pro compilador que esse Sum(...) é uma subrotina!".
Asked whether the section keeps its closing `;`, he chose (2026-10-13):
"Igual ao Type" — no `:` and no `;`.

Risk: normal (parser of one section; no lowering, runtime or ownership
change).

## What Changes

- `State` is a section like `Type`: `State`, then one declaration per line;
  it ends where the next module item starts (Use, Type, Const, State, Var or
  a routine header). A module may have several State sections.
- Rejected with a migration message: `State :`, a closing `;` after the
  State entries, and the old entry form `Name : Type` in State and Const
  (`:` only declares a routine).
- The constitution records the idea of the syntax ("write less, do more")
  and the double role of `:` in the maintainer's words (CANON-4); CANON-5
  "State" changes; specs `language/module-state` and
  `language/declaration-sections` change.
- Every program, test, gap probe and manual example with `State :` is
  migrated; grammar `state_section` changes.

Constitution requirements touched: CANON-4 (Semicolon closes, colon
declares; Sections and removed forms), CANON-5 State, the section "The idea
of the syntax". Out of scope: Type and Const (already without `:`).
Behavior that must not change: the meaning of every State program.
