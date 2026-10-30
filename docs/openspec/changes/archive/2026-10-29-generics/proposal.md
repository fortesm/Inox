## Why

Marcelo Fortes, 2026-10-13: perhaps move on to the heavy parts of the language: pointers, composition with reuse of routines, generics. Composition is decided (change `method-reuse`); what remains is
generics, in the simplest possible form, to grow later (Marcelo
Fortes, 2026-10-13). Pointers use them: a list of owners is `Vector<Own<T>>`
(change `safe-pointers` builds `Own<T>` and `Handle<T>` in without waiting).

Already law: generic arguments are written `<T>`, and `<...>` is generic only
in type position (CANON-8: `[T]` could be ambiguous with array indexing `X[i]`). `Slice<T>` is the one generic type implemented. FUTURE: no
traits, protocols or interfaces as nominal entities.

Status: **approved and implemented** (canon r3.77). Risk: high (a new
compiler phase: instantiation). The three decisions of design.md were
answered with the recommendations:
Marcelo Fortes, 2026-10-17: the recommendations for the three questions are approved
No constraints for now (`{ }` stays reserved), inference only, and T may be
any type, also one that is only moved, with the error at the instance.

## What Changes (simplest form)

- Generic routines: `Max<T>(A T, B T) T :`.
- Generic structs: `TPair<K, V> Struct` … `;`. Their methods, and
  invariants and reused parts in them, come in a later change; a generic
  routine does the job meanwhile (`Push<T>(ref mut S TStack<T>, X T)`).
- No constraints and no protocols: each use is checked with its real types
  (`Max(3, 7)` checks the body with T = Integer); an error names the use and
  the line of the generic body.
- Type arguments are inferred from the arguments or the target; never
  written in an expression.
- One compiled copy per type (monomorphization), no run-time cost.
- Later, only if real programs ask for it: written constraints.
- Built-in generic types that follow, each its own change: `Option<T>`,
  `Result<T, E>`, `Vector<T>`.
