# Inox test layout

The test suite is organized by compiler layer while preserving the older
`examples/` and `tests/invalid/` smoke tests.

- `examples/*.inox` are human-readable valid programs and tutorial snippets.
- `tests/parser/valid/*.inox` must parse and pass the current frontend checks.
- `tests/parser/invalid/*.inox` must be rejected, preferably by the parser.
- `tests/semantic/valid/*.inox` must pass semantic analysis.
- `tests/semantic/invalid/*.inox` must be rejected by semantic analysis.
- `tests/codegen/*.inox` are LLVM emission fixtures with explicit IR fragments in the runner.
- `tests/integration/` is reserved for future compile-link-run tests.

All test files must be portable and must not depend on host-specific paths.
