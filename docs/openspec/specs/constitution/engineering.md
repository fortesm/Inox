<!-- SPDX-License-Identifier: MPL-2.0 -->
# Inox engineering directive

Part of the constitution (G6) and locked with it (G8). It binds every human
contributor and every AI agent, for all new code and every area a change
touches. The Inox compiler is engineered as a maintainable, testable,
portable, auditable production compiler: delivery speed never justifies
hidden coupling, silent failure, invalid LLVM IR, unclear ownership, platform
leakage or untested behavior. Exceptions to a rule are explained in the
change; convenience alone is no justification.

## Sources of influence
Inox adapts, without copying literally, Object Calisthenics, SOLID and
responsibility-driven design, modern C++20 and RAII, production LLVM-based
compiler architecture, portability-oriented systems programming, and
regression-driven development with verifiable gates. Object Calisthenics
was an object-oriented exercise; Inox keeps its intent (cohesion, small
units, low nesting, meaningful names, encapsulation, testability) and adapts
its letter to ASTs, value types, visitors, passes, diagnostics, symbol
tables, lowering and platform abstractions.

## E1. Shallow control flow
A function normally has at most one meaningful level of nested control flow.
Prefer guard clauses, early returns, small helpers, explicit state machines,
table-driven dispatch and pass decomposition. Do not extract mechanically
when it hides the algorithm; deeper nesting in a short parser or lowering
loop needs a local, reviewable reason.

## E2. No unnecessary else
After `return`, `continue`, `break`, a fatal diagnostic, an exception or a
trap, continue with the normal path instead of `else`. Use `else` only for a
genuine two-way decision.

## E3. Small, cohesive functions, classes and files
Each unit has one coherent responsibility. No function combines unrelated
phases (parsing, semantic validation, diagnostics, LLVM emission, process
spawning, filesystem policy). Size limits are review triggers, never a reason
for meaningless micro-functions.

## E4. Meaningful names
Names reveal intent. Only standard abbreviations (AST, IR, CFG, SSA, ABI,
API, CLI, EOF, UTF, LLVM) are used; no opaque names (`tmpTy`, `sym2`) in
long-lived code; `i`, `j`, `ch` only in tiny scopes.

## E5. Domain concepts are explicit
Where a value carries compiler meaning or an invariant, use a type
(SourceLocation, SymbolName, TypeName, DiagnosticId, TargetTriple,
TokenKind, ...). Raw strings and integers are for low-level boundaries.

## E6. Collections with invariants are abstractions
A collection that owns lookup, ordering, scope, uniqueness, diagnostics or
lifecycle is a named abstraction (SymbolTable, ScopeStack, DiagnosticBag,
ModuleRegistry, TypeRegistry, PassPipeline), not a naked container passed
around.

## E7. No deep navigation into another subsystem
Prefer intention-revealing queries over long member chains. Local, immutable
AST navigation that leaks no representation across phases is fine.

## E8. Behavior over getters and setters
Expose operations that keep invariants, not mutable internals. Read-only
views and immutable accessors are fine; outside mutation of internal
containers is not.

## E9. Explicit ownership and lifetime
RAII; `std::unique_ptr` for exclusive ownership; values and references where
lifetime is clear; `std::string_view` only over storage that outlives it;
`std::filesystem::path` for paths; const-correct interfaces; no raw owning
pointers, no ordinary `new`/`delete`, no release that depends on exceptional
control flow. Non-owning pointers are obvious from the API.

## E10. Minimal mutable and global state
No hidden global mutable state. Prefer immutable AST nodes, pass-local state,
explicit context objects and narrow mutation boundaries; shared state has a
documented owner and lifecycle.

## E11. Compiler phase boundaries
The pipeline is source → lexer → parser → AST → semantic analysis →
(typed/lowered representation) → LLVM IR → object/link → executable. A later
phase never repairs what an earlier one omitted: the parser does not
type-check, semantic analysis does not emit LLVM, codegen does not invent
symbols, the driver does not hide frontend failures.

DECISION P-C (Marcelo Fortes, 2026-10-09): after semantic analysis, later
phases consume its resolved decisions (which symbol, type, overload,
conversion, whether `Self` is mutable, which operation) and never rediscover
or reinterpret them. A construct may be "valid Inox, not yet lowered"
(reported and measured), never "accepted but unknown to the backend".
Migration is incremental: (1) the emitter takes the `SemanticResult` (done
in v3.18); (2) the result records more decisions; (3) typed nodes replace
duplicated information; (4) the emitter stops reinterpreting the raw AST;
(5) a dedicated lowered IR.

