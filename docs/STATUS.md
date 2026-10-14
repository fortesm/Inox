<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Implementation status

What the compiler does today, against the law in
`openspec/specs/constitution/` (status layer, G1: this page follows the
code and may be updated by anyone to match it; it never changes the law).
Verified on 2026-10-14 against Inox 0.2.0 pre-alpha, canon r3.70.
`docs/BACKEND_GAPS.md` is the generated, measured list of constructs the
analyzer accepts and the backend does not lower; `CHANGELOG.md` says what
each version added.

## Implemented

- Driver: `inox file.inox`, `run`, `build`, `check`, `emit-llvm`, `help`,
  `version` (the `--` switches are aliases; `--parse-only`, `--dump-tokens`,
  `--dump-types` for development); local `Module`/`Use` with
  recursive loading and cycle rejection; library discovery; toolchain
  failures reported with the tool's own messages.
- Lexical rules, blocks, declarations (inline, grouped, chained, compound),
  unread-local rule, Const (Integer, Bool, String, constant expressions), State (scalars,
  structs, arrays, Strings), routines, no empty parentheses.
- Types: Bool, Integer/Int64, Natural, Int8/16/32, Byte/UInt8, UInt16/32,
  Float/Float64 (Float32 partly), String, Char literals as `case`
  choices; checked arithmetic, conversions `T(X)` between
  integer types, runtime faults with exit status 70.
- Control flow: if/elif/else, unless, conditional expression, while,
  repeat/until, for with step, leave/continue, case (Ada style), with.
- Structs with defaults, named construction, nesting, associated methods;
  method reuse (`Struct (TPart)`, reused fields and methods at every level,
  `Override`); methods of built-in types (`String.UpdateStr`); fixed arrays (multi-dimensional, in fields and State, of structs and
  Strings); read-only slices; `ref`/`ref mut` parameters.
- Design by contract (CANON-24): `pre`/`pos` with `old` and `Result`,
  struct `invariant` (also of reused parts), `pre else`/`pos then` in an
  `Override`, loop `invariant`/`variant`; violations are runtime traps
  (exit status 70); always on through 1.0 (monitoring levels per module
  are FUTURE, after 1.0).
- Strings: reference-counted UTF-8 values everywhere a value lives,
  equality and ordering, `GetLn(S)`.
- Exceptions: try/except/On/Else/ensure/Raise/Retry with the standard
  taxonomy, Itanium landing pads on Linux and MSVC funclets on Windows.
- Text files: Std.Files (type and basic operations) / Std.IOUtils
  (conveniences).
- Standard library: Std.System, Std.IO, Std.Math, Std.Errors, Std.Debug
  (Assert), Std.Files, Std.IOUtils.
- Windows x64 and Linux x64 run the full suite in CI
  (`ci/windows-known-failures.txt` is empty).

## Conformance gaps (the law is ahead of the code)

- UInt64/UInteger values (change `uint64`).
- Enum, Range and Set: parsed and shape-checked only; enum types are not
  declared yet, so enum `case` coverage, enum `for`, `Ord`/`Succ`/`Pred` on
  enums and the enum initializer rule wait for change `enum-range-set`.
- Currency and Crypto: names registered, no arithmetic or literals;
  BigCurrency absent.
- Vector, `Option`, `Result`: not implemented.
- Conversions involving Float (`Integer(F)`, `Float(I)`, `Float32(X)`): accepted,
  not lowered ("not yet implemented in the LLVM backend").
- A Float `Const` is not lowered, so Std.Math exposes `Pi`, `Tau` and `E` as
  functions.
- Char values: a Char local or comparison is accepted and not lowered, and
  `PutLn(C)` with a Char is rejected ("cannot be printed yet").
- A function called as a statement (its result unused) passes the analyzer
  and is "not yet implemented" in the backend (left as it is, r3.65).
- Module exports, visibility and package search beyond local `Use`.
- Console output goes through the C `printf` and Integer input through
  `getchar`-based helpers: a temporary ABI.
- Final runtime ABI and toolchain discovery; clang is still the external
  driver (by design until E22's criteria hold).
- Quality gates still to come: clang-format, clang-tidy, cppcheck,
  coverage.

## Over-acceptance (the code accepts what the law does not define)

These are bugs to fix (G1, G4), not features:

- `TPoint.M(Self TPoint)` is rejected, but with a generic parse message
  instead of one that names the rule (CANON-9).

(Until r3.66, `Self` could write its receiver, a field of a value parameter
could be assigned, and `with` on a parameter could write it; they are
rejected now.)

(`Length(S)` of a String and `Ord(C)` of a Char were accepted until r3.62;
they are rejected now.)

## Implementation debt

- The AST node `VarBlockStatement` keeps its old name; it groups inline
  declarations.
- Reference counts of Strings and Files are not atomic (Inox has no
  threads yet).
