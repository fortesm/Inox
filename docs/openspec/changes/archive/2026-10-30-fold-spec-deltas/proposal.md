## Why

Four implemented changes kept their spec deltas outside
`docs/openspec/specs`: `debug-assert`, `design-by-contract`, `method-reuse`
and `safe-pointers`. The capability specs `stdlib/debug`,
`language/contracts` and `language/pointers` did not exist, although the
constitution points to `language/contracts`, and `language/structs` lacked
method reuse and methods of built-in types. Folding them was decided by
Marcelo Fortes, 2026-10-30.

Risk: normal (specs and records only; the compiler, the runtime and the
language do not change).

Status: **implemented** (canon r3.107).

## What Changes

1. Each delta was compared with the constitution (CANON-9, CANON-18,
   CANON-19, CANON-24, CANON-25) and the compiler; every test it names
   exists and passes.
2. Two texts were brought up to date before folding:
   - `language/pointers`: `Option<X>` of a value is an error until
     `Option<T>` comes, after methods of generic structs; it no longer
     says it waits for generics, which exist since r3.77.
   - `language/contracts`: an invariant may name private fields, and a
     write to a public field from another module checks it (r3.99), with
     its scenario (`tests/modules/invariant-after-public-write`).
3. The four changes are archived with their deltas applied; the new specs
   get their Purpose, and `language/structs` names its new topics.
4. Their last open tasks, the gates and CI, are ticked: main has been
   green on Linux and Windows since their merges, and the gates were run
   again here.

## Out of scope

No rule changes: every requirement folded was already law in the
constitution. The directions and designs not done stay open
(binary-streams, delimiters-and-annotations, lifecycle, routine-types,
variant-structs, safe-pointers-followups).
