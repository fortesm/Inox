# Inox testing strategy

The Inox compiler test suite is deliberately layered so contributors and agents
can understand what a fixture is meant to prove.

## Human examples

`examples/*.inox` contains small valid programs that demonstrate language
features. These files double as smoke tests and should remain readable for
humans learning the language.

## Parser fixtures

- `tests/parser/valid/*.inox` contains syntax-focused programs that must parse
  and pass the current frontend checks.
- `tests/parser/invalid/*.inox` contains syntax-focused programs that must be
  rejected, preferably before semantic analysis.

## Semantic fixtures

- `tests/semantic/valid/*.inox` contains programs that validate name resolution,
  type checking, mutability, flow constraints, and related semantic rules.
- `tests/semantic/invalid/*.inox` contains programs that must be rejected by the
  semantic analyzer.

## Codegen fixtures

`tests/codegen/*.inox` contains programs used specifically to verify LLVM IR
emission. Each file must be registered in both test runners with explicit
required IR fragments.

## Integration tests

`tests/integration/` is reserved for future tests that emit LLVM IR, compile or
link it with the host toolchain, execute the resulting binary, and verify
stdout/stderr and exit status.

## Portability rules

Tests must not depend on host-specific absolute paths. Platform-specific
behavior belongs in the test runners or in clearly isolated future integration
helpers.
