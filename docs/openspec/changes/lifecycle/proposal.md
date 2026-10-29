## Why

Object-oriented languages run code when a value is born (constructor) and
when it dies (destructor). Marcelo Fortes, 2026-10-14: "eu preciso analisar
o caso dos construtores e destrutores, e se precisaremos", and "Registre
tudo isso para eu não me perder ou esquecer".

Status: **open analysis of the maintainer** (FUTURE "Encapsulation, routine
types and lifecycle", r3.70). Nothing is decided; no code.

## What Inox has today

- Birth: every variable is born with a value (CANON-5): field defaults
  (`FVida Integer := 100`) and named construction
  (`TConta(TTitular := T, FSaldo := 10)`). A routine of the module
  (`NovaConta(Nome String) TConta`) plays the constructor when the value
  needs computation, and a struct `invariant` (CANON-24) checks every new
  value.
- Death: String and File clean themselves (reference counting; a File
  closes when its routine exits by any path, also an exception). A struct
  that holds them is cleaned with them. There is no code of the programmer
  that runs at death.

## Questions for the analysis

1. Constructors: are module routines plus invariants enough? With hidden
   fields (change `encapsulation`), a routine of the module becomes the only
   way to build such a value, as in Rust.
2. Destructors: they matter when a value owns something the runtime does
   not know (a texture, a socket, a lock), above all with `Own<T>` (change
   `safe-pointers`), where the owner's release is the moment to
   release it. Options: none; a reserved method (`TName.Finalizar(Self
   mut)`, like Rust's Drop or Delphi's destructor) called exactly once when
   the value dies; or only `ensure` blocks.
3. Order of destruction of parts (CANON-9) and of array elements; what
   happens if a destructor raises.
