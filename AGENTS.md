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
- `Var` blocks and `var`/`mut var` declarations were removed (CANON-5); `Var` and `mut` stay reserved and the parser rejects them with a migration diagnostic. Declare locals inline.
- `Struct` syntax is `TName Struct ... ;`.
- `Range` declarations do not use `;`.
- `if`/`elif`/`else` use no `then` and no `:`. `then` exists only in the
  conditional expression `if C then A else B` (ADR-0013).
- `case Expression` uses no `of`, `when`, `=>`, `:`, or `do`.
- `for I in A..B step S` uses no `do` and no `:`; the old `(S)` step form is
  rejected (ADR-0010). `step` is a reserved word.
- `repeat` closes with `;`; `until` is an internal statement.
- Loop exit is `leave` and the try cleanup clause is `ensure` (v3.20, ADR-0007).
  `break` and `finally` are NOT keywords; never emit them as Inox syntax.

## Type and semantic rules agents must not regress

- `Integer = Int64`; `Float = Float64`; canonical boolean type is `Bool`.
- `String` is UTF-8, immutable, non-null, default `""`.
- `Char` is Unicode scalar value.
- No universal `null`/`nil`.
- Integer `/` is invalid; use `div` and `mod`.
- Integer overflow is invalid; do not promise wraparound.
- Parameters are immutable by default.
- Local variables are declared inline and are mutable.
- Every variable is born with a value: a scalar declaration requires an initializer (`C Integer := 0`, never `C Integer`); only structs/aggregates may omit `:=` (type-default initialization).
- Associated receivers are `Self` or `Self mut`; do not write `Self TPoint`.
- `Self mut` is required for mutating methods.
- `Exit` is not allowed in functions with return values.
- `Return Expression` is not allowed in subroutines without return types.
- Structs are nominal value types.
- `Vector<T>` future semantics are ownership/move, not reference aliasing.
- `Set<T>` requires finite ordinal base, not arbitrary `Integer`/`String`.
- Generics use `<T>`, not `[T]`; `<...>` is generic syntax only in type
  position. `[...]` remains for indexing, slicing and array/range bounds. (The
  compiler still accepts `[T]` until its conformance PR.)
- Future concurrency starts with `do`, never `go`; its semantics wait for an
  ADR. The `|...|` capture-clause idea is not a language decision.


## Local scope and shadowing rules

- `Name Type := Expression` is a declaration.
- `Name := Expression` is assignment to an existing mutable symbol.
- Compound assignment `+=` `-=` `*=` `/=` `^=` (ADR-0012) is a statement on an
  existing variable or field; not chainable; `/=` never on Integer; `^=` is power.
- Shadowing is forbidden in same and nested scopes.
- Case-only differences are not distinct names.
- Use before declaration is invalid.
- A local variable that is never read is a compile error (OPEN-4, v3.32);
  assigning is not reading. Parameters, `for` iterators, exception bindings,
  State and Const are exempt. Tests and examples must read what they declare.
- A `Const` never has a written type: `Const Name := Value`; use a conversion
  (`Const Mask := UInt8($FF)`) to choose another type (OPEN-5, v3.38).
- A local symbol dies at the end of its block.
- `for` iterators are implicit, read-only, loop-scoped, and cannot conflict with visible symbols.
- Sequential `for` loops may reuse an iterator name after the previous loop scope is closed; nested loops may not reuse an outer iterator name.

## Implementation discipline

- Keep the compiler portable C++20.
- Validate Linux with `cmake --build build` and `bash scripts/run-tests.sh`.
- Both runners include `tools/grammar_consistency.py`: a change to syntax, the
  lexer keywords or CANON-20 must update `grammar/grammar.ebnf` in the same PR.
