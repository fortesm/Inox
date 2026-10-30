## Why

Inox has no inheritance (CANON-1), and composition of data was all it had:
reusing a struct's methods in another struct meant forwarding methods at
every level. Marcelo Fortes asked for "Composição com reuso de subrotinas"
(2026-10-13) and, after comparing Go embedding, Rust traits, a micro-contract
and Object Pascal classes, chose composition with reuse of methods:
"Então mediante tudo isso podemos ter uma forma de composição mais
simplificada que a de rust pois não somos afetados por muito que ele tem.
Vamos só ressiginficar as coisas, troque herança por reaproveitamento de
métodos isso já satifaz!" and approved the four decisions of design.md:
"Aprovo tudo como você sugeriu 1a, 2a, 3b e 4a+b" (Marcelo Fortes,
2026-10-13). The ChatGPT discussion he shared (micro-contract, receiver
rebinding, member mapping) was read and set aside by that choice: the
contract and the mapping are not needed while every reused member keeps its
name.

Status: **approved and implemented**, canon r3.66. Risk: normal (resolved at
compile time; no runtime, no dispatch table, no subtyping).

## What Changes

- `TName Struct (TPart, ...)` reuses structs. Each part is a field named
  after its type (`A.TPessoa`); its fields and methods are offered by the
  struct directly, at every level (decision 1a).
- A reused method runs on its part (delegation, decision 2a):
  `A.UpdateNome(X)` is `A.TPessoa.UpdateNome(X)`.
- `TName.Method(...) Override :` replaces a reused method for callers of the
  struct, with the same signature; without `Override`, a method with a
  reused name is an error (decision 3b).
- Any name reachable by two paths, or a struct's own field with a reused
  name, is an error at the struct's declaration. No subtyping: a struct is
  never its part.
- Methods of built-in value types: `String.UpdateStr(Self mut, V String) :`
  and `A.FPlano.UpdateStr("x")`; `Self mut` is the caller's variable
  (decision 4a). A field of `Self mut` is a writable place for `ref mut`
  arguments (decision 4b).
- Conformance fixes found on the way: `Self` (read-only) can no longer
  write its receiver; a field of a value parameter cannot be assigned; a
  `with` binding is as writable as its target; a `Self mut` method needs a
  writable receiver; a `ref mut` argument cannot reach the receiver.
- The constitution: CANON-1 and CANON-9 describe method reuse; the FUTURE
  requirement on contracts and protocols becomes neutral (no nominal traits,
  protocols or interfaces are decided).
