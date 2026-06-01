# Inox Runtime Canonical Reference

The runtime for 0.1 is intentionally small and safe.

## Output

`Put` and `PutLn` are in `Sys.IO` / prelude. The current LLVM smoke-test backend may lower them through `printf` for `Integer`, `Bool`, `Char`/String milestones, and string literals. This is temporary and not the final ABI.

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
