## Why

FUTURE Concurrency asks for Chapel-style structured parallelism and
Go-inspired concurrency with data-race safety, spelled `do`, and an
approved design before any implementation. Marcelo Fortes asked on
2026-10-31 for an architectural study first: how Inox programs can run
tasks and operations on large data sets on several cores of one SMP
machine, with strong, verifiable guarantees against data races, without
making the programmer manage threads, locks or a borrow system, and
without an overly sophisticated compiler.

Status: **closed** (revision 4 approved as technical direction by Marcelo Fortes, 2026-10-31; section 26 of design.md; surface syntax decided the same day, section 27). No code, no change to the
constitution or the specs. `design.md` holds the study, the alternatives,
a recommendation and the decisions that belong to the maintainer.

Risk: high (when a direction is approved: runtime ABI, memory ownership,
a new lowering).

Touches, if a direction is approved: FUTURE Concurrency, CANON-1 (identity
and safety defaults), CANON-5 State, CANON-14 Strings, CANON-15
exceptions, CANON-19 traps, CANON-24 contracts, CANON-25 owners and
Links, ADR-0016 (ref mut exclusivity), ADR-0017 (slices); open changes
`delimiters-and-annotations` (`| |` capture clause, `{ }` effect clause),
`routine-types`, `lifecycle`, `safe-pointers-followups`.

## Scope of the study

In: shared-memory SMP in user space; task concurrency and data
parallelism; transfer, sharing and synchronization of data; memory model;
errors and cancellation; determinism of reductions; what the compiler,
the runtime and the library each own.

Out: kernels, OS schedulers, drivers; low-level threads exposed to the
programmer; distributed arrays, locales, transparent distribution; a Rust
lifetime system; interfaces, classes or a new type system for
concurrency.