Contract of the phases (Marcelo Fortes, 2026-10-31, change
`phase-contracts`). Each phase receives the output of the one before,
produces its own, and never does the work of another:

| Phase | Produces | Never |
|---|---|---|
| Lexer | tokens with line and column | knows grammar, names or types |
| Parser | the AST of what was written, with source positions | checks names or types, lowers or desugars, invents names |
| Module resolution | one program, each name meaning one declaration | types expressions |
| Contract lowering | checking wrappers and invariant methods | types expressions |
| Semantic analysis | every type, symbol, instance and decision later phases need, and diagnostics with their position | emits LLVM, leaves a decision to be rediscovered |
| Code generation | valid LLVM IR or a reported gap | derives a type, resolves a name, re-checks a rule |
| Driver | the executable | hides a frontend failure |

Only the lowering phases and the two rewrites named in
`docs/ARCHITECTURE.md` change the AST. That file details each contract,
names the files that own each phase, and lists the deviations of the code
from this section, each with the step that removes it; it is updated with
any change that moves a boundary.

## E12. Platform code is isolated
Platform macros and APIs appear only in the portability layer
(`src/compiler/support/`, small detection headers), `cmake/`, platform
scripts and the runtime's platform files; never in the lexer, parser, AST,
semantic analysis, diagnostics or ordinary backend logic (the exception ABI
is chosen in one place, `codegen/ExceptionAbi`). Windows x64 (clang, MSVC
ABI) and Linux x64 (clang) are the primary targets, both validated by
running the full suite in CI. Other targets (macOS, the BSDs, Illumos,
Solaris, AIX, HP-UX, UnixWare, Android) are honest stubs until built and
tested on real or representative systems; Docker validates Linux
distributions only. A report says whether a result was executed or only
cross-built.

## E13. No silent failure
Invalid input, failed lookup, unsupported lowering, process failure,
overflow, path discovery failure and malformed LLVM never become zero,
success, a default type or partial output. Until a structured error path
exists, an explicit failure or trap beats silent corruption.

## E14. Diagnostics are compiler output
Diagnostics are precise, stable and testable: source path, line, column, the
symbol or type, and actionable wording. Implementation accidents do not leak
into user diagnostics outside explicit debug modes.

## E15. Valid LLVM IR or a clear stop
A backend feature exists only when its IR verifies and the executable
behavior passes tests. An unsupported construct gets a clear diagnostic;
emitting invalid, partial or misleading IR is a release blocker.

## E16. Every behavior and every fix has tests
Each behavior gets the narrowest coverage that applies: lexer and parser
valid/invalid, semantic valid/invalid, IR checks, execution output, expected
traps. A bug fix without a regression test is incomplete unless automation
is impossible and the reason is written down.

## E17. Design patterns that earn their place
Visitor or explicit traversal; pass pipeline; symbol table and scope stack;
diagnostic engine; Strategy for backend or target behavior (the exception
ABI); ports and adapters for platform and process services; factories where
invariants need controlled creation; RAII wrappers for files, processes,
temporaries and LLVM resources. Patterns for decoration or speculative
flexibility are forbidden.

## E18. Modern C++ baseline
Explicit C++20: standard library, RAII, value semantics, const-correctness,
explicit conversions, clear error handling; no C-style casts or
undefined-behavior assumptions. Include what you use (another standard
library may not include it transitively).

## E19. Quality gates
A gate may start as a report before it blocks, but a known failure is never
hidden or called success. A green suite alone never proves the absence of
regressions (Marcelo Fortes, 2026-10-09): a compiler change is verified by
every check below that applies, and its report says which ran.

- `tools/scripts/run-tests.sh` and `tools/scripts/run-tests.ps1` — specified behavior;
  the same checks on both; Windows failures must equal
  `tests/ci/windows-known-failures.txt` (empty today); the suite also builds with
  the `linux-clang-debug` preset against libstdc++ 14;
- the suite on an AddressSanitizer + UndefinedBehaviorSanitizer build, also
  with a 1 MiB stack (`ulimit -s 1024`, the Windows default);
- `tools/mutation_fuzz.py` on the sanitizer build (seeds 1 and 7): no
  CRASH, TIMEOUT or CGERR;
