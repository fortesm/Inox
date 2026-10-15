## Why

Marcelo Fortes, 2026-10-13: "Talvez já fosse melhor pularmos para o peso
pesado da linguagem: Ponteiros, Composição com reuso de subrotinas,
Generics". Composition is decided (change `method-reuse`); what remains is
generics, "da forma mais simples possível e depois podemos evoluir" (Marcelo
Fortes, 2026-10-13). Pointers use them: a list of owners is `Vector<Own<T>>`
(change `safe-pointers` builds `Own<T>` and `Handle<T>` in without waiting).

Already law: generic arguments are written `<T>`, and `<...>` is generic only
in type position (CANON-8: "[T] pode gerar ambiguidade e problemas com
Arrays X[i]"). `Slice<T>` is the one generic type implemented. FUTURE: no
traits, protocols or interfaces as nominal entities.

Status: **Awaiting approval** of the decisions in design.md. Risk: high (a
new compiler phase: instantiation). No code before approval.

## What Changes (proposed, simplest form)

- Generic routines: `Max<T>(A T, B T) T :`; generic methods follow.
- Generic structs: `TPair<K, V> Struct` … `;`.
- No constraints and no protocols: each use is checked with its real types
  (`Max(3, 7)` checks the body with T = Integer); an error names the use and
  the line of the generic body.
- Type arguments are inferred from the arguments or the target; never
  written in an expression.
- One compiled copy per type (monomorphization), no run-time cost.
- Later, only if real programs ask for it: written constraints.
- Built-in generic types that follow, each its own change: `Option<T>`,
  `Result<T, E>`, `Vector<T>`.
