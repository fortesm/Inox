# AGENTS.md

This file is the operational contract for AI agents working on Inox.

## Canonical truth hierarchy

Use this hierarchy when modifying Inox:

1. `docs/decisions/ADR-0006-inox-0.1-constitution.md` for settled 0.1 language decisions.
2. `docs/canonical/language-reference.md` as the consolidated tutorial/reference.
3. Topic-specific files under `docs/canonical/`.
4. `grammar/grammar.ebnf` as the grammar mirror.
5. `docs/site/index.html` as the human HTML manual.
6. Examples and tests as executable evidence of implemented subsets.

If these disagree, do not invent a third behavior. Fix the documentation/code mismatch explicitly or ask for a design decision.

## Non-negotiable language identity

Inox is post-object-oriented. It has no classes, classical inheritance, Java-style interfaces, mixins, duck typing, or OO visibility model. Structs are data. Associated methods are behavior outside the struct. Future contracts/protocols/behaviors are static capability checks, not OO inheritance.

## Syntax rules agents must not regress

- Inox is case-insensitive.
- Line comments use `==`.
- `;` closes blocks; it is not a general statement terminator.
- `End`/`end` is not a keyword and must never be accepted as a block closer.
- `Module` has no `;`; EOF closes the module.
- `Use` is semantic dependency, not textual inclusion.
- `Type` has no `:` and no closing `;`.
- `Var` has no `:` and closes with `;`.
- `Struct` syntax is `TName Struct ... ;`.
- `Range` declarations do not use `;`.
- `if`/`elif`/`else` use no `then` and no `:`.
- `case Expression` uses no `of`, `when`, `=>`, `:`, or `do`.
- `for I in A..B (S)` uses no `do` and no `:`.
- `repeat` closes with `;`; `until` is an internal statement.

## Type and semantic rules agents must not regress

- `Integer = Int64`; `Float = Float64`; canonical boolean type is `Bool`.
- `String` is UTF-8, immutable, non-null, default `""`.
- `Char` is Unicode scalar value.
- No universal `null`/`nil`.
- Integer `/` is invalid; use `div` and `mod`.
- Integer overflow is invalid; do not promise wraparound.
- Parameters are immutable by default.
- Local variables in `Var` are mutable.
- Associated receivers are `Self` or `Self mut`; do not write `Self TPoint`.
- `Self mut` is required for mutating methods.
- `Exit` is not allowed in functions with return values.
- `Return Expression` is not allowed in subroutines without return types.
- Structs are nominal value types.
- `Vector[T]` future semantics are ownership/move, not reference aliasing.
- `Set[T]` requires finite ordinal base, not arbitrary `Integer`/`String`.

## Implementation discipline

- Keep the compiler portable C++20.
- Validate Linux with `cmake --build build` and `bash scripts/run-tests.sh`.
- Validate Windows with `cmake --build build --config Debug` and `pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1`.
- Do not use `git add .`; add only task-scoped paths.
- Every language change must update code, tests, docs, `docs/site/index.html`, and ADRs when applicable.
- If a feature is canonical but not implemented, record it as a conformance gap instead of changing the spec.

## Current major conformance gaps

- canonical `case Expression` parser/lowering;
- full multi-file `Module`/`Use` compile/link;
- arrays/ranges/enums/sets implementation;
- vector runtime;
- checking-mode overflow traps;
- final runtime ABI and `--build`/`--run` driver.