- `tools/backend_gaps.py`: 0 BUG (with clang, the emitted IR is compiled);
- `tools/eh_state_balance.py`: every captured exception state released;
- `tools/string_leak_check.py`: the String and File programs under
  AddressSanitizer with leak detection;
- `inoxrt-tests`: unit tests of libinoxrt on Linux and Windows;
- `tools/calc_differential_test.py`: arithmetic against an independent
  oracle;
- `tools/grammar_consistency.py`: grammar, lexer, parser and CANON-20 agree;
- `openspec validate --all --strict` (run from `docs/`, OpenSpec's root)
  and `tools/constitution_check.py check`: specs valid, the law unchanged unless approved, named tests exist;
- `tools/ir_snapshot.py record`/`compare` for a refactor that must not
  change behavior (v3.54): IR, types, diagnostics and exit codes of every
  program before and after; every difference is listed as intended or the
  refactor is not done.

- `tools/version_check.py`: the version and the canon revision of
  `Version.h` are the ones `revisions.md`, `README.md`, `CHANGELOG.md` and
  `docs/STATUS.md` state.

Where they run (change `ci-gates`, r3.111): the CI workflow runs the suites
on Linux and Windows, backend_gaps, eh_state_balance, string_leak_check,
OpenSpec, the constitution lock and the version check on every pull request
and push; the Gates workflow runs the sanitized suite with a 1 MiB stack,
the mutation fuzz (seeds 1 and 7) and the differential test on every pull
request that touches code, tests or tools, on every push to main and every
night, and the IR snapshot against the base on a pull request labelled
`refactor`. A gate that fails blocks the merge; its log is kept as an
artifact.

Still to come: clang-format, clang-tidy, cppcheck, coverage and complexity
reports.

## E20. Refactoring
Apply this directive incrementally: improve the area a change touches and
test it. No repository-wide stylistic rewrite that hides functional changes
or creates unbounded risk.

## E21. Module structure of the compiler
Split reactively, not predictively: extract a module only when it is
cohesive (one well-defined thing), stable (its boundary does not move with
every feature) and reused (more than one consumer, or one imminent). File
size or aesthetics alone are no reason; two responsibilities that change for
different reasons are. A file past roughly 500–800 lines that mixes such
responsibilities is split along the responsibility seam, never by line
count. A structural split is proposed as an OpenSpec change, approved by the
maintainer and verified with `tools/ir_snapshot.py` (the 2026 split of the
emitter and analyzer, change `compiler-modularization`, is the model);
`docs/ARCHITECTURE.md` keeps the map of where each concern lives.

## E22. Backend strategy: clang as the external driver
The external clang driver is a build-time dependency, not technical debt
(Zig used it for years; rustc calls the system linker). Embedding libLLVM
adds no language capability and brings a large C++ API that breaks between
releases, so the effort goes into the language and its runtime instead.

Intended order (the versions are in `docs/ROADMAP.md`): keep clang while
libinoxrt grows (runtime faults, I/O, strings, files); then emit object
files and integrate the linker; then use the LLVM libraries directly,
keeping `--emit-llvm` for development; then cross-targets and packaging;
1.0 has its own frontend, runtime, integrated backend and driver.

Clang is replaced only when one of these is concretely true: it is a real
bottleneck; a needed capability exists only through the LLVM API (JIT,
incremental code generation, pass control); distribution needs one
self-contained binary. Until then, work on how clang is called (process
spawning, paths, diagnostics) improves that call and is never an on-ramp to
remove it; replacing the backend needs an approved ADR.

## E23. Examples, releases and documentation
- Examples are classified honestly: runnable examples build and run with
  deterministic output (interactive ones through controlled stdin);
  examples of unimplemented features are marked and never shipped as
  runnable.
- A release is validated as a user would use it: configure, build, run the
  suite and the example checks, package, extract into a clean directory,
  run the package smoke tests, and check library discovery, the output
  directory, documentation and licenses. A package whose examples do not
  match their advertised status is invalid. Debug builds are never
  distributed.
- The law is the constitution; README files, the manual, tutorials and
  release notes derive from it, never contradict it, and are fixed or
  regenerated when they do.
- Licensing: MPL-2.0 without the "Incompatible With Secondary Licenses"
  notice; copyright and SPDX headers are preserved; build output
  (`build*/`, packages) is never committed.