- Validate Windows with `cmake --build build --config Debug` and `pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1`.
- Do not use `git add .`; add only task-scoped paths.
- Every language change must update code, tests, docs, `docs/site/index.html`, and ADRs when applicable.
- If a feature is canonical but not implemented, record it as a conformance gap instead of changing the spec.
- Lower every block (function bodies, loop bodies, `if`/`elif`/`else` branches, `try` regions) through the single statement dispatcher in the LLVM emitter. Do not add per-container lists of allowed statements; a construct that lowers in one block must lower in every block context where semantic analysis accepts it (v3.21). Context rules such as `leave`/`continue` only inside a loop, `until` only inside a repeat, or `Retry` only in a handler belong to the semantic analyzer, not to the emitter.
- Run `python tools/backend_gaps.py <inox>` after backend changes: 0 BUG is required, and with clang on PATH it also checks that the emitted IR is accepted.
- Keep `stdlib/` portable across Windows and Linux. It must not depend on GC,
  unsafe features, or C interop.

## Standard library 0.1

- `Std.Core` is the conceptual prelude/core module and may document compiler intrinsics.
- `Std.IO` is the canonical facade for `Put` and `PutLn`.
- `Std.Math` contains pure Integer helpers implemented in Inox.
- `Std.Debug` reserves `Assert` until canonical trap/abort behavior exists.
- Explicit `Use` resolution checks the entry-file directory before `stdlib/`.

## Current major conformance gaps

- full `case` lowering and enum exhaustiveness checks;
- module exports, visibility, and package search beyond local `Module`/`Use`;
- arrays/ranges/enums/sets implementation;
- vector runtime;
- final runtime-fault infrastructure beyond the checked arithmetic already implemented;
- final runtime ABI beyond the temporary Clang-backed `--build`/`--run` driver.

## Licensing and empty-parentheses rules

The project is licensed under MPL-2.0. Do not add the MPL "Incompatible With
Secondary Licenses" notice. Preserve copyright and SPDX headers.

Do not write empty parentheses in Inox code. Use `Main :`, `Report :`, `Report`,
and `Account.Print`. Forms like `Main() :`, `Report() :`, `Report()`, and
`Account.Print()` are invalid and must remain covered by regression tests.


## Std.IO variadic output

`Put` and `PutLn` accept one or more arguments. Emit arguments sequentially; do not require string concatenation. `PutLn` appends exactly one newline after the final argument. Examples: `Put("J=", J)`, `PutLn("Ciclo numero ", J)`, `PutLn("A", 10, "B", true)`.

For `for` iterator conflicts with an existing symbol, prefer the diagnostic `loop iterator conflicts with existing symbol: Name`; keep `shadowing is forbidden` for general non-iterator shadowing.

## Pull requests and merges

- Never push directly to `main`. Every change goes through a branch and a pull
  request.
- A pull request is reviewed by an agent other than its author, and CI must be
  green.
- Only Marcelo merges, and only after he explicitly approves that merge.

## Machine identity

Marcelo works on Inox from more than one machine (for example, one at his
workplace for implementation tests and one at home for development). Each of
them has a file `.inox-machine` in the repository root, created from the
tracked template `.inox-machine.example`. The file is in `.gitignore` and must
never be committed or copied into tracked files.

- `.inox-machine` is local machine configuration, not project authority. It
  selects the machine identity and toolchain and may further restrict what an
  agent does on that machine. It never expands what this file, the canonical
  governance or Marcelo's current explicit instructions allow.
- Do not create or modify `.inox-machine` unless Marcelo explicitly asks.
- At the start of a session on a local checkout, read `.inox-machine` and apply
  its `restrictions`.
- If the file is absent, or its `id` or `role` is still `CHANGE-ME`, the machine
  is unidentified: say so (you may be in CI or in a cloud environment) and do not
  invent an identity.
- Use its toolchain entries (`compiler`, `build_preset`, `test_command`) instead
  of guessing; results differ between toolchains.
- Name the machine (`id`) when reporting test results, in commit messages that
  record a validation, and in pull request descriptions: "validated on: <id>".
