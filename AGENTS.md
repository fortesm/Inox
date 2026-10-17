# AGENTS.md

This file is the operational contract for AI agents working on Inox.

## Start here

Before any task, read `openspec/specs/constitution/spec.md` — the law of
Inox: what it is, its syntax and semantics, and who decides — then
`engineering.md` and `revisions.md` next to it (current version, revision
and open questions). The detail of a topic is in
`openspec/specs/{language,stdlib}/<topic>/spec.md`; what the compiler does today is
`docs/STATUS.md`. This file only says how to work in this repository; it
does not restate the language rules, so they cannot drift apart.

When two sources disagree (constitution, capability spec, grammar, manual,
this file, code), do not invent a third behavior and do not pick one
silently: fix the mismatch explicitly or ask Marcelo for a decision
(constitution G1, G4).

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
  `openspec archive <name> -y`), which folds the deltas into `openspec/specs/`.
- `openspec validate --all --strict` must pass (CI runs it). Set
  `OPENSPEC_TELEMETRY=0`.
- Specs describe Inox programs and diagnostics, never C++ internals; every
  scenario names its test or says "test to add".
- The constitution is locked (G8). After an approved change edits
  `openspec/specs/constitution/spec.md` or `engineering.md`, run
  `python3 tools/constitution_check.py relock --approval <change>`; CI runs
  `check`, which also fails when a spec names a test file that does not
  exist. Every approved change adds a revision line to `revisions.md`.
- High-risk changes (proposal says `Risk: high`) wait for Marcelo's approval
  of `design.md` before any code is written; the approval is quoted in the
  design with its date. A change is high-risk when it touches exception
  lowering (Itanium or MSVC funclets), the runtime ABI, memory ownership, or
  rewrites a compiler phase. Other changes go from approved proposal to code.
- A refactor that must not change behavior records `tools/ir_snapshot.py`
  before the change, compares after it, and lists every intended difference
  with `--allow` in the PR (engineering E19):

      python3 tools/ir_snapshot.py record build/inox /tmp/snap   # before
      python3 tools/ir_snapshot.py compare build/inox /tmp/snap \
          --allow tests/x.inox                                    # after

## Where things live (compiler sources)

Kept current by the OpenSpec change `compiler-modularization`; read the
module that owns a concern instead of searching the whole emitter.

- The compiler is the static library `inox-compiler`; the `inox` driver
  (`main/main.cpp`) and `inox-compiler-tests` (`tests/compiler-unit/`, unit
  tests of modules that need no whole program, run by both runners) link it.
- `src/compiler/types/` — the types of a program (OpenSpec change
  type-representation); depends only on the C++ standard library:
  - `Type.h` — one immutable type: its kind (Bool, Integer with bits and
    range, Float, Char, Decimal, String, File, Exception, Struct with
    fields, Array, Slice, Own, Option, Link, None, Void) and predicates;
  - `TypeContext.*` — creates and interns every type (equal types are one
    object, compared by pointer), the built-ins and their aliases, struct
    declaration and late completion;
  - `TypeSyntax.*` — reads the written form the parser keeps (`Array[1..N]
    T`, `Link<T>`) into a tree, for the analyzer to resolve.
  The analyzer produces types; codegen only reads them (`typeOf`,
  `declaredType`, `signatureOf` of `SemanticResult`) and never derives one.
- `src/compiler/ast/AstClone.*` — deep copies of expressions and statements,
  with an expression rewriter and a type-name rewriter (contracts, generic
  instances).

- `src/compiler/codegen/LlvmIrEmitter.cpp` — the module: globals,
  declarations, internal helpers, `main` (`LlvmIrEmitter::emit`,
  `emitFunction`).
