# Inox LLVM Backend Canonical Reference

LLVM is the official backend for Inox. The current implementation uses textual LLVM IR for incremental validation. This is acceptable for 0.1 pre-alpha because it is readable, testable, and accepted by Clang.

## Portability

The compiler must remain portable C++20. Host-specific code belongs in CMake or scripts unless a future runtime boundary genuinely requires platform APIs.

## Integer arithmetic

`div` lowers to signed integer division (`sdiv`). `mod` lowers to signed remainder (`srem`). Integer `/` is not valid Inox and must not lower to `sdiv`.

Do not emit `nsw`/`nuw` until overflow checking and optimization policy are proven. Integer overflow is invalid at the language level.

## Structs

Structs may lower to LLVM aggregate types. Ordinary struct parameters and returns are values. Associated-method receivers may lower to pointers for implementation convenience, but this does not create reference semantics at the language level.

Canonical associated receiver syntax:

```inox
TPoint.Move(Self mut, DX Integer, DY Integer) :
```

## Current smoke-test backend

The current backend supports a restricted executable subset: scalar integer/bool operations, local variables, selected control flow, simple structs, associated methods, field defaults, struct values, subroutines, and temporary `printf`-based output.

## Temporary native driver

`inox --build file.inox` emits textual LLVM IR under `build/inox/` and invokes
the external `clang` toolchain to create a native executable in the same
directory. `inox --run file.inox` builds and executes that artifact.

The driver loads local `Use` dependencies recursively before emission. It
checks `A.B.inox` and then `A/B.inox` relative to the entry file directory,
rejects dependency cycles, and emits the loaded 0.1 subset as one textual LLVM
module. This is a minimum module model, not a package manager or final linker
architecture.

## Future backend work

- final runtime ABI;
- richer module linking, exports, visibility, and package search;
- arrays, enums, ranges, sets, char, and strings beyond literals;
- checking-mode traps;
- final toolchain discovery and runtime ABI;
- optional migration from textual IR to LLVM C++ API where justified.
