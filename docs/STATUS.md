<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Implementation status

What the compiler does today, against the law in
`openspec/specs/constitution/` (status layer, G1: this page follows the
code and may be updated by anyone to match it; it never changes the law).
Verified on 2026-10-18 against Inox 0.2.0 pre-alpha, canon r3.87.
`docs/BACKEND_GAPS.md` is the generated, measured list of constructs the
analyzer accepts and the backend does not lower; `CHANGELOG.md` says what
each version added.

## Implemented

- Driver: `inox file.inox`, `run`, `build`, `check`, `emit-llvm`, `help`,
  `version` (the `--` switches are aliases; `--parse-only`, `--dump-tokens`,
  `--dump-types` for development); modules named after their files and local `Use` with
  recursive loading and cycle rejection; library discovery; toolchain
  failures reported with the tool's own messages.
- 42 reserved words and 11 predefined names (`True`, `False`, `Exit`,
  `leave`, `continue`, `IIf`, `Fill`, `bitand`, `bitor`, `bitxor`, `bitnot`) that
  can never be declared; `{` `}` reserved (CANON-2).
- Lexical rules, blocks, declarations (inline, grouped, chained, compound),
  unread-local rule, Const (Integer, Bool, String, constant expressions), State (scalars,
  structs, arrays, Strings), routines, no empty parentheses.
- Types: Bool, Integer/Int64, Natural, Int8/16/32, Byte/UInt8, UInt16/32,
  UInt64/UInteger (0..2^64-1, unsigned arithmetic, comparisons, `case`,
  `for`, input and output, r3.79), Float/Float64 and Float32, String, Char;
  checked arithmetic, conversions `T(X)` between integer types, Float and
  Float32 (r3.84), real Consts, runtime faults with exit status 70.
- Control flow: if/elif/else, unless, conditional expression `IIf(C, A, B)`, while,
  repeat/until, for with step, leave/continue, case (Ada style), with
  (any place as target, evaluated once, checked through pointers).
- Structs with defaults, named construction, nesting, associated methods;
  method reuse (`Struct (TPart)`, reused fields and methods at every level,
  `Override`); methods of built-in types (`String.UpdateStr`); fixed arrays (multi-dimensional, in fields and State, of structs and
  Strings); read-only slices; `ref`/`ref mut` parameters.
- Design by contract (CANON-24): `pre`/`pos` with `old` and `Result`,
  struct `invariant` (also of reused parts), `pre else`/`pos then` in an
  `Override`, loop `invariant`/`variant`; violations are runtime traps
  (exit status 70); always on through 1.0 (monitoring levels per module
  are FUTURE, after 1.0).
- Safe pointers (CANON-25): `Own<T>` (one owner, never copied, `Move`),
  `Option<Own<T>>` and `None`, `Link<T>` to owned objects, locals, fields,
  array elements and State, checked by a generation on every use;
  `New`, `Free`, `Move`, `Value` (read and written), `Link`, `Alive`,
  `Same`; a definite-move analysis over every path; owners freed when
  overwritten, at `Free`, at routine exit (also by an exception) and when
  their declaration runs again, without recursion; owner parameters and
  results; contracts on owner parameters. Runtime `runtime/OwnRuntime.cpp`;
  the leak check covers the pointer programs.
- Generics (CANON-8, r3.77): generic routines (`Max<T>(A T, B T) T :`) and
  generic structs (`TPair<K, V> Struct`), one instance per list of types,
  type arguments inferred from the arguments or the target, no
  constraints (an error names the instance); across modules, with
  contracts and exceptions; the leak check covers the generic programs.
- Enum, Range and Set (CANON-13, r3.80): enums with Ord, checked TEnum(I),
  Low/High, checked Succ/Pred, `for` and `case` coverage, printed by name;
  Ranges of Integer checked when stored (kind 11); Set<T> with `in`, the
  comparisons, the six functions (`Include`/`Exclude` since r3.81; a call
  as a statement is rejected) and `[A, B]`; `X in A..B`.
- `case` on a String (r3.85): String literal and Const choices, `else`
  always.
