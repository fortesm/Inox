# Inox Standard Library 0.1

This directory contains the minimal standard-library modules agreed for Inox 0.1:

- `Std.Core.inox` — prelude/core documentation and compiler intrinsics.
- `Std.IO.inox` — I/O facade for `Put` and `PutLn` built-ins/runtime lowering.
- `Std.Math.inox` — pure Integer helpers that can be implemented in Inox 0.1.
- `Std.Debug.inox` — debug/checking facade; `Assert` requires trap/runtime support.

These modules are intentionally small. They must remain portable across Windows and Linux and another unices and must not introduce unsafe pointers, GC, platform-specific APIs, or undocumented runtime dependencies.

## Runtime status

`stdlib/` is not yet a complete standalone language runtime. It is the beginning of the Inox standard library and currently contains partial, dummy, or compiler-recognized declarations used for tests and early language development.

Inox does not yet ship a dedicated runtime library such as:

```text
libinoxrt.a
inoxrt.lib
inox_runtime.dll
```

The current compiler backend may lower selected operations, such as `Put` and `PutLn`, through host facilities such as `printf` while the final runtime ABI is still being designed.

## Discovery

The compiler searches for `stdlib/` using `INOX_STDLIB`, the release package layout, the current working directory, and source-file parent directories. See `docs/release/prebuilt-usage.md` for user-facing setup instructions.
