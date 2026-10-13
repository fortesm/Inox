## Why

Inox has no inheritance (CANON-1): "composition instead of inheritance".
Fields compose data; what is missing is reuse of behavior — "composição em
vez de herança (com funções compostas reutilizáveis, não só campos)"
(design chat; the maintainer's memory notes that reuse of routines by
composition was still undecided). Marcelo Fortes, 2026-10-13, asked for it
among the heavy features: "Composição com reuso de subrotinas".

Status: **Awaiting approval** (design.md lists the decisions). Risk: normal
for embedding (resolved at compile time, no new runtime), high for protocol
values if they are chosen.

## What Changes (proposed)

- **Embedding**: a struct names another struct type as a field without a
  name; the outer struct gets the inner one's fields and associated
  routines, called on the inner part. Static, explicit, no subtyping, no
  virtual dispatch.
- **Protocols**: `TShape Protocol` … `;` lists routines; a struct declares
  the protocols it satisfies (no duck typing); generic code accepts
  `<T TShape>` (change `generics`).
- Later, only if wanted: protocol values for lists of different types
  (dynamic dispatch through a table of routines).