- `codegen/FunctionEmitter.h` — the class that lowers one routine; its
  member functions live by responsibility in:
  - `FunctionEmitter.cpp` — construction and the routine body;
  - `FunctionEmitterControlFlow.cpp` — if, unless, case, loops, Return,
    Exit, leave, continue, until, with;
  - `FunctionEmitterExceptions.cpp` — try/except/ensure, Raise, Retry,
    capture and rethrow (both ABIs);
  - `FunctionEmitterStatements.cpp` — the statement dispatcher,
    declarations, assignments, calls;
  - `FunctionEmitterIo.cpp` — Put/PutLn, Get/GetLn;
  - `FunctionEmitterStrings.cpp` — String ownership (retain/release, owned
    slots, statement temporaries);
  - `FunctionEmitterAggregates.cpp` — struct defaults and construction,
    places, fields, methods, assignment;
  - `FunctionEmitterArrays.cpp` — indexing, bounds, slices;
  - `FunctionEmitterExpressions.cpp` — expression types and lowering;
  - `FunctionEmitterFiles.cpp` — the Std.IOUtils file functions (calls into
    libinoxrt with `invoke` inside protected regions);
  - `FunctionEmitterContracts.cpp` — contract checks (`llvm.expect.i1`
    branch, cold noreturn `__inox_contract_fault`) and struct invariants;
  - `FunctionEmitterPointers.cpp` — safe pointers (CANON-25): the pointer
    built-ins, owner frees through generated drop functions, disarming of
    fresh values, Link construction and generation checks, tombstone
    blocks for linked locals.
- `codegen/TypeLayout.*` — the LLVM type of each kind of `types::Type` (the
  one place an LLVM type is spelled from an Inox type), array storage, struct
  fields, routine signatures lowered from the analysis, State globals.
