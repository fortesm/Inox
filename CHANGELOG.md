<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Changelog — Inox compiler

This file tracks the versions of Inox, the language and its compiler, which
share one version (constitution VERSIONING). Numbers such as `v3.57` or
`r3.61` are canon revisions, entries of the decision log
(`openspec/specs/constitution/revisions.md`), not versions.

## 0.2.0 pre-alpha — unreleased

### Added
- Design by contract, as in Eiffel (canon r3.67, CANON-24, OpenSpec change `design-by-contract`): `pre` (Eiffel `require`) and `pos` (Eiffel `ensure`, with `old X` and `Result`) as the first lines of a routine, each with an optional `, "message"`; `invariant` lines in a Struct, checked after construction, before and after every qualified method call, after a field write from outside and after a `ref mut` argument, and inherited from the reused parts; `pre else` / `pos then` in an `Override` (weaken / strengthen the inherited contract); `invariant` and `variant` on the first lines of a loop (`variant` in while and repeat). A contract only reads (no `Self mut` method, `ref mut` argument or file in it). A broken contract is a runtime trap (CANON-19, exit status 70), not an exception: "Inox runtime error: precondition violated: TPilha.Empilhar: ...". The checks are always on in every build through 1.0 (canon r3.68; monitoring levels per module, as in Eiffel, come after 1.0); the backend marks them expected to hold (`llvm.expect.i1`) and keeps the failure path in a `cold noreturn` runtime helper. `examples/contracts.inox`.
- Examples of method reuse and a bank teller (canon r3.66): `examples/reuse-shapes.inox` (one behavior reused by three structs), `examples/reuse-registry.inox` (two parts, Override that combines them), `examples/reuse-inventory.inox` (three levels, arrays, methods of Integer and String), and `examples/bank-atm.inox`, a terminal teller with a numbered menu (balance, deposit, withdrawal, transfer, cancellation, statement) that keeps the accounts and the statement in text files between runs.
- Method reuse (canon r3.66, OpenSpec change `method-reuse`): `TAssinatura Struct (TPessoa)` reuses TPessoa; the part is a field named after its type (`A.TPessoa`), and its fields and methods are offered directly at every level (`A.UpdateNome(X)`, `A.Mudar(...)` from a part's part), running on the part. `TName.Method(...) Override :` replaces a reused method with the same signature. A name reached by two paths, a field hiding a reused one, a method with a reused name without `Override`, or an `Override` that replaces nothing are errors at the declaration. No subtyping.
- Methods of built-in types (canon r3.66): `String.UpdateStr(Self mut, V String) :`, `Integer.Twice(Self) Integer :`, called on any variable, field or value of that type (`A.FPlano.UpdateStr("x")`, `21.Twice`). A field of `Self mut` can be passed to a `ref mut` parameter.
- `examples/file_io.inox`: a tour of every text-file operation of Std.Files and Std.IOUtils and every way one fails (FileNotFound, AlreadyExists, IOError for a closed file, a file opened for writing and a directory, Retry, ensure, Else, nested try with re-raise, close on exception). The suite now builds and runs every example and compares its output with `tests/examples/NAME.out` (and feeds `NAME.in` when present); an example without an expected output fails, so every program in `examples/` must run. `examples/types.inox` and `examples/type-checking.inox` no longer use Currency, Crypto or Char values, which are not lowered yet.
- Driver commands (canon r3.63, OpenSpec change `driver-commands`): `inox file.inox` and `inox run` build and run (before, `inox file.inox` only checked), `inox build`, `inox check`, `inox emit-llvm`, `inox help`, `inox version`. `--run`, `--build`, `--emit-llvm`, `--help` and `--version` stay as aliases.
- `Std.Debug.Assert(Condition)` and `Assert(Condition, Message)` (canon r3.63, change `debug-assert`): a failed Assert stops the program with "Inox runtime error: assertion failed" (and the message), exit status 70, like an overflow; it is never compiled out.
- Text files (canon r3.59, OpenSpec change `file-io`): `Use Std.IOUtils` gives the type `File` (Std.Files) and `OpenRead`, `Create`, `OpenAppend`, `ReadLn`, `Eof`, `Write`, `WriteLn`, `Close`, `Exists`, `DeleteFile`, `RenameFile`. A File closes itself when its routine exits by any path, also an exception; it is never copied (a local or a `ref mut` parameter). Failures raise `FileNotFound`, `PermissionDenied`, `AlreadyExists`, `DiskFull` or `IOError`, handled with try/except On/ensure/Retry. UTF-8 text, `\n` written on every system. Runtime `runtime/FileRuntime.cpp`; the leak check covers the file programs.
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
- Module pairs (canon r3.64, OpenSpec change `module-pairs`): `Std.Files` now holds the File type and the basic operations (OpenRead, Create, OpenAppend, ReadLn, Eof, Write, WriteLn, Close); `Std.IOUtils` the conveniences (Exists, DeleteFile, RenameFile) and everything of Std.Files. Programs with `Use Std.IOUtils` are unchanged.
- `State` has no `:` (canon r3.62, OpenSpec change `state-without-colon`): `State`, its variables on the next lines, and `;`. `State :`, a State without its `;` and the old `Name : Type` form (also in `Const`) are rejected with a message showing the right form. `:` now appears only after a routine's name and signature.
- `Length(S)` of a String and `Ord(C)` of a Char are rejected (canon r3.62, change `char-not-ordinal`): `Length` takes an array or a slice, String lengths come with Std.StrUtils, and a Char is not an ordinal.
- The roadmap is renumbered for the version scheme (`docs/ROADMAP.md`, change `roadmap-renumber`).
- The canon becomes the constitution (canon r3.61, OpenSpec change `constitution`): `docs/INOX_CANONICAL.md` is retired; the language law lives, minimized and without duplicates or stale status, in `openspec/specs/constitution/` (`spec.md`, `engineering.md`, `revisions.md`), locked by `constitution.lock` and checked in CI by `tools/constitution_check.py`, which also verifies that every test a spec names exists. Status moves to `docs/STATUS.md` and `docs/ROADMAP.md`; the old change log is frozen in `docs/history/`. `inox --version` prints `(canon r3.61)`.
- Version scheme (canon r3.60): `MAJOR.MINOR.RELEASE.BUILD` plus the stage, today `0.2.0.<BUILD> pre-alpha`. The BUILD is the commit count of the compiled history, written at every build; `inox --version` prints e.g. `inox 0.2.0.206 pre-alpha (canon r3.60)`. CI clones the full history.
- Compiler sources (OpenSpec change `compiler-modularization`, step 4): `SemanticAnalyzer.cpp` (3,358 lines) is split into eight files by responsibility (declarations, built-ins, statements, constant evaluation, expressions, calls, types) and `SemanticSupport` for the shared helpers; types, diagnostics and IR are identical for every program.
- Compiler sources (OpenSpec change `compiler-modularization`, step 3): the exception ABI is a Strategy (`ItaniumExceptionAbi`, `MsvcExceptionAbi`) chosen once per module; the libinoxrt functions are one table (`RuntimeAbi`); the emitter tells the driver whether to link libinoxrt instead of the driver searching the IR text. The emitted IR is identical for every program, with both ABIs.
- Compiler sources (OpenSpec change `compiler-modularization`, step 2): `FunctionEmitter` gets its own header and nine source files by responsibility (control flow, exceptions, statements, I/O, String ownership, aggregates, arrays, expressions); `LlvmIrEmitter.cpp` keeps only the module emission (264 lines). The emitted IR is identical for every program.
- Compiler sources (OpenSpec change `compiler-modularization`, step 1): the support code of the LLVM emitter moves out of `LlvmIrEmitter.cpp` (5,154 → 3,704 lines) into `TypeLayout`, `AstQueries`, `LlvmText`, `Operators`, `MathBuiltins`, `ExceptionSupport` and `RuntimeHelpersIr`; the emitted IR is identical for every program. A stray `LlvmIrEmitter.cpp.diff` is deleted.
- Versioning (canon r3.58): Inox is `0.2.0-pre-alpha`; `inox --version` prints the version and the canon revision. The `v3.xx` numbers are canon revisions, written `r3.NN` from now on.
- `Std.Core` is renamed `Std.System` (canon r3.58). Library modules come in pairs: `Std.X` holds definitions, `Std.XUtils` helper functions.
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
- A String `Const` (`Const Arquivo := "dados.txt"`) passed the analyzer and was not lowered; it is now a value like its literal.
- `Self` (without `mut`) could write its receiver, and the change reached the caller; a field of a value parameter could be assigned; `with` on a parameter could write it; a `Self mut` method could be called on a parameter or a literal; a `ref mut` argument could reach the receiver of the same call. All are rejected now (CANON-1, CANON-10, ADR-0016).
- A routine that takes arguments, called by its bare name (`Show` for `Show(N Integer)`), passed the analyzer and stopped in the backend; it is now "Show expects 1 argument, got 0". A function called as a statement is reported as a backend gap instead of a codegen error. Found by mutation fuzzing (seed 7).
- The release scripts copied a manual that no longer existed and did not ship `libinoxrt`, so packaging failed and packaged compilers could not build programs that use exceptions, Strings or Files; packages now hold the manual, the constitution and the runtime library beside the compiler.
- `=` and `#` on two Bool variables, `<` on Strings and `=`/`<` on structs passed the analyzer and then produced invalid IR. Bool and String comparisons now work; struct comparison is a compile error, as in Pascal (canon v3.56, v3.57).
- A function called with no arguments inside `try` (`N := Fail`) now reaches the handler; it was emitted with `call` instead of `invoke` and its exception aborted the program.

### Known limitations
- `UInt64`/`UInteger` values are not lowered yet.
- Not lowered yet: Float32 conversion.
- Vectors, sets, enums and ranges are not implemented (Range and Enum declarations are parsed and checked for shape only).
- String concatenation, indexing and length functions are reserved (not in 0.1); slices of String arrays are not supported yet.
- Generic types other than `Slice<T>` (`Vector<T>`, `Set<T>`) are not parsed yet.

## Compiler milestone — 2026-10-09 (labeled "V0.4" at the time; never released or tagged)
- Compositional lowering of nested statements (v3.21), toolchain diagnostics, CI on Linux and Windows (clang, MSVC ABI).
