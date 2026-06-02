# Inox

Inox is a compiled, strongly typed, post-object-oriented systems language for high-integrity software. It is built around structs as data, associated methods as behavior, composition instead of inheritance, explicit mutability, strong typing, bounds checking, and an LLVM backend.

## Documentation

Start here:

- `docs/canonical/language-reference.md` — consolidated tutorial/reference.
- `docs/site/index.html` — browsable HTML manual.
- `docs/decisions/ADR-0006-inox-0.1-constitution.md` — frozen 0.1 decisions.
- `AGENTS.md` — operational instructions for Codex and AI agents.
- `docs/open-questions/OPEN_QUESTIONS.md` — deferred 0.2+ architecture topics.

## Build

Linux:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
bash scripts/run-tests.sh
```

Windows:

```powershell
cmake --build build --config Debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

## Compile and run Inox

The temporary 0.1 driver uses Clang as an external toolchain:

```powershell
build\Debug\inox.exe --build tests\integration\run-hello.inox
build\Debug\inox.exe --run tests\integration\run-hello.inox
```

Generated `.ll` files and executables are written under `build/inox-artifacts/`.
Install LLVM/Clang or put `clang` in `PATH` before using `--build` or `--run`.

`Use` resolves local modules from the entry file directory. For `Use Math.Basic`,
the driver checks `Math.Basic.inox` first and `Math/Basic.inox` as a fallback.

## Standard library

The initial portable 0.1 standard library lives under `stdlib/`:

- `Std.Core` is the conceptual prelude/core anchor for compiler intrinsics.
- `Std.IO` documents the canonical `Put` and `PutLn` facade.
- `Std.Math` provides pure Integer helpers implemented in Inox.
- `Std.Debug` documents future `Assert` support, pending canonical trap/abort behavior.

`Use Std.Math` and the other explicit standard-library imports resolve through
`stdlib/`. The standard library must remain portable across Windows and Linux
and must not depend on GC, unsafe features, or C interop.

## Current compiler capabilities

The compiler currently includes lexer, parser, semantic analyzer, layered tests, typed dumps, a textual LLVM backend for a restricted executable subset, native build/run through Clang, and minimal local multi-file `Module`/`Use` support. The backend is intentionally incremental and test-driven.

## Design stance

Inox rejects unsafe defaults: universal null, implicit narrowing, integer wraparound guarantees, unchecked bounds, implicit aliasing, classes, inheritance, and Java-style interfaces. Future work includes modules, arrays, vectors, sets, contracts/protocols/behaviors, arenas, borrowing, unsafe boundaries, and structured parallelism.

## License

Inox is free software licensed under the Mozilla Public License Version 2.0
(MPL-2.0). See `LICENSE` for the full license text.

Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.

The project does not use the MPL 2.0 "Incompatible With Secondary Licenses"
notice. See also `NOTICE.md`, `AUTHORS.md`, `CONTRIBUTING.md`, and
`TRADEMARK.md`.

## Canonical no-argument syntax

Inox forbids empty parentheses. Use `Main :`, not `Main() :`. Use
`Account.Print`, not `Account.Print()`.


## Std.IO variadic output

`Put` and `PutLn` accept one or more arguments. Arguments are printed sequentially without string concatenation, and `PutLn` adds one newline after the final argument.

```inox
Put("J=", J)
PutLn("Ciclo numero ", J)
PutLn("A", 10, "B", true)
```