- `codegen/AstQueries.*` — read-only AST walks (`forEachStatement`, "uses
  try / exceptions").
- `codegen/LlvmText.*` — names, literals, string constants in IR text.
- `codegen/Operators.*` — LLVM instruction or predicate of each operator.
- `codegen/MathBuiltins.*` — Std.Math functions lowered to intrinsics/libm.
- `codegen/ExceptionAbi.*` — the exception ABI Strategy: `ItaniumExceptionAbi`
  (landingpad) and `MsvcExceptionAbi` (funclets), chosen once per module by
  `hostExceptionAbi()`; personality and capture pad. Never test the host
  OS for exception lowering anywhere else.
- `codegen/RuntimeAbi.*` — the one table of libinoxrt functions and their
  exact declarations. Generated code reaches libinoxrt only through
  `FunctionEmitter::requireRuntime`, which also tells the driver to link it
  (`LlvmModule::needsInoxRuntime`).
- `codegen/ModuleContext.h` — what the routines of a module share (module
  lines, string literal counter, exception ABI, runtime need).
- `codegen/RuntimeHelpersIr.*` — IR text of the internal helpers: runtime
  faults (CANON-19), checked arithmetic, console input.
- Helpers live in `namespace inox::compiler::codegen::detail`.
- `src/compiler/support/` — the portability layer, the only compiler code
  that may test the host OS (E12): `Environment`, `FileSystem`, `Platform`,
  `Process`, and `Version.h` (the version numbers).
- `runtime/` — libinoxrt: exception transport, `StringRuntime.cpp`,
  `FileRuntime.cpp`, `OwnRuntime.cpp` (generation blocks and per-size free
  lists of owned objects), `StringBlock.h` (internal); unit tests in
  `tests/runtime-unit/`.
- `src/compiler/semantic/SemanticAnalyzer.h` — the analyzer's interface; its
  member functions live by responsibility in:
  - `SemanticAnalyzer.cpp` — construction, the entry point, errors;
  - `SemanticDeclarations.cpp` — routine signatures, scopes and symbols,
    module items, Type/Const/State sections, structs;
  - `SemanticBuiltins.cpp` — built-in types and routines (Put/Get, prelude);
  - `SemanticStatements.cpp` — routine bodies, statements, `case`;
  - `ConstantEvaluator.cpp` — constant evaluation and folding, static values;
  - `SemanticExpressions.cpp` — expression typing (operators, indexing,
    slices, members, side effects);
  - `SemanticCalls.cpp` — calls, methods, construction, `ref` arguments;
  - `SemanticTypes.cpp` — written types resolved into `types::Type`, type
    predicates, assignability;
  - `SemanticFiles.cpp` — Std.Files / Std.IOUtils: the File type and file
    functions (visible only with `Use`), the never-copied rule;
  - `SemanticReuse.cpp` — method reuse (`Struct (TPart)`, `Override`): the
    members a struct offers through its parts, the declaration errors, and
    the one AST rewrite of the analyzer (`S.FName` becomes
    `S.TPerson.FName`, so codegen sees ordinary field access);
  - `SemanticContracts.cpp` — design by contract (CANON-24): the
    `__check`/`__pure` intrinsics, read-only contracts, and the places where
    a struct invariant is checked;
  - `ContractLowering.cpp` — runs once on the merged program before the
    analyzer: each routine with `pre`/`pos` (own or inherited through
    `Override`) becomes a checking wrapper plus a hidden `__Name_body`, and
    each struct with `invariant` lines gets `TName.__invariant(Self)`; the
    parser lowers loop `invariant`/`variant` itself;
  - `SemanticPointers.cpp` — safe pointers (CANON-25): pointer type
    rules, the pointer built-ins, and the definite-move analysis
    (`movedOwners_`, merged over every path);
  - `SemanticDebug.cpp` — Std.Debug: Assert;
  - `SemanticSupport.*` — shared helpers (`semantic::detail`).

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
- **Codegen does not infer types.** It once re-derived types from written
  text and from the shape of expressions; the two disagreed silently. Every
  type comes from the analysis; a missing one is a bug of the analysis
  (type-representation, 2026-10-17).
- **Never print in the iteration order of an unordered container.** Struct
  types came out in `unordered_map` order, which differs between standard
  libraries, so Linux and Windows produced different IR (2026-10-17).
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
- **Runtime library layout.** `runtime/StringBlock.h` is internal to
  libinoxrt (String block, line reader shared by GetLn and ReadLn);
  `runtime/FileRuntime.cpp` owns every `FILE*`. A runtime function that can
  raise has no `nounwind` in `codegen/RuntimeAbi.cpp` and is called through
  `FunctionEmitter::emitRuntimeCall`, which uses `invoke` inside a
  protected region.
- **Windows does not delete or rename an open file.** A File declared in a
  loop stays open until the next round or the routine's exit; tests that
  delete their files close them first.
- **`Step` and `In` are keywords** (`for ... step`, `for ... in`), as are `Pre`, `Pos`, `Old`, `Invariant`, `Variant` and `Override`; `True`, `False`, `Exit`, `Leave`, `Continue`, `IIf` and the `bit...` operators are predefined names (r3.76); do not use any of them as test variables. A test file has no `Module` line: its name is its module's name.
- **GitHub GraphQL is blocked here.** Use `gh api` REST routes (`gh pr view`
  and `gh pr create` fail); job logs come from check-run annotations.

## Implementation discipline

- Keep the compiler portable C++20.
- Validate Linux with `cmake --build build` and `bash scripts/run-tests.sh`.
- Both runners include `tools/grammar_consistency.py`: a change to syntax, the
  lexer keywords or CANON-20 must update `grammar/grammar.ebnf` in the same PR.
- The quality gates and their commands are in `BUILD_AND_TEST_INSTRUCTIONS.md`
  (engineering E19).
- Validate Windows with `cmake --build build --config Debug` and `pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1`.
- Do not use `git add .`; add only task-scoped paths.
- Every language change updates code, tests, the specs, `docs/site/index.html`, `CHANGELOG.md`, `docs/STATUS.md` and a revision line in `openspec/specs/constitution/revisions.md`.
- If a feature is in the law but not implemented, record it as a conformance gap in `docs/STATUS.md` instead of changing the spec.
- Versions (constitution VERSIONING): MAJOR.MINOR.RELEASE.BUILD plus a stage, in `src/compiler/support/Version.h` (`0.2.0`, `pre-alpha`); MAJOR, MINOR, RELEASE and the stage change only by the maintainer's decision. BUILD is never edited: `cmake/InoxBuildNumber.cmake` writes the commit count at every build (CI needs `fetch-depth: 0`). Revisions are `r3.NN` (never "version"); update `kCanonRevision` and the top of `revisions.md` together — the suite checks `inox --version` against it.
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
