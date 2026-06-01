# Inox Open Questions

There are no blocking language-design questions for the current 0.1 safe-core path. Decisions 1-20 have been consolidated in:

- `docs/canonical/language-reference.md`
- `docs/decisions/ADR-0006-inox-0.1-constitution.md`
- topic files under `docs/canonical/`
- `AGENTS.md`

## Deferred but directionally decided

These items are not permission for agents to invent semantics. They are deferred architectural topics and require explicit ADRs before implementation:

- ownership/borrow model beyond `Self`/`Self mut`;
- `Self owned`;
- `ref` and `ref mut` parameters;
- arena memory management;
- unsafe blocks, `Pointer[T]`, and C interop;
- contracts/protocols/behaviors;
- Chapel-style structured parallelism;
- data-race safety and non-mutable concurrent data defaults;
- vector runtime and move semantics;
- full string/Unicode runtime;
- full module export/visibility system;
- variant structs and metadata tags.

## Known implementation conformance gaps

The canonical specification may be ahead of the compiler in these areas:

- canonical `case Expression` without `:`;
- module exports, visibility, package search, and richer linking beyond the
  minimum local `Module`/`Use` driver;
- arrays, enum, range, set implementation;
- vector implementation;
- checking-mode overflow traps;
- final runtime ABI.

When closing a gap, update code, tests, docs, HTML manual, and ADRs together.
