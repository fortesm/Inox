# AGENTS.md

This file is the operational contract for AI agents working on Inox.

## Canonical truth hierarchy

Use this hierarchy when modifying Inox:

1. `docs/INOX_CANONICAL.md` (Layer A and its ADRs) for every language decision.
   It replaced the former `docs/decisions/` and `docs/canonical/` files.
2. `openspec/specs/` as the requirement-and-scenario mirror of the canon, one
   capability per folder, each scenario tied to a test. A capability becomes
   law in OpenSpec only when Marcelo Fortes approves its migration (canon G7);
   until then the canon wins.
3. `grammar/grammar.ebnf` as the grammar mirror.
4. `docs/site/index.html` as the human HTML manual.
5. Examples and tests as executable evidence of implemented subsets.

If these disagree, do not invent a third behavior. Fix the documentation/code mismatch explicitly or ask for a design decision.

## OpenSpec workflow (adopted 2026-10-12, canon v3.48)

- Every language or compiler change starts as an OpenSpec change in
  `openspec/changes/<name>/` (proposal, spec deltas, design, tasks), created
  with `/opsx:propose` or `openspec new change <name>`.
- A Layer A change is implemented only when its proposal quotes Marcelo
  Fortes's approval with the date. Open questions stay listed in the proposal
  until he answers them; agents implement only the approved parts.
- Implement with `/opsx:apply`, ticking `tasks.md`. The E19 gates are the last
  tasks of every change.
- After the PR is merged, archive with `/opsx:archive` (or
  `openspec archive <name> -y`), which folds the deltas into `openspec/specs/`,
  and add the canon change-log entry as usual.
- `openspec validate --all --strict` must pass (CI runs it). Set
  `OPENSPEC_TELEMETRY=0`.
- Specs describe Inox programs and diagnostics, never C++ internals; every
  scenario names its test or says "test to add".
- The canon is being migrated into OpenSpec (plan approved 2026-10-12,
  `openspec/README.md`). After editing `docs/INOX_CANONICAL.md`, run
  `python3 tools/canon_trace.py update`; CI runs `check`, which fails when a
  canon unit is missing from `openspec/migration/CANON_MAP.md`, an archived
  ADR is no longer verbatim, or a mirrored unit changed without its spec.
- High-risk changes (proposal says `Risk: high`) wait for Marcelo's approval
  of `design.md` before any code is written; the approval is quoted in the
  design with its date. A change is high-risk when it touches exception
  lowering (Itanium or MSVC funclets), the runtime ABI, memory ownership, or
  rewrites a compiler phase. Other changes go from approved proposal to code.
- A refactor that must not change behavior records `tools/ir_snapshot.py`
  before the change, compares after it, and lists every intended difference
  with `--allow` in the PR (canon E19, v3.54):

      python3 tools/ir_snapshot.py record build/inox /tmp/snap   # before
      python3 tools/ir_snapshot.py compare build/inox /tmp/snap \
          --allow tests/x.inox                                    # after

## Known pitfalls (lessons already paid for)

Read these before working on the compiler; add a line when a new trap costs
time. Each names where it bit.

- **Context can be lost mid-task.** A session can restart (usage limit,
  crash) after edits but before the commit. Before starting, run
  `git status`: uncommitted work in a clone may be your own unfinished change
  (2026-10-12, item 1 was found complete but uncommitted). Commit in small,
  gated steps and keep `tasks.md` ticked as you go.
- **A copied compiler binary does not find `stdlib/`.** Discovery is relative
  to the executable; set `INOX_STDLIB` (ir_snapshot does) or every `Use
  Std.*` program fails with "could not resolve module".
