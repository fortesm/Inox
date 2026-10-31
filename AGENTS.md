# AGENTS.md

This file is the operational contract for AI agents working on Inox.

## Start here

Before any task, read `docs/openspec/specs/constitution/spec.md` — the law of
Inox: what it is, its syntax and semantics, and who decides — then
`engineering.md` and `revisions.md` next to it (current version, revision
and open questions). The detail of a topic is in
`docs/openspec/specs/{language,stdlib}/<topic>/spec.md`; what the compiler does today is
`docs/STATUS.md`; how the compiler is built, phase by phase, and which file
owns what is `docs/ARCHITECTURE.md` (read it before changing the compiler).
This file only says how to work in this repository; it
does not restate the language rules, so they cannot drift apart.

When two sources disagree (constitution, capability spec, grammar, manual,
this file, code), do not invent a third behavior and do not pick one
silently: fix the mismatch explicitly or ask Marcelo for a decision
(constitution G1, G4).

## OpenSpec workflow (adopted 2026-10-12, canon v3.48)

- OpenSpec lives in `docs/openspec` and the grammar in `docs/grammar`
  (r3.100). `docs/` is OpenSpec's project root: run every `openspec`
  command from `docs/` (`cd docs && openspec validate --all --strict`); its
  printed paths (`openspec/changes/<name>`) are relative to `docs/`. The
  repository tools (`tools/constitution_check.py`, `tools/*.py`) still run
  from the repository root. From the repository root `openspec list --json`
  reports `"root": null`: never run `openspec new` or `openspec init` there,
  which would create a second, empty `openspec/`.

- Every language or compiler change starts as an OpenSpec change in
  `docs/openspec/changes/<name>/` (proposal, spec deltas, design, tasks), created
  with `/opsx:propose` or `openspec new change <name>`.
- A Layer A change is implemented only when its proposal records Marcelo
  Fortes's approval with the date. Records state the technical decision in
  English, who approved it and when; they never transcribe a conversation
  (no chat quotations in docs, specs, changes, revisions, CHANGELOG or code
  comments, r3.102). Open questions stay listed in the proposal
  until he answers them; agents implement only the approved parts.
- Implement with `/opsx:apply`, ticking `tasks.md`. The E19 gates are the last
  tasks of every change.
- After the PR is merged, archive with `/opsx:archive` (or
  `openspec archive <name> -y`), which folds the deltas into `docs/openspec/specs/`.
- `openspec validate --all --strict` must pass (CI runs it). Set
  `OPENSPEC_TELEMETRY=0`.
- Specs describe Inox programs and diagnostics, never C++ internals; every
  scenario names its test or says "test to add".
- The constitution is locked (G8). After an approved change edits
  `docs/openspec/specs/constitution/spec.md` or `engineering.md`, run
  `python3 tools/constitution_check.py relock --approval <change>`; CI runs
  `check`, which also fails when a spec names a test file that does not
  exist. Every approved change adds a revision line to `revisions.md`.
- High-risk changes (proposal says `Risk: high`) wait for Marcelo's approval
  of `design.md` before any code is written; the design records the approval
  with its date. A change is high-risk when it touches exception
  lowering (Itanium or MSVC funclets), the runtime ABI, memory ownership, or
  rewrites a compiler phase. Other changes go from approved proposal to code.
- A refactor that must not change behavior records `tools/ir_snapshot.py`
  before the change, compares after it, and lists every intended difference
  with `--allow` in the PR (engineering E19):

      python3 tools/ir_snapshot.py record build/inox /tmp/snap   # before
      python3 tools/ir_snapshot.py compare build/inox /tmp/snap \
          --allow tests/x.inox                                    # after

## Where things live (compiler sources)

`docs/ARCHITECTURE.md` holds the phases of the compiler, the contract of
each (what it receives, produces, guarantees and must never do), the
deviations from that contract, and the map of which file owns each
concern. Read it before changing the compiler, and update it in the same
pull request when a phase boundary, an owner or a deviation changes.

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
- **A String borrowed from a place can die mid-statement.** `PutLn(Log =
  Mark)` with Mark assigning the State variable Log read freed memory; the
  leak check missed it because the generated code was not instrumented
  (std-strings). The generated code is now instrumented, and borrowed
  reads are retained (`protectBorrowed`).
- **Codegen does not infer types.** It once re-derived types from written
  text and from the shape of expressions; the two disagreed silently. Every
  type comes from the analysis; a missing one is a bug of the analysis
  (type-representation, 2026-10-17).
- **Never print in the iteration order of an unordered container.** Struct
  types came out in `unordered_map` order, which differs between standard
  libraries, so Linux and Windows produced different IR (2026-10-17).
- **The token scanners are gone.** Type/Const/State are typed declarations
  (v3.53); never re-scan tokens of a section — extend the parser instead.
- **Console I/O and memory stay in libinoxrt.** On Windows the program and
  libinoxrt may use different C runtimes: a `FILE*` or a `malloc` block must
  never cross between them. Since r3.88 the console is libinoxrt's alone
  (`src/runtime/ConsoleRuntime.cpp`: `__inox_con_*`, `__inox_term_*`); generated
  code never calls `printf`, `getchar` or `fflush`, every program links
  libinoxrt, and the fault functions call `__inox_runtime_shutdown` (flush
  and give the terminal back) before `_exit`. Only libinoxrt allocates or
  frees (v3.55).
- **String literals are immortal; leak tests need input.** A test made only of
  literals allocates nothing, so it cannot leak; feed lines through `GetLn`
  (`NAME.in`) and run `tools/string_leak_check.py`. clang has no ASan runtime
  in the agent container; the tool falls back to linking with g++ (v3.55).
