<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# What object orientation gives, and Inox's answer

Inox is post-object-oriented (CANON-1): no classes, no inheritance, no
interfaces. This page keeps, in one place, what object-oriented languages
(Java, C#, Delphi, C++) still give a programmer, and how Inox answers each
point: already in the language, decided for the future, or still open.
Recorded on 2026-10-14 (canon r3.69 and r3.70) at the request of Marcelo
Fortes: "Registre tudo isso para eu não me perder ou esquecer". The law is
`openspec/specs/constitution/spec.md`; this page only points to it.

## Already in Inox

| What OO gives | Inox | Where |
|---|---|---|
| A behavior written once and shared by many types | Method reuse: `TArqueiro Struct (TSoldado)` offers `TSoldado.Correr` without writing it again | CANON-9; `examples/reuse-army.inox` |
| A type that runs a shared behavior differently | `Override` | CANON-9 |
| Contracts | `pre`, `pos`, `invariant`, `variant`, as in Eiffel (most OO languages have none) | CANON-24; `examples/contracts.inox` |

A behavior is written once per **type**, never per value: a thousand
soldiers in one array share one `Correr`, in Inox as in C or Java.

## Gaps, and the answer recorded for each

| # | What OO gives | Inox's answer | Status |
|---|---|---|---|
| 1 | One collection that mixes kinds, where one call runs each element's own version (dynamic dispatch) | Variant structs: a value that is one of several structs, examined with `case`; never subtyping. Today: one array per kind, as the Entity Component System of game engines | FUTURE (r3.69), change `variant-structs`, after `enum-range-set` |
| 2 | Open extension: a library calls types it does not know (frameworks, plugins, GUI events) through an interface | Routine types (a routine kept in a variable, field or parameter, as Pascal's procedural types); named routines first, closures after safe pointers. With generics, they cover most uses of interfaces | FUTURE (r3.70), change `routine-types` |
| 3 | Generic code over "anything that has `Correr`" | Generics in their simplest form: each use is checked with its real types, one compiled copy per type | Design awaiting approval, change `generics` |
| 4 | Shared objects and graphs (two lists see the same soldier, trees, parent links) | Safe pointers: `Ref<T>` (reference counted, never null), `Weak<T>`, arenas | Design awaiting approval, change `safe-pointers` |
| 5 | Encapsulation (`private` fields) | A design close to Object Pascal and Rust | FUTURE (r3.70), change `encapsulation`, design to write |
| 6 | Constructors and destructors | Today: defaults, named construction, module routines and invariants; String and File clean themselves. Whether more is needed is open | Maintainer's analysis (r3.70), change `lifecycle` |
| 7 | Ecosystem and habit: patterns, frameworks, tools, programmers who know it | Not a language feature: the standard library, a package manager (FUTURE), editor support and time | — |

## Override or routine type?

| | `Override` | Routine type |
|---|---|---|
| behavior varies per | type (every knight runs alike) | value (each soldier reacts its own way) |
| chosen at | compile time | run time |
| used for | specializing a type | callbacks, events, strategies, sorting, game AI |
