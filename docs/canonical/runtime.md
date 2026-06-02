# Inox Runtime Canonical Reference

The runtime for 0.1 is intentionally small and safe.

## Output

`Put` and `PutLn` are exposed canonically through `Std.IO`. They accept one or more arguments and emit them sequentially without requiring string concatenation. `PutLn` appends exactly one newline after the final argument. The current LLVM smoke-test backend may lower them through `printf` for `Integer`, `Bool`, `Char`/String milestones, and string literals. This is temporary and not the final ABI.

Examples:

```inox
Put("J=", J)
PutLn("Ciclo numero ", J)
PutLn("A", 10, "B", true)
```

## Standard library boundary

The initial `stdlib/` modules are portable Inox modules and documentation
anchors:

- `Std.Core` is the conceptual prelude/core module for compiler intrinsics.
- `Std.IO` is the `Put`/`PutLn` facade.
- `Std.Math` contains pure Integer helpers written in Inox.
- `Std.Debug` reserves `Assert` until a canonical trap/abort operation exists.

The 0.1 standard library must not introduce GC, unsafe features, C interop, or
a complex runtime dependency.

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
- arenas;
- borrow checker;
- deterministic destructors/finalizers;
- full concurrency runtime.
