<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Compiler architecture: phases, contracts and owners

Status layer (it follows the code, like `docs/STATUS.md`). The rules it
details are law in `docs/openspec/specs/constitution/engineering.md` (E3,
E11, E14, E21); when this file and the law disagree, the law wins and the
difference is a deviation to list below. Written by change
`phase-contracts` (canon r3.110), approved by Marcelo Fortes, 2026-10-31.

**Read this before changing the compiler.** Find the phase your change
touches, respect its contract, and update this file in the same pull
request when a boundary, an owner or a deviation changes (OpenSpec rule:
every proposal names the phases it touches).

## The pipeline

```text
source ─▶ 1 lexer ─▶ 2 parser ─▶ 3 module resolution ─▶ 4 contract lowering
       ─▶ 5 semantic analysis ─▶ SemanticResult ─▶ 6 code generation (LLVM IR text)
       ─▶ 7 driver (clang++: object, link with libinoxrt) ─▶ executable
                                   8 runtime (libinoxrt) runs with the program
```

The driver (`main/main.cpp`) runs phases 1-2 for each module file as it is
loaded (`Use`), then 3-4 once on the whole program (`loadProgram`), then 5,
6 and 7.

## Contracts of the phases

Each phase: what it receives, what it produces, what it guarantees, what it
must never do, and the files that own it.

### 1. Lexer
- **In:** the text of one file. **Out:** tokens with line and column.
- **Guarantees:** every lexical error has a line and a column.
- **Never:** knows grammar, names or types.
- **Owner:** `lexer/`.

### 2. Parser
- **In:** the tokens of one module. **Out:** its AST.
- **Guarantees:** syntax errors with line and column; the AST says what
  was written.
- **Never:** checks types or names, resolves modules, lowers or desugars a
  construct, invents names (E11).
- **Owner:** `parser/`, `ast/`.

### 3. Module resolution
- **In:** the ASTs of every module. **Out:** one program in which every
  name means one declaration (internal qualified names).
