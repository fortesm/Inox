# Inox Standard Library 0.1

This directory contains the minimal standard-library modules agreed for Inox 0.1:

- `Std.Core.inox` — prelude/core documentation and compiler intrinsics.
- `Std.IO.inox` — I/O facade for `Put` and `PutLn` built-ins/runtime lowering.
- `Std.Math.inox` — pure Integer helpers that can be implemented in Inox 0.1.
- `Std.Debug.inox` — debug facade; `Assert` requires canonical trap/runtime support.
- `Std.Errors.inox` — standard nominal exception types and taxonomy.

These modules are intentionally small. They must remain portable across Windows, Linux, and the other Unix/Unix-like targets represented by the platform architecture, and must not introduce unsafe pointers, GC, platform-specific APIs, or undocumented runtime dependencies.

## Runtime status

`stdlib/` is not yet a complete standalone language runtime. It is the beginning of the Inox standard library and currently contains partial, dummy, or compiler-recognized declarations used for tests and early language development.

Inox now ships a minimal native runtime support library (`libinoxrt.a` on Unix-like builds and `inoxrt.lib` on Windows builds) for exception transport. It is intentionally small and is not a VM or managed execution environment. The current compiler backend may still lower selected operations, such as `Put` and `PutLn`, through host facilities such as `printf` while the broader runtime ABI is being designed.

## Discovery

The compiler searches for `stdlib/` using `INOX_STDLIB`, the release package layout, the current working directory, and source-file parent directories. See `docs/release/prebuilt-usage.md` for user-facing setup instructions.
