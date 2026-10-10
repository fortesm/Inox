# INOX_CANONICAL.md
# ============================================================================
# THE SINGLE CANONICAL DOCUMENT FOR THE INOX PROGRAMMING LANGUAGE
# ============================================================================
# This file is the authoritative canonical document for the Inox language,
# compiler contract, examples, tests, releases, documentation policy, and
# AI-agent operating rules. It replaces and supersedes all older independent
# specification fragments, README-derived language rules, generated manuals,
# stale docs, prior chat summaries, and previous agent instructions.
#
# Maintainer / sole design authority: Marcelo Fortes
# Version: v3.32 (Layer A: OPEN-4 closed; a local variable that is never read
#          is a compile error)
# Last updated: 2026-10-09
# Repository: github.com/fortesm/Inox
# License: Mozilla Public License 2.0 (MPL-2.0), without the "Incompatible With"
#          "Secondary Licenses" notice.
# ============================================================================

# DOCUMENT MAP

- SECTION 00 - AUTHORITY AND NON-NEGOTIABLE RULES
- SECTION 01 - LANGUAGE PURPOSE AND PHILOSOPHY
- SECTION 02 - VERSIONING AND RELEASE POLICY
- SECTION 03 - AGENT / AI OPERATING RULES
- SECTION 04 - OPEN QUESTIONS AND DEFERRED DECISIONS
- SECTION 05 - HISTORICAL DECISIONS INCORPORATED INTO THE SPEC
- SECTION 06 - CLI CONTRACT
- SECTION 07 - SOURCE FILE STRUCTURE
- SECTION 08 - LEXICAL RULES
- SECTION 09 - MODULES AND USE
- SECTION 10 - BLOCK STRUCTURE
- SECTION 11 - DECLARATIONS
- SECTION 12 - TYPE SYSTEM
- SECTION 13 - VARIABLES, MUTABILITY AND ASSIGNMENT
- SECTION 14 - SCOPING AND NO SHADOWING
- SECTION 15 - FUNCTIONS, SUBROUTINES, RETURN AND EXIT
- SECTION 16 - CONTROL FLOW
- SECTION 17 - LOOPS
- SECTION 18 - STRUCTS AND ASSOCIATED METHODS
- SECTION 19 - STRINGS AND CHARS
- SECTION 20 - NUMERIC SEMANTICS
- SECTION 21 - ARRAYS, VECTORS, SETS AND FUTURE TYPES
- SECTION 22 - STANDARD LIBRARY STATUS
- SECTION 23 - RUNTIME STATUS
- SECTION 24 - LLVM BACKEND SUPPORT MATRIX
- SECTION 25 - EXAMPLES POLICY
- SECTION 26 - TESTING POLICY
- SECTION 27 - RELEASE VALIDATION POLICY
- SECTION 28 - DOCUMENTATION GENERATION POLICY
- SECTION 29 - PORTABILITY TARGETS
- SECTION 30 - FEATURE STATUS MATRIX
- SECTION 31 - ROADMAP TO 1.0



# ============================================================================
# SECTION 00 - AUTHORITY AND NON-NEGOTIABLE RULES
# ============================================================================

This section defines document authority, reading order, governance, and the rules that must not be silently bypassed.

## HOW TO READ THIS DOCUMENT
# ============================================================================
#
# TWO LAYERS that must never be confused:
#
#   LAYER A — THE CONSTITUTION (the LAW). Design decisions TRUE FOREVER until
#       changed by an approved ADR. NEVER goes stale. If the code disagrees with
#       Layer A, THE CODE HAS A BUG — fix the code, never edit Layer A to match.
#
#   LAYER B — IMPLEMENTATION STATUS (VOLATILE). The current state of the
#       compiler/toolchain/tests/examples. An AI MAY update Layer B to match the
#       code. An AI MUST NOT update Layer A to match the code.
#
# Reading order for a context-less AI:
#   HOW TO READ -> GOVERNANCE -> FIRST-ORDER ENGINEERING DIRECTIVE ->
#   CHANGE LOG -> LAYER A -> LAYER B.
#
# ============================================================================

## GOVERNANCE — RULES THAT PROTECT THIS DOCUMENT
# ============================================================================
#
# G1. PRECEDENCE WHEN DOCUMENT AND CODE DISAGREE
#     - On LANGUAGE DESIGN  -> THE DOCUMENT WINS. The code has a bug. Fix code.
#     - On CURRENT STATUS   -> THE CODE WINS. Update Layer B.
#     - ANY divergence is a BUG TO REPORT, never silently resolved either way.
#
# G2. AUTHORITY TO CHANGE DESIGN
#     - ONLY Marcelo Fortes approves changes to Layer A.
#     - An AI MAY PROPOSE a change but MUST mark it PROPOSAL in Section B-PROPOSALS
#       and MUST NOT apply it as fact. Locked (ADR) items change only via a new,
#       dated, approved ADR. Never by silent edit.
#
# G3. CHANGE LOG IS MANDATORY
#     - Every approved design change adds a dated, attributed entry below.
#     - If code contradicts a RECENT change-log entry, the CODE is behind and
#       must be updated. The decision is intentional, not a mistake to "fix".
#
# G4. NO SILENT SCOPE EXPANSION
#     - If a behavior is not covered here, DO NOT infer it from another language
#       (C, C++, Rust, Ada, Pascal, Python, Go, VB...). STOP and request a
#       design decision; record it via ADR + CHANGE LOG once approved.
#       (This is the canonical "Agent rule" from the former vision.md.)
#
# G5. VERIFIABILITY
#     - Critical rules are backed by test fixtures. "Code obeys the document" is
#       PROVEN by running the suite, not asserted.
#
# G6. ENGINEERING QUALITY IS CONSTITUTIONAL
#     - Modern compiler-engineering quality is a first-order canonical rule, not
#       an optional style preference. New code and every code area touched by a
#       change MUST follow the directive below. Existing legacy code is improved
#       incrementally; broad cosmetic rewrites without tests are forbidden.
#
# ============================================================================

## FIRST-ORDER ENGINEERING DIRECTIVE — MODERN C++ COMPILER CONSTRUCTION

STATUS: CANONICAL, BINDING, AND READ BEFORE ALL TOPICAL IMPLEMENTATION RULES.

PURPOSE:
The Inox compiler must be engineered as a maintainable, testable, portable,
auditable production compiler. Delivery speed never justifies hidden coupling,
silent failure, invalid LLVM IR, unclear ownership, platform leakage, or
untested behavior.

This directive applies to every human contributor and every AI coding agent.
It governs all new code and all existing code modified by a task.

### Sources of influence

This policy adapts, rather than blindly copies:
- Object Calisthenics;
- SOLID and responsibility-driven design;
- modern C++20 practices and RAII;
- production compiler architecture and LLVM-based compiler construction;
- portability-oriented systems programming;
- regression-driven development and verifiable quality gates.

Explanatory references (non-normative; this document remains authoritative):
- https://medium.com/@rafaelcruz_48213/desenvolva-um-c%C3%B3digo-melhor-com-object-calisthenics-d5364767a9ba
- https://developerhandbook.stakater.com/architecture/object-calisthenics.html

Object Calisthenics originated as an object-oriented design exercise. Inox
adopts its intent — cohesion, small units, low nesting, meaningful names,
encapsulation, testability, and reduced accidental complexity — but adapts its
literal rules to ASTs, value types, visitors, compiler passes, diagnostics,
symbol tables, LLVM lowering, and target/platform abstractions.

### E1. Shallow control flow is the default

A function or method SHOULD normally contain no more than one meaningful level
of nested control flow. Prefer guard clauses, early returns, small helpers,
explicit state machines, table-driven dispatch, and pass decomposition.

Do not extract code mechanically when extraction makes the algorithm harder to
understand. A short, obvious parser or lowering loop may justify deeper nesting,
but the reason must be local and reviewable.

### E2. Avoid unnecessary `else`

After `return`, `continue`, `break`, fatal diagnostic, exception, or trap, do not
add an `else`. Continue with the normal path. Use `else` only when it expresses
a genuine two-way domain decision more clearly than guard clauses.

### E3. Keep functions, classes, and files small and cohesive

Each function, class, module, and translation unit must have one coherent
responsibility. Large functions and files are review signals and must be split
when they mix concerns or become difficult to test.

No function may combine unrelated phases such as parsing, semantic validation,
diagnostics construction, LLVM emission, process spawning, and filesystem
policy.

Numeric size limits are review triggers, not excuses for meaningless
micro-functions. Cohesion and clarity are the deciding criteria.

### E4. Use meaningful names; avoid project-local abbreviations

Names must reveal domain intent. Abbreviations are permitted only when they are
standard in compiler or LLVM engineering, including AST, IR, CFG, SSA, ABI, API,
CLI, EOF, UTF, and LLVM.

Do not introduce opaque names such as `tmpTy`, `cfgx`, `sym2`, or `modreg` in
public or long-lived code. Very small local scopes may use conventional names
such as `i`, `j`, or `ch` when meaning is obvious.

### E5. Model domain concepts explicitly

Primitive obsession is forbidden where a value carries compiler meaning or an
invariant. Prefer explicit types or abstractions such as:
- SourceLocation and SourceRange;
- ModuleName, SymbolName, and TypeName;
- DiagnosticId and DiagnosticBag;
- TargetTriple, OperatingSystem, BuildMode, and OutputPath;
- TokenKind, NodeKind, and TypeKind.

Raw strings and integers are acceptable at low-level boundaries, but domain
logic must not depend on unvalidated primitive conventions scattered throughout
the project.

### E6. Collections with invariants are first-class abstractions

A collection that owns lookup rules, ordering, scope, uniqueness, diagnostics,
or lifecycle behavior must become a named abstraction rather than a naked
`std::vector` or `std::unordered_map` passed throughout the compiler.

Approved examples include SymbolTable, ScopeStack, DiagnosticBag,
ModuleRegistry, TypeRegistry, SourceFileSet, PassPipeline, and TargetRegistry.

### E7. Do not navigate deeply through another subsystem's internals

Avoid long chains of member access and knowledge of nested representation.
Prefer intention-revealing queries and subsystem APIs. Direct immutable AST
navigation is acceptable when local and when it does not leak representation
across phase boundaries.

### E8. Behavior-oriented APIs over mechanical getters and setters

Do not expose mutable internals through boilerplate getters/setters as a
substitute for design. Prefer operations that preserve invariants and express
intent. Immutable AST/value accessors and read-only compiler views are valid.
External mutation of internal containers or invariants is forbidden.

### E9. Ownership and lifetime must be explicit

Use RAII and explicit ownership. Required direction:
- `std::unique_ptr` for exclusive ownership;
- values and references where lifetime is clear;
- `std::string_view` only when the referenced storage lifetime is guaranteed;
- `std::filesystem::path` for paths where appropriate;
- const-correct interfaces;
- no raw owning pointers;
- no ordinary manual `new`/`delete`;
- no resource release dependent on exceptional control flow.

Non-owning pointers/references must be obvious from API and lifetime context.

### E10. Minimize mutable and global state

Hidden global mutable state is forbidden. Prefer immutable AST nodes where
practical, pass-local state, explicit context objects, and narrow mutation
boundaries. Shared state must have a documented owner and lifecycle.

### E11. Preserve compiler phase boundaries

The canonical pipeline is:
source text -> lexer -> parser -> AST -> semantic analysis -> typed/lowered
representation when available -> LLVM IR -> object/link pipeline -> executable.

A later phase must not silently repair correctness omitted by an earlier phase.
The parser does not type-check; semantic analysis does not emit LLVM; codegen
does not invent missing symbols; the driver does not hide frontend failures.

DECISION P-C (approved by Marcelo Fortes, 2026-10-09). After semantic analysis,
later phases must not rediscover or reinterpret semantic information already
resolved by earlier phases; the backend consumes semantically resolved
information (which symbol, which type, which overload, which conversion, whether
`Self` is mutable, which concrete operation). The law is stated about phases,
not about C++ classes. During 0.x a construct may legitimately be "valid Inox
but not yet implemented in the LLVM backend" (reported as such and measured by
`tools/backend_gaps.py`); it is not legitimate for the backend to discover that
it does not know what an accepted construct means.
Migration is incremental, never a big-bang rewrite:
  1. AST + semantic result -> LLVM emitter (DONE in v3.18: the emitter takes
     the `SemanticResult`; module `Const` values are its first consumer);
  2. the semantic result records more resolved decisions;
  3. typed/semantic nodes where information is duplicated;
  4. the LLVM emitter progressively stops reinterpreting the raw AST;
  5. eventually a dedicated lowered IR.

### E12. Platform-specific code is isolated

Platform macros and platform-specific APIs may appear only in the portability
layer, CMake/toolchain files, and platform-specific scripts. They must not be
scattered through lexer, parser, AST, semantic analysis, diagnostics, or normal
backend logic.

Windows and Linux are the primary validation targets. Validation reports must
distinguish actual execution from cross-build/link evidence and must state
feature-specific exceptions. In the v3.18 hardening environment the full suite
executed on Linux, while Windows artifacts were cross-built and linked but not
executed. Native Windows exception lowering remains explicitly open as
EH-v3.16a. Other platform entries may remain honest stubs marked
STUB/EXPERIMENTAL/UNSUPPORTED until built and tested on the actual operating
system.

### E13. No silent failure or valid-looking fallback

Invalid input, failed symbol lookup, unsupported lowering, process failure,
overflow, invalid path discovery, and malformed LLVM must never be converted
silently into zero, success, a default type, or partial output.

Until a structured `Result[T,E]` path exists, explicit failure or fail-fast/trap
is preferable to silent corruption.

### E14. Diagnostics are first-class compiler output

Diagnostics must be precise, stable, and testable. Include source path, line,
column, symbol/type information, and actionable wording where available.
Implementation accidents must not leak into user diagnostics except in explicit
debug modes.

### E15. Codegen must emit valid LLVM IR or stop clearly

A backend feature is implemented only when generated LLVM IR verifies and the
supported executable behavior passes tests. Unsupported features must produce a
clear compiler diagnostic. Emitting known-invalid, partial, or misleading IR is
a release blocker.

### E16. Every behavior and every bug fix requires tests

Every language behavior needs the narrowest applicable coverage:
lexer/parser valid and invalid tests; semantic valid and invalid tests; LLVM IR
verification; execution/output tests; and expected-trap tests where applicable.

A bug fix without a regression fixture is incomplete unless a documented,
exceptional reason makes automation impossible.

### E17. Approved compiler design patterns

Use patterns only when they reduce coupling, clarify ownership, preserve phase
boundaries, or improve testability. Appropriate patterns include:
- Visitor or explicit traversal for AST operations;
- Pass Pipeline for staged analysis and lowering;
- Symbol Table and Scope Stack abstractions;
- Diagnostic Engine / Diagnostic Bag;
- Strategy for backend or target-specific behavior;
- Adapter/Ports-and-Adapters for platform and process services;
- Factory functions when AST or domain invariants require controlled creation;
- RAII wrappers for files, processes, temporary artifacts, and LLVM resources.

Patterns introduced merely for decoration or speculative flexibility are
forbidden.

### E18. Modern C++ baseline

The compiler uses an explicit C++20 baseline. New and modified code must prefer
standard-library facilities, RAII, value semantics, const-correctness, explicit
conversions, and clear error handling. C-style casts and undefined-behavior
assumptions are forbidden in ordinary compiler code.

### E19. Progressive quality gates are mandatory

The project must progressively enforce:
- compiler warnings;
- CTest plus a single principal C++ unit-test framework;
- integration and regression scripts;
- LLVM IR verification;
- release example validation;
- clang-format;
- clang-tidy;
- cppcheck;
- sanitizers where supported;
- coverage and complexity reporting;
- Windows and Linux CI.

A gate may begin as reporting before becoming blocking, but known failures must
not be hidden or mislabeled as success.

Verification principle (approved by Marcelo Fortes, 2026-10-09): a green
regression suite ("N/N passed") alone is NEVER sufficient to declare the absence
of regressions. The checks below answer different questions and a change to the
compiler is verified by all of those that apply:
- `scripts/run-tests.sh` and `scripts/run-tests.ps1`: specified behavior; the
  two runners must report the same number of checks;
- `tools/backend_gaps.py`: semantic acceptance vs. backend lowering, 0 BUG;
- the suite on a build with AddressSanitizer + UndefinedBehaviorSanitizer:
  memory and undefined-behavior errors the suite cannot see;
- the suite with a 1 MiB stack (`ulimit -s 1024`; the Windows default): the
  implementation limits of CANON-19 and the `limits-boundary-ok` tests;
- `tools/mutation_fuzz.py` on the sanitizer build: no CRASH, TIMEOUT or CGERR
  (inputs nobody wrote a test for);
- `tools/calc_differential_test.py`: arithmetic against an independent oracle.
A report states which of these ran and which did not.

### E20. Refactoring policy

Apply this directive incrementally. When a file or subsystem is changed, improve
the touched area and add tests. Do not launch repository-wide stylistic rewrites
that obscure functional changes, invalidate review, or create unbounded risk.

Exceptions to this directive require an explicit explanation in the change,
review, or canonical proposal. Convenience alone is not justification.

## NON-NEGOTIABLE RULES

- This document wins over README files, tutorials, generated HTML, stale Markdown, old ADR drafts, previous chats, and AI memory.
- The FIRST-ORDER ENGINEERING DIRECTIVE in SECTION 00 is mandatory for all new code and every existing code area touched by a change.
- Layer A / constitutional language rules must not be edited merely because the current code is behind.
- Layer B / implementation status may be updated to match the code.
- Every language change requires tests and a dated entry in the change log.
- Empty parentheses are invalid for zero-argument declarations and calls.
- The current canonical source is this file: `docs/INOX_CANONICAL.md`.


