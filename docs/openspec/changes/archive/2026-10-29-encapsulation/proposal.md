## Why

Every symbol of a module is public today (CANON-3), and every field of a
struct can be written from outside it; a struct invariant (CANON-24) is
checked after such a write, but the write is not prevented. Object-oriented
languages hide fields behind `private`.

Marcelo Fortes, 2026-10-14: Inox will have encapsulation, close to Object Pascal and Rust, still to be designed, and
record encapsulation in FUTURE so it is not lost.

Status: **superseded** by `modules-encapsulation` (r3.99), which answers
the questions below: private by default, privacy per module, a struct with
a private field without default built only by its module, reuse across
modules sees public members, the invariant still checked after outside
writes to public fields.

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