- **Include what you use.** The CI runner's clang uses libstdc++ 14, which
  pulls in fewer headers transitively than 13: `std::find` without
  `<algorithm>` built here and failed in CI (PR #36). Build with the
  `linux-clang-debug` preset against libstdc++ 14 before pushing.
- **Runtime library layout.** `src/runtime/StringBlock.h` is internal to
  libinoxrt (String block, line reader shared by GetLn and ReadLn); blocks
  made by libinoxrt carry a hidden header (capacity, character count)
  before the count, and literals (count -1) do not, so always allocate and
  free through `allocateString` / `freeString`;
  `src/runtime/FileRuntime.cpp` owns every `FILE*`. A runtime function that can
  raise has no `nounwind` in `codegen/RuntimeAbi.cpp` and is called through
  `FunctionEmitter::emitRuntimeCall`, which uses `invoke` inside a
  protected region.
- **Windows does not delete or rename an open file.** A File declared in a
  loop stays open until the next round or the routine's exit; tests that
  delete their files close them first.
- **`Step` and `In` are keywords** (`for ... step`, `for ... in`), as are `Pre`, `Pos`, `Old`, `Invariant`, `Variant` and `Override`; `True`, `False`, `Exit`, `Leave`, `Continue`, `IIf`, `Fill` (r3.81) and the `bit...` operators are predefined names (r3.76); do not use any of them as test variables or routine names. Every array in a test is declared with `:= Fill(...)` (r3.81), and a struct with an enum, array or type-parameter field without default is built by construction, never declared bare (r3.82, `requiredFieldWithoutDefault`). A test file has no `Module` line: its name is its module's name. A module a test uses marks with `Public` what the test reaches (r3.99); a multi-module scenario is a directory of `tests/modules/` with `Main.inox` and `Main.out`, `Main.err` or `Main.trap`.
- **`.gitignore` can hide a test fixture.** A folder named `env/` (a Python
  pattern) was never committed, so CI lacked the INOX_PATH modules that
  passed locally (r3.99). Before pushing new test folders, run
  `git status --ignored` or the suite from a clean clone.
- **Windows CI compares failures with a list.** `tools/ci_known_failures.py`
  must see every `[FAIL]`: a driver case named by a label counts by its
  label (it once counted only `.inox` paths, and a broken `--debug` case
  passed unseen). In PowerShell, `2>&1 1>$null` discards both streams.
- **Releases publish themselves.** `.github/workflows/release.yml` publishes
  `vMAJOR.MINOR.RELEASE` when a push to main carries a version without a
  tag (r3.101): a change of `Version.h` is a release, and its CHANGELOG
  section (`## X.Y.Z ...`) is required, or the workflow fails. Between
  releases, CHANGELOG entries go under `## Unreleased`; the PR that changes
  the version renames that heading to `## X.Y.Z <stage>`.
- **argv[0] is not the executable's path.** `inox` run through PATH gives
  argv[0] = "inox"; `support::executableDirectory` asks the operating
  system first, or the packaged compiler misses its libinoxrt (r3.101).
- **The runners work in a temporary directory** (r3.104): programs under
  test run from it, with `INOX_OUTPUT_DIR` at `build/inox-artifacts` and
  `INOX_STDLIB` at the repository's `stdlib/`; a test that needs the
  repository root uses `$repo_root` (bash) or `$repoRoot` (PowerShell).
- **GitHub GraphQL is blocked here.** Use `gh api` REST routes (`gh pr view`
  and `gh pr create` fail); job logs come from check-run annotations.

## Implementation discipline

- Keep the compiler portable C++20.
- Validate Linux with `cmake --build build` and `bash tools/scripts/run-tests.sh`.
- Both runners include `tools/grammar_consistency.py`: a change to syntax, the
  lexer keywords or CANON-20 must update `docs/grammar/grammar.ebnf` in the same PR.
- The quality gates and their commands are in `docs/BUILD.md`
  (engineering E19).
- Validate Windows with `cmake --build build --config Debug` and `pwsh -ExecutionPolicy Bypass -File .\tools\scripts\run-tests.ps1`.
- Do not use `git add .`; add only task-scoped paths.
- Every language change updates code, tests, the specs, `docs/site/index.html`, `CHANGELOG.md`, `docs/STATUS.md` and a revision line in `docs/openspec/specs/constitution/revisions.md`.
- If a feature is in the law but not implemented, record it as a conformance gap in `docs/STATUS.md` instead of changing the spec.
- Versions (constitution VERSIONING): MAJOR.MINOR.RELEASE.BUILD plus a stage, in `src/compiler/support/Version.h` (`0.4.0`, `pre-alpha`; CMake reads it from there); MAJOR, MINOR, RELEASE and the stage change only by the maintainer's decision. BUILD is never edited: `cmake/InoxBuildNumber.cmake` writes the commit count at every build (CI needs `fetch-depth: 0`). Revisions are `r3.NN` (never "version"); update `kCanonRevision` and the top of `revisions.md` together — the suite checks `inox --version` against it.
- Licensing: MPL-2.0 without the "Incompatible With Secondary Licenses" notice; preserve copyright and SPDX headers in every file.
- Lower every block (function bodies, loop bodies, `if`/`elif`/`else` branches, `try` regions) through the single statement dispatcher in the LLVM emitter. Do not add per-container lists of allowed statements; a construct that lowers in one block must lower in every block context where semantic analysis accepts it (v3.21). Context rules such as `leave`/`continue` only inside a loop, `until` only inside a repeat, or `Retry` only in a handler belong to the semantic analyzer, not to the emitter.
- Run `python tools/backend_gaps.py <inox>` after backend changes: 0 BUG is required, and with clang on PATH it also checks that the emitted IR is accepted.

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
  agent does on that machine. It never expands what this file, the constitutional
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