# ============================================================================
# SECTION 01 - LANGUAGE PURPOSE AND PHILOSOPHY
# ============================================================================

## CANON-1. VISION AND RATIONALE (was canonical/vision.md)

Inox is a compiled, strongly typed, post-object-oriented systems language for
software where silent failure is unacceptable. Its goal is not to imitate one
existing language. Inox deliberately takes inspiration from several traditions
and rejects defaults that are unsafe, obsolete, ambiguous, or hostile to
large-scale engineering. Inox prefers explicit safety over convenience when the
two conflict.

### Language references and influences (drinks from, does not copy mistakes)
- C and C++: performance, low-level realism, convenient operators — while
  rejecting undefined behavior as a design principle.
- Ada and SPARK: robustness, soundness, contract-oriented thinking,
  mission-critical discipline.
- Modula-2, Modula-3, Oberon, Component Pascal, Zonnon: modules, clarity,
  restraint, neglected good ideas.
- Eiffel and Sather: correctness, contracts, design-by-contract.
- Chapel: structured high-performance parallelism, data-locality.
- Modern Object Pascal, Delphi, Free Pascal: productivity, Pascal-family ergonomics.
- Go: composition, slices, simple concurrency ideas — while rejecting implicit
  aliasing hazards and GC-dependent latency as core assumptions.
- Rust: ownership, explicit mutability, composition, memory safety without GC.
- Swift, Kotlin, C#, Java: modern ergonomics and mature ecosystem lessons.
- Vala: useful systems-programming ergonomics where applicable.
- Julia: mathematics, scientific computing, finance, numerical expressiveness.
- Perl, Ruby, Python, PHP: expressive manipulation of strings, lists, maps,
  reductions, regular expressions, high-level data transformation.

### Mission-critical target domain
Aviation, air-traffic control, high-precision industry, finance, stock
exchanges, cryptoasset infrastructure, international monetary systems, scientific
computing, aerospace, medicine, hospital machinery, nuclear and hydroelectric
plants, electrical grid infrastructure, cryptography, large-scale parallel
computation.

In such domains these failure modes can cost lives, destroy capital, or
compromise infrastructure, and Inox is designed to prevent them: buffer overflow,
null dereference, use-after-free, silent integer overflow, silent division by
zero, accidental mutation, unchecked indexing, implicit narrowing, hidden
aliasing, nondeterministic latency.

### Core safety stance (Inox 0.1 defaults)
- no universal `null` or `nil`;
- no unsafe pointers in the safe language core;
- no silent integer overflow as guaranteed semantics;
- no integer `/`; use explicit `div` and `mod`;
- no unchecked array bounds;
- no implicit narrowing conversions;
- no implicit aliasing for future `Vector[T]`;
- parameters are immutable by default;
- mutating methods require `Self mut`;
- structs are data, not classes;
- associated methods provide behavior without inheritance;
- contracts/protocols/behaviors are future static capability checks, not
  Java-style interfaces.

### Post-object-oriented design
No classes, classical inheritance, Java-style interfaces, mixins, duck typing, or
class taxonomies. It keeps the ergonomic call form `Object.Method(args)` without
turning data into objects in the classical OO sense. Foundation: `Struct` for
data; free functions and subroutines; associated methods declared outside
structs; composition instead of inheritance; future contracts/protocols/
behaviors for static capability checks; strong nominal typing.

### Performance model
LLVM is the backend. The compiler itself must remain portable C++20. Windows and
Linux are the primary validated build targets; the platform architecture already
models macOS, FreeBSD, NetBSD, OpenBSD, DragonFlyBSD, Illumos, Solaris, AIX,
HP-UX, UnixWare, Android, and other targets explicitly. Those entries are not
support claims until validated on real or representative systems.
Inox should not rely on a tracing GC for the language core. Future memory work
should prefer explicit ownership, moves, arenas, deterministic resource
management, and controlled borrowing.

### Agent rule (canonical, from vision.md — see also GOVERNANCE G4)
When a design question is not covered by the canonical documentation, do not
infer from another language. Ask for a language decision and update the canonical
specification, ADRs, manual HTML, and tests.


# ============================================================================
# SECTION 02 - VERSIONING AND RELEASE POLICY
# ============================================================================

## CHANGE LOG (newest first — dated, attributed, append-only)
# ============================================================================
#
# v3.32 — 2026-10-10 — OPEN-4 closed: unread locals are errors (Layer A,
#         decided by Marcelo Fortes on 2026-10-10: "seguirmos como Go e dar erro
#         quando uma variável é criada e nunca usada ... só dar o erro resolva a
#         situação para todos os casos", without a `_` placeholder syntax).
#   - CANON-5 rule 7 is restated: a misspelled name in FORM 1 declares a new
#     variable (rule 1), and since nothing reads it the compiler rejects it:
#     "local variable never read: Coutner". Rules 1 and 7 no longer conflict.
#   - A local declared in a routine (any form: `X := 1`, `X T := 1`, `P TStruct`,
#     grouped) must be read before its block ends; assigning it is not reading
#     it. Parameters, `for` iterators, exception bindings, `State` and `Const`
#     are not covered. Every unread local of a block is listed in declaration
#     order.
#   - Six examples and tests declared locals they never read; they now print
#     them. The `float32-conversion` probe reads its variable.
#   - Tests: diagnostics `unread-typo`, `unread-assigned-only`,
#     `unread-nested-scope`, `unread-several`; semantic-valid
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
#         change). Requested by Marcelo Fortes: "acho que o arquivo grammar.ebnf
#         está completamente errado e desatualizado".
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
#         (Layer A, decided by Marcelo Fortes on 2026-10-09: "Uma sintaxe
#         semelhante a Ada2005, Spark/Ada ... adaptado às regras e Sintaxe de
#         inox"; "otherwise deve ser obrigatório sempre que os braços não
#         cobrirem todos os valores possíveis").
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
#         Marcelo Fortes on 2026-10-09: "Para o for use o step como kotlin").
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
#         A, decided by Marcelo Fortes on 2026-10-09: "Inox deve suportar
#         A := B := C := D := E := 4").
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
#         on 2026-10-09: "Duas variáveis na mesma linha: A, B Integer := 2 é
#         perfeitamente legal!").
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
#         "Esse bloco Main : / : / PutLn(1) / ; / ; É completamente ilegal!!
#         Deve ser registrado no canônico e corrigido no compilador!").
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
#         change approved by Marcelo Fortes on 2026-10-09: "Aprovo a precedência
#         que tomaram! Para .. e in").
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
#         Fortes on 2026-10-09: "Variável escalar sem valor inicial são proibidas
#         em Inox"; "o parser não deveria mais aceitar [Var]".
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
#     table is IMMUTABLE: changing it requires a human-approved ADR.
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
#     `Tipo()` invalid (use `C TPoint` for all-defaults). Closes OPEN-1.
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

## DEV-2. RELEASE / PREBUILT PACKAGES (was docs/release/*)

Packages (GitHub Release assets):
- inox-windows-x64.zip — https://github.com/fortesm/Inox/releases/latest/download/inox-windows-x64.zip
- inox-linux-x64.zip   — https://github.com/fortesm/Inox/releases/latest/download/inox-linux-x64.zip

Windows quick start:
    Expand-Archive .\inox-windows-x64.zip -DestinationPath C:\Tools
    cd C:\Tools\inox-windows-x64
    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
    .\set-inox-env.ps1
    inox .\examples\hello.inox
Linux quick start:
    unzip inox-linux-x64.zip -d "$HOME/tools"
    cd "$HOME/tools/inox-linux-x64"
    source ./set-inox-env.sh
    inox ./examples/hello.inox
The setup script sets PATH (includes bin/), INOX_STDLIB (-> stdlib/),
INOX_OUTPUT_DIR (-> output/).

Package layout (Windows / Linux analogous):
    inox-<plat>-x64/
        README.md
        set-inox-env.(ps1|sh)
        bin/inox(.exe)
        stdlib/
        examples/
        output/
        docs/ (or manual/index.html)   == bundled documentation
        licenses/
The `stdlib/` directory MUST ship with the compiler (used by `Use Std.*`). The
bundled documentation must be a copy of THIS file (or generated from it). The old
docs/LANGUAGE_REFERENCE.md + docs/index.html are superseded; regenerate any
shipped doc from INOX_CANONICAL.md.


# ============================================================================
# GROUP: FUTURE (absorbs docs/future/* — all were EMPTY files; topics preserved)
# ============================================================================
# These are deferred 0.2+ architectural topics. Per OPEN-QUESTIONS, they are NOT
# permission to invent semantics; each requires an explicit, approved ADR before
# implementation. The source files were empty; their TOPICS are recorded so no
# direction is lost.


# ============================================================================
# SECTION 03 - AGENT / AI OPERATING RULES
# ============================================================================

## AGENT RULES (binding behavioral contract)
# ============================================================================
1. This document is the single source of truth. Read it and the CHANGE LOG
   before any task. It replaces the entire docs/ directory.
2. Read and apply the FIRST-ORDER ENGINEERING DIRECTIVE in SECTION 00 before
   planning or modifying code. It is mandatory for every new or touched area.
3. Obey GOVERNANCE G1–G6. They override convenience.
4. Never reopen ADR-tagged/locked decisions without a new approved ADR.
5. Never edit Layer A to match the code. Code disagreeing with Layer A is a bug.
6. You MAY update Layer B to match the code. You MAY PROPOSE Layer A changes only
   in Section B-PROPOSALS, marked PROPOSAL, never applied as fact.
7. Never infer behavior from another language (G4 / the canonical Agent rule). If
   uncovered, STOP and ask for a language decision.
8. `:` is EXCLUSIVE to function/subroutine declaration. NO control structure
   (if/elif/else/while/for/repeat/until/case/with) uses `:`. (CANON-4.)
9. `End`/`end` never closes a block. Only `;`.
10. No empty parentheses on zero-arg declarations/calls.
11. `Var` block is removed; declarations are inline-only; `Var` keyword is rejected.
12. Scalars and enums REQUIRE initializers; structs may use type-default init;
    bare identifiers and assignment-to-undeclared are errors.
13. Inference picks the most general safe family type; restrictions only by
    annotation; no implicit cross-family or to/from-decimal conversion.
14. Overflow is error, never wraparound (especially Currency/Crypto). No nsw/nuw.
15. No null/nil. No `mut` parameters (reserved).
16. `with` uses VB dot-prefix; `.Member` binds innermost; `:=` for assignment; no
    `:`; non-shadowing prevails.
17. Every language change updates code + tests + this document's Layer B + a dated
    CHANGE LOG entry if it is a design change.
18. For every bug fixed, add a regression fixture in the narrowest layer
    (lexer/parser/semantic/codegen/integration).
