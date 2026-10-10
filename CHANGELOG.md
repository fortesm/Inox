<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Changelog — Inox compiler

This file tracks compiler and language releases. The versions `v3.xx` belong to
the canonical document (`docs/INOX_CANONICAL.md`), which has its own change log.

## Unreleased

### Added
- Precedence of `..` and `in` in CANON-20 (ADR-0008).
- Grouped declarations `A, B T := X`, with `X` evaluated once.
- Chained assignment `A := B := C := X` (ADR-0009).
- `for I in A..B step S` (ADR-0010).
- Ada/SPARK-style `case` with `|`, static ranges, and `otherwise` required unless coverage is proven (ADR-0011).
- Compound assignment `+=` `-=` `*=` `/=` `^=` (ADR-0012).
- Conditional expression `if C then A else B` (ADR-0013); only the chosen branch is evaluated.
- Digit separator `_` in numeric literals; hexadecimal as `$FF` or `0xFF`.
- A local variable that is never read is a compile error (OPEN-4).
- `grammar/grammar.ebnf` rebuilt from the canon, checked by `tools/grammar_consistency.py`.

### Changed
- `Var` blocks and scalars without an initializer are rejected (CANON-5).
- A bare `:` block inside a routine is rejected (CANON-4).
- `:=` is a statement, never an expression; named arguments only in struct construction (CANON-9).
- The `for` step form `(S)` is removed with a migration message.
- `Const` values that are constant expressions are folded (`Const K := 5 + 1` was silently 5).
- A `Const` has no written type: `Const Mask UInt8 := $FF` is rejected; write `Const Mask := UInt8($FF)` (OPEN-5).

### Known limitations
- Exceptions on Windows (MSVC ABI) wait for the EH bridge.
- Not lowered yet: `case`, `unless`, `State`, `String` locals, struct construction and initializers, Float32 conversion.
- Arrays, vectors, sets, enums and ranges are not implemented.
- The compiler is not yet aligned with the canonical `<T>` generic syntax; its existing generic syntax still uses `[T]`.

## V0.4 — 2026-10-09
- Compositional lowering of nested statements (v3.21), toolchain diagnostics, CI on Linux and Windows (clang, MSVC ABI).
