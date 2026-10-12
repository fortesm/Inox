<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Changelog — Inox compiler

This file tracks compiler and language releases. The versions `v3.xx` belong to
the canonical document (`docs/INOX_CANONICAL.md`), which has its own change log.

## Unreleased

### Added
- Bool ordering `false < true` (canon v3.57).
- String ordering (canon v3.56): `<` `>` `<=` `>=` compare Strings byte by byte (Unicode code point order, a prefix first), combining with `not`, `and`, `or`, `xor`, `unless` like any Bool.
- String runtime (canon v3.55, OpenSpec change `string-runtime`): Strings in locals, parameters, results, struct fields (with a String literal default or `""`), array elements, State and `ref` parameters, as reference-counted immutable blocks in libinoxrt that the compiler frees exactly once, also when an exception leaves a routine. `GetLn(S)` reads a line into a String (UTF-8 checked). New runtime faults "out of memory" and "invalid UTF-8 input". `tools/string_leak_check.py` runs the String programs under AddressSanitizer with leak detection in the Linux CI; `inoxrt-tests` unit-tests the runtime on Linux and Windows.
- `tools/ir_snapshot.py` (canon v3.54): records IR, types, diagnostics and exit codes of every program and compares them after a refactor; an E19 gate. AGENTS.md gains "Known pitfalls"; high-risk OpenSpec changes wait for design approval.
- Type, Const and State sections are parsed into typed declarations (OpenSpec change `declaration-sections`, canon v3.53): a struct field may be a fixed array, `State` may hold structs and arrays (with or without initializer), and malformed entries report their line.
- Struct values (canon v3.52): struct initializers `Q T := Expr`, named construction `T(F := V)` as an expression, nested struct fields and field paths, fields and literal defaults of every scalar type, methods on fields and elements, arrays of struct. A struct that contains itself is a compile error.
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
- Language decisions recorded (canon v3.50): UInt64 literals up to 2^64-1 in UInt64 context, no unary minus on UInt64, no UInt64/Integer mixing; implicit Range checks; Enum `Succ`/`Pred`/`Low`/`High`; Set functions, literal and 256-value limit; `GetLn(S)` for String, non-atomic reference counts. Enum printing prints the literal's name (v3.51). Implementation follows.

### Changed
- The Windows test script feeds `.in` files to programs byte for byte and reads their output as UTF-8; `.gitattributes` keeps `.in` files unchanged by Git.
- A Type line `TName := Type` is an error: Inox has no type alias form (canon v3.54).
- Example files no longer carry the `llvm-` prefix (`examples/put-output-basic.inox`, ...).
- Windows (MSVC ABI): exceptions are lowered with LLVM funclets (`__CxxFrameHandler3`); every exception test passes natively on the CI runner (EH-v3.16a closed).
- `Var` blocks and scalars without an initializer are rejected (CANON-5).
- A bare `:` block inside a routine is rejected (CANON-4).
- `:=` is a statement, never an expression; named arguments only in struct construction (CANON-9).
- The `for` step form `(S)` is removed with a migration message.
- `Const` values that are constant expressions are folded (`Const K := 5 + 1` was silently 5).
- A `Const` has no written type: `Const Mask UInt8 := $FF` is rejected; write `Const Mask := UInt8($FF)` (OPEN-5).

### Fixed
- `=` and `#` on two Bool variables, `<` on Strings and `=`/`<` on structs passed the analyzer and then produced invalid IR. Bool and String comparisons now work; struct comparison is a compile error, as in Pascal (canon v3.56, v3.57).
- A function called with no arguments inside `try` (`N := Fail`) now reaches the handler; it was emitted with `call` instead of `invoke` and its exception aborted the program.

### Known limitations
- `UInt64`/`UInteger` values are not lowered yet.
- Not lowered yet: Float32 conversion.
- Vectors, sets, enums and ranges are not implemented (Range and Enum declarations are parsed and checked for shape only).
- String concatenation, indexing and length functions are reserved (not in 0.1); slices of String arrays are not supported yet.
- The compiler is not yet aligned with the canonical `<T>` generic syntax; its existing generic syntax still uses `[T]`.

## V0.4 — 2026-10-09
- Compositional lowering of nested statements (v3.21), toolchain diagnostics, CI on Linux and Windows (clang, MSVC ABI).
