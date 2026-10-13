## Why

Linked lists, trees, graphs and shared objects need values that refer to
other values. Inox forbids unsafe pointers in its safe core (CANON-1) and
records the direction "ownership beyond Self/Self mut and ref/ref mut: Self
owned, moves, arenas and regions; unsafe boundaries, Pointer<T> and C
interop outside the safe core" (FUTURE). The design chat discussed "ponteiros
seguros com arenas", and the language direction recorded "ARC-based memory
management" (Strings and Files are already reference-counted). Marcelo
Fortes, 2026-10-13: "Ponteiros".

Status: **Awaiting approval** (design.md lists the decisions). Risk: high
(memory ownership, runtime ABI). Depends on change `generics` (`Ref<T>`,
`Option<Ref<T>>`).

## What Changes (proposed)

- `Ref<T>`: a reference-counted heap object (ARC, like Strings): never null,
  freed deterministically when the last reference goes, no garbage
  collector. `Weak<T>` breaks cycles.
- Absence is `Option<Ref<T>>`, never null.
- `Arena`: a region whose objects are all freed together when it goes out of
  scope; references into it cannot escape it (phase 2).
- `Pointer<T>`, address arithmetic and C interop only inside a future
  `Unsafe` boundary, never in the safe core.
