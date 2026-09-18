# Inox

**A compiled, strongly typed, post-object-oriented systems language designed for software where silent failure is unacceptable.**

Inox prioritizes explicit safety, deterministic behavior, and clarity over convenience when the two conflict. It is intended for mission-critical domains in which buffer overflows, null dereferences, silent integer overflow, unchecked mutation, or hidden aliasing can cost lives, capital, or infrastructure.

## Design Philosophy

Inox is deliberately **post-object-oriented**. It has no classes, classical inheritance, Java-style interfaces, mixins, or duck typing. Data lives in nominal value types (`Struct`). Behavior lives in free functions, subroutines, and associated methods declared outside the struct. The familiar call form `Object.Method(args)` is retained for ergonomics without turning data into classical objects.

Core safety defaults include:

- No universal `null` or `nil`
- No unsafe pointers in the language core
- No silent integer overflow
- Explicit integer division (`div` / `mod`) instead of `/`
- Parameters immutable by default
- Mutating methods require an explicit `Self mut` receiver
- Strong nominal typing with no implicit narrowing
- Composition preferred over inheritance

The language draws carefully from several traditions (Ada/SPARK robustness, Rust ownership and mutability discipline, Modula/Oberon modular clarity, modern Pascal ergonomics, C/C++ performance realism, and others) while rejecting defaults that introduce undefined behavior or ambiguity.

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

The reference compiler is written in portable C++20 and targets LLVM. It currently builds and is validated on Windows and Linux. The language core does not rely on a tracing garbage collector; future memory management work favors explicit ownership, moves, arenas, and deterministic resource control.

Inox remains under active development. The current focus is a coherent, well-specified 0.1 foundation with a complete compiler pipeline (lexer → parser → semantic analysis → LLVM IR → executable) and a growing standard library surface.

## Building

See [`BUILD_AND_TEST_INSTRUCTIONS.md`](BUILD_AND_TEST_INSTRUCTIONS.md) for current build, test, and release instructions on Windows and Linux.

## Documentation

The single authoritative source of truth for language design, compiler contract, and project rules is:

- [`docs/INOX_CANONICAL.md`](docs/INOX_CANONICAL.md)

Additional materials (examples, tests, build instructions, and contribution guidelines) live in the repository and are kept consistent with the canonical document.

## Status

Inox is not yet production-ready. The design is stable in its core principles; the implementation continues to expand toward a complete, reliable 1.0 while preserving the safety and engineering standards established for the language.

## License

Mozilla Public License 2.0 (MPL-2.0).

## Author

Marcelo Fortes

---

Inox is named for stainless steel: resistant, reliable, and free of corrosion by design.
