<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Changelog — Inox compiler

This file tracks compiler and language releases. The versions `v3.xx` belong to
the canonical document (`docs/INOX_CANONICAL.md`), which has its own change log.

## Unreleased

### Added
- Read-only slices `A[L..H]`, `A[..H]`, `A[L..]`, `A[..]` of type `Slice<T>` (ADR-0017), checked bounds, never outliving their array.
- `ref X T` and `ref mut X T` parameters (ADR-0016): references to the caller's variable, field or array element, with exclusivity checked at compile time.
- Fixed arrays `Array[Low..High] T` with checked indexing, multi-dimensional `M[I, J]`, `Low`/`High`/`Length`, value copy, array parameters and results; `A[I] += 1`.
- Fixed-width integers Int8, Int16, Int32, Byte/UInt8, UInt16, UInt32 are compiled with overflow and conversion range checks; a constant such as `B Byte := 200` or `B + 1` is read in the fixed-width type.
- `bitnot` is compiled.
- Module `State` variables (Integer, Natural, Bool, Float) are compiled; they are initialized in source order before `Main` runs.
- `case` and `unless` are compiled to native code (the selector is evaluated once).
- Precedence of `..` and `in` in CANON-20 (ADR-0008).
- Grouped declarations `A, B T := X`, with `X` evaluated once.
- Chained assignment `A := B := C := X` (ADR-0009).
- `for I in A..B step S` (ADR-0010).
- Ada/SPARK-style `case` with `|`, static ranges, and `otherwise` required unless coverage is proven (ADR-0011).
- Compound assignment `+=` `-=` `*=` `/=` `^=` (ADR-0012).
- Conditional expression `if C then A else B` (ADR-0013); only the chosen branch is evaluated.
- Digit separator `_` in numeric literals; hexadecimal as `$FF` or `0xFF`.
- A local variable that is never read is a compile error (OPEN-4).
- `Natural` is the non-negative subtype of `Integer` (0..Int64.Max, ADR-0014). As in Ada, storing an Integer into a Natural checks the range at run time with no conversion written (ADR-0015); a negative constant is a compile error.
- `Byte` is registered as `UInt8`; integer conversions such as `Byte(300)` or `Int8(-129)` with an out-of-range constant are compile errors.
- `grammar/grammar.ebnf` rebuilt from the canon, checked by `tools/grammar_consistency.py`.
- OpenSpec workflow (canon v3.48, G7): `openspec/specs/` mirrors seven language capabilities with test-backed scenarios; `openspec/changes/` holds the proposals for struct values, declaration sections, UInt64, Enum/Range/Set and the String runtime; CI runs `openspec validate --all --strict`.
- Canon migration to OpenSpec, phases 1 and 3a (canon v3.49): `tools/canon_trace.py` and `openspec/migration/CANON_MAP.md` trace every line of the canon to its destination (checked in CI); ADR-0001..0017 imported verbatim as archived OpenSpec changes; six new diagnostics tests for `case` syntax and array types.

### Changed
- Example files no longer carry the `llvm-` prefix (`examples/put-output-basic.inox`, ...).
- Windows (MSVC ABI): exceptions are lowered with LLVM funclets (`__CxxFrameHandler3`); every exception test passes natively on the CI runner (EH-v3.16a closed).
- `Var` blocks and scalars without an initializer are rejected (CANON-5).
- A bare `:` block inside a routine is rejected (CANON-4).
- `:=` is a statement, never an expression; named arguments only in struct construction (CANON-9).
- The `for` step form `(S)` is removed with a migration message.
- `Const` values that are constant expressions are folded (`Const K := 5 + 1` was silently 5).
- A `Const` has no written type: `Const Mask UInt8 := $FF` is rejected; write `Const Mask := UInt8($FF)` (OPEN-5).

### Known limitations
- `UInt64`/`UInteger` values are not lowered yet.
- Not lowered yet: `State` of struct/String type, `String` locals, struct construction and initializers, Float32 conversion.
- Vectors, sets, enums and ranges are not implemented; arrays are not yet allowed in struct fields or State.
- The compiler is not yet aligned with the canonical `<T>` generic syntax; its existing generic syntax still uses `[T]`.

## V0.4 — 2026-10-09
- Compositional lowering of nested statements (v3.21), toolchain diagnostics, CI on Linux and Windows (clang, MSVC ABI).
