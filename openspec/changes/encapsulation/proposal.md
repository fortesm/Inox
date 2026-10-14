## Why

Every symbol of a module is public today (CANON-3), and every field of a
struct can be written from outside it; a struct invariant (CANON-24) is
checked after such a write, but the write is not prevented. Object-oriented
languages hide fields behind `private`.

Marcelo Fortes, 2026-10-14: "é previsto encapsulamento para Inox, algo bem
semelhante a do ObjectPascal + Rust mas precisamos desenhar ainda", and
"Sim registre no future para não nos perdermos a questão do
encapsulamento".

Status: **direction recorded** (FUTURE "Encapsulation, routine types and
lifecycle", r3.70); the design is still to be written. No code before the
design is approved.

## Starting points for the design

- Object Pascal: `private` (visible in the unit), `strict private` (only the
  type's own methods), `protected`, `public`, `published`; the
  interface/implementation sections of a unit.
- Rust: everything private by default; `pub`, `pub(crate)`, `pub(super)`;
  privacy is per module, and a struct with a private field can only be built
  inside its module (which makes "constructors" functions of the module).
- Inox already reserves `Export` and interface/body separation (FUTURE
  "Modules, variants and numbers").

## Open questions (not decided)

1. Default: public (Pascal, today's Inox) or private (Rust)?
2. Granularity: per module, per struct (`strict private`), or both?
3. Can a struct with hidden fields be built with named construction outside
   its module, or only through a routine of the module?
4. Hidden fields and method reuse (CANON-9): does a struct that reuses
   `TPessoa` see the hidden fields of its part?
5. With hidden fields, the invariant check after outside writes (CANON-24)
   becomes unnecessary for them, as in Eiffel.
