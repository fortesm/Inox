# Inox test layout

The test suite is organized by compiler layer while preserving the older
`examples/` and `tests/invalid/` smoke tests.

- `examples/*.inox` are human-readable valid programs and tutorial snippets.
- `tests/lexer/valid/*.inox` are tokenization fixtures checked with `--dump-tokens`.
- `tests/lexer/invalid/*.inox` must be rejected by tokenization or early parse handling.
- `tests/parser/valid/*.inox` must parse and pass current frontend checks; selected fixtures are also checked with `--parse-only`.
- `tests/parser/invalid/*.inox` must be rejected, preferably by the parser.
- `tests/semantic/valid/*.inox` must pass semantic analysis.
- `tests/semantic/invalid/*.inox` must be rejected by semantic analysis.
- `tests/codegen/*.inox` are LLVM emission fixtures with explicit IR fragments in the runners.
- `tests/integration/*.inox` are optional compile-link-run fixtures with matching `.out` files. They run when `clang` is available.

The command-line driver exposes dedicated regression modes:

- `--dump-tokens`
- `--parse-only`
- `--dump-types`
- `--emit-llvm`
- `--build`
- `--run`

All test files must be portable and must not depend on host-specific paths.
When fixing a regression, add the narrowest fixture that would have failed before the fix.
