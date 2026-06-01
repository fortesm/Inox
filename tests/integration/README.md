# Integration tests

Integration fixtures compile generated LLVM IR, link it, execute the binary,
and verify stdout/stderr and process exit codes on each supported host.

The suite covers the temporary Clang-backed `--build` and `--run` driver,
single-file execution, minimal local multi-file `Module`/`Use`, and cycle
diagnostics. It also verifies standard-library lookup through `Use Std.Math`
and `Use Std.IO`. Clang-dependent checks report `[SKIP]` when Clang is
unavailable.