- **Windows traps: compare stdout and stderr separately.** Merging the two
  streams made `fault-not-catchable` flaky (PR #30).
- **Never compute a value inside an `ostream <<` chain that also emits IR.**
  Evaluation order inside the chain wrote instructions after their use (array
  store bug, v3.45). Compute into a variable, then write.
- **Builtins must not shadow user names.** Declaring `Low`/`High` as builtins
  broke user Consts named Low/High; intercept the builtin only when its
  argument is an array (v3.45).
- **A bare identifier may be a call.** `V[Next] += 1` looked side-effect free;
  the semantic check `isSideEffectFree` is the guard (v3.45).
- **Sanitizer builds:** use `-DINOX_ENABLE_ASAN=ON -DINOX_ENABLE_UBSAN=ON`
  (with `CXX=g++`), not raw `CMAKE_CXX_FLAGS`, which break the libinoxrt link;
  run the suite with `ulimit -s 1024`.
- **Two C runtimes on Windows break typed exception handlers.** libinoxrt
  links the static release CRT (commit 6773ee8); keep it that way.
- **The token scanners are gone.** Type/Const/State are typed declarations
  (v3.53); never re-scan tokens of a section — extend the parser instead.
- **Console I/O stays in the program, memory stays in libinoxrt.** On Windows
  the program and libinoxrt may use different C runtimes: a `FILE*` or a
  `malloc` block must never cross between them. String output is the
  program's `printf`, `GetLn` reads through the program's `getchar` passed as
  a function pointer, and only libinoxrt allocates or frees (v3.55).
- **String literals are immortal; leak tests need input.** A test made only of
  literals allocates nothing, so it cannot leak; feed lines through `GetLn`
  (`NAME.in`) and run `tools/string_leak_check.py`. clang has no ASan runtime
  in the agent container; the tool falls back to linking with g++ (v3.55).
- **Include what you use.** The CI runner's clang uses libstdc++ 14, which
  pulls in fewer headers transitively than 13: `std::find` without
  `<algorithm>` built here and failed in CI (PR #36). Build with the
  `linux-clang-debug` preset against libstdc++ 14 before pushing.
- **`Step` is a keyword** (`for ... step`); do not use it as a test variable.
- **GitHub GraphQL is blocked here.** Use `gh api` REST routes (`gh pr view`
  and `gh pr create` fail); job logs come from check-run annotations.

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
- Parameters are immutable copies by default; `ref X T` passes a read-only
  reference and `ref mut X T` a writable one (ADR-0016): exact type, a
  writable place for `ref mut`, and no other argument may use that variable.
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
- `Natural` is the non-negative subtype of `Integer` (0..Int64.Max, i64; ADR-0014),
  not UInt64. Storing an Integer into a Natural is range-checked implicitly,
  as in Ada (ADR-0015); never require `Natural(X)`. A negative constant is a
  compile error; arithmetic on Natural yields Integer. `Byte` is `UInt8`.
  There is no `Double` and no `Variant`/`OleVariant`.
- Slices (ADR-0017): `A[L..H]`, `A[..H]`, `A[L..]`, `A[..]` are read-only
  `Slice<T>` views that keep the array's indices, are never empty, are bound
  once, are never returned and only slice a named array.
- Fixed-width integers never wrap: overflow traps. A constant Integer is read in
  a fixed-width type when it fits (`B Byte := 200`, `B + 1`); an Integer variable
  narrows only through `T(X)`, which is range-checked (CANON-8, v3.44).
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
- Versions (canon SECTION 02): the language and compiler version lives in `src/compiler/support/Version.h` (`0.2.0-pre-alpha`) and is bumped only by the maintainer. Canon change-log entries are revisions, `r3.NN` from r3.58 on (never "version"); update `kCanonRevision` and the canon header together — the suite checks `inox --version` against the header.
- Lower every block (function bodies, loop bodies, `if`/`elif`/`else` branches, `try` regions) through the single statement dispatcher in the LLVM emitter. Do not add per-container lists of allowed statements; a construct that lowers in one block must lower in every block context where semantic analysis accepts it (v3.21). Context rules such as `leave`/`continue` only inside a loop, `until` only inside a repeat, or `Retry` only in a handler belong to the semantic analyzer, not to the emitter.
- Run `python tools/backend_gaps.py <inox>` after backend changes: 0 BUG is required, and with clang on PATH it also checks that the emitted IR is accepted.
- Keep `stdlib/` portable across Windows and Linux. It must not depend on GC,
  unsafe features, or C interop.

## Standard library

- `Std.System` (formerly `Std.Core`) is the conceptual prelude module and may document compiler intrinsics.
- Modules come in pairs (canon r3.58): `Std.X` holds only definitions and general data (like a C header); `Std.XUtils` holds helper functions and may use `Std.X`. Never create a catch-all module.
- `Std.IO` is the canonical facade for `Put` and `PutLn`.
- `Std.Math` contains pure Integer helpers implemented in Inox.
- `Std.Debug` reserves `Assert` until canonical trap/abort behavior exists.
- Explicit `Use` resolution checks the entry-file directory before `stdlib/`.

## Current major conformance gaps

- enum exhaustiveness checks for `case` (case itself is lowered since v3.42);
- module exports, visibility, and package search beyond local `Module`/`Use`;
- ranges/enums/sets; arrays in struct fields and State, String/struct array
  elements (fixed arrays otherwise work since v3.45);
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
