## Why

A game keeps thousands of soldiers. With method reuse (CANON-9) the
behavior is written once per type: `TSoldado.Correr` serves every
TSoldado, `TArqueiro Struct (TSoldado)` reuses it without writing it again,
and `TCavaleiro.Correr(Self mut) Override` runs differently
(`examples/reuse-army.inox`). Each kind lives in its own array, which is
also how modern game engines organize data (Entity Component System: Unity
DOTS, Bevy, Overwatch).

What object-oriented languages still give, and Inox does not, is one
collection that mixes kinds (archers and knights together) where one call
`S.Correr` runs the right version for each element. That needs subtyping,
which CANON-9 rejects. The direction recorded here keeps CANON-9: a
**variant struct**, a value that is one of several structs, examined with
`case`.

Marcelo Fortes, 2026-10-14, after the explanation above ("Se um dia a lista
misturada fizer falta, o caminho natural no Inox é um tipo variante com
case"): "Sim, registre isso".

Status: **direction recorded**, design to be written after change
`enum-range-set` (the variant's tag is an enum). Risk: high (new type
layout, `case` lowering, String ownership inside a variant). No code before
the design is approved.

## What Changes

- Constitution FUTURE "Modules, variants and numbers": variant structs are
  how a collection mixes kinds without subtyping.
- `examples/reuse-army.inox`: one behavior for thousands of soldiers, three
  kinds, one Override, one array per kind.
- Manual (method reuse): why there is no mixed collection today and what
  replaces it.

## Open questions for the design (not decided)

1. Syntax of the declaration (for example `TTropa Variant (TSoldado,
   TArqueiro, TCavaleiro)`).
2. How `case` names the member and gives access to its fields.
3. Whether a method that every member offers with the same signature
   (`Correr`, reused or overridden) may be called on the variant directly,
   the compiler generating the `case` (closed dispatch, no subtyping, no
   table of functions), or must always be written as a `case`.
4. Default value of a variant (CANON-5: every variable is born with a
   value): the first member with its defaults, or an explicit initializer.
