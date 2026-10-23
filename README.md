# Inox

**A compiled, strongly typed, post-object-oriented systems language designed for software where silent failure is unacceptable.**

Inox prioritizes explicit safety, deterministic behavior, and clarity over convenience when the two conflict. It is intended for mission-critical domains in which buffer overflows, null dereferences, silent integer overflow, unchecked mutation, or hidden aliasing can cost lives, capital, or infrastructure.

## Design Philosophy

Inox is deliberately **post-object-oriented**. It has no classes, classical inheritance, Java-style interfaces, mixins, or duck typing. Data lives in nominal value types (`Struct`). Behavior lives in free functions, subroutines, and associated methods declared outside the struct. The familiar call form `Object.Method(args)` is retained for ergonomics without turning data into classical objects.

Core safety defaults include:

- No universal `null` or `nil`
- No unsafe pointers in the language core
- No silent integer overflow
- `/` divides by the operands' type, as in Ada: integer division for integers, real for reals; never mixed. `mod` and `rem` as in Ada
- Parameters immutable by default
- Mutating methods require an explicit `Self mut` receiver
- Strong nominal typing with no implicit narrowing
- Composition instead of inheritance: `TSubscriber Struct (TPerson)` reuses the fields and methods of `TPerson`, at every level, with `Override` to replace a reused method
- Design by contract as in Eiffel: `pre` and `pos` (with `old` and `Result`) on routines, `invariant` on structs, `invariant` and `variant` on loops; a broken contract stops the program
- Safe pointers without reference counting or garbage collection: one owner `Own<T>` that is moved, never copied, and frees its object deterministically; checked references `Link<T>` that never reach a freed object
- Unicode Strings with `+` that appends in place, and Std.Strings / Std.StrUtils functions (Substr, Position, Trim, UpperCase, SameText...) that count characters, never bytes, and follow no single human language
- Generics without constraints or run-time cost: `Max<T>(A T, B T) T` and `TPair<K, V> Struct` get one compiled copy per list of types, inferred from the arguments, never written in a call

The language draws carefully from several traditions for robustness, discipline, modular clarity, ergonomics, performance realism while rejecting defaults that introduce undefined behavior or ambiguity.

## Target Domains

Inox is aimed at systems where correctness and predictability matter:

- Aviation and air-traffic control  
- High-precision industrial systems  
- Finance, exchanges, and monetary infrastructure  
- Scientific and numerical computing  
- Aerospace, medicine, and hospital equipment  
- Energy infrastructure (nuclear, hydroelectric, grids)  
- Cryptography and large-scale parallel computation  

## Implementation

The reference compiler is written in portable C++20 and targets LLVM. Windows x64 (Clang, MSVC ABI) and Linux x64 are the primary targets; both run the full regression suite in CI, including native exception handling (MSVC funclets on Windows, Itanium landing pads on Linux). The language core does not rely on a tracing garbage collector; memory management favors explicit ownership, moves, and deterministic resource control (arenas may come later as an option).

Inox is at version 0.3.0, pre-alpha: the complete compiler pipeline (lexer → parser → semantic analysis → LLVM IR → executable) works, and the language and its standard library are growing toward 1.0. What works today and what is still missing is in [`docs/STATUS.md`](docs/STATUS.md).

## Building

See [`BUILD_AND_TEST_INSTRUCTIONS.md`](BUILD_AND_TEST_INSTRUCTIONS.md) for current build, test, and release instructions on Windows and Linux.

## Documentation

- The law of the language — the constitution: [`openspec/specs/constitution/spec.md`](openspec/specs/constitution/spec.md), with the engineering directive, the current revision and the open questions next to it, and one detailed spec per topic in [`openspec/specs/`](openspec/specs/).
- The manual: [`docs/site/index.html`](docs/site/index.html).
- Status and plans: [`docs/STATUS.md`](docs/STATUS.md), [`docs/ROADMAP.md`](docs/ROADMAP.md), [`CHANGELOG.md`](CHANGELOG.md).
- How to contribute (humans and AI agents): [`AGENTS.md`](AGENTS.md).

Everything else (README files, the manual, examples) derives from the constitution and is corrected when it disagrees.

## Status

Inox is not yet production-ready. The design is stable in its core principles; the implementation continues to expand toward a complete, reliable 1.0 while preserving the safety and engineering standards established for the language.

## License

Mozilla Public License 2.0 (MPL-2.0).

## Author

Marcelo Fortes

---

Inox is named for stainless steel: resistant, reliable, and free of corrosion by design.
