## Why

Marcelo Fortes, 2026-10-13: "Talvez já fosse melhor pularmos para o peso
pesado da linguagem: Ponteiros, Composição com reuso de subrotinas,
Generics". Generics come first because the other two need them: a pointer
that may be absent is `Option<Ref<T>>`, a list of nodes is `Vector<T>`, and
protocols (change `composition`) are the constraints of generic code.

Already law: generic arguments are written `<T>`, and `<...>` is generic only
in type position (CANON-8, decided in the design chat: "[T] pode gerar
ambiguidade e problemas com Arrays X[i]"). `Slice<T>` is the one generic
type implemented. Direction (FUTURE): constraints tied to
contracts/protocols, never duck typing.

Status: **Awaiting approval** of the decisions below (design.md). Risk:
high (a new compiler phase: instantiation). No code before approval.

## What Changes (proposed)

- Generic structs: `TPair<K, V> Struct` … `;` in a Type section.
- Generic routines: `Max<T Ordered>(A T, B T) T :` — the type parameters
  follow the name; a constraint is written like any Inox declaration, `Name
  Type`, without `:`.
- The type arguments of a call are inferred from the arguments (or from the
  target type); they are never written in an expression, so `<` stays a
  comparison there.
- Constraints: the built-in protocols `Equatable`, `Ordered`, `Numeric` now;
  user protocols with change `composition`.
- Compilation by monomorphization: each distinct instance is compiled once,
  with no run-time type information and no cost at run time; a generic body
  is checked once, against its constraints, so errors point at the generic
  code, not at each use.
- Built-in generic types that follow: `Vector<T>`, `Option<T>`,
  `Result<T, E>`, `Set<T>` (each its own change).