- Char values (r3.83): literals with escapes, `Char(I)` checked,
  `Integer(C)`, `String(C)`, `CharAt`, comparison, Char ranges in `for`,
  `case` and `in`, Char Ranges and sets over them, output in UTF-8.
- Required fields (r3.82): enum, array and type-parameter fields without
  default are given in the construction; type defaults only when complete,
  nested and in parts; array field defaults `:= Fill(Literal)`.
- Every local and State array is initialized where it is declared;
  `Fill(Value)` by element or row, in declarations, State and assignments,
  for every element type (r3.81).
- Strings: reference-counted UTF-8 values everywhere a value lives,
  equality and ordering, `GetLn(S)`; `+` and `+=` (one allocation per
  chain, appends in place when the String alone holds its block);
  Std.Strings (Length, ByteLength, Substr, Position, UpperCase, LowerCase)
  and Std.StrUtils (Trim, LTrim, RTrim, Left, Right, PadLeft, PadRight,
  Replicate, Replace, Contains, StartsWith, EndsWith, SameText,
  CompareText) by Unicode 15.1 rules (r3.78); a String read from State, a
  `ref` parameter or through a pointer is held for its statement.
- Exceptions: try/except/On/Else/ensure/Raise/Retry with the standard
  taxonomy, Itanium landing pads on Linux and MSVC funclets on Windows.
- Text files: Std.Files (type and basic operations) / Std.IOUtils
  (conveniences).
- Standard library: Std.System, Std.IO, Std.Math, Std.Errors, Std.Debug
  (Assert), Std.Files, Std.IOUtils, Std.Strings, Std.StrUtils.
- Windows x64 and Linux x64 run the full suite in CI
  (`ci/windows-known-failures.txt` is empty).

## Conformance gaps (the law is ahead of the code)

- Strings: `S[I]` and `GraphemeLength` are reserved; collation,
  normalization, Split/Join and conversions between numbers and text come
  in later changes; case mapping is one code point to one (`ß` stays `ß`).
- Char: reading one Char from the input (`Get(C)`) is not implemented, and
  a Char array element, stored in 32 bits like a narrow integer, cannot be
  passed by `ref` yet.
- Currency and Crypto: names registered, no arithmetic or literals;
  BigCurrency absent.
- Vector, `Option`, `Result`: not implemented (`Option` exists only as
  `Option<Own<T>>`).
- Safe pointers: a Link to a parameter is rejected ("not supported yet");
  struct invariants are not checked after a write through a pointer;
  a Link to a local always places the local in a generation block (the
  escape analysis that keeps it on the stack at no cost is not done); an
  owner used after Move or Free on a path the analysis does not follow
  (`leave`/`continue` out of a loop) is caught at run time (trap "use of
  an owner after Move or Free"), not at compile time; an owner declared
  inside a loop is freed when its declaration runs again or the routine
  returns, not at the end of each iteration. Each is a task of change
  `safe-pointers-followups`.
- Real literals have no exponent form (`1.0e3`), as the grammar says.
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
- Std.Strings: a statement that calls a routine and names a State or `ref`
  String may get an empty cleanup pad when the String is passed by address
  rather than read (the scan that decides is conservative).
- Types (change `type-representation`): the parser keeps written type text
  (`Array[1..N] T`, `Link<T>`), which the analyzer reads with
  `types::TypeSyntax` and resolves; a typed AST for types is a later step.
- Exception handlers are matched by the written type name
  (`exceptions/ExceptionTypes`); the exception hierarchy (identifiers,
  parents) is not part of `types::Type` yet.
- Generics (change `generics`): methods of generic structs, and invariants
  and reused parts in a generic struct, are rejected for now; semantic
  messages carry no line numbers, so an error in an instance names the
  instance and the routine that used it but not the line of the generic
  body.
- The parser's `clonePureExpression` and `cloneChainTarget` duplicate
  `ast::cloneExpression`; `codegen/AstQueries` holds AST walks that are not
  specific to codegen; the parser never builds `BlockStatement`.