19. Keep `dist/` examples in sync with `examples/`. Never commit build/, dist/*.zip.


# ============================================================================
# SECTION 04 - OPEN QUESTIONS AND DEFERRED DECISIONS
# ============================================================================

This section contains open questions and deferred topics. Closed items are retained for traceability, but are no longer permission to invent semantics.

## B-OPEN. OPEN DESIGN QUESTIONS (0.1 — decided items removed as they close)

These are NOT permission to invent semantics (GOVERNANCE G4). They are tracked
here so they are never lost to a context reset. Each needs a maintainer decision.

OPEN-1 — CLOSED (v3.2). Named struct construction `TPoint(FX := 10, FY := 20)`
  is now canonical law (see CANON-9, Named struct construction).

OPEN-2 — CLOSED (v3.3). In-context: a real literal in a decimal-typed context
  is read directly as that decimal type, exact, excess precision a compile-time
  error (Ada model). Context-free: use the type constructor `Currency(19.99)` /
  `Crypto(0.001)`, coherent with struct construction (CANON-9). See CANON-8,
  "Decimal literal form".

OPEN-3 — CLOSED (v3.13). Operator precedence and associativity are now canonical
  law (see CANON-20 in SECTION 20). The table follows mathematical convention
  (relationals tighter than logicals — not the Pascal mistake; bitwise tighter
  than relationals — not the C mistake; `and` > `xor` > `or`; `^` right-assoc).
  Parentheses are optional where the math is clear and REQUIRED where mixing
  ambiguous operator families (and/or/xor mixed; different bitwise families
  mixed; bitwise mixed with shift), with a parse error rather than a silent
  guess. The parser implements this and CANON-20 lists the verifying tests.

OPEN-4 — CLOSED (v3.32): Marcelo Fortes chose the Go rule; an unread local is
  a compile error, which catches the typo (CANON-5 rule 7). History follows.
  Raised in v3.27 by ChatGPT's review of ADR-0009. CANON-5 rule 1
  ("first appearance of a name is a DECLARATION") and rule 7 ("ASSIGNMENT TO A
  NON-EXISTENT NAME is an ERROR. A typo stays a bug.") contradict each other for
  FORM 1: in `Counter := 1` / `Coutner := Counter + 1`, nothing in the syntax
  separates a new name from a typo. The compiler implements rule 1. The
  maintainer decides how rule 7 is reconciled; until then neither rule is
  edited.

OPEN-5 — OPEN (raised in v3.29 by ChatGPT's review of ADR-0011). CANON-5 defines
  `Const Name := Expr` (inferred type). The parser also accepts a typed form,
  `Const Mask Integer := $FF`, used by one test. It is Layer B over-acceptance,
  not canonical syntax, until the maintainer decides whether Inox has typed
  Consts.

(Reference for the decided items: C# requires the `m` suffix and forbids implicit
float<->decimal conversion; Ada reads decimal literals by context and rejects
excess precision at compile time; Inox follows the Ada model in-context.)

## FUTURE-1. CONCURRENCY (was future/concurrency.md [EMPTY])
Direction: Chapel-style structured parallelism and Go-inspired concurrency,
WITHOUT unsafe shared mutable defaults. Data-race safety; non-mutable concurrent
data defaults. Requires ADR before implementation.

## FUTURE-2. CONTRACTS (was future/contracts.md [EMPTY])
Direction: design-by-contract (Eiffel/Sather lineage) as future static capability
checks — preconditions/postconditions/invariants — NOT Java interfaces, NOT OO
subtyping, NOT mixins, NOT duck typing. Requires ADR.

## FUTURE-3. PROTOCOLS / BEHAVIORS (was future/protocols.md [EMPTY])
Direction: a type satisfies a protocol/behavior by providing required operations
(likely associated methods); satisfaction explicit or derivable; struct
declarations must not duplicate method signatures. Static capability checks, not
Rust traits copied verbatim. Requires ADR.

## FUTURE-4. ADVANCED GENERICS (was future/generics-advanced.md [EMPTY])
Direction: generics beyond the current `Vector[T]`/`Set[T]`/`Array[..]` bracket
forms — constraints/bounds tied to contracts/protocols. Requires ADR.

## FUTURE-5. PACKAGE MANAGER (was future/package-manager.md [EMPTY])
Direction: a package/build manager and richer linking beyond the minimum local
`Module`/`Use` driver — exports, visibility, package search, dependency
resolution. Requires ADR.

## FUTURE-6. SYMBOLIC MATH (was future/symbolic-math.md [EMPTY])
Direction: mathematics/CAS direction leveraging Julia-inspired numerical and
symbolic expressiveness for scientific/financial computing. Requires ADR.

## FUTURE-7. OTHER DEFERRED 0.2+ TOPICS (from roadmap + open-questions + ADR-0006)
- ownership/borrow system beyond `Self`/`Self mut`; `Self owned`; `ref`/`ref mut`
  parameters; arena allocation and deterministic memory regions; unsafe
  boundaries, pointers, C interop;
- vector runtime with move semantics; full string/Unicode runtime;
- module visibility/export; interface/body separation;
- variant structs; JSON/DB metadata tags;
- explicit type conversions `TypeName(x)`; multi-variable declaration;
  `ReadLn` String input once the String/runtime I/O ABI exists;
- BigCurrency (+ big-integer runtime).


# ============================================================================
# GROUP: OPEN-QUESTIONS (absorbs docs/open-questions/OPEN_QUESTIONS.md)
# ============================================================================

There are NO blocking language-design questions for the current 0.1 safe-core
path. Decisions 1–20 are consolidated in the CANONICAL and DECISIONS groups above.

Deferred but directionally decided (NOT permission to invent semantics; each
requires an explicit ADR before implementation): ownership/borrow beyond
`Self`/`Self mut`; `Self owned`; `ref`/`ref mut` parameters; arena memory
management; unsafe blocks/`Pointer[T]`/C interop; contracts/protocols/behaviors;
Chapel-style structured parallelism; data-race safety and non-mutable concurrent
data defaults; vector runtime and move semantics; full string/Unicode runtime;
full module export/visibility system; variant structs and metadata tags.

Known implementation conformance gaps (spec may be ahead of the compiler): full
`case` lowering and enum exhaustiveness; module exports/visibility/package search
beyond the minimum local driver; arrays/enum/range/set implementation; vector
implementation; final runtime ABI; canonical trap/abort before
`Std.Debug.Assert`.

When closing a gap, update code, tests, this document (Layer B), HTML manual, and
ADRs together.

# ============================================================================
# AGENT RULES (binding behavioral contract — applies across all groups)
# ============================================================================
1. This document is the single source of truth. Read it and the CHANGE LOG
   before any task. It replaces the entire docs/ directory.
2. Read and apply the FIRST-ORDER ENGINEERING DIRECTIVE in SECTION 00 before
   planning or modifying code. It is mandatory for every new or touched area.
3. Obey GOVERNANCE G1–G6. They override convenience.
4. Never reopen ADR-tagged/locked decisions without a new approved ADR.
5. Never edit Layer A to match the code. Code disagreeing with Layer A is a bug.
6. You MAY update Layer B to match the code. You MAY PROPOSE Layer A changes only
   in Section B-PROPOSALS, marked PROPOSAL, never applied as fact.
7. Never infer behavior from another language (G4 / the canonical Agent rule). If
   uncovered, STOP and ask for a language decision.
8. `:` is EXCLUSIVE to function/subroutine declaration. NO control structure
   (if/elif/else/while/for/repeat/until/case/with) uses `:`. (CANON-4.)
9. `End`/`end` never closes a block. Only `;`.
10. No empty parentheses on zero-arg declarations/calls.
11. `Var` block is removed; declarations are inline-only; `Var` keyword is rejected.
12. Scalars and enums REQUIRE initializers; structs may use type-default init;
    bare identifiers and assignment-to-undeclared are errors.
13. Inference picks the most general safe family type; restrictions only by
    annotation; no implicit cross-family or to/from-decimal conversion.
14. Overflow is error, never wraparound (especially Currency/Crypto). No nsw/nuw.
15. No null/nil. No `mut` parameters (reserved).
16. `with` uses VB dot-prefix; `.Member` binds innermost; `:=` for assignment; no
    `:`; non-shadowing prevails.
17. Every language change updates code + tests + this document's Layer B + a dated
    CHANGE LOG entry if it is a design change.
18. For every bug fixed, add a regression fixture in the narrowest layer
    (lexer/parser/semantic/codegen/integration).
19. Keep `dist/` examples in sync with `examples/`. Never commit build/, dist/*.zip.


# ############################################################################
# #              LAYER B — IMPLEMENTATION STATUS (VOLATILE)                   #
# #     True at the date below; may go stale as code advances. An AI MAY     #
# #     update this layer. NEVER treat Layer B as law.                       #
# ############################################################################
# Layer B last verified against source: 2026-10-07















# ============================================================================
# SECTION 05 - HISTORICAL DECISIONS INCORPORATED INTO THE SPEC
# ============================================================================

Historical decisions are locked traceability records. Their operative language rules are reflected in the topical sections below.

## ADR-0001 — LLVM backend  [SOURCE FILE WAS EMPTY]
Topic intended: choice of LLVM as the official backend. Its substance is now in
CANON-20 (LLVM Backend) and CANON-19 (Runtime). LLVM is the official backend;
textual LLVM IR for 0.1; portable C++20 compiler; external Clang for --build/--run.

## ADR-0002 — Block syntax  [SOURCE FILE WAS EMPTY]
Topic intended: block/statement syntax. Its substance is now in CANON-4 (the `:`
rule), CANON-12 (control flow). `;` closes blocks (not a terminator); functions/
subroutines open with `:`; control structures use no `:`; `End`/`end` is not a
keyword.

## ADR-0003 — Type system  [SOURCE FILE WAS EMPTY]
Topic intended: type system. Its substance is now in CANON-8 (types/numeric) and
CANON-13 (aggregates). Integer=Int64, etc.; exact decimals; no implicit narrowing.

## ADR-0004 — Exceptions  [SOURCE FILE WAS EMPTY]
Topic intended: exception model. Its substance is now in CANON-15. `try`/`except`/
`finally`/`raise` exist in 0.1 syntax; lowering incremental; future Option/Result
complement, not replace.

Historical supersession note (v3.20; the locked ADR-0004 text above is unchanged):
the try cleanup clause spelled `finally` above is spelled `ensure` since v3.20
(ADR-0007). The operative rules are in CANON-15.

## ADR-0005 — Consolidated 0.1 Language Decisions  (Status: Accepted)
Context: early decisions were clarified incrementally; consolidated to prevent
drift between human intent, Codex prompts, and implementation.
Non-negotiable rules:
- Inox is post-object-oriented.
- No classes, inheritance, Java-style interfaces, mixins, or duck typing.
- `;` closes blocks and is not a statement terminator.
- `if`/`elif`/`else` do not use `then` or `:` and are closed by one final `;`.
- `Type` is a section/declarator, not a block; no `:` and no closing `;`.
- `TName Struct ... ;` is the canonical struct form.
- Structs declare fields only. Methods are associated outside structs.
- Associated methods use explicit receiver parameters and call-site sugar.
- `repeat` is a general loop; `until Condition` is an internal conditional-exit.
- Compiler stays portable C++20 across Windows/MSVC and Linux/GCC/Clang, future
  Unix portability a goal.
Consequence: agents must update AGENTS.md, canonical docs, grammar, examples,
tests on any change; must not drift toward ObjectPascal/Java/Go/Rust/Python/C++
defaults unless explicitly accepted.

Historical portability note (v3.19; ADR-0005 wording above is unchanged): the
platform architecture now explicitly models Windows, Linux, macOS, the BSDs,
Illumos/Solaris, AIX, HP-UX, UnixWare, Android, and related targets. Support is
claimed only after validation; see E12, SECTION 29, and EH-v3.16a.

## ADR-0006 — Inox 0.1 Language Constitution  (Status: Accepted)
Context: records foundational decisions so humans, ChatGPT, Codex, and future
agents do not re-open settled matters or import other languages' defaults.
Decisions 1–20 (safe core):
 1. Receivers `Self`, `Self mut`; `Self owned` future. Do not repeat receiver
    type in `Self` (the method prefix supplies it).
 2. Params immutable by default. Locals (inline or formerly Var) mutable.
    `mut X Integer` reserved and rejected in 0.1.
 3. `Exit` only in subroutines without return values and in `Main`. Functions use
    `Return Expression`.
 4. Integer overflow invalid. Constant overflow = compile-time error. Runtime
    overflow should trap in checking mode. No wraparound promise.
 5. Integer `/` invalid; use `div`/`mod`. Future `/` is real division for Float.
 6. Narrowing never implicit. Explicit conversion `TypeName(Expression)`.
 7. `String` UTF-8, immutable, non-null, defaults to `""`.
 8. `Char` Unicode scalar value, not byte/integer/grapheme cluster.
 9. Fixed arrays `Array[Low..High] Type`; ranges part of the type; bounds-checked
    value types.
10. `Vector[T]` future dynamic 0-based owning/move; assignment moves; `Clone` copies.
11. `for I in A..B (S)` inclusive; direction from `A..B`; positive step; iterator
    implicit, read-only, loop-scoped, cannot shadow any visible symbol.
12. `Range` declarations in `Type` are line declarations, no `;`.
13. Enums short and multi-line; nominal and ordinal; no implicit Integer conversion.
14. `Set[T]` requires finite ordinal base (Enum or finite Range); not a hash set.
15. `case Expression` no fall-through; enum cases without `otherwise` exhaustive.
16. `Module` first, no `;`, EOF closes. `Use` semantic dependency, not textual
    inclusion. Multi-file support in 0.1.
17. No public/private/protected/published in 0.1. Future visibility prefers `Export`.
18. Future contracts/protocols/behaviors are static capability checks, not Java
    interfaces, OO subtyping, duck typing, or mixins.
19. No universal `null`/`nil`. Future absence `Option[T]`; failure `Result[T,E]`.
20. No raw pointers, `Pointer[T]`, `unsafe`, or C interop in the 0.1 safe core.
Local scope and shadowing: `Name Type := Expr` declares; `Name := Expr` assigns;
shadowing forbidden in all local scopes incl. case-only differences; local
visible declaration-point -> end of block; use-before-declaration invalid.
Block closing token: `End`/`end` is not a keyword and not a synonym for `;`.
Consequence: agents must not alter these without a new ADR; if implementation
lags spec, document the gap rather than changing the language silently.
Deferred 0.2+: borrowing, arenas, unsafe boundaries, contracts, Chapel-style
parallelism, vector runtime, string/Unicode runtime, module export, variant
structs, metadata tags, full package/build tooling.

Historical supersession note (v3.19; the locked ADR-0006 text above is unchanged):
ADR-0006 decision 4 used the phrase “Runtime overflow should trap in checking
mode.” That policy is superseded by DECISION P-A, approved 2026-10-09: runtime
arithmetic faults are deterministic Inox traps in every conforming build. This
note records the later approved decision without rewriting the locked ADR.

## ADR-0011 — `case` in the Ada/SPARK style  (Status: Accepted, 2026-10-09)
Decision (approved by Marcelo Fortes, 2026-10-09):

    case Value
        1 | 2
            PutLn("small")
        3..9
            PutLn("medium")
        10 | 20..29 | 40 PutLn("mixed")
        otherwise
            PutLn("other")
    ;

- The selector is discrete: the Integer family, Char, Bool, Enum, Range. It is
  evaluated once.
- A choice is a static value (literal, Const, constant expression) or a static
  inclusive range `A..B` with A <= B, of the selector's type. `|` separates the
  alternatives of an arm; it is punctuation, not an operator.
- Overlap between any two choices, in one arm or in different arms, is a
  compile error. There is no fall-through.
- `otherwise` appears at most once and is the last arm. It is REQUIRED unless
  the compiler proves that the choices cover every value of the selector type
  (Bool, Enum and finite ranges directly; Integer types by the union of their
  choices). UInt64 and Natural are never provable with Int64 constants.
- Arms keep the Inox block rules: no `of`, `when`, `=>`, `:` or `do`; a
  single-line arm is allowed.
Rationale: Ada and SPARK make a case total. A case on an error code that
handles 404 and silently does nothing for 500 is the bug this rule prevents
(ChatGPT's example). Static, non-overlapping choices make the arm that runs
unambiguous and let the compiler check coverage.
Consequence: `|` becomes a token; the comma form is rejected with a migration
message. Lowering is unchanged by this ADR (the case lowering is Phase 2).

## ADR-0010 — `step` in the for header  (Status: Accepted, 2026-10-09)
Decision (approved by Marcelo Fortes, 2026-10-09): the step of a counted loop is
written with the reserved word `step`, as in Kotlin:
`for I in A..B step S`. The form `for I in A..B (S)` is removed, with a
migration diagnostic. The bounds are range-level expressions (CANON-20).
Rationale: the parenthesized step could not be told apart from a call
(`A..F(2)`), so the old grammar limited the upper bound to a primary
expression; `1..N + 1` needed parentheses. A keyword removes the ambiguity and
reads as English. Direction, inclusivity, the positive-step rule and the
evaluate-once rule (DECISION P-B) are unchanged.
Consequence: `step` is reserved. Lowering is unchanged.

## ADR-0009 — Chained assignment  (Status: Accepted, 2026-10-09)
Decision (approved by Marcelo Fortes, 2026-10-09): `A := B := C := X` is valid
Inox. X is evaluated exactly once; the value is stored in C, then B, then A.
Each target is a variable or a field path, and each follows CANON-5 rule 1
(the first appearance of a name declares it, with the inferred type). The chain
is a STATEMENT form: `:=` is never an expression, so `X + (A := 3)`,
`if (A := 3) = 3` and `A Integer := B := 3` are compile errors.
Rationale: the chain is a common, readable way to give several variables one
value, and keeping it at statement level avoids the C hazard of assignment
inside conditions (`if (A = B)` typed for `if (A == B)`).
Index targets (`V[I]`) are excluded: storing right to left means the next
target reads the previous one back, and an index expression could change in
between.
Consequence: the parser desugars the chain into one assignment per target
(`C := X`, `B := C`, `A := B`); no new AST node and no lowering change.

## ADR-0008 — Precedence of `..` and `in`  (Status: Accepted, 2026-10-09)
Decision (approved by Marcelo Fortes, 2026-10-09): CANON-20 places range
construction `..` at level 10 and membership `in` at level 11, between `bitor`
(9) and the relational operators (12). Both are non-associative. The table is
renumbered with integer levels; the relative order of the older levels does not
change.
Rationale: the operands of a range are arithmetic, so arithmetic and bitwise
operators must resolve first (`1..N + 1` is `1..(N + 1)`). Membership tests a
value against a range or set, so the range must be complete before `in` applies
(`X in 1..N + 1` is `X in (1..(N + 1))`). Membership yields a Bool that is
compared or combined like any relational result, so `in` binds tighter than the
relational and logical operators (`X in S = Flag` is `(X in S) = Flag`;
`X in S and Y in T` needs no parentheses). Chaining has no meaning for either
operator (a range has exactly two bounds; `X in A in B` would test a Bool for
membership), so Inox rejects the chain instead of picking a grouping
(CANON-20 refuses to guess).
Consequence: two parse diagnostics for the chains. No change to lowering.

## ADR-0007 — Loop exit and try cleanup keywords  (Status: Accepted, 2026-10-09)
Decision (approved by Marcelo Fortes, 2026-10-09): the loop exit statement is
spelled `leave` and the try cleanup clause is spelled `ensure`. They replace
`break` and `finally`, which stop being reserved words. `continue` is unchanged.
Rationale: Inox spells control flow with its own vocabulary instead of carrying
C/Java defaults (ADR-0005/ADR-0006, GOVERNANCE G4). `ensure` states what the
clause guarantees (its body runs on every departure) and does not collide with
design-by-contract, which keeps `Pre`/`Pos`/`Invariant`.
Consequence: lexical change only. Semantics, lowering and runtime behavior are
unchanged. No compatibility or migration diagnostic is kept for the old words.


# ============================================================================
# GROUP: DEVELOPMENT (absorbs docs/development/toolchain.md + docs/release/*)
# ============================================================================













# ============================================================================
# SECTION 06 - CLI CONTRACT
# ============================================================================

## Canonical user-facing CLI contract

The user-facing compiler driver SHOULD converge on this contract:

```text
inox file.inox          build and run the program
inox run file.inox      explicitly build and run the program
inox build file.inox    build the program and leave the executable in the output directory
inox check file.inox    parse and semantic-check only
inox emit-llvm file.inox emit LLVM IR for development/debugging
inox help               print help
inox version            print version
```

Legacy developer switches such as `--parse-only`, `--dump-tokens`, `--dump-types`, `--emit-llvm`, `--build`, and `--run` may remain temporarily, but they are not the preferred final user contract. If the implementation still differs, record that in SECTION 30.

## Current observed driver status

See SECTION 30 / B-WORKS for the current implementation status.


# ============================================================================
# SECTION 07 - SOURCE FILE STRUCTURE
# ============================================================================

## Source file structure

- One source file defines one logical module.
- A source file begins with optional comments/license text followed by the `Module` declaration.
- `Module Name` declares the logical module name.
- `Use` declarations import module dependencies without textual inclusion.
- A source file ends at EOF; there is no global `end` token.

The detailed module/import rules are in SECTION 09. Lexical/comment rules are in SECTION 08.


# ============================================================================
# SECTION 08 - LEXICAL RULES
# ============================================================================

## CANON-2. LEXICAL RULES (was canonical/language-reference §Lexical + semantics)

- Case-INSENSITIVE for keywords and identifiers. `PutLn`, `putln`, `PUTLN` are
  equivalent. Docs use canonical spelling. Name comparison is case-insensitive,
  so `Valor` and `valor` CONFLICT.
- Line comments use `==` until end of line: `X := 10  == comment`.
- Block comments do NOT exist in 0.1.
- String literals use double quotes `"..."`. Character literals use single
  quotes `'a'`, `'é'`, `'😀'`.
- Integer literal `42`; hex `$2A`, `$FF`. Real literal `3.14`, `0.0`.
- DIGIT SEPARATOR `_` (CHANGE LOG v3.3): an underscore may separate digits in any
  numeric literal for readability and is purely cosmetic (the lexer ignores it
  when forming the value). `10_000_000` == `10000000`. Applies to all numeric
  literals: integers `1_000_000`, reals `3.141_592_653`, hex `$FF_FF_FF`, and
  decimals `1_000_000.00`. RULES: `_` may appear ONLY between digits — never at
  the start (`_1000`), never at the end (`1000_`), never doubled (`1__000`); each
  of those is an error. No ambiguity with identifiers: a numeric literal starts
  with a digit, identifiers start with a letter or `_`.
- Operators: `:=` declare/assign-init, `=` equality, `#` inequality,
  `< <= > >=`, `..` range, `:` block-open for functions/subroutines ONLY (see
  CANON-4), `;` block-close (NOT a statement terminator), `.` member access,
  `,` separator, `^` exponentiation (never XOR).
- `End`/`end` is NOT a keyword and is NEVER a block closer. Only `;` closes
  blocks. (This deliberately avoids retaining Pascal/Ruby legacy alternatives.)
- `leave` (loop exit, CANON-17) and `ensure` (try cleanup clause, CANON-15) are
  reserved words since v3.20 (ADR-0007). `break` and `finally` are NOT reserved;
  they are ordinary identifiers.


# ============================================================================
# SECTION 09 - MODULES AND USE
# ============================================================================

## CANON-3. MODULES AND USE (was canonical/language-reference §Modules + semantics)

- `Module Name` is the FIRST declaration in a file. No `;`. EOF closes the module.
- One `.inox` file = one logical module.
- `Use Name` declares a SEMANTIC dependency. It is NOT textual inclusion, NOT
  `#include`, NOT source copying, NOT manual linking. It tells the compiler to
  load imported module types and signatures.
- Compact form allowed: `Module Calc.Core Use Sys.IO Use Math.Basic Use Calc.Types`.
- Multi-file compilation is required for the 0.1 direction (implementation is
  incremental).
- In 0.1 ALL module symbols are public by default. `Export`, interface/body
  separation, aliases, selective imports, and visibility controls are reserved
  for future versions.
- The minimum 0.1 driver resolves imported modules relative to the entry-file
  directory and through the configured standard-library directory.
  `Use Math.Basic` checks `Math.Basic.inox`, then `Math/Basic.inox`, in each
  search root. Dependencies load recursively; cycles are REJECTED; imported
  signatures participate in semantic analysis before textual LLVM IR is emitted.


# ============================================================================
# SECTION 10 - BLOCK STRUCTURE
# ============================================================================

## CANON-4. BLOCKS AND STATEMENT SYNTAX — THE `:` RULE (was language-reference + syntax + ADR-0002/0005)

THIS IS THE RULE THAT MOST OFTEN TRIPS AIs. READ CAREFULLY.

`;` closes blocks. It is NOT a general statement terminator.
`End`/`end` is not part of Inox syntax and must not be accepted as a block
closer; only `;` closes Inox blocks.

The `:` symbol has ONE meaning in Inox: it DECLARES a function or subroutine AND
opens its body. `:` REPLACES the keyword other languages spell `function`,
`def`, `fn`, `sub`, `proc`, `procedure`, `method`, `defun`. Inox has none of
those words — the `:` IS that word.

    Sum(A Integer, B Integer) Integer :     == the `:` means "this is a
        Return A + B                        ==  subroutine/function; body starts"
    ;

NO CONTROL STRUCTURE USES `:`. Control structures open their body with the
newline after the header (Ruby style) and close with `;` (Ruby's `end` == Inox `;`).

A BARE `:` BLOCK IS ILLEGAL (v3.25, decided by Marcelo Fortes). Because `:` only
declares a routine, a `:` that opens an anonymous block inside a routine body,
at any depth, is a compile error. Inox has no anonymous nested blocks; write
the statements directly in the enclosing body.

    Main :                             == ERROR: "a bare ':' block is illegal
        :                              ==  (CANON-4)"
            PutLn(1)
        ;
    ;

CORRECT (Inox):                    WRONG (the Python/C vice that breaks AIs):
    if Condition                       if Condition :      == ERROR: stray `:`
        ...                                ...
    ;                                  ;

    while I > 0                        while I > 0 :        == ERROR
        ...                                ...
    ;                                  ;

These two are EXACTLY equivalent (the second uses Ruby `end` only to build
intuition; `end` is NOT valid Inox — only `;` closes):
    if Condition          ===          if Condition
        ...                                ...
    ;                                  end

Openers/closers summary:
- Function/subroutine: opens with `:`, closes with `;`.
- if/elif/else, while, for, repeat/until, case/otherwise, with: NO `:`; newline
  opens; `;` closes.
- `Type` section: no `:`, no closing `;` of its own.
- `State :` ... `;` for global mutable state.
- `Const Name := Expr`: single-line module-level declaration.
- `Var` block DOES NOT EXIST. `Var` is a RESERVED keyword the parser MUST REJECT
  with a migration diagnostic. (See CANON-5 / CHANGE LOG v2.0.)

## CANON-7. EMPTY PARENTHESES ARE FORBIDDEN (was language-reference + syntax + semantics)

Inox does not use C/Java-style empty parentheses. Parentheses exist only when
there is at least one parameter or argument.

Valid declarations without parameters:   Main :   / PrintReport :
Valid calls without arguments:            PrintReport   /   Account.Print
Invalid:                                  Main() :   /   PrintReport()   /   Account.Print()
Calls/declarations WITH parameters still use parens: Add(A Integer, B Integer) Integer : ; PutLn(42); Account.Deposit(100)


# ============================================================================
# SECTION 11 - DECLARATIONS
# ============================================================================

## CANON-5. DECLARATIONS — VARIABLE MODEL (was language-reference §Declarations, CORRECTED to v2.0)

NOTE: the former docs described an OLD `Var` block. That model is REMOVED. What
follows is the corrected, current law.

### Type section
`Type` is a section/declarator, not a block. It has NO `:` and NO closing `;` of
its own.

    Type
        TPoint Struct
            FX Integer
            FY Integer
        ;
        TMonthRange Range 1..12
        TCardSuit (Club, Diamond, Heart, Spade)

`Struct` and multi-line `Enum` open blocks and close with `;`. `Range` and short
`Enum` are line declarations and do NOT use `;`.

### Local variables — three valid forms ONLY
    A := 5            FORM 1 — inferred type. Declares A (Integer), init 5.
    B Integer := 6    FORM 2 — explicit type + value. Declares B, init 6.
    P TPoint          FORM 3 — struct/aggregate, type-default init. Declares P.

Declarations appear INLINE, anywhere in a block, mixed freely with statements.
The OLD `Var ... ;` block is gone.

### Grouped declarations (v3.26, decided by Marcelo Fortes)
FORM 2 and FORM 3 may declare several names of the same type on one line:

    A, B, C Integer := Next(10)   == Next runs ONCE; A, B and C hold its value
    P, Q TPoint                   == both get the type defaults (rule 4)

- The initializer is evaluated EXACTLY ONCE. The first name receives the value;
  each later name is initialized from the first, in order, with the type's copy
  semantics. Afterwards the names are independent variables.
- Rules 2–8 apply to every name. `A, B Integer` (scalar, no `:=`) is a compile
  error, like `A Integer`.
- Move-only types (`Vector[T]`, future) are left to the Vector ADR.

### Assignment is a statement; chained assignment (v3.27, ADR-0009)
`:=` forms a statement. It never appears inside an expression, a condition or
an initializer ("':=' is a statement, not an expression"). The one exception
in an argument list is named struct construction, `TPoint(FX := 10)`
(CANON-9), where the left side names a field, not a variable.

An assignment statement may be chained:

    A := B := C := Next(10)   == Next runs ONCE; stored in C, then B, then A
    P.X := P.Y := 0           == field targets are allowed
    with P
        .X := .Y := 2         == so are `.Field` targets inside `with`
    ;

- The value is evaluated exactly once and stored right to left.
- Each target follows rule 1: a new name is declared (FORM 1, inferred type);
  an existing one is assigned and must accept the value's type.
- Targets are variables or field paths. A call or an index (`V[I]`) is not a
  chain target.

HARD RULES:
1. First appearance of a name is a DECLARATION; later appearances are ASSIGNMENT.
2. Every variable is born with a well-defined value. No uninitialized state.
3. SCALARS REQUIRE an initializer. `C Integer` (scalar, no `:=`) is a COMPILE
   ERROR: "scalar declaration requires initializer: C".
4. STRUCTS/AGGREGATES may omit `:=`. `P TPoint` is VALID — initialized with the
   type's field defaults. This is type-default init, not an uninitialized var.
5. ENUMS follow STRICT ADA RULES: an enum variable REQUIRES an explicit
   initializer. `Suit TCardSuit` is an ERROR; `Suit TCardSuit := Club` is valid.
6. A BARE IDENTIFIER is NEVER a declaration. `apple` alone is an ERROR. Inox is
   strongly typed (Object Pascal / Ada 2005), NOT Python/JS.
7. A TYPO STAYS A BUG. By rule 1 a misspelled name declares a new variable, and
   a local variable that is never read is a COMPILE ERROR ("local variable
   never read: Name"), so `Coutner := Counter + 1` is rejected. Assigning a
   variable is not reading it. Parameters, `for` iterators, exception
   bindings, State and Const are exempt. (v3.32, OPEN-4 decided by Marcelo
   Fortes, Go-style, with no `_` placeholder.)
8. SHADOWING is FORBIDDEN (current or any outer scope; case-insensitive). `:=` to
   a name visible in an OUTER scope is assignment to that outer variable.

Valid assignment to an outer variable:
    Main :
        X Integer := 1
        if true
            X := 2          == assigns the outer X (allowed)
        ;
    ;
Invalid shadowing:
    Main :
        X Integer := 1
        if true
            X Integer := 2  == ERROR: shadows outer X
        ;
    ;

Variables declared inside if/elif/else/while/repeat/for/case-arm/try/except/
ensure/with do not escape that block. Use before declaration is an error.

### Safe type inference (Ada/SPARK universal-literal — CHANGE LOG v2.0)
- Integer literal -> `Integer` (Int64). Real literal -> `Float` (Float64).
- Function-call initializer -> the function's return type.
- `5` and `5.0` infer DIFFERENT families; no implicit cross-family coercion.
- Inference picks the MOST GENERAL SAFE family type, NEVER a narrow subtype.
  Restrictions (`Natural`, `Byte`, fixed widths, `Currency`, `Crypto`) only by
  explicit annotation. These are equivalent: `Veritas := 5` and
  `Veritas Integer := 5`.

### Const and State
`Const Name := Expr` declares an immutable constant (single line, inferred type).
Mutable global state must be explicit via `State :` ... `;`. Global mutable state
should be rare and visible. State scalars still require initializers; `mut` inside
State is forbidden.


# ============================================================================
# SECTION 12 - TYPE SYSTEM
# ============================================================================

## CANON-8. TYPES AND NUMERIC SEMANTICS (was type-system + language-reference, CORRECTED/EXPANDED to v2.0)

### Canonical type hierarchy (THIS is the authoritative table)
Friendly aliases map to exactly one LLVM-backed base. At most one friendly alias
per width (no Delphi Cardinal/LongWord duplication).

  BOOLEAN
    Bool        -> i1      True/False. (`Boolean` is NOT canonical.)
  INTEGER — friendly aliases
    Integer     -> Int64   (i64)  default integer; target of integer inference
    Natural     -> UInt64  (i64)  floor 0; negatives are a RANGE ERROR (Ada semantics)
    Byte        -> UInt8   (i8)   0..255
  INTEGER — explicit fixed widths
    Int8(i8) Int16(i16) Int32(i32) Int64(i64)
    UInt8(i8) UInt16(i16) UInt32(i32) UInt64(i64)   UInteger -> UInt64
  FLOATING POINT
    Float       -> Float64 (double)  default real; target of real inference
    Float32 (float)   Float64 (double)
    [80-bit Extended EXCLUDED — x86-only, non-portable.]
  EXACT DECIMAL (fixed-point, exact, NO binary rounding) — TO BE IMPLEMENTED in 0.1
    Currency    i64  scaled 10^6   6 decimals   global money
    Crypto      i128 scaled 10^18  18 decimals  crypto assets
    BigCurrency BigInteger scaled 10^6  6 decimals  macro aggregates [0.2+, needs runtime]
  TEXT
    Char        i32   Unicode scalar U+0000..U+10FFFF (excl. surrogates D800..DFFF)
    String      ptr   UTF-8, immutable, non-null, default ""

`Decimal` is NOT a built-in 0.1 type.

Decimal rationale (locked): Currency = Int64 x 10^6 (6 dp covers all ISO 4217
currencies incl. 3-decimal dinars and 4-decimal cases, with margin for
intermediate FX/interest/proration precision; range ~±9.2 trillion; maps to i64;
add/sub direct, mul then /10^6, div by pre-scaling). Crypto = Int128 x 10^18 (ETH
wei needs 18 dp; ETH supply exceeds Int64; i128 native LLVM; one-wei rounding is a
consensus bug). BigCurrency = BigInteger x 10^6 for values exceeding Int64
(sovereign debt, global M2, total market cap); needs heap runtime -> 0.2+. All
decimals: value types; overflow is ERROR; no implicit conversion to/from float;
explicit conversion only.

### Generics use square brackets
    Vector[Integer]   Set[TCardSuit]   Array[1..10] Integer

### Conversions
Implicit conversions allowed ONLY for safe widening explicitly defined by Inox.
Narrowing is NEVER implicit. Explicit conversion uses `TypeName(Expression)`.
`Integer(FloatExpr)` truncates toward zero and traps/errors if out of range.
Constant narrowing with loss is a compile-time error.

### Decimal literal form (CHANGE LOG v3.3 — closes OPEN-2)
A real literal in a DECIMAL-TYPED CONTEXT is read directly as that decimal type,
exact, never through Float; excess decimal places for the type's precision are a
COMPILE-TIME ERROR (Ada model), not silent rounding:
    Preco Currency := 19.99        == 19.99 read directly as Currency (exact)
Without a type context, a real literal infers `Float` (CANON-5). To denote a
decimal value with NO context, use the TYPE CONSTRUCTOR — the same `Tipo(...)`
mechanism used for structs (CANON-9), so construction is ONE concept across the
language:
    X := Currency(19.99)           == constructs a Currency from the literal (exact)
    Y := Crypto(0.000000000000000001)  == constructs a Crypto (18 dp)
    SomaTudo(Currency(19.99))      == decimal value in a context-free position
Two readings of `Currency(...)`, both natural:
  - `Currency(19.99)` with a LITERAL  -> exact construction (no Float in between).
  - `Currency(someFloat)` with a Float EXPRESSION -> the EXPLICIT float->decimal
    conversion (implicit float<->decimal is forbidden; this is the allowed
    explicit form, with documented possible loss/range error).
Excess precision in the literal (more decimals than the type holds) is a
compile-time error, exactly as in the in-context case.

### Operators and numeric law
- Boolean: `and  or  xor  not`.
- Integer bitwise: `bitand  bitor  bitxor  bitnot  shl  shr`. `bitnot` is prefix.
- `^` is exponentiation, never XOR.
- Integer division: `A div B`, `A mod B`. Integer `/` is a COMPILE-TIME ERROR
  with a message directing to `div`. Future `Float` `/` lowers to LLVM `fdiv`.
- Integer overflow is INVALID behavior — not wraparound, not saturation. Constant
  overflow is always a compile-time error. Runtime integer overflow deterministically
  traps in every conforming build (DECISION P-A). Do NOT emit LLVM `nsw`/`nuw`
  until checks and optimization policy are mature.
- Division by zero for `div`/`mod`:
  - CONSTANT divisor zero (`X := 5 div 0`) is a COMPILE-TIME ERROR (like constant
    overflow). Rejected, as in Ada/SPARK/Delphi.
  - DYNAMIC divisor that becomes zero at runtime (e.g. an iterator passing through
    0, `X := 5 div A`) is a RUNTIME TRAP. The compiler cannot know the value, so
    the program stops with a clear error instead of producing garbage.
- Exponentiation `^` (CHANGE LOG v3.1):
  - On integers, `^` returns an Integer. The exponent MUST be non-negative
    (Ada `**` style). `2 ^ 10` = 1024.
  - Negative integer exponent (`2 ^ -1`) is an ERROR — it would require a
    fractional/float result, and Inox forbids implicit cross-family conversion.
    Use an explicit float (`Float(2) ^ -1`) or a future `Pow` function.
  - Integer `^` overflow is an ERROR/trap, like any integer overflow (ADR-0006).
  - On floats (future), `^` with a real exponent uses the floating definition
    (e^(y*ln x)) and returns a Float.


# ============================================================================
# SECTION 13 - VARIABLES, MUTABILITY AND ASSIGNMENT
# ============================================================================

This section groups the variable model, mutability model, assignment model, and ownership direction.

- Local declarations are inline-only. `Var` blocks are removed from the canonical language.
- Assignment to an existing variable uses `:=`.
- Parameters are immutable by default.
- Mutating associated methods require `Self mut`.
- Ownership rules for future heap-managed values are deferred, but the safe-language core must not rely on unsafe pointer ownership.

### Variable declaration rules

See SECTION 11 for the full declaration model.

## CANON-10. MUTABILITY AND OWNERSHIP (was language-reference + semantics; ADR-0006)

- Parameters are immutable by default.
- Local variables (inline or formerly Var) are mutable.
- Declaration `Name Type := Expr` vs assignment `Name := Expr` are distinct.
- Shadowing is forbidden in all local scopes (incl. case-only differences).
- Receiver mutation requires `Self mut`.
- `mut X Integer` for ordinary mutable parameters is RESERVED for a future
  version and MUST be a clear error in 0.1.
- Local declaration visible only declaration-point -> end of current block.
- Use before declaration is an error.
- Scoping blocks: subroutine/function body, if/elif/else, while, repeat, for
  body, each case arm, try/except/ensure, with body.
- Future ownership work: `Self owned`, `ref X T`, `ref mut X T` — NOT part of the
  0.1 executable subset.

Canonical diagnostics:
  "shadowing is forbidden: Name"
  "loop iterator conflicts with existing symbol: Name"
  "cannot assign to read-only loop iterator: Name"
  "use before declaration: Name"
  "scalar declaration requires initializer: Name"


# ============================================================================
# SECTION 14 - SCOPING AND NO SHADOWING
# ============================================================================

## Scoping and no shadowing

- Local scopes must reject accidental shadowing of existing symbols.
- A `for` iterator is introduced implicitly by the loop, is read-only, and is scoped only to that loop.
- Reusing the same iterator name in two sequential loops is valid after the first loop scope has ended.
- Using a loop iterator outside its loop is invalid.
- Declaring a local variable with the same name as an active iterator, parameter, or existing local is invalid.
- `with` dot-prefix lookup must obey innermost binding and the no-shadowing rule.

Related rules: SECTION 13 for mutability, SECTION 17 for loops, SECTION 18 for associated methods.


# ============================================================================
# SECTION 15 - FUNCTIONS, SUBROUTINES, RETURN AND EXIT
# ============================================================================

## CANON-6. FUNCTIONS, SUBROUTINES, RETURN, EXIT (was language-reference + semantics)

    Sum(A Integer, B Integer) Integer :   == function: has a return type
        Return A + B
    ;
    PrintValue(X Integer) :               == subroutine: no return type
        PutLn(X)
    ;
    Main :                                == subroutine, no params, NO parens
        PutLn("hello")
    ;

- `Return Expression` is REQUIRED in functions with return values; FORBIDDEN in
  subroutines.
- `Exit` terminates the current subroutine without a value; allowed ONLY in
  subroutines without return values and in `Main`; FORBIDDEN in functions.
- Falling through the end of a subroutine is allowed.
- Functions must not fall through without returning a value.


# ============================================================================
# SECTION 16 - CONTROL FLOW
# ============================================================================

## Control-flow constructs

## CANON-12. CONTROL FLOW (was language-reference + syntax + semantics)

NONE of these use `:` (CANON-4). All close with `;`.

### if / elif / else
    if A > B
        Return A
    elif A = B
        Return 0
    else
        Return B
    ;
No `then`; no `:`; one final `;` closes the whole structure; no `;` between
branches.

## CANON-15. EXCEPTION HANDLING

**LANGUAGE STATUS: CANONICAL. IMPLEMENTATION STATUS: IMPLEMENTED CORE (v3.17).**

Exception handling is structured control flow. It does not introduce classical
OO inheritance and does not require general RTTI. Exception identifiers are
nominal exception TYPES with a dedicated exception-taxonomy relationship.

Canonical forms:

    try
        RiskyOperation
    except
        On RangeError
            RecoverRange
        ;
        On E IOError
            RecoverIO
        ;
        Else
            RecoverUnknown
        ;
    ensure
        Cleanup
    ;

    try
        AcquireAndUse
    ensure
        Release
    ;

    try
        RiskyOperation
    except
        RecoverAll
    ;

    try
        Operation
    except
        On FileNotFound
            Log("retrying")
            Retry(3)
        ;
    ensure
        CleanupAttempt
    ;

Rules:

1. `try`, `except`, `On`, `Else`, `ensure`, `Raise`, and `Retry` are
   case-insensitive reserved words where applicable. Exception control-flow
   headers use NO `:` and typed handlers use NO `Do`.
2. Every `try` MUST contain `except`, `ensure`, or both. A bare `try ... ;`
   is invalid.
3. A PLAIN `except` body is a catch-all handler for Inox exceptions reaching
   that protected region. Its final `;` is the `try` terminator when no
   `ensure` follows.
4. A TYPED `except` contains one or more explicitly closed handlers:

       On ExceptionType
           Statements
       ;

       On Name ExceptionType
           Statements
       ;

   The optional `Name` binds the caught exception value in that handler only;
   the binding is immutable. `Name: ExceptionType` and `Do` are invalid syntax.
   Each `On` handler has its own closing `;`. An `Else` handler also has its own
   closing `;`. A separate final `;` closes the complete `try` construct.
5. Typed handlers are tested in SOURCE ORDER and the FIRST matching handler is
   selected. Duplicate handlers are invalid. A handler is also invalid when an
   earlier handler already covers its type through exception-taxonomy matching
   (for example, `On IOError` before `On FileNotFound`).
6. `Else` is valid only after a typed `On` list and handles an exception not
   matched by any preceding `On`. If there is no matching `On` and no `Else`,
   the exception propagates outward automatically.
7. `Exception` is the standard generic Inox exception type and matches every
   standard Inox exception. It must therefore be the last typed handler; an
   `Else` after `On Exception` is unreachable.
8. `Raise ExceptionExpression` throws a new exception value. Bare `Raise` is
   valid only while executing an exception handler and rethrows the exception
   currently being handled, preserving its identity/type.
9. `Retry(N)` is valid only while an explicit `On` or `Else` handler is active.
   It requests up to `N` ADDITIONAL executions of the complete body of the
   associated `try`; the original execution does not count against N. Thus
   `Retry(3)` allows at most four executions of that try body. The retry counter
   belongs to that try activation and survives across its retries.
10. `Retry` does not retry only the failed instruction and does not restart the
    entire function. External state established before the `try` remains; code
    and locals inside the try body are re-entered according to normal lexical
    semantics.
11. If an `ensure` exists, it executes before each new retry. Only after cleanup
    completes does execution return to the beginning of the protected try body.
    If the retry budget is exhausted, `Retry(N)` behaves as a bare rethrow of the
    current exception; it never silently continues after the handler.
12. `Retry` itself provides no delay/backoff policy. A handler may execute normal
    Inox statements such as logging, sleeping, reconnecting, or state updates
    before requesting `Retry(N)`.
13. In nested exception handling, Retry always refers to the try whose currently
    active explicit `On`/`Else` handler requested it. A nested handler shadows an
    outer retry context while that nested handler is executing.
14. `Retry` is not valid in a normal function body, in an unhandled protected
    region, in a plain `except` body, or inside `ensure`.
15. `ensure` is CLEANUP, not a handler. It executes exactly once whenever
    control leaves the protected construct normally or exceptionally, including
    handled exceptions, unmatched/propagating exceptions, explicit `Raise`,
    `Retry`, `Return`, `Exit`, and loop exit/continue transfers that cross the
    protected region. If code executed by `ensure` raises a new exception, that
    new exception becomes the propagating exception.
16. Inox deliberately permits the combined form `try ... except ... ensure ... ;`.
17. Standard exception types currently defined by the prelude/standard-library
    surface are:

       Exception
           IOError
               FileNotFound
               PermissionDenied
               AlreadyExists
               DiskFull
           ArithmeticError
               DivisionByZero
               OverflowError
               DomainError
           RangeError
               IndexError

    These are TYPES and categories in a dedicated nominal exception taxonomy.
    They are NOT enum constants and this relationship does NOT grant struct/class
    inheritance, inherited fields, virtual methods, or general polymorphism.
18. A handler matches its exact type and all descendant exception types. Thus
    `On IOError` matches `IOError`, `FileNotFound`, `PermissionDenied`,
    `AlreadyExists`, and `DiskFull`; `On RangeError` matches `RangeError` and
    `IndexError`.
19. The standard exception taxonomy is part of the prelude/standard-library
    contract. The native runtime (`libinoxrt`) transports opaque exception
    identities and does not define the source-language taxonomy.
20. User-defined exception declaration syntax, exception payload field layout,
    payload reflection/access, and future compositional/capability matching are
    still DEFERRED. They must not be inferred from another language without an
    approved Inox design decision.
21. Exceptions represent exceptional control flow. A future Result/Error value
    model complements exceptions for expected/domain failures; it does not erase
    `Raise`/exception semantics. Contract violations (`Pre`/`Pos`/`Invariant`)
    remain conceptually distinct even if a future runtime chooses an exception
    as one reporting mechanism.

### Current v3.17 lowering note (Layer B, not language law)

The bootstrap implementation assigns stable RuntimeTypeId values to standard
exception types and transports them through `libinoxrt`. The compiler owns the
name/taxonomy table and lowers category matching to nominal RuntimeTypeId tests;
`libinoxrt` remains generic and transports only opaque ids through the native
exception mechanism. On Unix-like Itanium-ABI targets, LLVM `invoke`/`landingpad`
plus the platform unwinder are used. `Retry(N)` lowers to a hidden per-try retry
counter and action dispatcher. `ensure` cleanup edges cover rethrow, retry,
Return, Exit, leave and continue in the currently supported LLVM subset.

## CANON-11. `with` STATEMENT (CHANGE LOG v2.2 — Visual Basic dot-prefix model)

**IMPLEMENTATION STATUS: IMPLEMENTED (v3.15)**

`with` is a RESERVED keyword and a control structure. It opens a block bound to
an expression (typically a struct value). NO `:`; newline opens; `;` closes.
Inside the block:
- A DOT-PREFIXED name `.Member` resolves as a member of the `with` expression.
- An UNPREFIXED name resolves by NORMAL scope rules (not a member access).
- Assignment is `:=` (never `=`).

    with theCustomer
        .Name := "Alice Smith"        == theCustomer.Name
        .City := "Seattle"            == theCustomer.City
        .AccountBalance := 250.50     == theCustomer.AccountBalance
        OtherThing                    == normal scope resolution
    ;

NESTED `with`: `.Member` ALWAYS binds to the INNERMOST enclosing `with` (VB rule).
For an outer object use its full name (`Outer.Member`).
NON-SHADOWING ALWAYS PREVAILS: `with` introduces no new symbols; the dot-prefix
makes member access explicit, so it never collides with scope names. The VB
dot-prefix fixes the classic Object Pascal `with` ambiguity.

**Verifying tests (v3.15)**:
- `tests/parser/valid/with-basic.inox` — basic member read/write
- `tests/parser/valid/with-scope.inox` — unprefixed names use normal scope
- `tests/parser/valid/with-nested.inox` — nested `with`, innermost binding
- `examples/with-statement.inox` — end-to-end LLVM emission and execution


# ============================================================================
# SECTION 17 - LOOPS
# ============================================================================

### while
    while I > 0
        I := I - 1
    ;
`leave` exits the nearest loop; `continue` proceeds to the next iteration.

### repeat / until
`repeat` is a general loop; `until` is an INTERNAL conditional exit, not the
terminator. `until Condition` exits the nearest repeat when true; it may appear
at the beginning, middle, or end, and MORE THAN ONCE. `repeat` closes with `;`.
    repeat
        Work
        until Done
        MoreWork
    ;

### for in range
    for I in A..B
        ...
    ;
    for I in A..B step S
        ...
    ;
- the header bounds are range-level expressions (CANON-20 level 10), so
  `for I in 1..N + 1 step 2` needs no parentheses; `step` is a reserved word
  (ADR-0010). The old form `for I in A..B (S)` is a compile error with a
  migration message;
- range endpoints are inclusive; direction comes from `A..B` (A<B ascending,
  A>B descending, A=B executes once);
- step is always positive; step zero/negative is an error if constant, or a
  runtime trap if dynamic;
- `continue` goes to the step/next iteration; `leave` exits;
- iterator is declared implicitly, is read-only, visible only inside the loop
  body, and must not conflict with any already visible symbol;
- two SEQUENTIAL `for` loops may reuse the same iterator name after the first
  ends; a NESTED `for` must not reuse an outer loop's iterator name;
- enum ranges are valid in `for`;
- DECISION P-B (approved by Marcelo Fortes, 2026-10-09): the expressions that
  determine the start bound, the end bound and the step of a `for` are evaluated
  exactly once, before the first iteration, in textual order (start, end, then
  the step if one is written), and their values do not change while that loop
  runs. `for I in Start..Finish` calls `Start` once and `Finish` once.
  Assigning inside the body to a variable used in a bound does not change the
  range. Without an explicit step the direction comes from the bounds
  (Start <= End: +1; Start > End: -1). The iterator never takes a value outside
  `A..B` and the loop never computes a value past Int64.Min/Max.

### case (ADR-0011, v3.29)
    case Value
        1 | 2
            PutLn("small")
        3..9
            PutLn("medium")
        10 | 20..29 | 40 PutLn("mixed")
        otherwise
            PutLn("other")
    ;
Single-line arms allowed: `Club PutLn("club")`.
- no `of`, `when`, `=>`, `:`, or `do`;
- the selector is discrete (Integer family, Char, Bool, Enum, Range) and is
  evaluated once; it ends at the header line;
- a choice is a static value or a static range `A..B` (A <= B) of the
  selector's type; `|` separates alternatives (the comma is an error);
- overlapping choices are a compile error; no fall-through;
- `otherwise` is at most one and is the last arm. It is REQUIRED unless the
  choices provably cover every value of the selector type (Marcelo Fortes,
  2026-10-09). For `Enum` this means every literal; for `Bool`, both values;
  for an Integer type, its whole range; UInt64 and Natural always need
  `otherwise`;
- a choice has the selector's type: a Const keeps its inferred type, while a
  literal-only expression is read in the selector's type and must fit it;
- `case` as an expression is reserved for a future version.

### unless
Negated single-condition guard (parsed; lowering incremental).


# ============================================================================
# SECTION 18 - STRUCTS AND ASSOCIATED METHODS
# ============================================================================

## CANON-9. STRUCTS AND ASSOCIATED METHODS (was language-reference + type-system + semantics + llvm-backend)

    Type
        TPoint Struct
            FX Integer
            FY Integer
        ;
        TConfig Struct
            FPort Integer := 8080
            FEnabled Bool := true
        ;

Rules:
- `Struct` is a reserved word; opens the struct body; `;` closes it.
- Struct declarations contain FIELDS ONLY. Structs do not declare methods and do
  not repeat method signatures.
- Type names conventionally begin with `T`; fields with `F` (style rules in 0.1,
  not fatal errors).
- Fields may have literal defaults for supported types.
- Structs are nominal VALUE TYPES. Assignment, ordinary parameter passing, and
  ordinary returns COPY the struct value. The backend may use pointers internally
  for associated receivers; this does not change language semantics and does not
  create reference semantics.

Associated methods are declared OUTSIDE the struct:
    TPoint.Move(Self mut, DX Integer, DY Integer) :
        Self.FX := Self.FX + DX
        Self.FY := Self.FY + DY
    ;
    TPoint.Sum(Self) Integer :
        Return Self.FX + Self.FY
    ;

- The receiver type is implied by the `TPoint.` prefix. `Self TPoint` and
  `Self mut TPoint` are NOT canonical (redundant).
- Call-site sugar: `P.Move(3, 7)` and `PutLn(P.Sum)` lower to static associated
  calls. NOT virtual dispatch; no classes/inheritance/interfaces/subtyping.
- `Self` = read-only receiver. `Self mut` = mutable receiver. `Self owned` is
  reserved for future ownership-consuming methods.

### Named struct construction (CHANGE LOG v3.2 — canonical)

A struct value may be constructed inline with NAMED fields using `:=` as the
field/value separator:

    P := TPoint(FX := 10, FY := 20)    == builds a TPoint with FX=10, FY=20

`:=` is used because filling a field IS conceptually an assignment ("FX receives
10"), consistent with `:=` everywhere else; and this keeps `:` exclusive to
function/subroutine declaration (CANON-4). Positional construction (`TPoint(10,
20)`) is NOT canonical: it binds to field ORDER, so reordering fields would
silently mis-fill — unacceptable for mission-critical code. Named construction is
immune to field reordering.

Rules:
- NAMED ONLY. Each argument is `FieldName := Expression`. Positional is invalid.
- ORDER IS FREE: `TPoint(FY := 20, FX := 10)` equals `TPoint(FX := 10, FY := 20)`,
  because each value names its field.
- OMITTED FIELDS: a field with a default uses its default; a SCALAR field with no
  default that is omitted is a COMPILE-TIME ERROR (consistent with "scalars are
  born with a value", CANON-5). A struct-typed field omitted uses its own
  type-default.
      C := TConfig(FPort := 9090)   == OK: FEnabled uses its default true
- DUPLICATE field (`TPoint(FX := 10, FX := 5)`) is a COMPILE-TIME ERROR.
- UNKNOWN field (`TPoint(FZ := 10)`, no such field) is a COMPILE-TIME ERROR.
- EMPTY `TPoint()` is INVALID (empty parentheses are forbidden, CANON-7). To build
  a struct using ALL defaults, use the type-default declaration form `C TPoint`
  (no parentheses). Thus the two creation forms coexist:
      C TPoint                       == all field defaults (no parens)
      C := TPoint(FX := 10, FY := 20)  == specify one or more fields by name
- The constructor is an EXPRESSION: it may appear anywhere a value is expected
  (assignment, argument, return), e.g.
      PutLn(SumPoint(CopyPoint(TPoint(FX := 10, FY := 20))))
  This makes hand-written "MakePoint"-style factory functions unnecessary.


# ============================================================================
# SECTION 19 - STRINGS AND CHARS
# ============================================================================

## CANON-14. STRINGS AND CHAR (was language-reference + semantics; ADR-0006 #7-8)

`String` is UTF-8, immutable, non-null, with `""` as its zero/default value.
There is no null string. Absence is future `Option[String]`.
0.1 string operations: string literal; local `S String := "..."`; parameter and
return type `String`; `Put`/`PutLn` for strings/literals; byte-by-byte equality
and inequality with `=` and `#`.
Reserved/not implemented in 0.1: `S[I]` indexing; string concatenation;
`ByteLength`, `CharLength`, `GraphemeLength`.
`Char` is a Unicode scalar value — not a byte, not an integer, not a grapheme
cluster; conceptually 32-bit; valid U+0000..U+10FFFF excluding surrogates
U+D800..U+DFFF; literals `'a'`, `'é'`, `'😀'`; surrogate literals are
compile-time errors. Conversions among `Char`, `Byte`, `Integer` are always
explicit.


# ============================================================================
# SECTION 20 - NUMERIC SEMANTICS
# ============================================================================

Numeric law is part of the type system, but is separated here for quick reference. The full type table appears in SECTION 12.

## Numeric semantics summary

- Integer `/` is forbidden; use `div` and `mod`.
- Constant division by zero is a compile-time error.
- Dynamic division by zero is a runtime trap.
- Integer overflow is an error/trap; it is never guaranteed wraparound.
- Implicit narrowing is forbidden.
- Decimal literals in decimal-typed context are exact; context-free decimal spelling uses type constructors such as `Currency(19.99)`.

See SECTION 12 / CANON-8 for the full numeric rules.

## CANON-20 — OPERATOR PRECEDENCE AND ASSOCIATIVITY (LAYER A LAW — IMMUTABLE)

This precedence table is canonical law. It is fixed and MUST NOT be changed by
any AI or contributor. Any change requires an explicit, human-approved ADR with a
dated CHANGE LOG entry; until then this table governs the parser, and the parser
must implement exactly this — divergence is a bug to fix in the parser, never a
reason to alter this table.

### Design intent (why this table)
Inox precedence follows true mathematical convention and deliberately avoids the
two classic historical mistakes:
- It is NOT the Pascal mistake: relational operators bind TIGHTER than the
  logical operators, so `A < B and C < D` means `(A < B) and (C < D)` with no
  parentheses required.
- It is NOT the C mistake: bitwise operators bind TIGHTER than relational
  operators, so `A bitand B = C` means `(A bitand B) = C`.

### Precedence table (strongest binding first → weakest binding last)
```text
Level  Operators                              Associativity
-----  -------------------------------------  -------------
 1     postfix: call f(...), index A[i]       left
 2     ^  (exponentiation)                    RIGHT
 3     unary: not  -x  +x  bitnot             (prefix, unary)
 4     *  /  div  mod                         left
 5     +  -                                   left
 6     shl  shr  (bit shifts)                 left
 7     bitand                                 left
 8     bitxor                                 left
 9     bitor                                  left
10     ..  (range construction)               NON-associative
11     in  (membership)                       NON-associative
12     = # < <= > >=  (relational)            left
13     and                                    left
14     xor                                    left
15     or                                     left
16     :=  (assignment, statement level)      right
```
(Level 16: `:=` is not an expression operator. Its right associativity exists
only inside the assignment statement, where `A := B := X` is a chain, ADR-0009.)
(Levels 10 and 11 were added by ADR-0008, v3.24; the levels below them were
renumbered without changing their relative order.)
Notes:
- `^` is RIGHT-associative: `2 ^ 3 ^ 2` = `2 ^ (3 ^ 2)`. It binds tighter than
  unary minus on its left operand per parsing, e.g. `2 * 3 ^ 2` = `2 * (3 ^ 2)`.
- All relational operators share one level (12).
- Among logical operators the order is `and` (13) > `xor` (14) > `or` (15),
  matching formal boolean algebra (conjunction binds tighter than disjunction).
- `..` (10) and `in` (11) are NON-associative: `A..B..C` and `X in A in B` are
  parse errors. A range has exactly two bounds, and membership yields a Bool.
  Their operands resolve first: `X in 1..N + 1` = `X in (1..(N + 1))`, and the
  result compares like any relational: `X in S = Flag` = `(X in S) = Flag`.

### MANDATORY-PARENTHESES RULE (Ada/SPARK safety on the ambiguous cases)
Precedence above is fully defined, but where the relative order of two operator
families is commonly memorized wrong, Inox REFUSES to guess and REQUIRES explicit
parentheses. The compiler raises a parse error (it does not silently pick an
order). Parentheses are otherwise OPTIONAL and may always be used for clarity.

Parentheses are REQUIRED when, without them, the expression would mix:
1. two DIFFERENT logical operators among `and` / `or` / `xor`
   - `A or B and C`   -> ERROR; write `A or (B and C)` or `(A or B) and C`
   - `A xor B and C`  -> ERROR; parenthesize the intended grouping
2. two DIFFERENT bitwise families among `bitand` / `bitxor` / `bitor`
   - `A bitand B bitor C` -> ERROR; write `(A bitand B) bitor C`
3. a bitwise operator mixed with a shift (`shl` / `shr`)
   - `A bitand B shl C` -> ERROR; write `(A bitand B) shl C` or `A bitand (B shl C)`

Parentheses are NOT required (the math is clear) when:
- chaining the SAME operator: `A and B and C`, `A + B + C`, `A bitand B bitand C`;
- combining arithmetic levels: `A + B * C`, `2 * 3 ^ 2`;
- a relational over arithmetic: `A + B < C`;
- a logical over relationals: `A < B and C < D` (relationals resolve first).

### Verifiability (CANON-20 tests)
- tests/parser/valid/precedence-relational-logical.inox  (`A < B and C < D` ok)
- tests/parser/valid/precedence-logical-grouped.inox     (parenthesized mix ok)
- tests/parser/valid/precedence-bitwise-grouped.inox     (parenthesized mix ok)
- tests/parser/invalid/precedence-mix-and-or.inox        (mix -> error)
- tests/parser/invalid/precedence-mix-and-xor.inox       (mix -> error)
- tests/parser/invalid/precedence-mix-bitand-bitor.inox  (mix -> error)
- tests/parser/invalid/precedence-mix-bitand-shl.inox    (mix -> error)
- tests/diagnostics/range-non-associative.inox           (`A..B..C` -> error)
- tests/diagnostics/in-non-associative.inox              (`X in A in B` -> error)
- tests/semantic/valid/precedence-in-range-additive.inox (`X in 1..N + 1`)
- tests/semantic/valid/precedence-in-relational.inox     (`X in S = Flag`)
- examples/operator-precedence.inox                      (worked demonstration)


# ============================================================================
# SECTION 21 - ARRAYS, VECTORS, SETS AND FUTURE TYPES
# ============================================================================

## CANON-13. AGGREGATES — ARRAY, VECTOR, RANGE, ENUM, SET (was language-reference + type-system; ADR-0006 #9-15)

### Array
    Values Array[1..10] Integer
    Matrix Array[1..10, 1..10] Integer
    Matrix[I, J] := 42
Fixed-size, bounds-checked, VALUE type. `Low(A)`, `High(A)`, `Length(A)` are
compile-time constants for fixed arrays. Low/High are part of the type. Array
literals are reserved for later.

### Vector (future)
    Items Vector[Integer]
`Vector[T]` is dynamic, 0-based, heap/runtime-managed, bounds-checked, distinct
from `Array`. Semantic direction is OWNERSHIP/MOVE: assignment and by-value
passing move the vector O(1) and invalidate the source. Deep copy requires
`Clone`. No implicit aliasing.

### Range
    Type
        TMonthRange Range 1..12
        TLetterRange Range 'A'..'Z'
`Range` does not open a block and does not close with `;`.

### Enum
Short: `TCardSuit (Club, Diamond, Heart, Spade)`.
Multi-line:
    Type
        TDayOfWeek Enum
            Monday
            Tuesday
            ...
        ;
Enums are nominal and ordinal. NO implicit conversion to/from `Integer`. Values
start at 0 by default. `Ord(E)` returns the ordinal. `TEnum(I)` converts
explicitly with bounds check/trap. Enum ranges are valid in `for`. Enum variable
declarations follow STRICT ADA init (require explicit initializer; see CANON-5).

### Set
    Suits Set[TCardSuit]
`Set[T]` is a finite mathematical set over a nominal ordinal base. `T` must be an
`Enum` or finite `Range`. `Set[Integer]`, `Set[Float]`, `Set[String]` are
INVALID. Sets are value types. Default is empty. Membership uses `in`. Equality
uses `=` and `#`. Subset/superset may use `<=` and `>=`. Canonical operations:
`Union`, `Intersection`, `Difference`, `SymmetricDifference`, `With`, `Without`.
Literal `[A, B, C]` is planned and contextual, but may be deferred. It is NOT a
generic hash set.


# ============================================================================
# SECTION 22 - STANDARD LIBRARY STATUS
# ============================================================================

## CANON-18. STANDARD LIBRARY (was language-reference §Standard library + runtime)

The initial portable 0.1 standard library lives under `stdlib/`:
- `Std.Core` — conceptual prelude/core; anchors fundamental names and compiler
  intrinsics such as future array-bounds operations. Conceptually IMPLICIT.
- `Std.IO` — canonical `Put`/`PutLn` output and minimal `Get`/`GetLn` Integer input facade. Explicit `Use Std.IO`.
- `Std.Math` — strategic mathematical library surface. The initial serious
  layer includes pure Inox Integer helpers such as `Min`, `Max`, `Clamp`,
  `EnsureRange`, `InRange`, `Sign`, `IsEven`, `IsOdd`, `Sqr`, `Cube`, `Gcd`,
  and `Lcm`, plus a temporary Float64 elementary-math surface lowered through
  LLVM/libm (`Sqrt`, `Cbrt`, `Sin`, `Cos`, `Tan`, `ArcSin`, `ArcCos`,
  `ArcTan`, `ArcTan2`, `Sinh`, `Cosh`, `Tanh`, `Exp`, `Ln`, `LnXP1`,
  `Log2`, `Log10`, `LogN`, `Power`, `Floor`, `Ceil`, `FMod`, `Hypot`,
  `Hypot3`, and angle conversions). Explicit `Use Std.Math`.
- `Std.Debug` — documents future `Assert`; intentionally UNAVAILABLE until
  trap/abort behavior is canonical.
The standard library must remain portable across Windows and Linux and must not
depend on GC, unsafe features, C interop, or a complex runtime. The current
`stdlib/` is an early layer + documentation anchor, NOT a complete standalone
runtime library.

## CANON-17. I/O — Put / PutLn / Get / GetLn (was language-reference + runtime §Output)

`Put`/`PutLn` are exposed canonically through `Std.IO`. They accept ONE OR MORE
arguments, Delphi/Object Pascal style, emitted SEQUENTIALLY — this is NOT string
concatenation and does NOT allocate a combined intermediate string. `PutLn`
appends exactly one newline after the final argument.
    Put("J=", J)
    PutLn("Ciclo numero ", J)
    PutLn("A", 10, "B", true)

`Get`/`GetLn` are the minimal console input surface for Inox 0.1. Empty
parentheses remain INVALID: `Get()` and `GetLn()` are rejected by the general
empty-parentheses rule. No-argument calls use no parentheses:
    Get
    GetLn

Initial 0.1 input semantics:
- `Get(X)` reads one value from stdin into mutable variable `X`;
- `GetLn(X)` reads one value from stdin into mutable variable `X` and consumes
  the rest of the current input line;
- `Get(A, B, C)` and `GetLn(A, B, C)` read values sequentially;
- `Get` without arguments reads/discards one input token/unit;
- `GetLn` without arguments reads/discards through end-of-line and is the
  Pascal-like console pause/read-line form;
- the first supported input types are `Integer`/`Int64`;
- input arguments must be assignable variables, not literals or expressions.

`ReadLn` returning `String`, String input, EOF modeling, encoding errors, and
`Result[T, E]`-based I/O errors are DEFERRED until the runtime ABI is settled.
The current implementation lowers `Put`/`PutLn` through C runtime `printf` and lowers minimal Integer `Get`/`GetLn` through internal LLVM helper functions built on C runtime `getchar`. This is a temporary ABI.


# ============================================================================
# SECTION 23 - RUNTIME STATUS
# ============================================================================

## CANON-19. RUNTIME MODEL (was canonical/runtime.md)

### Bootstrap exception runtime (v3.17)
`libinoxrt` now provides the compiler-facing exception transport ABI:
`__inox_raise`, exception capture/type query/release, and rethrow. Generated Inox
code targets these Inox-owned entry points rather than scattering C++ ABI calls
through the frontend. The Unix-like bootstrap implementation currently uses the
C++/Itanium unwinder underneath; that is an implementation detail and not part of
the source-language contract. A full Inox RTTI system is NOT required for this
mechanism: standard exceptions use compact nominal RuntimeTypeId values.

Three different things must NOT be confused:
  1. The Inox compiler executable (`inox.exe` / `inox`) — currently a native C++
     program. On Windows a Release build may require the MSVC Redistributable. It
     does not require LLVM dynamic libraries merely to start.
  2. The Inox standard library (`stdlib/Std.*.inox`) — beginning of the stdlib,
     not yet a complete runtime library.
  3. The Inox language runtime (`libinoxrt`, `inoxrt.lib`, or equivalent) — a
     minimal bootstrap now exists for exception transport. Its broader future
     scope may define startup, traps, allocation, strings, Unicode, arrays,
     vectors, I/O, and platform services.

Standard-library discovery order (supports source tree AND prebuilt ZIP):
  1. `INOX_STDLIB`, when set.
  2. `stdlib/` next to the release package root (executable under `bin/`).
  3. `stdlib/` next to the executable.
  4. `stdlib/` under the current working directory.
  5. `stdlib/` in the source file directory or one of its parents.

Build artifact directory: `build/inox-artifacts/`. `INOX_OUTPUT_DIR` overrides.
This is generated output and must NOT be versioned.
Runtime traps/errors include division by zero, checked integer overflow, invalid
shift counts, invalid `for` steps, negative integer exponents, bounds errors, range
errors, and invalid integer input. Arithmetic faults follow DECISION P-A.
Out of scope for the 0.1 safe core: raw `Pointer[T]`; `unsafe` blocks; direct C
interop; final ABI; complete standalone runtime library; arenas; borrow checker;
deterministic destructors/finalizers; full concurrency runtime.

### Checked integer arithmetic (v3.18)
Integer overflow is never wraparound and never undefined behavior (ADR-0006).
Compile time:
- an integer literal must fit Int64; `-9223372036854775808` is the only form that
  uses the magnitude 2^63; a hexadecimal literal above Int64.Max is an error;
- constant integer expressions are folded; overflow in `+ - *`, unary `-`, `div`,
  `^`, a zero divisor in `div`/`mod`, a shift count outside 0..63, a negative
  exponent and a constant `for` step <= 0 are compile-time errors;
- a module `Const` with an Integer value is a constant in these rules, so
  `Max + 1` with `Const Max := 9223372036854775807` is a compile-time error;
- a CONSTANT right operand that can never be valid is a compile-time error even
  when the left operand is only known at run time: `A div 0`, `A mod 0`,
  `A shl 64`, `A ^ (-1)` (CANON-8: "constant divisor zero is a compile-time
  error");
- “constant” in these rules means a constant expression under the canonical
  constant-expression rules. Flow-sensitive knowledge about an ordinary local
  does NOT turn a run-time expression into a compile-time error; for example,
  `X := 0` followed by `A div X` remains a run-time checked operation;
- the valid shift-count range follows the semantic bit width of the left operand.
  For the current `Integer` (= Int64) this is 0..63; fixed-width integer types use
  their own width when their arithmetic lowering is implemented.
Run time (these operations trap):
- `+ - *`, unary `-` and `Abs` on overflow (`Abs(Int64.Min)`, `-Int64.Min`);
- `div`/`mod` with a zero divisor; `Int64.Min div -1` (`Int64.Min mod -1` is 0);
- `shl`/`shr` with a count outside 0..63 (`shl` discards high bits; that is a bit
  operation, not an overflow);
- `^` on overflow or a negative exponent; a `for` step <= 0.
`for I in A..B step S` ends when the next value of `I` does not fit in Int64, so a
range ending at Int64.Max terminates.

#### Runtime arithmetic faults are deterministic Inox traps
DECISION P-A (approved by Marcelo Fortes, 2026-10-09). Language law:
- An arithmetic fault detected at run time terminates the program immediately and
  deterministically, with a diagnostic that identifies the category of the fault
  and a non-zero exit status.
- A trap is NOT an exception. There is no unwinding: `try`/`except` (typed
  handlers, `Else` and plain `except`) cannot catch it and `ensure` blocks do not
  run.
- "Trap" does not mean "let the CPU fail": the compiler emits an explicit check
  before every operation that can fault and never relies on a hardware exception.
- Two distinct categories, tested separately:
    compile-time arithmetic fault  -> compiler diagnostic (rules above)
    run-time arithmetic fault      -> deterministic Inox trap (this section)
- `DivisionByZero`, `OverflowError` and `ArithmeticError` stay in the CANON-15
  taxonomy for APIs that may raise them in the future. Primitive arithmetic does
  not raise them.

Diagnostic text (the category names are part of the contract and are checked by
`tests/runtime/*.trap`):
    Inox runtime error: integer overflow
    Inox runtime error: division by zero
    Inox runtime error: invalid shift count
    Inox runtime error: for-loop step must be positive
    Inox runtime error: negative exponent
    Inox runtime error: invalid integer input        (Get/GetLn, see CANON-17)

Lowering note (Layer B, not language law): every check calls one internal IR
function, `__inox_arith_fault(i32 kind)`, `noreturn nounwind cold`. It calls
`fflush(NULL)` so that output already written by the program is not lost, writes
the diagnostic to file descriptor 2 with `write` (`_write` on Windows; chosen by
the portability layer, `support::nativeErrorWriter`) and ends the process with
`_exit(70)` (`codegen::kRuntimeFaultExitStatus`). Status 70 is reserved by Inox
for deterministic runtime faults; its numeric value intentionally coincides with
BSD `EX_SOFTWARE` on platforms that define `sysexits`, but the Inox contract does
not depend on that convention. The emitted
IR therefore links against the C library alone, with no Inox runtime library.
`inox --run` adds "inox: program stopped by an Inox runtime error (exit code 70)".
Turning faults into exceptions later would be a change to this one function plus
`invoke` at call sites inside `try`; it would also make every program depend on
the exception runtime (see B-GAPS EH-v3.16a), which is why it was not chosen.

### Compiler implementation limits (v3.18)
The compiler rejects, with the diagnostic "maximum expression nesting depth
exceeded" / "maximum statement nesting depth exceeded", programs nested deeper
than:
- 128 levels of expression tree (`kMaxExpressionDepth`; for example a chain of
  127 additions, or 126 prefix `not`, inside a call),
- 256 nested expression-parser calls (`kMaxExpressionNesting`; one level of
  parentheses costs three calls, so about 83 nested parentheses),
- 64 levels of nested statements (`kMaxStatementNesting`).
The values are measured, not estimated. With a 1 MiB stack (the Windows main
thread default) and the largest stack frames we build (Debug + AddressSanitizer),
the compiler crashed at about 176 nested parentheses and 244 levels of operator
chains before these limits existed; each limit is about half of the measured
crash point. Plain Debug survives about 280/340. `tests/diagnostics/limit-*`
check one level past each limit and `tests/*/limits-boundary-ok` check the
deepest accepted program; both are generated at the exact boundary. Type
declarations are parsed iteratively and need no separate depth limit today.
If a pass becomes more recursive, re-measure (`ulimit -s 1024` on Unix) before
raising a limit.

## Temporary console I/O ABI

In the current implementation, `Put`/`PutLn` are lowered through the C runtime `printf`, and `Get`/`GetLn` Integer input is lowered through internal LLVM helper functions based on `getchar` (`__inox_read_i64`, `__inox_discard_token`, `__inox_discard_line`). This is a temporary ABI, not the final Inox runtime design. String input, EOF modeling, encoding, and `Result[T,E]` remain deferred.

In v3.18 Integer `Get`/`GetLn` is strict: leading whitespace (space, tab, CR, LF) is
skipped; an optional `-` is followed by at least one decimal digit; the token must
end at whitespace or end of input. EOF before a token, a malformed token, trailing
non-whitespace characters (`42abc`, `+5`) or a value outside Int64 trap with
"Inox runtime error: invalid integer input" (CANON-19 trap model).


# ============================================================================
# SECTION 24 - LLVM BACKEND SUPPORT MATRIX
# ============================================================================

## CANON-20. LLVM BACKEND (was canonical/llvm-backend.md)

LLVM is the OFFICIAL backend. Current implementation uses TEXTUAL LLVM IR for
incremental validation (readable, testable, accepted by Clang). The compiler must
remain portable C++20; host-specific code belongs in CMake or scripts.
- `div` -> `sdiv`; `mod` -> `srem`. Integer `/` is not valid Inox and must NOT
  lower to `sdiv`.
- Do NOT emit `nsw`/`nuw` until overflow checking and optimization policy are
  proven.
- Structs may lower to LLVM aggregate types. Ordinary struct params/returns are
  values. Associated-method receivers may lower to pointers for convenience —
  not reference semantics.
Current smoke-test backend supports a restricted executable subset: scalar
integer/bool ops, local variables, selected control flow, simple structs,
associated methods, field defaults, struct values, subroutines, temporary
`printf`-based output.
Temporary native driver: `inox --build file.inox` emits textual IR under
`build/inox-artifacts/` and invokes external `clang` to create a native exe.
Exception-enabled modules currently use `clang++` plus the bootstrap static
`libinoxrt`; ordinary modules retain the existing `clang` path. `inox --run
file.inox` builds and executes. `INOX_OUTPUT_DIR` overrides. The
driver loads local `Use` dependencies recursively (checks `A.B.inox` then
`A/B.inox` relative to entry dir and stdlib path), rejects cycles, emits the 0.1
subset as one textual LLVM module — a minimum module model, not a package manager
or final linker. A prebuilt compiler can parse/type-check/emit-IR without LLVM or
Clang installed.
Future backend work: final runtime ABI; richer module linking/exports/visibility/
package search; arrays/enums/ranges/sets/char/strings beyond literals; remaining
runtime-fault infrastructure; final toolchain discovery; optional migration from
textual IR to the LLVM C++ API where justified.

## Current backend support status

## B-WORKS. WHAT WORKS TODAY (verified in source audit)
Lexer: 48 keywords (`step` added by ADR-0010), case-insensitive normalization, `$XX` hex, `==` comments.
Parser: recursive descent, 2-token lookahead for typed local declarations.
Semantic: scoped symbol table, forward signature pass, inference (empty type +
  initializer -> initializer type), prelude calls (Put/PutLn/Clamp/Min/Max),
  struct/method resolution, loop-iterator read-only enforcement, shadowing
  rejection, integer `/` rejection.
Codegen (textual LLVM IR): integer/bool/Float64 scalars, locals
  (alloca/store/load), Float64 arithmetic and comparisons, checked Integer `^`
  through a backend helper, if/elif/else, while, repeat/until, for-range (+step),
  leave/continue, functions, subroutines, structs, field defaults, struct
  values, associated methods, Put/PutLn via printf including Float64; Get/GetLn
  Integer input via internal getchar-based LLVM helpers; core exception lowering
  (`try`/plain-or-typed `except`/`Else`/`ensure`/`Raise`/`Retry`) via `libinoxrt`. Elementary Float64 math
  functions are temporarily lowered through LLVM intrinsics and libm/CRT symbols.
Driver: --parse-only, --dump-tokens, --dump-types, --emit-llvm, --build, --run.
Types registered (31): Bool, Int8/16/32/64, UInt8/16/32/64, Natural, Float32/64,
  Currency, Crypto, Char, String + aliases Integer->Int64, UInteger->UInt64,
  Float->Float64 + 12 standard nominal exception types.

## B-PARSED. PARSED BUT NOT FULLY LOWERED
- `case`/`otherwise` (parsed and checked per ADR-0011, including coverage; LLVM
  lowering pending; enum coverage pending with Enum; Byte selectors pending
  with Byte).
- `unless` (parsed; not lowered).
- Enum short/block forms (parsed; not lowered; strict-init not enforced).

## B-GAPS. CONFORMANCE GAPS (Layer A says it should exist; code doesn't yet)
- BE-v3.18: constructs accepted by semantic analysis but not lowered by the
  LLVM backend are reported as "not yet implemented in the LLVM backend" and
  are measured by `tools/backend_gaps.py` (list in `docs/BACKEND_GAPS.md`):
  `case`, `unless`, module `State` in expressions, `String` locals, and a
  module `Const` whose value is not a single Integer or Bool literal. Nested loops and `if`/`elif`/`else`
  or local declarations inside loop bodies are lowered since v3.21. Any other
  codegen failure after semantic acceptance is a BUG.
- EH-v3.16a: the current native exception lowering is validated on Unix-like
  Itanium-ABI hosts. Windows/MSVC-style LLVM funclet (`catchswitch`/`catchpad`/
  `cleanuppad`) lowering remains to be implemented before exception-enabled
  Windows binaries can be claimed supported. This does not affect parsing,
  semantic checking, or non-exception Windows programs.
- EH-v3.17b: `ensure` cleanup edges now cover the currently supported nonlocal
  transfers `Return`, `Exit`, `leave`, and `continue`, including nested ensure
  regions, in addition to exceptional flow and Retry.
- EH-v3.17c: the optional `On Name Type` binding is scoped/type-checked, but
  runtime payload fields/reflection and user-defined exception declaration
  syntax remain deferred.
1. v2 variable model: user-visible `Var` syntax CLOSED (v3.23). The parser
   rejects `Var` blocks, `var`/`mut var` declarations and a module-level `Var`
   section with a migration diagnostic; `Var` and `mut` stay reserved
   (CANON-4/CANON-5); `SectionKind::Var` was removed. Residual implementation
   debt: the AST node `VarBlockStatement` keeps its old name; it is reused as the
   container for grouped inline declarations.
2. `with` (CANON-11): IMPLEMENTED (v3.15) — keyword, parse, semantic, LLVM codegen. CLOSED.
3. Scalar-requires-initializer enforcement (CANON-5 rule 3): CLOSED (v3.23)
   for locals and `State` declarations: "scalar declaration requires
   initializer: Name"; structs may omit `:=`.
4. Enum strict init (CANON-5 rule 5) — not enforced.
5. `Byte`->UInt8 alias not yet registered (CANON-8).
6. Natural range semantics (floor 0, negative=error) — name registered, check missing.
7. Currency arithmetic (i64 x10^6) — name registered, behavior missing.
8. Crypto arithmetic (i128 x10^18) — name registered, behavior missing, needs tests.
9. BigCurrency — not present; needs big-integer runtime (0.2+).
10. case LLVM lowering + enum exhaustiveness.
11. Array[Low..High] — not parsed or lowered.
12. Range/Enum/Set full implementation.
13. Checked integer arithmetic traps. — IMPLEMENTED (v3.18); see CANON-19.
14. Explicit type conversions `TypeName(x)`.
15. String indexing/concatenation/Unicode runtime.
16. Module exports/visibility.
17. Vector runtime (move semantics, Clone).
18. Final I/O ABI (replace printf/getchar helpers); String input and ReadLn.
19. Std.Debug.Assert (needs canonical trap/abort).
20. Std.Math integer helpers `Lcm`, `Sqr`, `Cube` overflow handling — CLOSED
    (v3.19 status audit). They are implemented in Inox source over checked integer
    arithmetic and therefore inherit DECISION P-A. Direct runtime verification with
    overflowing inputs confirmed deterministic `integer overflow` traps; no Std.Math
    implementation correction was required.
21. Float `Const` lowering: a `Const` of Float type passes semantic analysis but
    fails codegen ("unsupported expression"). Because of this, Std.Math exposes
    `Pi`/`Tau`/`E` as zero-argument functions rather than constants. Revisit them
    as `Const` once Float constant lowering is implemented.
22. Std.Math is still incomplete beyond the initial serious layer: arrays/vectors
    are required for statistics; Currency/BigCurrency are required for serious
    finance; final IEEE NaN/Infinity/rounding/exception policy remains open;
    many Float functions currently depend on LLVM/libm rather than Inox kernels.


# ============================================================================
# SECTION 25 - EXAMPLES POLICY
# ============================================================================

## Examples policy

Examples must be classified honestly. A release example is not valid merely because it parses.

- Runnable examples must build and run with deterministic output, unless explicitly marked interactive.
- Frontend-only examples may test parser/semantic behavior that the backend does not yet lower.
- Interactive examples using `Get`/`GetLn` require controlled stdin for automated validation.
- Examples using unimplemented features must not be advertised as runnable release examples.
- `dist/` examples must remain synchronized with `examples/`.

## B-EXAMPLES. EXAMPLES STATUS (against current compiler + v2 rules)
The top-level LLVM smoke examples have been repaired to be deterministic and compatible with the current executable subset. The repository regression suite currently validates all registered examples and integration fixtures on Linux.

Interactive examples using `Get`/`GetLn` must be tested through controlled stdin fixtures, not by requiring a human at the terminal.

Aspirational / frontend-only examples using unimplemented features must be clearly marked and must not be shipped as runnable release examples until backend/runtime support exists.


# ============================================================================
# SECTION 26 - TESTING POLICY
# ============================================================================

## CANON-21. TESTING STRATEGY (was canonical/testing.md)

The test suite is PART OF the Inox language specification. Every parser,
semantic, codegen, runtime, or documentation change must pass the full suite
before commit. Capture regressions as small fixtures in the most specific layer
that exposes the bug.
Layers:
- `examples/*.inox` — small valid programs; double as smoke tests; stay readable.
- `tests/lexer/{valid,invalid}/*.inox` — tokenization (`--dump-tokens`) and
  lexical errors (unterminated literals, invalid chars). Prove spelling,
  normalization, case-insensitivity, literal scanning, diagnostics.
- `tests/parser/{valid,invalid}/*.inox` — syntax (`--parse-only`); must reject
  bad syntax before semantic analysis. Prove `Type` without `;`, inline
  declarations, `if/elif/else` with one final `;`, control structures rejecting
  `:`, `Var` keyword rejected.
- `tests/semantic/{valid,invalid}/*.inox` — name resolution, type checking,
  mutability, flow constraints.
- `tests/codegen/*.inox` — LLVM IR emission; each file registered in both runners
  with explicit required IR fragments.
- `tests/runtime/*.inox` — execution tests: `NAME.out` (exit 0 + exact output),
  `NAME.trap` (must compile, run and trap), optional `NAME.in` (stdin). Pins the
  checked-arithmetic rules.
- `tests/diagnostics/*.inox` — must be rejected; `NAME.err` holds a substring the
  error output must contain, so the *reason* is tested, not just the rejection.
- `tests/integration/` — end-to-end with expected stdout; when `clang` is
  available, emit IR, link, execute, compare; else report `[SKIP]` without
  failing the frontend suite. Also verifies `Use Std.Math` resolves through the
  stdlib search path and runs via the Clang-backed `--run`.
Portability: tests must not depend on host-specific absolute paths.
Compiler modes used by tests: `--dump-tokens`, `--parse-only`, `--dump-types`,
`--emit-llvm`, `--build`, `--run`. `INOX_OUTPUT_DIR` / `INOX_STDLIB` override paths.
Regression rule: when a bug is fixed, add a fixture that would have failed before,
in the narrowest layer (lexer -> parser -> semantic -> codegen -> integration).

## Required regression layers

- C++ unit tests through CTest once the unit-test framework is integrated.
- Parser fixtures for syntactic acceptance/rejection.
- Semantic fixtures for type/scope/builtin rules.
- Codegen/integration fixtures for LLVM lowering and executable behavior.
- Release/example validation scripts.


# ============================================================================
# SECTION 27 - RELEASE VALIDATION POLICY
# ============================================================================

## Release validation policy

Before publishing a package, the project must validate the repository build and the extracted release package as a real user would use it. A release is invalid if shipped examples do not match their advertised status.

Minimum release gate:

```text
1. configure build
2. build compiler
3. run compiler tests
4. run parser/semantic/backend regression fixtures
5. validate runnable examples
6. validate frontend-only examples through check mode
7. package release
8. extract package into a clean directory
9. run package smoke tests as an end user
10. verify stdlib discovery, output directory, docs, licenses
```

## DEV-2. RELEASE / PREBUILT PACKAGES (was docs/release/*)

Packages (GitHub Release assets):
- inox-windows-x64.zip — https://github.com/fortesm/Inox/releases/latest/download/inox-windows-x64.zip
- inox-linux-x64.zip   — https://github.com/fortesm/Inox/releases/latest/download/inox-linux-x64.zip

Windows quick start:
    Expand-Archive .\inox-windows-x64.zip -DestinationPath C:\Tools
    cd C:\Tools\inox-windows-x64
    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
    .\set-inox-env.ps1
    inox .\examples\hello.inox
Linux quick start:
    unzip inox-linux-x64.zip -d "$HOME/tools"
    cd "$HOME/tools/inox-linux-x64"
    source ./set-inox-env.sh
    inox ./examples/hello.inox
The setup script sets PATH (includes bin/), INOX_STDLIB (-> stdlib/),
INOX_OUTPUT_DIR (-> output/).

Package layout (Windows / Linux analogous):
    inox-<plat>-x64/
        README.md
        set-inox-env.(ps1|sh)
        bin/inox(.exe)
        stdlib/
        examples/
        output/
        docs/ (or manual/index.html)   == bundled documentation
        licenses/
The `stdlib/` directory MUST ship with the compiler (used by `Use Std.*`). The
bundled documentation must be a copy of THIS file (or generated from it). The old
docs/LANGUAGE_REFERENCE.md + docs/index.html are superseded; regenerate any
shipped doc from INOX_CANONICAL.md.


# ============================================================================
# GROUP: FUTURE (absorbs docs/future/* — all were EMPTY files; topics preserved)
# ============================================================================
# These are deferred 0.2+ architectural topics. Per OPEN-QUESTIONS, they are NOT
# permission to invent semantics; each requires an explicit, approved ADR before
# implementation. The source files were empty; their TOPICS are recorded so no
# direction is lost.


# ============================================================================
# SECTION 28 - DOCUMENTATION GENERATION POLICY
# ============================================================================

## Documentation generation policy

- This file is the source of truth.
- README files, HTML manuals, tutorials, package docs, and release notes are derivative.
- Generated/user-facing documentation must not introduce language rules that contradict this file.
- Stale generated documentation must be deleted or regenerated.
- If a user-facing document conflicts with this file, this file wins and the user-facing document has a bug.
- Public docs should be regenerated from this canonical source or manually audited against it before release.

Known current conflicts are tracked in SECTION 30 / B-CONFLICTS.


# ============================================================================
# SECTION 29 - PORTABILITY TARGETS
# ============================================================================

## DEV-1. TOOLCHAIN REQUIREMENTS (was development/toolchain.md)

Usage profiles:
- Inox compiler developer: needs Clang/LLVM + C++ toolchain. Builds inox(.exe)
  from C++ source.
- Prebuilt-compiler user: needs nothing beyond the Release executable + normal OS
  runtime libraries.
- `--build`/`--run` user: needs a native toolchain (`clang` in PATH) for now; the
  driver delegates final native executable generation to external tools.

Compiler developers — Windows recommended setup: Git for Windows, PowerShell 7,
CMake, Ninja, LLVM/Clang, Visual Studio Build Tools with C++, Windows SDK.
    cmake -S . -B build -G "Ninja Multi-Config" -DCMAKE_CXX_COMPILER=clang++
    cmake --build build --config Debug
    pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
Windows build uses Clang with MSVC ABI target `x86_64-pc-windows-msvc`. VS Build
Tools provide SDK headers, import libs, MSVC STL, MSVC/UCRT runtime, linker.

Compiler developers — Linux/Unix:
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
    cmake --build build
    bash scripts/run-tests.sh

Users testing a prebuilt compiler need no LLVM/Clang/CMake/Ninja/C++ to run:
    inox examples/hello.inox
    inox --parse-only examples/hello.inox
    inox --emit-llvm examples/llvm-put-output-basic.inox
On Windows a Release build may require the MSVC Redistributable. A DEBUG build
must NOT be distributed (depends on non-redistributable Debug DLLs such as
MSVCP140D.dll, VCRUNTIME140D.dll, ucrtbased.dll).

Users building native programs need `clang` in PATH for now:
    inox --build examples/llvm-put-output-basic.inox
    inox --run examples/llvm-put-output-basic.inox
This does not require building the compiler source; the driver delegates final
native codegen and linking to an external platform toolchain. Future SDK work may
package backend/linker tools with Inox or replace this path.

## Portability targets

Inox must be engineered as a modern cross-platform C++ compiler, not as a codebase with ad-hoc platform branches scattered across the frontend and backend. Platform-specific code belongs in dedicated support modules, CMake configuration, and scripts.

Primary validation targets:
- Windows x64, Clang with MSVC ABI (`x86_64-pc-windows-msvc`).
- Linux x64, Clang.

Qualification: support claims must distinguish build/link from actual execution.
For the v3.18 hardening pass, Linux was built and executed with the complete suite;
Windows was cross-built and linked only in that environment. EH-v3.16a remains: native
Windows exception-enabled binaries are not yet claimed supported until LLVM funclet
lowering is implemented and validated on Windows.

Planned / stub targets, not yet advertised as supported:
- macOS.
- FreeBSD.
- NetBSD.
- OpenBSD.
- Illumos.
- Solaris.
- AIX.
- HP-UX.
- UnixWare.
- Android.

Structural policy:
- Common compiler code must stay platform-neutral C++20.
- `#ifdef`/platform probes are allowed in `src/compiler/support/`, `cmake/`, build scripts, and small platform-detection headers only.
- Parser, AST, semantic analyzer, diagnostics, and ordinary codegen logic must not grow host-OS branches.
- Current support modules live under `src/compiler/support/`:
  - `Environment.*` for environment variables (`_dupenv_s` on Windows, `std::getenv` on POSIX-like hosts).
  - `FileSystem.*` for executable-path discovery and filesystem helpers.
  - `Platform.*` for host OS classification, null-device path, executable suffix.
  - `Process.*` for shell command execution and tool discovery.
- Build policy lives under `cmake/`:
  - `InoxPlatform.cmake`.
  - `InoxCompilerOptions.cmake`.
  - `InoxWarnings.cmake`.
  - `InoxSanitizers.cmake`.
  - `InoxInstall.cmake`.
  - `cmake/toolchains/` for real and stub platform toolchains.
- `CMakePresets.json` defines the validated Windows/Linux flows.

Targets must not be claimed as supported until tested on real or representative systems. Docker validates Linux distributions, not non-Linux kernels such as FreeBSD, macOS, Solaris/Illumos, AIX, HP-UX, or UnixWare. Stub directories/toolchains document intent; they are not support claims.


# ============================================================================
# SECTION 30 - FEATURE STATUS MATRIX
# ============================================================================

This section is volatile implementation status. It may be updated to match the code. It must not override constitutional language rules above.

## B-WORKS. WHAT WORKS TODAY (verified in source audit)
Lexer: 48 keywords (`step` added by ADR-0010), case-insensitive normalization, `$XX` hex, `==` comments.
Parser: recursive descent, 2-token lookahead for typed local declarations.
Semantic: scoped symbol table, forward signature pass, inference (empty type +
  initializer -> initializer type), prelude calls (Put/PutLn/Clamp/Min/Max),
  struct/method resolution, loop-iterator read-only enforcement, shadowing
  rejection, integer `/` rejection.
Codegen (textual LLVM IR): integer/bool scalars, locals (alloca/store/load),
  if/elif/else, while, repeat/until, for-range (+step), leave/continue, functions,
  subroutines, structs, field defaults, struct values, associated methods,
  Put/PutLn via printf; Get/GetLn Integer input via internal getchar-based LLVM
  helpers; core exception lowering through `libinoxrt`.
Driver: --parse-only, --dump-tokens, --dump-types, --emit-llvm, --build, --run.
Types registered (31): Bool, Int8/16/32/64, UInt8/16/32/64, Natural, Float32/64,
  Currency, Crypto, Char, String + aliases Integer->Int64, UInteger->UInt64,
  Float->Float64 + 12 standard nominal exception types.

## B-PARSED. PARSED BUT NOT FULLY LOWERED
- `case`/`otherwise` (parsed and checked per ADR-0011, including coverage; LLVM
  lowering pending; enum coverage pending with Enum; Byte selectors pending
  with Byte).
- `unless` (parsed; not lowered).
- Enum short/block forms (parsed; not lowered; strict-init not enforced).

## B-GAPS. CONFORMANCE GAPS (Layer A says it should exist; code doesn't yet)
- BE-v3.18: constructs accepted by semantic analysis but not lowered by the
  LLVM backend are reported as "not yet implemented in the LLVM backend" and
  are measured by `tools/backend_gaps.py` (list in `docs/BACKEND_GAPS.md`):
  `case`, `unless`, module `State` in expressions, `String` locals, and a
  module `Const` whose value is not a single Integer or Bool literal. Nested loops and `if`/`elif`/`else`
  or local declarations inside loop bodies are lowered since v3.21. Any other
  codegen failure after semantic acceptance is a BUG.
- EH-v3.16a: the current native exception lowering is validated on Unix-like
  Itanium-ABI hosts. Windows/MSVC-style LLVM funclet (`catchswitch`/`catchpad`/
  `cleanuppad`) lowering remains to be implemented before exception-enabled
  Windows binaries can be claimed supported. This does not affect parsing,
  semantic checking, or non-exception Windows programs.
- EH-v3.17b: `ensure` cleanup edges now cover the currently supported nonlocal
  transfers `Return`, `Exit`, `leave`, and `continue`, including nested ensure
  regions, in addition to exceptional flow and Retry.
- EH-v3.17c: the optional `On Name Type` binding is scoped/type-checked, but
  runtime payload fields/reflection and user-defined exception declaration
  syntax remain deferred.
1. v2 variable model: user-visible `Var` syntax CLOSED (v3.23). The parser
   rejects `Var` blocks, `var`/`mut var` declarations and a module-level `Var`
   section with a migration diagnostic; `Var` and `mut` stay reserved
   (CANON-4/CANON-5); `SectionKind::Var` was removed. Residual implementation
   debt: the AST node `VarBlockStatement` keeps its old name; it is reused as the
   container for grouped inline declarations.
2. `with` (CANON-11): IMPLEMENTED (v3.15) — keyword, parse, semantic, LLVM codegen. CLOSED.
3. Scalar-requires-initializer enforcement (CANON-5 rule 3): CLOSED (v3.23)
   for locals and `State` declarations: "scalar declaration requires
   initializer: Name"; structs may omit `:=`.
4. Enum strict init (CANON-5 rule 5) — not enforced.
5. `Byte`->UInt8 alias not yet registered (CANON-8).
6. Natural range semantics (floor 0, negative=error) — name registered, check missing.
7. Currency arithmetic (i64 x10^6) — name registered, behavior missing.
8. Crypto arithmetic (i128 x10^18) — name registered, behavior missing, needs tests.
9. BigCurrency — not present; needs big-integer runtime (0.2+).
10. case LLVM lowering + enum exhaustiveness.
11. Array[Low..High] — not parsed or lowered.
12. Range/Enum/Set full implementation.
13. Checked integer arithmetic traps. — IMPLEMENTED (v3.18); see CANON-19.
14. Explicit type conversions `TypeName(x)`.
15. String indexing/concatenation/Unicode runtime.
16. Module exports/visibility.
17. Vector runtime (move semantics, Clone).
18. Final I/O ABI (replace printf/getchar helpers); String input and ReadLn.
19. Std.Debug.Assert (needs canonical trap/abort).
20. Std.Math integer helpers `Lcm`, `Sqr`, `Cube` overflow handling — CLOSED
    (v3.19 status audit). They are implemented in Inox source over checked integer
    arithmetic and therefore inherit DECISION P-A. Direct runtime verification with
    overflowing inputs confirmed deterministic `integer overflow` traps; no Std.Math
    implementation correction was required.
21. Float `Const` lowering: a `Const` of Float type passes semantic analysis but
    fails codegen ("unsupported expression"). Because of this, Std.Math exposes
    `Pi`/`Tau`/`E` as zero-argument functions rather than constants. Revisit them
    as `Const` once Float constant lowering is implemented.
22. Std.Math is still incomplete beyond the initial serious layer: arrays/vectors
    are required for statistics; Currency/BigCurrency are required for serious
    finance; final IEEE NaN/Infinity/rounding/exception policy remains open;
    many Float functions currently depend on LLVM/libm rather than Inox kernels.

## B-CONFLICTS. KNOWN DOC/CODE CONFLICTS TO RESOLVE
- Var block still in code (B-GAPS #1): CLOSED (v3.23).
- `mut` in State in variables.inox (violates CANON-10).
- Const inline syntax vs parser requiring `:` — adopt line form, update parser.
- Public docs/LANGUAGE_REFERENCE.md + docs/index.html are stale/superseded —
  delete; regenerate any shipped doc from THIS file.
- docs/canonical/type-system.md table was incomplete — superseded by CANON-8.

## B-EXAMPLES. EXAMPLES STATUS (against current compiler + v2 rules)
The top-level LLVM smoke examples have been repaired to be deterministic and compatible with the current executable subset. The repository regression suite currently validates all registered examples and integration fixtures on Linux.

Interactive examples using `Get`/`GetLn` must be tested through controlled stdin fixtures, not by requiring a human at the terminal.

Aspirational / frontend-only examples using unimplemented features must be clearly marked and must not be shipped as runnable release examples until backend/runtime support exists.

## B-PROPOSALS. AI-SUGGESTED DESIGN CHANGES (NOT law; await approval)
(AIs append PROPOSAL entries here, dated, never inline in Layer A.)

(P-2026-10-09-A, -B and -C were APPROVED by Marcelo Fortes on 2026-10-09 and
are now law: CANON-19 "Runtime arithmetic faults are deterministic Inox traps",
CANON-12 `for` in range, and E11. See CHANGE LOG v3.18.)


# ============================================================================
# SECTION 31 - ROADMAP TO 1.0
# ============================================================================

## CANON-22. ROADMAP — 0.1 PRIORITIES (was canonical/roadmap.md §0.1)

1. Keep tests passing on Windows/MSVC and Linux/GCC/Clang.
2. Enforce decided canonical syntax (now: inline declarations + `Var` keyword
   rejected; `Type` without `:`; canonical `Self`/`Self mut`; integer `/`
   rejection; control structures reject `:`).
3. Extend the Clang-backed `--build`/`--run` driver beyond the minimum local
   module workflow.
4. Complete `case` lowering and enum exhaustiveness checks.
5. Extend local multi-file `Module`/`Use` with future export/visibility/package.
6. Implement arrays with `Array[Low..High] T`, indexing, bounds checks, `Low`,
   `High`, `Length`.
7. Implement `Enum`, `Range`, `Ord`, enum-range `for`.
8. Implement `Set[TEnum]`/`Set[TRange]` if time allows.
9. Extend portable `stdlib/` without GC/unsafe/C-interop; define canonical
   trap/abort before implementing `Std.Debug.Assert`.
(Plus v2.0/v2.2 items: remove Var block; implement `with`; Byte alias; Natural/
Currency/Crypto semantics; safe-inference messages; scalar-requires-initializer.)

# ============================================================================
# GROUP: DECISIONS (absorbs docs/decisions/ADR-0001..0006)
# ============================================================================
# These ADRs are the locked decision history. Their content is reflected in the
# CANONICAL group above; they are preserved here verbatim-in-substance for
# traceability. They change ONLY via a new approved ADR (GOVERNANCE G2).



## Roadmap to 1.0

```text
0.1.x   foundation: canonical spec, CLI, examples, tests, release validation, backend subset
0.2.x   imperative core completion and selected deferred features
0.3.x   aggregates, arrays, enums, ranges, sets, case exhaustiveness
0.4.x   standard library/runtime baseline
0.5.x   portability hardening
0.6.x   quality gates and CI maturity
0.7.x   safety/runtime-fault maturity
0.8.x   advanced composition/protocol direction
0.9.x   beta/freeze
1.0.0   stable core language, reproducible releases, coherent docs and examples
```

## BACKEND STRATEGY DIRECTIVE (LOCKED — clang as external driver)

This directive governs HOW Inox turns LLVM IR into native executables, and WHEN
that mechanism may change. It exists to prevent a specific, tempting mistake:
embedding the LLVM C++ libraries (libLLVM) into the compiler too early and
derailing language work.

### Core principle (binding)
The external `clang` driver is a BUILD-TIME dependency, NOT technical debt.
- A user who RUNS an Inox-generated executable does not need clang. Clang is only
  invoked when GENERATING that executable. This is the same model Zig used for
  years and that Rust still uses for linking (rustc calls the system linker).
- Emitting textual LLVM IR and calling clang/llc + a linker is a legitimate,
  mature stage for an LLVM-based compiler — not a sign of immaturity.
- Embedding libLLVM adds NO new language capability. It only changes HOW the same
  IR becomes a binary. It is plumbing, not design.
- libLLVM is a large dependency whose C++ API breaks across major releases.
  Embedding it early means owning that maintenance burden instead of investing in
  what differentiates Inox: the type system (Currency, Crypto, Array, Vector,
  String, Enum), the error model, overflow traps, and the runtime.

### Backend maturation sequence (intended; not a promise of timing)
```text
0.1.x   keep clang as external driver; stabilize language, tests, examples,
        minimal I/O, current textual-IR backend.
0.2.x   expand the bootstrap libinoxrt introduced by v3.16; mature remaining
        runtime-fault support, I/O, strings, and the minimal general runtime.
        Checked arithmetic already follows DECISION P-A. KEEP clang.
        (This stage is intentionally LONG: full language + runtime maturation.)
0.3.x   emit object files in a controlled way; introduce lld / linker
        integration; begin reducing clang's role. Separate "emit .o" from "link".
0.4.x   integrate LLVM libraries directly; keep --emit-llvm as a debug/dev mode.
0.5.x   cross-target / sysroot / runtime packaging; Windows/Linux solid; BSD/
        Solaris/AIX/HP-UX remain HONEST stubs.
1.0     own frontend + own runtime + integrated LLVM backend + own driver.
```

### Objective criterion for leaving clang (do NOT leave on aesthetics)
Sophistication is not the goal; capability is. The external clang driver removes
NO capability today. It may be replaced (0.3/0.4) ONLY when one of these is
concretely true — not merely when it "looks unsophisticated":
1. Clang-as-driver becomes a real bottleneck (e.g. writing `.ll` to disk and
   re-parsing is too slow for large builds), OR
2. A needed capability exists only via the libLLVM API (JIT, incremental
   codegen, fine-grained pass control), OR
3. Distribution requires a single self-contained binary with no external
   toolchain dependency.
None of these is true at 0.1/0.2. Until one is, KEEP clang and mature the
LANGUAGE and RUNTIME instead. A language with an external clang driver and a
complete type system is far more useful than one with embedded libLLVM and only
Integer/Bool.

### Scope guard for backend-adjacent work (e.g. Process.cpp)
Work that touches HOW clang is invoked (process spawning, path handling, the
`--build`/`--run` driver) is IN SCOPE for 0.1/0.2 and must NOT be used as an
on-ramp to replace clang. When editing such files, the task is "improve how we
call clang" (security, robustness, paths with spaces), NEVER "eliminate clang".
Replacing the backend is a 0.3/0.4 decision requiring an explicit ADR.

## COMPILER MODULE ARCHITECTURE DIRECTIVE (when to split C++ modules)

This directive governs HOW the compiler's own C++ source is divided into modules.
It exists to avoid two opposite failure modes: the high-coupling monolith (giant
files mixing unrelated responsibilities) and premature fragmentation (splitting
before the code has revealed where its real seams are).

### Principle: REACTIVE, not PREDICTIVE
The ideal module structure becomes visible only AFTER the code reveals its axes
of change. While the compiler is still young and growing fast (new types, Float,
arrays, enums, runtime), every new feature can redraw where the natural
boundaries lie. Designing the module split up front means redesigning it
repeatedly — work that does not become language. Therefore: do NOT reorganize
into modules preemptively or for aesthetics. Split when it hurts, extract when
the criteria below are met.

### The three tests for extracting a module (the Go/UTF-8 criteria)
Extract a piece into its own module ONLY when it passes ALL THREE:
1. COHESION — it does one well-defined thing (e.g. UTF-8 handling, symbol table,
   a single IR-emission concern), not a grab-bag.
2. STABILITY — its boundary does not change every time a language feature is
   added. A stable surface is what makes extraction pay off.
3. REUSE — it has more than one consumer, or a clearly imminent second consumer.

Go separates UTF-8 into its own package precisely because UTF-8 satisfies all
three: cohesive, stable rules, many consumers. When Inox reaches strings/Unicode,
UTF-8 will be among the first legitimate extractions for the same reason.

Already correctly extracted under this rule: the `support/` layer (Platform,
Process, FileSystem, Environment) — cohesive OS-abstraction, stable surface,
multiple consumers.

### Anti-rules (do NOT use these as reasons to split)
- File size ALONE is not a reason. A large but cohesive file with a stable
  boundary stays whole until a second responsibility appears in it.
- Aesthetics/sophistication is not a reason. Splitting to "look like a real
  compiler" is fragmentation, not engineering.
- A file that mixes TWO responsibilities that change for DIFFERENT reasons IS a
  reason to split (e.g. if one emitter file holds type/struct emission AND
  runtime-helper emission AND expression emission, those are separable concerns).

### Working rule of thumb
When a single file passes ~500-800 lines AND mixes responsibilities that change
for different reasons, divide it along the responsibility seam — not down the
middle by line count. Capture the decision in the change log if it is structural.

### Timing
Big reorganizations are deferred until the 0.2 work (runtime, strings/UTF-8,
aggregates) exposes the real seams. Until then, apply the reactive rule
per-file. A context-less AI MUST NOT launch a sweeping module refactor on its
own initiative; propose it (Section B-PROPOSALS) and let the maintainer decide.

# ============================================================================
# END OF INOX_CANONICAL.md v3.8
# A context-less AI: read SECTION 00 including the FIRST-ORDER ENGINEERING
# DIRECTIVE, SECTION 02 change log, SECTION 03 agent rules, then the topical
# SECTIONs relevant to the task, and finally SECTION 30
# for current implementation status. Never confuse design law with status.
# ============================================================================
