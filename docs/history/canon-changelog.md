<!-- SPDX-License-Identifier: MPL-2.0 -->
# Change log of the retired canon (v1.0 to r3.60)

Frozen record. These entries were the change log of `docs/INOX_CANONICAL.md`,
copied verbatim when the canon was retired into the constitution at r3.61
(`openspec/specs/constitution/`). The full last text of the canon is the git
tag `canon-final`. Newer revisions are in
`openspec/specs/constitution/revisions.md`. Entries keep their historical
spelling (`finally`, `break`, `Var`, `[T]`); the law is the constitution.

```text
## CHANGE LOG (newest first — dated, attributed, append-only)
# ============================================================================
#
# r3.60 — 2026-10-12 — version scheme; file rules approved.
#         Marcelo Fortes, 2026-10-12: approved in full;
#         the version is MAJOR.MINOR.RELEASE.BUILD; the current
#         version is 0.2.0, BUILD not yet defined, pre-alpha.
#   - Layer A, CANON-17 "Files": the rules settled while implementing r3.59
#     are approved: the file names exist only with `Use Std.IOUtils` (the
#     File type also with `Use Std.Files`); RenameFile raises AlreadyExists
#     when the target exists, on every system; DeleteFile never deletes a
#     directory and Exists is true only for a regular file; ReadLn past the
#     end returns "" and Eof is true; a file declared in a loop is closed
#     when the next iteration stores another, the last one at routine exit.
#   - SECTION 02 VERSIONING: the version is MAJOR.MINOR.RELEASE.BUILD (0..99,
#     0..999, 0..999, 0..9999) plus the stage; today 0.2.0.<BUILD>
#     pre-alpha. The BUILD, left undefined by the maintainer, is defined as
#     the commit count of the compiled history, written at every build by
#     cmake/InoxBuildNumber.cmake (CI clones the full history). This
#     replaces the SemVer wording of r3.58 ("0.2.0-pre-alpha").
#   - The test scripts check `inox --version` against this header with any
#     BUILD.
#
# r3.59 — 2026-10-12 — text files: Std.Files and Std.IOUtils (Layer A and
#         Layer B). Decisions F1–F7 approved by Marcelo Fortes on 2026-10-12
#         (all approved, with I/O errors usable in try / except On /
#         ensure, not only with Retry, and the basic file functions); design
#         approved on 2026-10-12 (approved in full) with the module pair
#         Std.Files (the File type) / Std.IOUtils (the functions). OpenSpec
#         change `file-io`, Risk: high.
#   - Layer A, CANON-17 "Files": a File closes itself when the routine that
#     holds it exits by any path; OpenRead / Create / OpenAppend; errors are
#     FileNotFound, PermissionDenied, AlreadyExists, DiskFull or IOError,
#     ordinary exceptions; text only (ReadLn, Write, WriteLn, Eof, Close);
#     UTF-8, lines written with "\n", "\n" or "\r\n" read; a File is never
#     copied (a local that receives a file just opened, or a ref mut
#     parameter); Exists, DeleteFile, RenameFile.
#   - Rules settled while implementing (part of the approved design,
#     reported to the maintainer): the file names are visible only in a
#     program that uses the module (`Use Std.IOUtils`; the File type also
#     with `Use Std.Files`), so Create, Close, Write ... stay free elsewhere;
#     RenameFile raises AlreadyExists when the target exists, on every
#     system; DeleteFile never deletes a directory; Exists is true only for a
#     regular file; Write/WriteLn format values as PutLn; ReadLn past the end
#     returns "" (Eof is true); reading a file opened for writing, or using
#     a closed file, raises IOError; a file declared inside a loop is closed
#     when the next iteration stores a new one, and the last one when the
#     routine exits (close a file before deleting it: Windows refuses to
#     delete an open file).
#   - Layer B: runtime/FileRuntime.cpp (a reference-counted handle; every
#     FILE* stays in libinoxrt; binary mode; UTF-16 paths on Windows; errno
#     mapped to the exceptions); the String ownership paths of the emitter
#     own File values too (released, i.e. closed, at every exit and in the
#     cleanup pad).
#   - Tests: runtime files-text, files-close-on-exception, files-not-found,
#     files-system, file-names-free; 13 diagnostics (file-copy,
#     file-assign-copy, file-without-use, file-type-without-use, file-field,
#     file-parameter-by-value, file-return, file-state, file-array-element,
#     file-compare, file-write-without-value, file-argument-type,
#     file-name-taken); inoxrt-tests file runtime cases;
#     tools/string_leak_check.py also runs the file programs.
#
# r3.58 — 2026-10-12 — naming of the standard library; versioning.
#         Marcelo Fortes, 2026-10-12: Std.Core becomes Std.System;
#         Std.Strings holds only the definitions and general
#         data of Strings, like a C header; Std.StrUtils holds the conveniences
#         and may use Std.Strings; the same for Std.DateTime/Std.DateUtils and
#         Std.Files/Std.IOUtils; no catch-all module, and a Std.SystemUtils with
#         only system functions; the 3.XX numbering changes; the language is a 0.2
#         or 0.3 pre-alpha
#   - Layer A, CANON-18: `Std.Core` is renamed `Std.System` (the implicit
#     prelude, like Delphi's System unit).
#   - Layer A, CANON-18: library modules come in pairs. `Std.X` holds only
#     the definitions and general data of a subject (types, constants,
#     exception types, the facts the compiler knows), like a C header;
#     `Std.XUtils` holds the functions that help the programmer and may use
#     `Std.X`. Pairs planned: `Std.Strings` / `Std.StrUtils`,
#     `Std.DateTime` / `Std.DateUtils`, `Std.Files` / `Std.IOUtils`;
#     `Std.SystemUtils` holds only system-specific functions. No module is a
#     catch-all.
#   - SECTION 02 gains VERSIONING: the language is `Inox 0.2.0-pre-alpha`
#     (language and compiler share one version); the `v3.xx` numbers are
#     canon revisions, written `r3.NN` from now on. `inox --version` prints
#     both; CMake project version 0.2.0.
#   - The compact-form example of `Module`/`Use` said `Use Sys.IO`, a module
#     that never existed; it now says `Use Std.IO`.
#   - OpenSpec: `string-runtime` and `comparisons` archived (merged in #36,
#     #37, #38); `std-strings` and `file-io` follow the module pairs.
#
# v3.57 — 2026-10-12 — two Layer A decisions and a planned library module.
#         Marcelo Fortes, 2026-10-12: Bool is ordered, false < true, as in Pascal and Ada; structs are not compared with =/#; compare them
#         field by field, as in Pascal; it stays an error that says so; record these library functions as future
#         Std.Strings work OpenSpec changes `comparisons` (updated)
#         and `std-strings` (planned).
#   - Layer A, CANON-8: Bool is ordered, false < true; every comparison
#     operator applies (replaces the provisional v3.56 error).
#   - Layer A, CANON-9: structs are never compared with `=`, `#` or the
#     ordering operators, as in Pascal; compare their fields (the v3.56
#     error is now the rule, no longer provisional).
#   - CANON-18: `Std.Strings` is registered as a planned module (not
#     implemented): lengths, trimming, case mapping and case-insensitive
#     comparison by Unicode rules, language-neutral collation, search,
#     slicing, replacing, splitting, joining and padding. Names of the other
#     planned modules (the Delphi SysUtils / StrUtils / DateUtils / IOUtils
#     families) are awaiting the maintainer's choice.
#   - Tests: runtime `bool-ordering` (replaces the diagnostic of v3.56);
#     backend_gaps probe `bool-equality` becomes `bool-comparison`.
#
# v3.56 — 2026-10-12 — String ordering (Layer A) and comparison operand rules.
#         Marcelo Fortes, 2026-10-12: Strings are ordered (<, >, <=, >=, #, not,
#         unless, combined with or and and), as in Pascal.
#         OpenSpec change `comparisons`.
#   - Layer A, CANON-14: `<` `>` `<=` `>=` order Strings, as in the Wirth
#     family: byte by byte over UTF-8, the shorter first when one is a prefix
#     of the other. This is Unicode code point order and case-sensitive
#     ("Z" < "a" < "á"); it is not a locale collation (accent- or
#     case-insensitive ordering is a library function, not an operator).
#     The results combine with `not`, `and`, `xor`, `or`, `unless`, `elif`,
#     loop conditions and the conditional expression like any Bool.
#   - Comparison operands (until the maintainer decides otherwise, so that no
#     program is accepted and then miscompiled): Bool values compare with `=`
#     and `#` only (`false < true` is an error); structs are not compared
#     (compare their fields). Both used to pass the analyzer and then produce
#     invalid IR, as did `<` on Strings; `=` on two Bool variables did too.
#   - Tests: runtime `strings-ordering` (input lines, leak-checked);
#     diagnostics `bool-ordering`, `struct-equality`, `struct-ordering`;
#     `inoxrt-tests` covers `__inox_str_compare`. backend_gaps probes
#     `string-ordering`, `bool-equality` (GAP=2, OK=47).
#
# v3.55 — 2026-10-12 — String runtime (Layer B; CANON-14 and CANON-17 now
#         state the v3.50 decisions as implemented). OpenSpec change
#         `string-runtime`, Risk: high; design approved by Marcelo Fortes on
#         2026-10-12 (all approved).
#   - A String value is a pointer to an immutable, reference-counted block in
#     libinoxrt (`runtime/StringRuntime.cpp`); literals and `""` are immortal.
#     Strings live in locals, parameters, results, struct fields (with or
#     without a literal default), array elements (any dimension), State and
#     `ref`/`ref mut` parameters; `=`/`#` compare bytes.
#   - `GetLn(S)` reads one line into a String: without the line break, a CR
#     before it dropped, `""` at end of input. The line must be well-formed
#     UTF-8 (RFC 3629).
#   - Two runtime faults are added to CANON-19: kind 12 "out of memory" and
#     kind 13 "invalid UTF-8 input". (v3.50 called invalid UTF-8 "invalid
#     input"; it gets its own message instead of the Integer one, kind 6.)
#   - Changes from the approved design, reported to the maintainer: owned
#     Strings are released at every exit of the routine and when overwritten,
#     not at the end of every inner block; a routine holding Strings gets one
#     cleanup pad built on the existing exception capture and rethrow, not on
#     `cleanuppad` funclets.
#   - Fixed on the way: a function called with no arguments inside `try`
#     (`N := Fail`) was emitted with `call` instead of `invoke`, so its
#     exception aborted the program instead of reaching the handler.
#   - Tests: runtime `strings`, `strings-getln`, `strings-aggregates`,
#     `strings-exception-cleanup`, `string-local` (moved from diagnostics),
#     `exception-zero-argument-call`; trap `strings-invalid-utf8`; diagnostics
#     `string-without-initializer`, `get-string`; runtime unit tests
#     `inoxrt-tests`. E19 gains `tools/string_leak_check.py` (the String
#     programs under AddressSanitizer with leak detection; Linux CI).
#     backend_gaps GAP=2, OK=45.
#
# v3.54 — 2026-10-12 — workflow safeguards and one Layer A confirmation
#         (Marcelo Fortes, 2026-10-12: make the three adjustments, adopt what is worth
#         adopting, flag every change, keep OpenSpec up to date;
#         `TName := Integer` is an error). OpenSpec change
#         `workflow-safeguards`.
#   - Layer A, CANON-5: a Type section declares Struct, Range and Enum types
#     only; `TName := Type` is an error. Inox has no type alias form (the
#     built-in aliases of CANON-8, such as Integer -> Int64, are not affected).
#   - E19 gains `tools/ir_snapshot.py`, the no-regression gate for refactors.
#   - AGENTS.md: "Known pitfalls" (lessons already paid for); high-risk changes
#     (exception lowering, runtime ABI, memory ownership, a phase rewrite) wait
#     for the maintainer's dated approval of design.md before code;
#     `string-runtime` is the first such change.
#   - OpenSpec changes are archived right after their PR is merged, so
#     openspec/specs matches main (`struct-values` and `declaration-sections`
#     are archived).
#
# v3.53 — 2026-10-12 — Type, Const and State sections parsed into typed
#         declarations (Layer B; CANON-5, CANON-9 and CANON-13 unchanged).
#         OpenSpec change `declaration-sections`, item 2 of the order approved
#         on 2026-10-12.
#   - The parser builds, for Type: Struct (fields with their type and literal
#     default), Range (its `Low..High` expression) and short or multi-line
#     Enum (its literals); for Const and State: one name, type and
#     initializer per line. The analyzer and the emitter read these nodes; the
#     three token scanners are deleted (E11, E13). Every test and example gives
#     the same IR and types as before, except the three intended changes below.
#   - New: a struct field may be a fixed array (bounds may name a Const), with
#     checked indices, copies and defaults reaching struct elements inside it;
#     State may hold structs and arrays, with or without initializer
#     (`Origin TPoint`, `Counts Array[1..3] Integer`).
#   - Diagnostics name the line: a field without type, a default that is not
#     one literal, a Range without `Low..High`, two enum literals on one line,
#     a `TName := Type` line (not a Type form), `Type :`. A slice cannot be a
#     struct field or a State variable (ADR-0017); an array or struct field has
#     no literal default; grouped declarations in State are reported as not
#     supported yet (they were a misleading assignment error).
#   - Range and Enum declarations are parsed and shape-checked only; their
#     meaning comes with the change `enum-range-set`.
#   - Tests: runtime `struct-array-field`, `state-aggregates`; trap
#     `struct-array-field-out-of-bounds`; semantic-valid `type-section-forms`;
#     diagnostics `struct-field-without-type`, `struct-field-slice`,
#     `state-slice`, `struct-array-field-default`,
#     `struct-field-default-expression`, `state-grouped`,
#     `type-assignment-form`, `range-without-bounds`,
#     `enum-literals-per-line`. backend_gaps GAP=3, OK=42 (probes
#     `struct-array-field`, `state-aggregates`).
#
# v3.52 — 2026-10-12 — struct values lowered (Layer B; CANON-5 and CANON-9
#         unchanged). OpenSpec change `struct-values`, item 1 of the order
#         approved on 2026-10-12.
#   - Struct fields of every lowered scalar type (the Integer family, Natural,
#     Float, Float32, Bool) and of struct type, declared in any order; literal
#     defaults for each scalar type, optionally negative, checked against the
#     field type; a struct that contains itself, directly or through other
#     structs, is a compile error ("struct contains itself: T").
#   - `Q T := Expr` and grouped `P, Q T := Expr` (the initializer runs once,
#     each name gets a copy); named construction `T(F := V, ...)` as an
#     expression anywhere a value is expected; storing into a Natural or narrow
#     field keeps the ADR-0015 and CANON-8 checks.
#   - Field paths through fields and elements (`L.FB.FX`, `A[I].FX`), reads,
#     writes, `ref mut` arguments, `with` and methods on any struct place
#     (`L.FB.Move(1)`, `A[1].Sum`).
#   - Arrays of struct: elements start at the struct defaults; indices are
#     checked (kind 9); arrays of struct are parameters and results.
#   - Layer B: a struct is a named LLVM type; narrow integer fields are i64
#     (arrays keep narrow elements in their own width); Float32 defaults are
#     written as LLVM hexadecimal constants.
#   - Tests: runtime `struct-defaults`, `struct-construction`,
#     `struct-initializers`, `struct-nested`, `array-of-struct`; trap
#     `array-of-struct-out-of-bounds`; diagnostics `struct-recursive`,
#     `struct-recursive-indirect`, `struct-default-out-of-range`,
#     `struct-default-negative-natural`, `struct-float-default-integer`.
#     backend_gaps: GAP=3, OK=40 (`grouped-decl-struct` and
#     `named-struct-construction` closed; probes `struct-nested`,
#     `array-of-struct` added).
#   - Still to come (change `declaration-sections`): struct State, array
#     fields; String fields wait for the String runtime.
#
# v3.51 — 2026-10-12 — enum printing decided (Layer A, Marcelo Fortes,
#         2026-10-12: answer (a), the literal's name). Closes the item left open
#         in v3.50: `PutLn(E)` and `Put(E)` print the enum literal's name as it
#         is spelled in its declaration (`Hearts` for `TCardSuit (Clubs, Diamonds,
#         Hearts, Spades)`); `Ord(E)` gives the ordinal. Enum literals are
#         identifiers, never strings: `TCardSuit ("Clubs", ...)` is not an enum.
#
# v3.50 — 2026-10-12 — Layer A decisions for UInt64, Enum/Range/Set and String
#         (Marcelo Fortes, 2026-10-12, answering item by item the open
#         questions of the OpenSpec changes `uint64`, `enum-range-set` and
#         `string-runtime`). Implementation follows in those changes.
#   - UInt64: in a UInt64 context only, an integer literal may be up to
#     2^64-1 (decimal or hex); elsewhere CANON-19's "fits Int64" stays. Unary
#     minus on UInt64 is a compile error. A UInt64 and an Integer variable never
#     mix without a written conversion.
#   - Range: storing an Integer into a Range variable is checked implicitly, as
#     for Natural (ADR-0015): new fault kind 11 "value out of range"; a
#     constant outside is a compile error; Range arithmetic yields the base type.
#   - Enum: `TEnum(I)` out of range is fault kind 8; a literal name belongs to
#     one enum only; `Low(T)`/`High(T)` on enum and range types; `Succ`/`Pred`
#     checked. OPEN: whether `PutLn(E)` prints the literal's name (asked by the
#     maintainer; Phase A does not lower enum printing until decided).
#   - Set: operations are functions returning a new set (`S := With(S, Club)`,
#     `Union(A, B)`, ...); the literal `[A, B]` is accepted in a Set-typed
#     context; a base has at most 256 values.
#   - String: `GetLn(S)` reads a line into a String variable (invalid UTF-8 is
#     invalid input); reference counts are non-atomic in 0.1; concatenation and
#     length functions stay reserved; `S String` without `:=` stays an error for
#     locals and State, while String fields without default and array elements
#     start as `""`. For `GetLn(S)`, this ends the CANON-17 deferral of String
#     input.
#
# v3.49 — 2026-10-12 — migration of this document to OpenSpec, phases 1 and 3a
#         (plan approved by Marcelo Fortes on 2026-10-12: phased migration plan approved; the ADRs become archived OpenSpec changes). No Layer A rule changes; this document stays the law (G7).
#   - Phase 1, inventory: `tools/canon_trace.py` splits this document into
#     units that cover every line; `openspec/migration/CANON_MAP.md` records
#     each unit's hash, status (planned, mirrored, copied, approved, duplicate,
#     structural) and destination. CI runs `check`: an unmapped unit, an
#     archived ADR that is no longer verbatim, or a mirrored unit changed here
#     without its spec fails the build. After editing this document, run
#     `tools/canon_trace.py update`.
#   - Phase 3a: ADR-0001..ADR-0017 imported verbatim as archived OpenSpec
#     changes (`openspec/changes/archive/<date>-adr-NNNN-*`, decision in
#     proposal.md, rationale in design.md). Dates of ADR-0001..0006 come from
#     git (2026-05-28 and 2026-05-31). The original ADR-0005 and ADR-0006 files,
#     removed on 2026-06-15 when this document absorbed them, are restored
#     verbatim from git next to them. The ADR text below is unchanged.
#   - Duplicates found and recorded: DEV-2 and GROUP FUTURE appear twice,
#     identical; the AGENT RULES list appears twice (same rules).
#   - The case-statement and fixed-arrays specs now cover their canon units
#     completely (new diagnostics `case-of-keyword`, `case-arrow-arm`,
#     `case-when-arm`, `case-as-expression`, `array-bounds-part-of-type`,
#     `array-literal-reserved`); the seven baseline specs now say they are
#     mirrors, not law (one wrongly said "migrated").
#   - Status: 255 units (this entry included); 17 copied, 3 mirrored,
#     3 duplicate, 36 structural, 196 planned.
#
# v3.48 — 2026-10-12 — OpenSpec adopted for specifications and changes
#         (governance G7, requested by Marcelo Fortes on 2026-10-12: OpenSpec manages and coordinates the specifications
#         of the language and the compiler). No Layer A language rule changes.
#   - `openspec/` (OpenSpec 1.14.1): `config.yaml` carries the Inox context and
#     the proposal/spec/design/task rules; `.claude/` holds the `/opsx:*`
#     commands and skills.
#   - Baseline mirrors of Layer A, every scenario tied to an existing test:
#     `language/case-statement` (ADR-0011, CANON-12), `language/fixed-width-
#     integers` (CANON-8, v3.44), `language/natural` (ADR-0014, ADR-0015),
#     `language/module-state` (CANON-5 Const and State), `language/fixed-
#     arrays` (CANON-13 Array), `language/ref-parameters` (ADR-0016),
#     `language/slices` (ADR-0017). This document stays the law for them (G7).
#   - Open changes for the order approved on 2026-10-12 (decisions 1 to 5 approved): `struct-values`, `declaration-sections`, `uint64`,
#     `enum-range-set`, `string-runtime`. Their open questions (UInt64 Q1–Q3,
#     Enum/Range/Set Q1–Q7, String Q1–Q4) await the maintainer.
#   - AGENTS.md truth hierarchy corrected: it still named the removed
#     `docs/decisions/` and `docs/canonical/` files.
#   - CI validates `openspec validate --all --strict`.
#
# v3.47 — 2026-10-11 — ADR-0017: read-only slices (Layer A, approved by Marcelo
#         Fortes on 2026-10-11 with the order "Array, ref, Slice").
#   - `A[L..H]`, `A[..H]`, `A[L..]`, `A[..]`; type `Slice<T>`, the first generic
#     type the compiler accepts; indices kept; never empty; read-only; never
#     outlives its array (named base only, bound once, not returned).
#   - Layer B: a slice is `{ ptr, i64, i64 }` (data at element L, L, H); new
#     fault kind 10 "slice bounds out of range"; indices checked against the
#     slice bounds (kind 9).
#   - Tests: runtime `slices`, traps `slice-bounds-runtime`,
#     `slice-index-out-of-bounds`; diagnostics `slice-readonly`,
#     `slice-reassign`, `slice-return`, `slice-of-temporary`,
#     `slice-constant-bounds`, `slice-empty`, `slice-by-ref`.
#
# v3.46 — 2026-10-11 — ADR-0016: `ref X T` and `ref mut X T` parameters
#         (Layer A, approved by Marcelo Fortes on 2026-10-11 as part of the
#         order "Array, then ref, then Slice": do everything, in the proposed order). `ref` becomes a keyword (50).
#   - Rules in ADR-0016: exact type, writable place for `ref mut`, no other
#     argument may use a `ref mut` variable, references never outlive the call.
#   - Layer B: a reference parameter is an LLVM `ptr` to the caller's slot,
#     field or element; a `ref` to a non-variable value uses a temporary. A
#     narrow-integer array element (stored in its own width) cannot be passed
#     by reference yet.
#   - Tests: runtime `ref-parameters`, trap `ref-mut-range`; diagnostics
#     `ref-mut-aliasing`, `ref-mut-not-writable`, `ref-type-exact`,
#     `ref-readonly-assignment`, `ref-mut-from-value-parameter`.
#
# v3.45 — 2026-10-11 — fixed arrays `Array[Low..High] T` implemented (Layer B;
#         CANON-13 unchanged).
#   - Parser: `A Array[1..10] Integer`; a bound is an integer literal
#     (optionally negative) or a Const name; `Array[1..3, 1..4] T` is an array
#     of arrays and `M[I, J]` is `M[I][J]`. In a routine signature an array
#     type is one type token.
#   - Semantics: Const bounds are resolved (`Array[1..Size] T` equals
#     `Array[1..5] T` when Size is 5); Low > High is an error; an index is an
#     integer, and a constant index outside Low..High is a compile error.
#     `Low(A)`, `High(A)`, `Length(A)` are constants. Arrays are not compared.
#     `A[I] := X` needs a mutable array variable (parameters are immutable).
#   - ADR-0012 extension of the target rule: `A[I] += 1` is accepted when the
#     indices have no side effects (literals, variables, operators, field and
#     index reads), so reading and writing the element through the same
#     indices is exact; `A[Next] += 1` with a call stays an error.
#   - Lowering: an array is an LLVM `[N x T]` value (value type: assignment and
#     by-value parameters copy); narrow integer elements are stored in their
#     own width (Byte in i8) and widened when read. Every index is checked
#     (new fault kind 9, "array index out of bounds"). Arrays are allowed as
#     parameters and results.
#   - Still to come: arrays in struct fields and State, String/struct
#     elements, array literals, `ref` parameters (next).
#   - Tests: runtime `arrays`, trap `array-index-out-of-bounds`; diagnostics
#     `array-constant-index-out-of-bounds`, `array-compare`,
#     `array-empty-bounds`, `array-index-not-integer`,
#     `array-parameter-immutable`, `index-non-array`,
#     `array-element-type-unsupported`; `compound-index-target` now checks a
#     call in the index.
#
# v3.44 — 2026-10-11 — fixed-width integers lowered (Layer B) and a CANON-8
#         clarification of constant literals (Layer A text added under
#         "Conversions"; it applies existing rules, no new decision).
#   - Int8, Int16, Int32, UInt8/Byte, UInt16 and UInt32 are compiled. A value
#     is carried as i64 holding the mathematical value; every arithmetic
#     result of such a type is range-checked (fault kind 1, integer
#     overflow), so 255 + 1 in a Byte and 3 - 4 in a UInt8 trap and never
#     wrap. `T(X)` checks the range at run time (new fault kind 8, "integer
#     conversion out of range"); Get/GetLn into such a variable checks too
#     (kind 6). The shift count is 0..width-1 (kind 3). Works in locals,
#     parameters, returns, struct fields, State and case.
#   - Constant literals: `B Byte := 200`, `B += 55`, `P.R + 5` read the
#     constant in the fixed-width type; `B Byte := 256` and `B + 300` are
#     compile-time range errors; a narrow constant expression that overflows
#     is a compile error.
#   - `bitnot` is lowered (it was a backend gap): two's-complement NOT, and
#     `x xor (2^n - 1)` for an unsigned narrow type.
#   - Still a gap: UInt64/UInteger (outside the i64 range).
#   - Tests: runtime `fixed-width-integers`; traps `byte-overflow`,
#     `unsigned-underflow`, `int8-negate-min`, `byte-conversion-runtime-range`,
#     `byte-shift-count`; diagnostics `byte-literal-out-of-range`,
#     `byte-operand-out-of-range`, `byte-from-integer-variable`.
#
# v3.43 — 2026-10-11 — module `State` lowered (Layer B; CANON-5 unchanged).
#   - Each State variable of type Integer/Int64, Natural, Bool or Float is an
#     internal LLVM global (`@inox.state.<name>`), zero at load time and
#     initialized in source order at the start of `main`, so an initializer
#     may call a function or read an earlier State. Routines read and write it
#     like a local; a Natural State keeps the ADR-0015 range check.
#   - Still a gap: State of struct, String or fixed-width type ("State
#     variables of type T are not lowered yet").
#   - Tests: runtime `state-lowering`, trap `state-natural-range`.
#
# v3.42 — 2026-10-11 — `case` and `unless` lowered (Layer B; ADR-0011 is
#         unchanged).
#   - `case`: the selector is evaluated once; arms are tested in source order
#     with `icmp eq` for a value and a signed `sge`/`sle` pair for a static
#     range (unsigned for Bool); `|` alternatives share one arm body. A case
#     without `otherwise` is total (ADR-0011), so its fall-through edge is
#     `unreachable`. `unless C` branches to its body when C is false.
#   - A function whose last statement cannot fall through (for example a total
#     case whose arms all Return) is now lowered: semantic analysis already
#     proves every path returns, so the end is `unreachable`.
#   - Parser fix: a single-line arm body ends at its line, so a next arm that
#     starts with `-` (`-5..-1`) is no longer read as `Return 0 - 5 .. -1`.
#   - Tests: runtime `case-lowering`, `unless-lowering`; the diagnostic
#     `backend-gap-case` becomes `backend-gap-string-local` (String locals are
#     still a backend gap).
#
# v3.41 — 2026-10-10 — EH-v3.16a closed (Layer B only).
#   - The MSVC exception bridge (ChatGPT's patch, PR #21) is on main. On the
#     windows-2022 runner every exception test passes, so
#     `ci/windows-known-failures.txt` is empty, and the PoC criteria (i)-(v)
#     pass. The Windows job now runs the PoC script after the suite.
#
# v3.40 — 2026-10-10 — ADR-0015: storing into a Natural checks the range
#         implicitly, as in Ada (Layer A, decided by Marcelo Fortes on
#         2026-10-10: as in Ada, an assignment checks the range by itself,
#         with no conversion written).
#   - Supersedes ADR-0014 points 3 and 4 where they require `Natural(X)`:
#     initialization, assignment, compound assignment, argument passing,
#     return and field initialization of a Natural from an Integer value are
#     range-checked at run time with no conversion written. `N += 1` is valid.
#   - A negative constant stored into a Natural stays a compile-time error.
#   - An explicit conversion written as a cast is not a Natural feature: it
#     is only recorded as a possible future form (it may become unnecessary
#     with generics). The general `T(X)` conversion of CANON-8 is unchanged.
#   - Layer B: semantic analysis marks each checked expression; the backend
#     wraps it in the range check (fault kind 7).
#   - Tests: runtime `natural-implicit-check`, trap
#     `natural-assignment-negative`. The diagnostics `natural-from-integer`,
#     `natural-arithmetic-is-integer` and `natural-negation-is-integer` are
#     removed: those programs are valid now.
#
# v3.39 — 2026-10-10 — ADR-0014: Natural, Byte and the closed numeric set
#         (Layer A, decided by Marcelo Fortes on 2026-10-10: the Ada 2005 consensus, without negatives, is
#         accepted and recorded in the canon; no Double, Float stays; Inox has no Variant or OleVariant).
#   - Natural is the non-negative subtype of Integer, 0..Int64.Max, with the
#     Int64 (i64) representation; it is no longer described as UInt64 with
#     floor 0. UInt64 stays a distinct type. The CANON-8 table row changes.
#   - Natural -> Integer is implicit. Integer -> Natural is written
#     `Natural(X)`, checked at run time (fault "value out of range for
#     Natural", CANON-19); a constant >= 0 needs no conversion and a negative
#     constant is a compile-time error. Arithmetic on Natural is done in the
#     Integer family and yields Integer, as do `-N` and `bitnot N`.
#   - Because the domain is 0..Int64.Max, a case on Natural is provable with
#     Int64 constants: 0..9223372036854775807 is total (ADR-0011 coverage).
#   - No `Double` (Float/Float64 is the 64-bit real) and no `Variant` or
#     `OleVariant`: the numeric set is closed by CANON-8.
#   - Layer B: `Byte` is registered as UInt8. Integer conversions check a
#     constant argument against the target range (`Byte(300)`, `Int8(-129)`
#     are compile errors). Fixed-width integers other than Int64 are not yet
#     lowered by the LLVM backend (BACKEND_GAPS: fixed-width locals).
#   - Tests: runtime `natural-values`, trap `natural-conversion-negative`;
#     diagnostics `natural-negative-constant`,
#     `natural-conversion-negative-constant`, `natural-from-integer`,
#     `natural-arithmetic-is-integer`, `natural-negation-is-integer`,
#     `case-natural-missing-zero`, `byte-conversion-out-of-range`,
#     `int8-conversion-out-of-range`; semantic-valid `case-natural-total`,
#     `byte-is-uint8`. Diagnostic `case-natural-needs-otherwise` is removed:
#     its premise (Natural = UInt64) is superseded.
#
# v3.38 — 2026-10-10 — OPEN-5 closed (Layer A, decided by Marcelo Fortes on
#         2026-10-10: only `Const Name := Value`, with `UInt8($FF)`).
#   - A Const is written `Const Name := Value` only. Its type is the type of
#     the value; a conversion chooses another type: `Const Mask := UInt8($FF)`.
#   - Layer B: the parser rejected nothing before; `Const Mask UInt8 := $FF` is
#     now an error whose message shows the conversion form.
#     `tests/runtime/const-values.inox` drops its typed Const.
#   - Tests: diagnostic `const-typed-rejected`; semantic-valid
#     `const-conversion`.
#
# v3.37 — 2026-10-10 — conditional expression lowered (Layer B; ADR-0013 is
#         unchanged).
#   - `if C then A else B` is lowered as control flow, never as an LLVM
#     `select` (which would evaluate both branches, against property 3). Each
#     branch jumps to its own tail block and the phi names the tails, so a
#     branch that ends in an `invoke` continuation (inside `try`) or in a
#     nested conditional's merge stays valid. Design proposed by ChatGPT, who
#     handed this emitter change to Claude outside the EH-bridge paths.
#   - Tests: runtime `conditional-expression-lazy` (the branch not chosen would
#     trap with a division by zero and never runs; Float, Bool, chained) and
#     `conditional-expression-in-try`. Probe `conditional-expression` is OK.
#
# v3.36 — 2026-10-10 — maintainer decisions recorded from the design chat with
#         ChatGPT that were missing here (documentation only; no compiler
#         change). Found by reading the whole chat on Marcelo's request.
#   - FUTURE-1: the concurrency construct is spelled `do`, not `go` (in Inox it is `do`, not `go`). `do` is already reserved.
#   - Generics: `<T>` replaces `[T]` (`<T>` for generics instead of `[T]`, for a
#         pragmatic reason: `[T]` could be ambiguous with array indexing `X[i]`). Layer A now writes `Vector<T>`/`Set<T>` everywhere it is
#     normative (the locked ADR-0006 text and old changelog entries keep their
#     historical spelling); `<...>` is generic syntax only in type position
#     (ChatGPT's point). Layer B: the compiler still spells generics `[T]`, a
#     conformance gap; the change is its own PR.
#   - The compiler has its own `CHANGELOG.md` (Added / Changed / Known
#     limitations), separate from this document's versions ("Inox
#     language/compiler release: 0.2.0; INOX_CANONICAL document revision:
#     3.16, 3.17, ...").
#
# v3.35 — 2026-10-10 — ADR-0013: conditional expression (Layer A, decided by
#         Marcelo Fortes in the design chat with ChatGPT: a general form, as in `Result := if X < 0 then -X
#         else if X > Limit then Limit else X`; the five properties are the ones ChatGPT fixed there).
#   - `if C then A else B` is an expression that selects a value. `then` and
#     `else` are mandatory; each branch is an expression, never a block; only
#     the chosen branch is evaluated; the condition is Bool and the branches
#     have compatible types; it chains through `else if`. It may span lines.
#   - `then` becomes a reserved word (49 keywords) and exists only in this
#     expression: the if statement still has no `then`, and `if C then` as a
#     statement is rejected with a message pointing to the expression form.
#   - Layer B: parsed and checked; LLVM lowering pending (branch + phi, lazy
#     branches), so the probe `conditional-expression` is an honest GAP.
#   - Tests: semantic-valid `conditional-expression`; diagnostics
#     `conditional-missing-else`, `conditional-branch-types`,
#     `conditional-condition-bool`, `if-statement-then`,
#     `conditional-as-statement`.
#
# v3.34 — 2026-10-10 — `0xFF` next to `$FF` (Layer A, decided by Marcelo Fortes
#         on 2026-10-10: `$FF` and `0xFF` both, easier for those coming from C:
#         one of Inox's few pragmatic conveniences that break the one-way rule, like `i += 1` next to `i := i + 1`).
#   - CANON-2: a hexadecimal literal is `$` or `0x`/`0X` followed by hex digits,
#     with the digit separator `_` between digits. `0xFF` and `$FF` are the
#     same literal; later phases see `$FF`. A leading zero never means octal:
#     `017` is seventeen (the C trap is not imported).
#   - Supersedes the v3.31 rejection of `0x` (which existed because `0x`
#     reached clang as invalid IR; that path is now a normalized literal).
#   - Tests: runtime `hex-literals` (replaces the diagnostic `hex-0x-rejected`);
#     diagnostic `hex-0x-missing-digit`. Probe `hex-0x`.
#
# v3.33 — 2026-10-10 — ADR-0012: compound assignment (Layer A, decided by
#         Marcelo Fortes in the ChatGPT design chat: `+= -= *= /= ^=` are supported; `i := i + 1` stays
#         valid next to them, and the examples are updated; the semantics are the
#         ones ChatGPT proposed there).
#   - `L += R`, `L -= R`, `L *= R`, `L /= R`, `L ^= R` mean `L := L op R` with L
#     evaluated once. `/=` is valid only where `/` is (never for Integer:
#     `div`/`mod` stay the integer division); `^=` is power (XOR is `xor`).
#     `div=`/`mod=` are not part of this decision.
#   - Statement level only, like `:=` (ADR-0009): no chain (`A += B += 1`),
#     no compound operator inside an expression. The target is an existing
#     variable or a field path (`P.X`, `.X` in `with`); an index target waits
#     for arrays, whose lowering must evaluate the element once.
#   - With OPEN-4: updating a variable is not reading it, so `Total += I`
#     alone does not make `Total` read.
#   - The parser stores `L := L op R` (exact for side-effect-free targets);
#     lowering is unchanged. 13 examples now use the compound forms;
#     `llvm-local-assignment.inox` keeps the long form.
#   - Tests: runtime `compound-assignment`; diagnostics
#     `compound-integer-divide`, `compound-new-variable`, `compound-chained`,
#     `compound-index-target`, `compound-update-only-unread`. Probe
#     `compound-assignment`.
#
# v3.32 — 2026-10-10 — OPEN-4 closed: unread locals are errors (Layer A,
#         decided by Marcelo Fortes on 2026-10-10: as in Go, a variable created and never used is an
#         error, which settles every case, without a `_` placeholder syntax).
#   - CANON-5 rule 7 is restated: a misspelled name in FORM 1 declares a new
#     variable (rule 1), and since nothing reads it the compiler rejects it:
#     "local variable never read: Coutner". Rules 1 and 7 no longer conflict.
#   - A local declared in a routine (any form: `X := 1`, `X T := 1`, `P TStruct`,
#     grouped) must be read before its block ends; assigning it is not reading
#     it, and writing a field (`P.X := 10`) is not reading P. Parameters, `for`
#     iterators, exception bindings, `State` and `Const` are not covered (the
#     exception-binding exemption is pending a maintainer decision: an
#     exception value has no readable member yet, so a tracked binding could
#     never be used). Every unread local of a block is listed in declaration
#     order.
#   - Six examples and tests declared locals they never read; they now print
#     them. The `float32-conversion` probe reads its variable.
#   - Tests: diagnostics `unread-typo`, `unread-assigned-only`,
#     `unread-nested-scope`, `unread-several`, `unread-field-write-only`;
#     semantic-valid
#     `unread-exemptions`.
#
# v3.31 — 2026-10-10 — numeric literals follow CANON-2 (Layer B; no language
#         change). Both gaps were found by ChatGPT's review of the grammar.
#   - The digit separator `_` (canonical since v3.3) is lexed in integer, real
#     and `$` hexadecimal literals, only between two digits; `1__0`, `1_` and
#     `1_.5` are errors. Later phases see the value without separators.
#   - `0xFF` was accepted by the lexer and the analyzer and reached clang as
#     invalid IR (`store i64 0xFF`), a backend BUG by CANON E15. The canon writes
#     hexadecimal as `$FF`, so `0x` is now a lexical error with that hint. If
#     the maintainer wants `0x` as an alternative spelling, that is a Layer A
#     decision and the lexer change is one line.
#   - Tests: runtime `digit-separators`; diagnostics `hex-0x-rejected`,
#     `digit-separator-doubled`, `digit-separator-trailing`. Probe
#     `digit-separators`.
#
# v3.30 — 2026-10-10 — grammar mirror rebuilt and checked (Layer B; no language
#         change). Requested by Marcelo Fortes: grammar.ebnf is wrong and out of date.
#   - `grammar/grammar.ebnf` is rewritten from this document and covers v3.24 to
#     v3.29: CANON-20 levels 1–16 with `..`/`in` non-associative, `:=` at
#     statement level only (chains, named arguments), grouped declarations,
#     `for ... step`, the ADR-0011 `case`, the line-break rule for `(`, `[` and
#     `.`. `Main` names a module or a routine through `identifier_like`, and
#     `for_range` makes `..` mandatory. Its "Conformance gaps" list holds the
#     open items: array and generic types in declarations; sections kept as
#     token lists; OPEN-4 and OPEN-5; the digit separator `_` not lexed; `0x`
#     hex accepted (and lowered to invalid IR). The last four came from
#     ChatGPT's review.
#   - `tools/grammar_consistency.py` checks: every production defined once and
#     reachable; word terminals against the lexer's keywords (both directions,
#     and the kKeywords size); no forbidden terminal (`end`, `break`,
#     `finally`, `then`, `of`, `when`, `=>`, `var`) and no empty parentheses;
#     the grammar's level annotations against the CANON-20 table of this
#     document; a parser-function→production map, including the call order of
#     the expression functions. `run-tests.sh` and `run-tests.ps1` run it as one
#     check ("[SKIP]" when Python 3 is missing).
#
# v3.29 — 2026-10-09 — ADR-0011: `case` in the Ada/SPARK style, adapted to Inox
#         (Layer A, decided by Marcelo Fortes on 2026-10-09: a syntax like Ada 2005 and SPARK, adapted to Inox's
#         rules and syntax; `otherwise` is required whenever the arms do not
#         cover every possible value).
#   - Choices: a static value or a static range `A..B`; `|` separates the
#     alternatives of an arm (a new token, punctuation only; bitwise or stays
#     `bitor`). The comma is a migration error. Overlap anywhere in the case,
#     an empty range (`9..3`), a non-static choice, a choice of another type
#     and a non-discrete selector are compile errors. `otherwise` is at most one
#     and is the last arm. No fall-through.
#   - Coverage: without `otherwise` the choices must cover every value of the
#     selector type, or the case is rejected with the first missing value. The
#     rule generalizes the former enum-only exhaustiveness. UInt64 and Natural
#     (UInt64 with floor 0) cannot be proven with Int64 constants and always
#     need `otherwise`. A case accepted without `otherwise` is total, so the
#     return-path analysis no longer requires an `otherwise` arm.
#   - Static choices: literals, Consts (Integer, Bool and Char, including
#     Consts whose value is a constant expression, `Const K := 2 + 3`) and
#     constant expressions (`not False`, `(1 > 2)`, `2 + 3 * 10`). A choice has
#     the selector's type: a Const keeps its inferred type (`Const K := 5` is
#     Integer, not a UInt8 choice); a literal-only expression is read in the
#     selector's type and range-checked.
#   - Const values: the analyzer folds a Const whose initializer is a constant
#     expression and records its value and inferred type; the backend lowers
#     it (`Const K := 5 + 1` prints 6; runtime test `const-expression`).
#     Sections are analyzed before routines, so a Const may follow the routine
#     that uses it.
#   - OPEN-5 recorded: the parser accepts a typed Const (`Const Mask Integer
#     := $FF`, used by `tests/runtime/const-values.inox`), which CANON-5 does
#     not define (`Const Name := Expr`). Layer B over-acceptance until the
#     maintainer decides.
#   - Layer B status: `Byte` is canonical (CANON-8: Byte -> UInt8) but not
#     registered yet, so a `case` on Byte waits for the Byte implementation.
#   - The selector and each choice end at their line: an arm that starts with
#     `-` or `(` is not read as `X - 1` or `X(1)`.
#   - CANON-4 consistency (Layer B): a `(` or `[` that begins a new line no
#     longer continues the previous line as a call or index, the rule the
#     parser already applied to a leading `.`.
#   - The syntax and rules were agreed between Claude and ChatGPT; the minimum
#     test list is ChatGPT's. Pending with lowering (Phase 2): the selector is
#     evaluated once at run time. Pending with Enum: exhaustive and incomplete
#     enum cases. The AST records `hasOtherwise` so an empty `otherwise` arm
#     still counts.
#   - Tests: semantic-valid `case-ada-choices`,
#     `case-exhaustive-without-otherwise`, `case-char-choices`,
#     `case-arm-on-next-line`; diagnostics `case-overlap-range`,
#     `case-duplicate-across-arms`, `case-inverted-range`,
#     `case-integer-incomplete`, `case-bool-incomplete`, `case-comma-separator`,
#     `case-nonstatic-choice`, `case-string-selector`, `case-otherwise-not-last`,
#     `case-two-otherwise`, `case-uint64-needs-otherwise`,
#     `case-choice-type-mismatch`, `call-paren-on-next-line`,
#     `case-natural-needs-otherwise`, `case-typed-const-mismatch`,
#     `case-literal-out-of-range`; semantic-valid `case-exhaustive-returns`,
#     `case-static-expressions`, `case-const-expression-choice`; runtime
#     `const-expression` (replaces the diagnostic
#     `backend-gap-const-expression`). The review fixes (Natural, return paths,
#     static Bool/Char expressions, typed Consts, Byte status) came from
#     ChatGPT. Probe
#     `case-ada-choices` (GAP until lowering).
#
# v3.28 — 2026-10-09 — ADR-0010: `for I in A..B step S` (Layer A, decided by
#         Marcelo Fortes on 2026-10-09: `for` takes `step`, as in Kotlin).
#   - `step` is a reserved word (the lexer has 48 keywords). The bounds of the
#     header are full range-level expressions (CANON-20 level 10):
#     `for I in 1..N + 1 step 2` runs from 1 to N + 1.
#   - The old form `for I in A..B (S)` / `A..B(S)` is removed. The parser
#     recognizes it after a literal or `)` and answers with a migration message
#     ("the for-loop step is written 'step S' (ADR-0010)"). After an
#     identifier the group is a call, with or without a space (`1..Twice (N)`
#     = `1..Twice(N)`; whitespace has no meaning); when the identifier is a
#     value, the analyzer gives the same hint.
#   - The step must be an Integer expression ("for-loop step must be an
#     Integer expression"); before, `step 1.5` passed semantic analysis
#     although the lowering uses Int64 (found by ChatGPT's review).
#   - The header must be a range `A..B`, with or without `step`: `for I in N`
#     reached the backend before ("a for loop iterates over a range 'A..B'").
#   - Layer B: calling a value (`N(2)` with N a variable, iterator, constant or
#     State name) used to pass semantic analysis and fail in the backend; it is
#     now a semantic error ("'N' is a value, not a function").
#   - Before, the upper bound was parsed as a primary expression only, so
#     `for I in 1..N + 1` was a parse error; the restriction existed only to
#     tell the step group apart from a call.
#   - Fixed in the canon text: `for I in Start()..Finish()` used empty
#     parentheses, which CANON-7 forbids; it now reads `Start..Finish`.
#   - Migrated to `step`: 9 tests, `examples/control-flow.inox`,
#     `examples/llvm-for-range-step.inox`, the manual and AGENTS.md. The text of
#     the locked ADR-0006 keeps its historical `(S)` wording.
#   - New tests: runtime `for-step-keyword` (expression bounds with a step);
#     runtime `for-call-bound-spaced`; diagnostics `for-old-step-spaced`,
#     `for-old-step-glued-literal`, `for-old-step-glued-value`,
#     `for-step-without-range`, `for-without-range`, `for-step-noninteger`,
#     `for-bound-noninteger`,
#     `call-a-value`;
#     parser-valid `for-parenthesized-bound` (a bound in parentheses is not a
#     step). Probe `for-step-expression-bounds`.
#
# v3.27 — 2026-10-09 — ADR-0009: chained assignment; `:=` is a statement (Layer
#         A, decided by Marcelo Fortes on 2026-10-09: Inox supports `A := B := C := D := E := 4`).
#   - CANON-5 gains "Assignment is a statement; chained assignment".
#     `A := B := C := X` evaluates X once and stores it in C, then B, then A.
#     Each target follows rule 1 (first appearance declares, FORM 1). Targets
#     are variables or field paths (`P.X`, `.X` inside `with`). The operational
#     semantics (one evaluation, right-to-left stores, statement level only)
#     came from ChatGPT's review.
#   - `:=` never appears inside an expression, a condition or an initializer:
#     "':=' is a statement, not an expression (CANON-5)". CANON-20 level 16
#     already said "statement level"; the note there now says that its right
#     associativity exists only inside the assignment statement.
#   - Layer B: the parser used to accept `:=` anywhere an expression was
#     parsed, so `X := 2 + (A := 3)`, `if (A := 3) = 3` and `PutLn(A := 3)`
#     passed semantic analysis, and a chain was a nested expression the
#     backend did not lower. The chain is now desugared in the parser into one
#     statement per target; the emitter is unchanged.
#   - Layer B, CANON-9: an argument `Field := Value` was analyzed as an
#     assignment, so `TPoint(X := 1)` declared a local variable X. Named
#     arguments are now accepted only by struct construction, where the left
#     side is checked as a field: unknown field, duplicate field, positional
#     arguments, a type mismatch, and an omitted scalar field without default
#     are compile errors, as CANON-9 already required. Any other call rejects a
#     named argument.
#   - Const and State initializers are now parsed as expressions (stored in
#     the section next to its token list) and analyzed like any other
#     expression: `Const A := B := 3` and `A Integer := (B := 3)` are the
#     ADR-0009 error, `TPoint(Z := 1)` in State is the CANON-9 unknown-field
#     error, and the value must match a declared type. `Origin TPoint :=
#     TPoint(X := 1, Y := 2)` stays valid. The token scanners skip an
#     initializer as one unit, so the field names inside it are no longer read
#     as phantom State or Const declarations. Found by ChatGPT's reviews.
#   - Bug fixed on the way: `Const K := 5 + 1` recorded only the first token,
#     so K was silently 5. A Const value is now recorded only for a
#     single-token initializer; other forms stay unresolved and the backend
#     reports the gap (`backend-gap-const-expression`).
#   - `parseStatement`/`appendStatement` are private: a chain yields several
#     statements, so only whole statement lists are public parser API.
#   - OPEN-4 recorded: CANON-5 rules 1 and 7 contradict each other for FORM 1;
#     the maintainer decides. Neither rule is edited here.
#   - Tests: runtime `chained-assignment`; semantic-valid `named-construction`;
#     diagnostics `assignment-in-expression`, `assignment-in-condition`,
#     `assignment-in-initializer`, `assignment-in-const-initializer`,
#     `assignment-in-state-initializer`, `chained-assignment-call-target`,
#     `chained-assignment-index-target`, `named-argument-outside-construction`,
#     `conversion-named-argument`, `construction-unknown-field`,
#     `construction-duplicate-field`, `construction-positional`,
#     `construction-omitted-scalar`, `construction-type-mismatch`,
#     `backend-gap-const-expression`,
#     `assignment-in-state-initializer-parenthesized`,
#     `state-construction-unknown-field`, `state-initializer-type-mismatch`;
#     semantic-valid `state-named-construction-initializer`. Probes
#     `chained-assignment` (OK) and `named-struct-construction` (GAP: struct
#     construction is not lowered yet).
#
# v3.26 — 2026-10-09 — grouped declarations (Layer A, decided by Marcelo Fortes
#         on 2026-10-09: two variables on one line, `A, B Integer := 2`,
#         are legal).
#   - CANON-5 gains "Grouped declarations": `A, B, C T := X` declares every name
#     with type T. X is evaluated EXACTLY ONCE; the first name receives the
#     value and each later name is initialized from the first, in order, with
#     the type's copy semantics. `P, Q TPoint` (struct, no `:=`) gives each name
#     the type defaults. A scalar group still requires an initializer
#     (rule 3 names the first name).
#   - Layer B bug fixed: the parser re-parsed the initializer once per name, so
#     `A, B Integer := Next(1)` called `Next` twice and the names could hold
#     different values. Measured on 80fd713 before the fix.
#   - Open for the future Vector ADR: move-only types (`Vector[T]`) cannot be
#     initialized from the first name without moving it; the grouped form with
#     an initializer is to be decided there. ChatGPT raised the copy/move point.
#   - Tests: runtime `grouped-declaration-once` (the initializer prints once;
#     the names are independent afterwards); diagnostic
#     `grouped-scalar-without-initializer`. Probes: `grouped-decl-scalar` (OK),
#     `grouped-decl-struct` (GAP, the older struct-initializer gap).
#
# v3.25 — 2026-10-09 — a bare `:` block inside a routine is illegal (Layer A
#         clarification of CANON-4, decided by Marcelo Fortes on 2026-10-09:
#         the block `Main : / : / PutLn(1) / ; / ;` is illegal;
#         record it in the canon and fix the compiler).
#   - CANON-4 already gave `:` one meaning (it declares a function or
#     subroutine and opens its body). It now states the consequence: a `:` that
#     opens a block anywhere inside a routine body, at any depth, is illegal.
#   - Layer B: the parser used to accept `:` ... `;` as an anonymous nested
#     block. It now rejects it: "a bare ':' block is illegal (CANON-4)". No test,
#     example or stdlib file used the form.
#   - Tests: diagnostics `colon-block-in-routine` (directly in `Main`) and
#     `colon-block-in-loop` (inside a `while` body).
#
# v3.24 — 2026-10-09 — ADR-0008: `..` and `in` in the precedence table (Layer A
#         change approved by Marcelo Fortes on 2026-10-09: the precedence chosen for `..` and `in` is approved).
#   - CANON-20 gains two levels between `bitor` and the relational operators:
#     `..` (range construction, level 10) and `in` (membership, level 11). The
#     levels below are renumbered as integers: relational 12, `and` 13, `xor`
#     14, `or` 15, `:=` 16. The relative order of every pre-existing level is
#     unchanged.
#   - `..` and `in` are NON-associative: `A..B..C` and `X in A in B` are parse
#     errors ("'..' is non-associative (CANON-20)" / "'in' is non-associative
#     (CANON-20)"). Before, both failed with a generic "expected line break"
#     message.
#   - The parser already used this order (parseRelational > parseMembership >
#     parseRange > parseBitOr); this entry makes it law and adds the
#     non-associativity diagnostics. The `for` header keeps its own iterable
#     rule until the `step` ADR replaces the `(S)` form.
#   - Tests: diagnostics `range-non-associative`, `in-non-associative`;
#     semantic-valid `precedence-in-range-additive` (`X in 1..N + 1` is
#     `X in (1..(N + 1))`) and `precedence-in-relational` (`X in 1..9 = Flag` is
#     `(X in 1..9) = Flag`). Both valid tests are shape-discriminating: the
#     wrong grouping fails semantic analysis. The proposal and the levels were
#     agreed between Claude and ChatGPT before Marcelo approved them.
#
# v3.23 — 2026-10-09 — CANON-5 enforcement (Layer B: the implementation now
#         follows existing Layer A law; no language change). Requested by Marcelo
#         Fortes on 2026-10-09: a scalar variable without an initial value is
#         forbidden in Inox; the parser no longer accepts [Var].
#   - Semantic analysis rejects a scalar declaration without an initializer
#     (CANON-5 rule 3), for locals and for `State` declarations ("State scalars
#     still require initializers"): "scalar declaration requires initializer:
#     Name". Structs may still omit `:=` (type-default initialization, rule 4).
#     Closes B-GAPS #3.
#   - The parser rejects `Var` blocks, `var`/`mut var` declarations and a
#     module-level `Var` section with a migration diagnostic naming the inline
#     forms. `Var` and `mut` stay reserved. The dead parser paths
#     (parseVarStatement, parseVarBlockDeclarations) and `SectionKind::Var` are
#     removed. Closes the user-visible part of B-GAPS #1 and the matching
#     B-CONFLICTS entry; the AST node `VarBlockStatement` keeps its old name as
#     recorded implementation debt (it now groups inline declarations; renaming
#     it touches the emitter, reserved for the MSVC EH bridge).
#   - Correction to v3.21: its test `loop-body-locals` and the `float-uninit` /
#     `float32-uninit` probes relied on uninitialized scalars, which CANON-5
#     forbids; the v3.21 "zero initializer" fix made a forbidden program run
#     instead of rejecting it. The test now initializes its scalars, the two
#     probes are removed (such programs are rejected by semantic analysis), and
#     new diagnostics cover the rule. Float32 keeps coverage through the valid
#     probe `float32-conversion` (`F Float32 := Float32(0.0)`), which is an honest
#     backend GAP: accepted by semantic analysis, not lowered yet.
#   - Tests: 34 test files written with `Var` were rewritten with inline
#     declarations (invalid tests still fail for their documented reasons; the two
#     tests about `Var` itself, `var-colon` and `invalid-031`, now exercise the
#     removal). New diagnostics: `scalar-without-initializer`,
#     `float-without-initializer`, `state-scalar-without-initializer`,
#     `state-legacy-scalar-without-initializer` (the tolerated legacy
#     `Name : Type` form in State does not bypass the rule), `var-block-removed`,
#     `mut-var-removed`, `module-var-removed`; semantic-valid
#     `state-struct-default` (a struct in State may omit `:=`). AGENTS.md no
#     longer describes `Var` blocks. The State rule, the
#     `SectionKind::Var` removal and the Float32 probe came from ChatGPT's review.
#   - Note on the v3.22 entry: the "invalid-IR bug fixed in v3.21" it mentions
#     was a program CANON-5 already forbade (a scalar without an initializer); it
#     is now rejected by semantic analysis instead of compiled.
#
# v3.22 — 2026-10-09 — toolchain diagnostics in the driver (Layer B only; no
#         language change).
#   - `--build`/`--run`: when clang (or the linker it drives) fails, the error now
#     says which tool failed, its exit code and up to 40 lines of what it printed,
#     and points to the full log `<output dir>/<name>.toolchain.log`. Before, the
#     output was discarded and the user saw only "clang failed while building",
#     which hid the invalid-IR bug fixed in v3.21. The log is removed after a
#     successful build.
#   - `support::runProcessCapturingOutput` runs a process with stdout and stderr
#     redirected to a file (POSIX `posix_spawn` file actions; Windows inherited
#     file handle); `runProcess` shares the implementation.
#   - Test (both runners): `run-hello --build (toolchain diagnostics)` provokes a
#     link failure portably (a directory occupies the executable path) and checks
#     that the linker's own message is shown. Linux 322/322 (sh and ps1).
#
# v3.21 — 2026-10-09 — compositional statement lowering (Layer B only; no
#         language change). Prioritized by Marcelo Fortes on 2026-10-09;
#         reviewed with ChatGPT before implementation.
#   - The LLVM emitter lowers loop bodies and `if`/`elif`/`else` branches with the
#     same statement dispatcher as any other block. The container no longer
#     decides which statements it accepts: `emitLoopStatement`/`emitLoopIf` and
#     the separate repeat-body dispatcher were removed. Closed backend gaps:
#     `nested-for`, `while-in-for`, `for-in-while`, `repeat-in-for`,
#     `loop-if-elif`, `loop-if-else`, `loop-local-var`.
#   - `if` with any number of `elif` and an optional `else` lowers in every
#     position as one chain of conditional branches joined at a single block.
#   - `until` (SECTION 17 - LOOPS) lowers as a transfer to an explicit target, its nearest
#     repeat, not to the innermost loop: it is lowered wherever it appears in the
#     repeat body, including inside `if`, `try` and loops nested in the repeat,
#     and it runs every `ensure` between it and that repeat, innermost first, and
#     no other (requested in review by ChatGPT).
#   - Bug fix: a `Float`, `Float64` or `Float32` local declared without an
#     initializer emitted `store double 0` / `store float 0`, invalid LLVM IR that
#     surfaced only as "clang failed while building". It now starts at `0.0`
#     (`Float32` found in review by ChatGPT).
#   - Bug fix (found in review by ChatGPT, present since the transfers were
#     lowered): a `leave`, `continue`, `until`, `Return` or `Exit` that left an
#     exception handler (On, Else or plain except) never released the exception
#     state the handler had caught; every such transfer leaked it. The emitter now
#     tracks handler regions and releases, innermost first, the state of every
#     handler the transfer leaves, before any crossed `ensure` runs; the slot is
#     nulled so a later capture cannot release it twice. New tool
#     `tools/eh_state_balance.py` links a program with a counting wrapper
#     (`tools/eh_state_counter.cpp`, GNU ld `--wrap`, Linux) and requires captures
#     = releases; fixture `tests/eh-lifetime/handler-transfers` (8 of 9 states
#     leaked before the fix, 0 after).
#   - `tools/backend_gaps.py` now also compiles the emitted IR with clang when
#     clang is on PATH and reports a rejection as BUG (it found the bug above).
#     New probes: `until-in-if`, `until-across-loop`, `float-uninit`,
#     `float32-uninit`. Measurement: 0 BUG, 4 GAP, 20 OK (was 11 GAP, 9 OK). The
#     GAP count refers to the probes, not to every backend limitation. Backend diagnostics
#     for `case` and `unless` now name the construct.
#   - Tests: the fixture `tests/diagnostics/backend-gap-nested-for` became the
#     runtime test `nested-for-in-for`; new runtime tests for every closed gap and
#     for leave/continue/Return/Exit in if/elif/else at two loop levels
#     (`nested-loop-transfers`), `until` positions (`until-positions`) and loop-body
#     locals (`loop-body-locals`); new exception tests (`nested-loop-ensure-
#     transfers`, `nested-loop-retry-raise`, `until-crossing-ensure`); the
#     backend-gap diagnostic fixture is now `backend-gap-case`. Two LLVM fragment
#     checks no longer require the internal `repeatcontinue` label (behavior is
#     covered by execution tests). Expected outputs were written from the
#     language rules, not captured from the compiler.
#   - Validation (E19 principle): Linux clang Debug 321/321; `run-tests.ps1`
#     321/321 under PowerShell 7 on Linux; GCC Debug+ASan+UBSan 321/321, also
#     with a 1 MiB stack; `tools/backend_gaps.py` 0 BUG (IR checked by clang);
#     `tools/mutation_fuzz.py` on the sanitizer build, 4 000 mutants (seeds 41,
#     42): no CRASH, TIMEOUT or CGERR. Windows is expected to keep only the known
#     EH-v3.16a failures plus the three new exception tests (they use `try`) until
#     the MSVC exception bridge lands; they are listed in
#     `ci/windows-known-failures.txt`.
#   - CI: `.github/workflows/ci.yml` builds and tests every pull request on Linux
#     (full suite, backend gaps, exception state lifetime) and on Windows with the
#     MSVC ABI (windows-2022, clang 19.1.5). On Windows the set of failing tests
#     must EQUAL `ci/windows-known-failures.txt` (`tools/ci_known_failures.py`):
#     an unlisted failure or a listed test that passes fails the job. The list
#     describes that runner only; on it `fault-not-catchable` and
#     `nested-loop-ensure-transfers` pass, while on the maintainer's machine
#     (clang 22.1.6) the 13 EH-v3.16a failures were measured. Toolchain variation
#     is recorded here, not hidden in the list.
#
# v3.20 — 2026-10-09 — keyword rename approved by Marcelo Fortes on 2026-10-09
#         (lexical change only; no semantic change). Recorded as ADR-0007.
#   - `leave` replaces `break`: it exits the nearest enclosing loop (CANON-12/17).
#     `continue` is unchanged.
#   - `ensure` replaces `finally` as the try cleanup clause (CANON-15). Its
#     semantics are unchanged: it runs exactly once on every departure from the
#     protected construct. `ensure` is NOT a design-by-contract keyword; future
#     contracts keep `Pre`/`Pos`/`Invariant` (CANON-15 rule 21, FUTURE-2).
#   - `break` and `finally` are no longer reserved words. They are ordinary
#     identifiers; no migration diagnostic is kept for them. A bare `break` line
#     is therefore rejected as a non-statement (CANON-4) and a `finally` line in
#     the old position leaves the `try` without `except`/`ensure`.
#   - Implementation: lexer keyword table, parser, AST (`LeaveStatement`,
#     `hasEnsure`/`ensureBody`), semantic analyzer, AST dumper, LLVM emitter
#     (identifiers, labels `eh.ensure*`/`eh.leave*` and diagnostics), grammar,
#     examples, tests, test scripts, `tools/mutation_fuzz.py`,
#     `docs/BACKEND_GAPS.md` and the HTML manual. Test and example files, and their
#     module names, that spelled the old keywords were renamed accordingly.
#   - New tests: lexer (`Leave`/`Ensure` are keywords; `Break`/`Finally` are
#     identifiers), semantic (`former-keywords-as-identifiers`), diagnostics
#     (`leave-outside-loop`, `break-is-not-a-statement`, `finally-is-not-a-clause`).
#   - Historical change-log entries and locked ADR text keep the old spelling
#     (append-only); ADR-0004 carries a supersession note.
#   - Layer B correction found during this pass: the lexer has 47 keywords (the
#     status sections said 46). The count is unchanged by the rename.
#   - Validation: Linux full suite 309/309 (305 previous + 4 new). Windows is
#     expected to keep only the known EH-v3.16a failures until the MSVC exception
#     bridge lands.
#
# v3.19 — 2026-10-09 — canonical consistency pass over v3.18; no language/compiler behavior change
#   - Reconciled all ACTIVE references to the former “checking mode” wording with
#     DECISION P-A: runtime arithmetic faults are deterministic Inox traps in every
#     conforming build. Historical changelog text remains append-only. Locked ADR
#     wording remains unchanged and is annotated by a supersession note instead.
#   - Closed conformance gap #20 after direct runtime verification that `Sqr`,
#     `Cube`, and `Lcm` inherit v3.18 checked arithmetic and trap on Int64 overflow;
#     no Std.Math implementation change was required.
#   - Clarified exit status 70 as the Inox-defined `kRuntimeFaultExitStatus`; its
#     numeric value intentionally coincides with BSD `EX_SOFTWARE` where that
#     convention exists, but Inox does not depend on `sysexits`.
#   - Clarified portability status: Windows and Linux remain the primary validation
#     targets; the v3.18 hardening pass executed the full suite on Linux and only
#     cross-built/linked Windows artifacts in that environment. EH-v3.16a remains
#     the explicit Windows exception-lowering gap.
#
# v3.18 — 2026-10-09 — review + hardening pass; decisions P-A, P-B, P-C and the
#         verification principle approved by Marcelo Fortes on 2026-10-09
#   - DECISION P-A: a run-time arithmetic fault is a deterministic Inox trap with
#     the diagnostic "Inox runtime error: <category>" on stderr and exit status
#     70; no unwinding, not catchable by try/except, finally does not run
#     (CANON-19). Previously the program died with `llvm.trap` and no message.
#   - DECISION P-B: `for` start, end and step are evaluated exactly once, before
#     the first iteration, in textual order (CANON-12).
#   - DECISION P-C: later phases consume semantically resolved information
#     (E11). Stage 1 done: `LlvmIrEmitter::emit(module, semanticResult)`; module
#     `Const` values are resolved once by semantic analysis and read from there.
#   - Verification principle (E19): a green suite alone never proves the absence
#     of regressions; `tools/mutation_fuzz.py` added for that purpose.
#   - CANON-12 "Exit is FORBIDDEN in functions" is now enforced by semantic
#     analysis (only the backend rejected it; found by mutation fuzzing).
#   - CANON-8 is now enforced for a constant divisor with a non-constant dividend
#     (`A div 0`, `A mod 0` were accepted and trapped at run time); likewise a
#     constant shift count outside 0..63 and a constant negative exponent.
#   - Checked integer arithmetic (backlog item 13) is implemented. See CANON-19
#     "Checked integer arithmetic (v3.18)". `+ - *`, unary `-`, `Abs`, `div`,
#     `mod`, `shl`, `shr`, `^` and the `for` step trap instead of wrapping or
#     being undefined behavior; constant overflow is a compile-time error.
#   - Integer literals must fit Int64. `$FFFFFFFFFFFFFFFF` is no longer silently
#     reinterpreted; it is a compile-time error.
#   - `for` loops: a constant step <= 0 is a compile error, a runtime step <= 0
#     traps, and a range whose end is Int64.Max terminates (no wraparound).
#   - `Get`/`GetLn` Integer input is strict: EOF before a token, a malformed token,
#     trailing non-whitespace characters, or a value outside Int64 trap. Previously
#     overflow wrapped and EOF/invalid input silently produced 0.
#   - CANON-4 is now enforced: a simple statement must end at a line break (or at
#     `;`/end of input) and `;` must be followed by a line break. `A := 1 B := 2`,
#     `PutLn(1) PutLn(2)` and `X := 1 ; Y := 2` are parse errors. A bare name or
#     literal is not a statement.
#   - Documented implementation limits (expression depth, statement nesting) turn
#     stack exhaustion into ordinary diagnostics ("maximum expression nesting
#     depth exceeded"). The values are MEASURED: the largest nesting each pass
#     survives on a 1 MiB stack (Windows main thread) in a Debug+ASan build,
#     divided by ~2. A large `Type` section is parsed in linear time (it was
#     quadratic).
#   - `for I in A..B(S)`: A > B now iterates downward as CANON-12 already stated
#     (the backend ran a descending range zero times) and the loop never computes
#     a value past Int64.Min/Max. The backend evaluated A, B and S again on every
#     iteration; it now evaluates them once (DECISION P-B above).
#   - Every runtime fault goes through one IR function, `__inox_arith_fault(kind)`
#     (DECISION P-A above); the policy lives in that one place.
#   - A construct the semantic analyzer accepts but the backend cannot lower is
#     now reported as "not yet implemented in the LLVM backend" (CANON E15), not
#     as a program error. The gaps are measured by `tools/backend_gaps.py` and
#     listed in `docs/BACKEND_GAPS.md`.
#   - Codegen: allocas are hoisted to the function entry block (a local declared in
#     a loop no longer grows the stack on every iteration); sibling scopes may
#     reuse a local or `for` iterator name; user identifiers can no longer collide
#     with compiler temporaries/labels (`tmp0`, `then0`, `entry`).
#   - `inox --run` reports "program stopped by an Inox runtime error (exit code
#     70)" after a runtime fault and "program terminated abnormally (exit code N)"
#     for any other non-zero exit; `inox --help` prints usage and exits 0.
#   - Tests: new `tests/runtime` (execution + expected trap) and `tests/diagnostics`
#     (rejection with an expected message) layers; the seven `get-integer-*` tests
#     that no runner executed are now run; the missing `tests/invalid/invalid-028`
#     fixture was added. A `.trap` file now names the expected runtime
#     diagnostic; an optional `.out` beside it pins the complete output. Both
#     runners contain the same 305 checks.
#   - Semantic analysis now enforces CANON-12 "functions must not fall through
#     without returning a value" (it only checked that some Return existed; the
#     backend caught some cases with a misleading message). Found by fuzzing.
#   - Duplicate struct field detection is linear (a 40 000-field struct took 7 s
#     in semantic analysis, now 0.12 s). Found by fuzzing.
#   - Verification for this version (E19 principle): Debug, Release, GCC and
#     Debug+ASan+UBSan builds pass 305/305 checks, ASan+UBSan also with a 1 MiB
#     stack; `run-tests.ps1` passes 305/305 under PowerShell 7 on Linux;
#     `tools/backend_gaps.py`: 0 BUG, 11 GAP; `tools/mutation_fuzz.py` on the
#     ASan+UBSan build, 16 000 mutants (seeds 31, 34, 35, 36) after the last fix:
#     no CRASH, TIMEOUT or CGERR; `tools/calc_differential_test.py` agrees on 300
#     expressions; the compiler cross-builds for Windows with MinGW-w64 + UCRT
#     (warning-free, links as a PE executable) and the IR in its Windows variant
#     links against both msvcrt and UCRT (`_write`, `_exit`, `fflush`). Not
#     verified here: running on Windows (no working Windows runtime in the
#     review environment) and the MSVC preset.
#
# v3.17 — 2026-10-08 — approved by Marcelo Fortes
#   - Typed exception handlers now follow native Inox block style: `On Type` or
#     `On Name Type`, newline opens the handler body, and that handler closes
#     with its own `;`. The former `Do` and `Name: Type` forms are invalid.
#   - Added canonical `Retry(N)`: valid only while an explicit `On`/`Else`
#     handler is active. `N` is the number of ADDITIONAL attempts, so Retry(3)
#     permits at most four executions including the original try body.
#   - Retry restarts the complete associated try body. A `finally` block executes
#     before every new attempt. Exhausting the budget is equivalent to bare
#     rethrow of the currently handled exception.
#   - Standard exception identifiers are genuine nominal TYPES, not enum values.
#     Canonical taxonomy now includes Exception; IOError -> FileNotFound,
#     PermissionDenied, AlreadyExists, DiskFull; ArithmeticError ->
#     DivisionByZero, OverflowError, DomainError; RangeError -> IndexError.
#     Handler matching includes descendants (e.g. On IOError catches
#     FileNotFound). This taxonomy is not classical OO inheritance.
#   - Standard exception types belong to the prelude/standard-library surface;
#     libinoxrt remains a generic native transport mechanism over opaque type ids.
#   - Backend cleanup lowering now routes Return, Exit, and loop break/continue
#     across active finally blocks, including nested finally regions.
#
# v3.16 — 2026-10-07 — approved by Marcelo Fortes
#   - Exception handling promoted from aspirational/parser-only status to CANON-15
#     and implemented across Lexer -> Parser -> AST -> Semantic -> LLVM lowering.
#   - Canonical form: `try` with `except`, `finally`, or both; typed handlers use
#     `On [Name:] ExceptionType Do`; `Else` is the typed-handler catch-all; plain
#     `except` is a catch-all; `Raise X` throws and bare `Raise` rethrows only
#     from an active handler. `finally` is cleanup and runs on normal and
#     exceptional departure. No `:` is used by these control-flow blocks; one
#     final `;` closes the complete try construct.
#   - Initial runtime matching is nominal through stable RuntimeTypeId values for
#     the standard exceptions: Exception, RangeError, IndexError,
#     DivisionByZero, OverflowError, IOError. This is deliberately not a Delphi/
#     C++/Java inheritance tree and does not require general RTTI. User-defined
#     throwable payloads and any future compositional/capability matching remain
#     deferred design work.
#   - Added bootstrap `libinoxrt`: generated Inox calls a small stable Inox ABI;
#     the current Unix-like implementation uses the platform C++/Itanium unwinder
#     underneath. The frontend does not call `__cxa_*` directly. Added parser,
#     semantic, LLVM smoke, execution/rethrow regression tests and executable
#     examples.
#
# v3.15a — 2026-06-21 — approved by Marcelo Fortes
#   - Fixed two bugs in the v3.15 `with` implementation found by running the
#     full suite: (1) parser glued a leading-dot statement on a new line to the
#     previous expression (`with P` then `.FX := 10` then `.FY := 20` parsed the
#     `10` and the next-line `.FY` as `10.FY`), causing "unknown field FY in
#     Int64". Fixed: parsePostfix no longer consumes a `.` that begins a new
#     line, since newlines terminate statements in Inox. `with` now runs
#     end-to-end (example yields 30). (2) Updated run-tests `--emit-llvm`
#     required-fragment lists to the `inox_`-prefixed symbol names introduced in
#     v3.14 (user/stdlib functions are `@inox_<name>`); the fragments still named
#     the old unprefixed symbols and were failing independently of `with`.
#     Full suite green except sandbox-only --run/--build (clang shim), which pass
#     on real clang.
#
# v3.15 — 2026-06-21 — approved by Marcelo Fortes
#   - `with` statement implemented across all compiler layers (Lexer → Parser →
#     AST → Semantic → LLVM codegen). Closes B-GAP #2. Implementation follows
#     CANON-11 exactly: dot-prefix model (Visual Basic style); `.Member` inside
#     body expands to `__member(__with_N, Member)` — reusing the existing member-
#     access path; no new scope symbols introduced; nested `with` correctly binds
#     the innermost target; `;` closes. Codegen aliases `__with_N` to the target
#     variable's slot in `locals_`, so mutations are visible on the original.
#     New files:
#       tests/parser/valid/with-basic.inox  — basic member read/write via 'with'
#       tests/parser/valid/with-scope.inox  — unprefixed names use normal scope
#       tests/parser/valid/with-nested.inox — nested 'with' innermost binding
#       examples/with-statement.inox        — end-to-end example; --emit-llvm test
#                                             added to scripts/run-tests.sh
#
# v3.14 — 2026-06-17 — approved by Marcelo Fortes
#   - Codegen fix: user/stdlib function symbols are now emitted with an `inox_`
#     prefix (e.g. Std.Math `Log` -> `@inox_log`) so they never collide with C
#     library symbols. This eliminates an infinite-recursion hang: `Ln`/`Log`
#     lowered `llvm.log.f64`, which the linker resolved to libm `log`, which had
#     been shadowed by the stdlib's own `@log` — calling itself forever. `@main`
#     is unaffected (still `@main`). Added tools/calc_differential_test.py, a
#     differential calculator test that generates N distinct random expressions,
#     evaluates each in Python and in Inox, and compares (exact for integers,
#     tolerance for floats). 300/300 random expressions agree with Python.
#
# v3.13 — 2026-06-16 — approved by Marcelo Fortes
#   - Operator precedence and associativity fixed as canonical law (CANON-20,
#     SECTION 20) and marked OPEN-3 CLOSED in SECTION 04. Table follows
#     mathematical convention (no Pascal mistake: relationals > logicals; no C
#     mistake: bitwise > relationals; and > xor > or; ^ right-associative).
#     MANDATORY-PARENTHESES rule (Ada/SPARK safety) added: the parser raises a
#     parse error instead of silently guessing when mixing and/or/xor, mixing
#     different bitwise families, or mixing bitwise with shift; parentheses stay
#     optional elsewhere. Parser implements the guard; added examples/
#     operator-precedence.inox and 7 parser tests (3 valid, 4 invalid). This
#     table is IMMUTABLE: changing it requires a human-approved. ADR.
#
# v3.12 — 2026-06-16 — approved by Marcelo Fortes
#   - Added COMPILER MODULE ARCHITECTURE DIRECTIVE (SECTION 31): how the
#     compiler's own C++ source is split into modules. Principle: REACTIVE not
#     PREDICTIVE. Extract a module only when it passes the three Go/UTF-8 tests
#     (cohesion + stability + reuse). File size or aesthetics alone are NOT
#     reasons to split; two responsibilities changing for different reasons ARE.
#     Big reorganizations deferred to 0.2 (runtime/strings/UTF-8/aggregates),
#     when the real seams are visible. AIs must not launch sweeping refactors on
#     their own; propose and let the maintainer decide.
#
# v3.11 — 2026-06-16 — approved by Marcelo Fortes
#   - Std.Math hardening + end-to-end exercise. Fixed Pi/Tau/E literals to
#     Float64-honest precision (~16-17 sig digits; extra digits were decorative).
#     Added examples/math-showcase.inox and tests/integration/modules/
#     math-showcase.* calling integer-exact helpers (Min/Max/Clamp/Sign/Sqr/
#     Cube/Gcd/Lcm) and Float intrinsics (Sqrt/Hypot/Floor/Ceil) from Main, all
#     verified end-to-end. Registered in both run-tests.sh and run-tests.ps1.
#     Recorded conformance gaps #20 (Lcm/Sqr/Cube overflow until checking mode)
#     and #21 (Pi/Tau/E are functions until Float Const lowering exists).
#
# v3.10 — 2026-06-16 — approved implementation update
#   - Expanded Std.Math from a minimal helper module into the first serious
#     mathematical standard-library layer. Std.Math now includes additional
#     Integer helpers implemented in Inox source and a Float elementary
#     function surface lowered through LLVM/libm as a temporary 0.x backend path.
#   - Added initial backend support for Float64 arithmetic, Float64 printing,
#     Float64 comparisons, Float64 local inference, Float64 user functions,
#     elementary math intrinsics/functions, and checked Integer exponentiation
#     through a backend helper.
#   - This does NOT complete the final scientific/numerical library: arrays,
#     vectors, statistics, decimal finance, full IEEE policy, tolerance-based
#     tests, and clean-room Inox kernels remain roadmap items.
#
# v3.8 — 2026-06-16 — approved by Marcelo Fortes
#   - Made modern compiler-engineering quality a FIRST-ORDER constitutional rule
#     in SECTION 00, before topical language and implementation rules.
#   - Canonically adopted an Inox-specific adaptation of Object Calisthenics,
#     SOLID, modern C++20, LLVM/compiler architecture, explicit ownership, phase
#     boundaries, platform isolation, first-class diagnostics, valid-IR-only
#     codegen, regression tests, approved compiler design patterns, and
#     progressive quality gates.
#   - Required incremental application to every new or modified code area and
#     prohibited unbounded repository-wide cosmetic rewrites without tests.
#
# v3.7 — 2026-06-15 — approved by Marcelo Fortes
#   - Added BACKEND STRATEGY DIRECTIVE (SECTION 31): clang remains the external
#     driver through 0.1.x/0.2.x. The external clang driver is a BUILD-TIME
#     dependency, NOT technical debt (same model Zig used for years; Rust still
#     calls the system linker). Embedding libLLVM adds no language capability and
#     is deferred to 0.3/0.4. Recorded the backend maturation sequence (0.1→1.0),
#     the OBJECTIVE criterion for leaving clang (bottleneck / API-only capability
#     / single-binary distribution — none true at 0.1/0.2), and a SCOPE GUARD:
#     work on how clang is invoked (e.g. Process.cpp process spawning, path
#     handling) is in scope and must NOT become an on-ramp to replace clang.
#
# v3.6 — 2026-06-15 — approved implementation update
#   - Introduced a dedicated C++ portability support layer under
#     src/compiler/support for environment variables, filesystem executable path
#     discovery, process execution, host OS detection, executable suffixes, and
#     null-device paths. Platform-specific preprocessor conditionals must remain
#     isolated there and in CMake/scripts, not scattered through parser, semantic,
#     AST, diagnostics, or backend logic.
#   - Added cmake/ modular build policy files, CMakePresets.json for validated
#     Windows Clang/MSVC and Linux Clang flows, and stub toolchains for future
#     BSD, macOS, Illumos/Solaris, AIX, HP-UX, UnixWare, and Android validation.
#   - Replaced temporary scanf-based Integer input lowering with internal LLVM
#     helper functions based on getchar: __inox_read_i64,
#     __inox_discard_token, and __inox_discard_line. This removes the Windows
#     lld-link undefined-symbol failure for scanf while preserving the temporary
#     C runtime ABI.
#   - Repaired LLVM smoke examples so the repository suite is green on Linux.
#
# v3.5 — 2026-06-15 — approved by Marcelo Fortes
#   - Canonical document layout reorganized into SECTION 00 through SECTION 31.
#     Existing CANON/ADR/DEV/FUTURE/B status material is preserved under stable
#     section headings for better navigation by humans, ChatGPT, and Codex.
#
# v3.4 — 2026-06-15 — approved by Marcelo Fortes
#   - Minimal console input adopted for Inox 0.1: `Get`, `GetLn`, `Get(X)`,
#     `GetLn(X, ...)` are canonical. Empty parentheses remain invalid: `Get()`
#     and `GetLn()` are rejected. Initial implementation supports Integer/Int64
#     input through the temporary C runtime ABI. String input remains deferred.
#
# v3.3 — 2026-06-14 — approved by Marcelo Fortes
#   - DIGIT SEPARATOR `_` adopted: `10_000_000` == `10000000`, cosmetic, all
#     numeric literals, only between digits (CANON-2).
#   - DECIMAL LITERALS context-free form decided: type constructor
#     `Currency(19.99)` / `Crypto(0.001)`, coherent with struct construction.
#     Closes OPEN-2 (CANON-8). The 0.1 language now has ZERO open design questions.
#
# v3.2 — 2026-06-14 — approved by Marcelo Fortes
#   - NAMED STRUCT CONSTRUCTION is canonical: `TPoint(FX := 10, FY := 20)`.
#     Named-only (positional invalid); `:=` separator (keeps `:` exclusive to
#     function declaration); order free; omitted field uses default or is an error
#     if a scalar without default; duplicate/unknown field is an error; empty
#     `TType()` invalid (use `C TPoint` for all-defaults). Closes OPEN-1.
#
# v3.1 — 2026-06-14 — approved by Marcelo Fortes
#   - DIVISION BY ZERO: constant divisor zero is a COMPILE-TIME ERROR; dynamic
#     divisor zero is a RUNTIME TRAP (CANON-8).
#   - EXPONENTIATION `^`: integer `^` returns Integer with non-negative exponent
#     only; negative exponent is an error (no implicit float); integer `^`
#     overflow is an error/trap; float `^` (future) returns Float (CANON-8).
#   - DECIMAL LITERALS (in-context): a real literal in a decimal-typed context is
#     read directly as that decimal type, exact, with excess-precision being a
#     compile-time error (Ada model). Context-free decimal literal spelling left
#     OPEN (see Layer B / B-OPEN).
#   - Tracked OPEN-1 (struct value construction) and OPEN-2 (context-free decimal
#     literal) in Layer B / B-OPEN.
#
# v3.0 — 2026-06-14 — approved by Marcelo Fortes
#   - FULL CONSOLIDATION: the entire docs/ tree is absorbed into this one file as
#     named groups (CANONICAL, DECISIONS, DEVELOPMENT, FUTURE, OPEN-QUESTIONS),
#     preserving the actual content of every non-empty file verbatim-in-substance
#     (not paraphrased away). docs/ is to be deleted.
#   - Added: full influences list (incl. Vala), mission-critical failure modes,
#     complete Set operations, Std.Math helper list, three runtime boundaries,
#     5-step stdlib discovery order, full test-mode list, Integer(FloatExpr)
#     truncation, MPL-2.0 license, prebuilt package layouts, toolchain profiles.
#
# v2.2 — 2026-06-14 — approved by Marcelo Fortes
#   - `with` added (VB dot-prefix model). `:` rule made explicit (functions only).
#
# v2.1 — 2026-06-14 — approved by Marcelo Fortes
#   - Two-layer structure (Constitution / Status) + Governance G1–G5.
#
# v2.0 — 2026-06-14 — approved by Marcelo Fortes
#   - `Var` block REMOVED; inline-only declarations; safe Ada/SPARK inference;
#     scalars/enums require initializers, structs use type-default init; type
#     hierarchy finalized; Currency i64x10^6, Crypto i128x10^18, BigCurrency 0.2+;
#     enum init strict Ada.
#
# v1.0 — 2026-06-13 — baseline audit of Inox.zip source tree.
```