- **Guarantees:** visibility of declarations (CANON-3); no cycle of `Use`.
- **Never:** types expressions (field and method visibility is phase 5's).
- **Owner:** `semantic/ModuleResolution.*`.

### 4. Contract lowering
- **In:** the resolved program. **Out:** the same program, with each routine
  that has `pre`/`pos` split into a checking wrapper and `__Name_body`, and
  `TName.__invariant(Self)` for each struct with invariants.
- **Guarantees:** runs once, before analysis, on the whole program
  (inherited contracts cross modules).
- **Never:** types expressions.
- **Owner:** `semantic/ContractLowering.*`.

### 5. Semantic analysis
- **In:** the lowered program. **Out:** `SemanticResult`: the type of every
  expression, every symbol and signature, the instances of generics, and
  the decisions later phases need.
- **Guarantees:** a program it accepts is valid Inox; every decision a
  later phase needs is recorded (E11, decision P-C); every diagnostic says
  where (E14).
- **Never:** emits LLVM; leaves a decision for codegen to rediscover.
- **Owner:** `semantic/` (one file per responsibility, map below), `types/`.

### 6. Code generation
- **In:** the analyzed AST and `SemanticResult`. **Out:** the LLVM IR text
  of the program.
- **Guarantees:** valid IR or a clear stop (E15): "valid Inox, not yet
  lowered" is a reported gap, never a crash.
- **Never:** derives a type, resolves a name, re-checks a rule, tests the
  host OS outside `ExceptionAbi` (E11, E12).
- **Owner:** `codegen/`.

### 7. Driver
- **In:** IR text. **Out:** an executable (clang++, libinoxrt).
- **Guarantees:** a toolchain failure shows the toolchain's own output.
- **Never:** hides a frontend failure.
- **Owner:** `main/`, `support/` (the only code that tests the host OS).

### 8. Runtime (libinoxrt)
- **Guarantees:** the faults of CANON-19, String and File ownership, the
  owned blocks and Link generations of CANON-25.
- **Limits today:** single-threaded: its state is global and its reference
  counts are not atomic (concurrency comes through its own change).
- **Owner:** `src/runtime/`.

### Who may rewrite the AST
Today: phase 4 (contracts), and in phase 5 only `SemanticReuse.cpp`
(`S.FName` becomes `S.TPerson.FName`) and `SemanticGenerics.cpp` (copies a
generic routine for each instance). The parser rewrites too, which is a
deviation (D2). No other code changes the AST.

## Deviations from the contracts

What the code does today against the law. Each has the foundation step
that removes it (ROADMAP, "Foundation"); a fix updates this table.

| # | Deviation | Law | Step |
|---|---|---|---|
| D1 | AST nodes carry no source position; semantic errors have no file, line or column | E14 | F2 |
| D2 | The parser desugars: chained assignment (`pendingStatements_`), `with` bindings (`__with_N`), loop `invariant`/`variant` (`lowerLoopContracts`) | E11 | F5 |
| D3 | The parser keeps routine headers as text tokens (`signatureTokens`), which the analyzer parses (`registerFunctionSignatureIn`) | E11 | F3 |
| D4 | Types are text in the AST (`typeName`), read by `types::TypeSyntax` in phase 5; generic instances rewrite type names as text (`TypeNameRewriter`) | E11 (steps 3-4) | F4 |
| D5 | Codegen reads the raw AST beside 13 side tables of `SemanticResult`; no typed or lowered representation between phases 5 and 6 | E11 (steps 3-5) | F6 |
| D6 | Exception handlers match the written type name; the hierarchy is not in `types::Type` | E11 | F7 |
| D7 | The quality gates of E19 are run by the agent for each pull request, not all by CI (ASan+UBSan suite, mutation fuzz, differential test, IR snapshot) | E19 | F1 |

## Source map

Moved here from `AGENTS.md` (change `phase-contracts`); first written by the
change `compiler-modularization`. Read the module that owns a concern
instead of searching the whole emitter.

- `src/compiler/lexer/` — `Lexer.*`, `Token.h`: source text to tokens, each
  with its `SourceLocation` (line, column).
- `src/compiler/parser/` — `Parser.*`: tokens to the AST of one module
  (see the contract of phase 2 and its deviations).

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
- `src/compiler/ast/ModuleInterface.*` — `inox interface`: a module's public
  surface printed as Inox from its parsed file.
- `src/compiler/semantic/ModuleResolution.*` — runs once on the parsed
  modules, before contract lowering (CANON-3, r3.99): gives declarations of
  used modules internal qualified names (`Mod::Routine`, `Mod.Type`),
  resolves every name a module writes (own first, then the one public name
  of its uses; qualified names, aliases, selective uses), and checks that
  nothing public exposes something private. Field and method visibility,
  which need expression types, are the analyzer's (`requireFieldVisible`,
  `requireMethodVisible`, `requireBuildableHere`).

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
    slots, statement temporaries) and borrowed reads: in a statement that
    calls a routine, a String read from State, a `ref` parameter, a pointer
    path or a `with` binding is retained for the statement;
  - `FunctionEmitterEnums.cpp` — enum and Range built-ins, and the table of
    literal names each enum prints from;
  - `FunctionEmitterSets.cpp` — sets as whole-byte integers, `in` (also
    `X in A..B`), the set comparisons and functions;
  - `FunctionEmitterStringFunctions.cpp` — `+` on Strings (one
    `__inox_str_concat` per chain, `__inox_str_append` in place) and the
    Std.Strings / Std.StrUtils calls, with their status out-parameter and
    the module helper `@__inox_str_fault`;
  - `FunctionEmitterAggregates.cpp` — struct defaults and construction,
    places, fields, methods, assignment;
  - `FunctionEmitterArrays.cpp` — indexing, bounds, slices, `Fill`;
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
- `contrib/` (community editor support and integrations), `include/` (public C
  headers, for FFI after 1.0), `libs/` (vendored third-party libraries, none
  today), `utils/` (programs for Inox users: formatter, package tool) — each
  README says what belongs there; development tools are in `tools/`.
- `src/runtime/` — libinoxrt: exception transport, `StringRuntime.cpp`,
  `StringFunctions.cpp` (concatenation, append and the Std.Strings /
  Std.StrUtils functions; `UnicodeTables.inc` is generated by
  `tools/gen_unicode_tables.py`, never edited by hand), `FileRuntime.cpp`, `OwnRuntime.cpp` (generation blocks and per-size free
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
  - `SemanticEnums.cpp` — the ordinal built-ins of enums and Ranges (Ord,
    Succ, Pred, Low and High of a type) and `TEnum(I)` (CANON-13);
  - `SemanticSets.cpp` — Set<T>: its base, the literal `[A, B]`, `in`, the
    six set functions (CANON-13);
  - `SemanticArrays.cpp` — `Fill(Value)` typed by its target array, and
    the diagnostic of an array declared without a value (r3.81);
  - `SemanticStrings.cpp` — Std.Strings / Std.StrUtils (CANON-14): the
    names visible with their `Use`, argument checks; the table of the
    functions is `StdStrings.h`, shared with codegen;
  - `SemanticGenerics.cpp` — generics (CANON-8): templates, inference of
    type arguments from the arguments or the target, instances of generic
    routines (copied with `ast::cloneStatements`, checked after the
    program's routines) and of generic structs; `SemanticResult` keeps the
    instances, and codegen compiles them like routines of the program;
  - `SemanticSupport.*` — shared helpers (`semantic::detail`).
