# Inox Standard Library 0.1

This directory contains the minimal standard-library modules agreed for Inox 0.1:

- `Std.Core.inox` — prelude/core documentation and compiler intrinsics.
- `Std.IO.inox` — I/O facade for `Put` and `PutLn` built-ins/runtime lowering.
- `Std.Math.inox` — pure Integer helpers that can be implemented in Inox 0.1.
- `Std.Debug.inox` — debug/checking facade; `Assert` requires trap/runtime support.

These modules are intentionally small. They must remain portable across Windows and Linux and must not introduce unsafe pointers, GC, platform-specific APIs, or undocumented runtime dependencies.
