# Inox Roadmap

## 0.1 pre-alpha: safe compiled core

Closed language decisions are recorded in `docs/decisions/ADR-0006-inox-0.1-constitution.md` and `docs/canonical/language-reference.md`.

Priority implementation items:

1. Keep current tests passing on Windows/MSVC and Linux/GCC/Clang.
2. Enforce canonical syntax already decided: `Var` without `:`, `Type` without `:`, canonical associated `Self`/`Self mut`, integer `/` rejection.
3. Extend the implemented Clang-backed `--build` and `--run` driver beyond the
   minimum local-module workflow as needed.
4. Complete `case` lowering and enum exhaustiveness checks now that the canonical parser form is in place.
5. Extend the implemented local multi-file `Module`/`Use` support with future
   export, visibility, and package decisions.
6. Implement arrays with `Array[Low..High] T`, indexing, bounds checks, `Low`, `High`, `Length`.
7. Implement `Enum`, `Range`, `Ord`, enum range `for`.
8. Implement `Set[TEnum]` / `Set[TRange]` if time allows.
9. Extend the initial portable `stdlib/` modules without adding GC, unsafe
   features, or C interop; define canonical trap/abort before implementing
   `Std.Debug.Assert`.

## 0.2+ design topics

These are not open ambiguities; they are deferred architectural work:

- ownership/borrow system beyond receivers;
- `Self owned`;
- `ref` / `ref mut` parameters;
- arena allocation and deterministic memory regions;
- unsafe boundaries, pointers, and C interop;
- contracts/protocols/behaviors;
- parallelism inspired by Chapel and Go, without unsafe shared mutable defaults;
- concurrency data-race safety;
- vector implementation with move semantics;
- string runtime and Unicode library;
- module visibility/export;
- interface/body separation;
- variant structs;
- JSON/DB metadata tags;
- package/build manager.
