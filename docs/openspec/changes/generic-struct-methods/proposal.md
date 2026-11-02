## Why

A generic struct (`TPair<K, V>`, `TStack<T>`, CANON-8, r3.77) cannot have
methods: `TPair<K, V>.Swap(Self mut)` is rejected ("methods of generic
structs are not in Inox yet"), and a program writes a generic routine
instead (`Push<T>(ref mut S TStack<T>, X T)`). Every collection of 0.5
(`Option<T>`, `Result<T, E>`, `Vector<T>`) needs methods on a generic
struct, so this comes first.

Approval of the direction: Marcelo Fortes, 2026-11-01: the 0.5 items come
before F5-F7 (deferred, not cancelled), in the order methods of generic
structs; critical safe-pointers follow-ups; `Option<T>` and
`Result<T, E>`; `Vector<T>` and Clone. This change is the first. Its
design waits for his dated approval before any code.

Touches: CANON-8 (generics), CANON-9 (methods, `Self`), CANON-24
(contracts), CANON-3 (modules); the capability spec `language/generics`
(the requirement "Methods of generic structs are not yet in the language"
becomes the rule) and the FUTURE line of the constitution that names them.
Compiler phases: 2 parser (accepts the header it rejects today), 3 module
resolution (no new rule), 4 contract lowering (generic wrappers, as for
generic routines), 5 semantic analysis (instantiation on demand, method
lookup), `ast` (`methodDot`, `inox interface`). Codegen: none (an instance
is an ordinary routine). `docs/ARCHITECTURE.md` updated when implemented.

Risk: normal (a language feature on the existing generic machinery; no
exception lowering, runtime ABI or ownership change). Design first, as
asked.

Status: **design awaiting approval**. No code before it.

## What Changes

1. `TPair<K, V>.Swap(Self mut) :` declares a method of every instance of
   `TPair`; `P.Swap` on a `TPair<String, Int64>` instantiates it on first
   use, with K = String and V = Int64, and calls it.
2. The method's type parameters are the struct's, in order; `Self` and
   `Self mut` are the instance; contracts (`pre`, `pos`) work as on any
   method; the method is declared in its struct's module and seen from
   others when Public.
3. Diagnostics name the instance and point at the generic body (F2).
4. Out of this change (decisions Q1-Q5 of `design.md`): type parameters of
   the method itself, invariants and reused parts of a generic struct,
   Override.

## Decisions

See `design.md`, "Decisions asked of the maintainer" (Q1-Q5).
