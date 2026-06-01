# Inox testing strategy

The test suite is part of the Inox language specification. Every parser, semantic, codegen, runtime, or documentation change must pass the full suite before it is committed. Regressions should be captured as small fixtures in the most specific layer that exposes the bug.

The Inox compiler test suite is deliberately layered so contributors and agents
can understand what a fixture is meant to prove.

## Human examples

`examples/*.inox` contains small valid programs that demonstrate language
features. These files double as smoke tests and should remain readable for
humans learning the language.

## Lexer fixtures

- `tests/lexer/valid/*.inox` contains tokenization-focused sources that are checked with `inox --dump-tokens`.
- `tests/lexer/invalid/*.inox` contains lexical errors such as unterminated literals and invalid characters.

Lexer fixtures are intentionally allowed to be minimal. They prove token spelling, normalization, case-insensitivity, literal scanning, and invalid-token diagnostics before parser or semantic concerns are involved.

## Parser fixtures

- `tests/parser/valid/*.inox` contains syntax-focused programs checked with both the normal frontend and `inox --parse-only`.
- `tests/parser/invalid/*.inox` contains syntax-focused programs that must be rejected, preferably before semantic analysis.

Parser fixtures should prove canonical syntax, such as `Type` without a closing `;`, `Var` without `:`, and `if/elif/else` with a single final `;`.

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

`tests/integration/` contains end-to-end programs with expected output files. When `clang` is available on the host, the runners emit LLVM IR, link it, execute the resulting binary, and compare stdout. If `clang` is not available, these executable checks are skipped without failing the frontend suite.

## Portability rules

Tests must not depend on host-specific absolute paths. Platform-specific
behavior belongs in the test runners or in clearly isolated future integration
helpers.

## Compiler modes used by tests

The command-line driver exposes small diagnostic modes for compiler-layer tests:

- `inox --dump-tokens file.inox` dumps lexer tokens and exits before parsing.
- `inox --parse-only file.inox` tokenizes and parses, then exits before semantic analysis.
- `inox --dump-types file.inox` runs semantic analysis and dumps the typed AST.
- `inox --emit-llvm file.inox` emits textual LLVM IR.
- `inox --build file.inox` emits `build/inox/*.ll` and builds a native executable with Clang.
- `inox --run file.inox` builds and executes the native artifact.

These modes are intentionally stable enough for regression tests. Output should remain readable and deterministic.

When Clang is absent, build/run integration checks report `[SKIP]`. Lexer,
parser, semantic, and textual LLVM checks remain mandatory.

The integration suite also verifies that `Use Std.Math` resolves through the
project `stdlib/` directory and that its Inox implementations execute through
the Clang-backed `--run` driver.

## Regression rule

When a bug is fixed, add a fixture that would have failed before the fix. Prefer the narrowest layer:

1. lexer fixture for tokenization defects;
2. parser fixture for grammar defects;
3. semantic fixture for symbol/type/flow defects;
4. codegen fixture for LLVM shape defects;
5. integration fixture for executable behavior.
