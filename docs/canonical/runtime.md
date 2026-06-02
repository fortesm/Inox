# Inox Runtime Canonical Reference

The runtime for 0.1 is intentionally small and safe. Inox does not yet provide a complete standalone language runtime.

## Runtime boundaries

There are three different things that must not be confused:

1. The Inox compiler executable (`inox.exe` or `inox`).
2. The Inox standard library (`stdlib/Std.*.inox`).
3. The future Inox language runtime (`libinoxrt`, `inoxrt.lib`, or equivalent).

The compiler executable is currently a native C++ program. On Windows, a Release build may require the normal Microsoft Visual C++ Redistributable runtime. It does not require LLVM dynamic libraries merely to start unless future implementation choices introduce such a dependency.

The `stdlib/` directory is the beginning of the Inox standard library. It is not yet a complete runtime library.

A complete Inox runtime is future work. It may eventually define ABI, startup, traps, allocation, strings, Unicode, arrays, vectors, I/O, and platform services.

## Output

`Put` and `PutLn` are exposed canonically through `Std.IO`. They accept one or more arguments and emit them sequentially without requiring string concatenation. `PutLn` appends exactly one newline after the final argument. The current LLVM smoke-test backend may lower them through `printf` for Integer, Bool, Char/String milestones, and string literals. This is temporary and not the final ABI.

Examples:

```inox
Put("J=", J)
PutLn("Ciclo numero ", J)
PutLn("A", 10, "B", true)
```

## Standard library boundary

The initial `stdlib/` modules are portable Inox modules and documentation anchors:

- `Std.Core` is the conceptual prelude/core module for compiler intrinsics.
- `Std.IO` is the `Put`/`PutLn` facade.
- `Std.Math` contains pure Integer helpers written in Inox.
- `Std.Debug` reserves `Assert` until a canonical trap/abort operation exists.

The 0.1 standard library must not introduce GC, unsafe features, C interop, or a complex runtime dependency.

## Standard library discovery

The compiler searches for `stdlib/` in this order:

1. `INOX_STDLIB`, when set.
2. `stdlib/` next to the release package root, when the executable is under `bin/`.
3. `stdlib/` next to the executable.
4. `stdlib/` under the current working directory.
5. `stdlib/` in the source file directory or one of its parent directories.

This supports both source-tree development and prebuilt ZIP releases.

## Build artifact directory

The temporary Clang-backed `--build` and `--run` driver writes generated LLVM IR and native executable files to:

```text
build/inox-artifacts/
```

The environment variable `INOX_OUTPUT_DIR` overrides this default.

This is generated output and must not be versioned.

## Strings

`String` is non-null, immutable UTF-8. The empty string `""` is the zero/default value. Full allocation, interning, Unicode normalization, indexing, and length APIs are future runtime/library work.

## Errors

Runtime traps/errors include division by zero, bounds errors, range errors, and future overflow checks in checking mode.

## Memory management

The 0.1 safe core does not define raw pointers, unsafe blocks, or C interop. Future memory work should cover ownership, moves, arenas, deterministic resource management, borrowing, and explicit unsafe boundaries. No tracing GC is assumed as a core language requirement.

## Out of scope for 0.1 safe core

- raw `Pointer[T]`;
- `unsafe` blocks;
- direct C interop;
- final ABI;
- complete standalone runtime library;
- arenas;
- borrow checker;
- deterministic destructors/finalizers;
- full concurrency runtime.
