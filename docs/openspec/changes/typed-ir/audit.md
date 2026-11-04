Audit of the coupling between the compiler's phases, made with step 2 of F6
(change `typed-ir`), on main a98cc75 (2026-11-04). Only step 2 is
implemented; everything else here is a backlog for later steps, with the
step that owns it. Facts are confirmed in the code; risks and hypotheses
are marked as such.

## Confirmed facts

| Where | Category | Evidence | Impact | Severity | Remedy | Step |
|---|---|---|---|---|---|---|
| `codegen/` (106 sites, `tools/codegen_decisions.py`) | Stringly Typed Dispatch, Semantic Leakage | builtins recognized by `equalsIgnoreCase(name, "Low")`, ... | a builtin renamed or added changes codegen; the analysis's decision is redone | high | dispatch on `BuiltinOp`, which the analysis records since step 1 | F6-3 |
| `FunctionEmitterExpressions.cpp` `conversionTarget` (l. 140), the conversion test (l. 590, 1099); `FunctionEmitterEnums.cpp` (l. 38) | Architectural Boundary Violation | `semantics_.types().lookup(<written name>)`: codegen resolves a type by name | the name is resolved twice; a shadowing or module rule could make them disagree | high | the call's meaning is `Conversion` and the TIR's `Convert` has the target type | F6-3 |
| `FunctionEmitterStrings.cpp` `placeMayChange` (l. 413); `SemanticExpressions.cpp` (l. 612, 1258) | Primitive Obsession | a `with` binding recognized by the prefix `__with` | the F5 debt; a binding is a name convention | medium | `WithBinding` places (since step 1) and the origin of the symbol | F6-4 |
| `FunctionEmitterContracts.cpp` `emitInvariantOf` (l. 70) | Stringly Typed Dispatch | the invariant routine found as `<struct>.__invariant` in the signature map | a struct's invariant is reached by text | medium | the analysis records the invariant routine of a struct; `InvariantCheck` carries it | F6-4 |
| `LlvmIrEmitter.cpp` (l. 155) | Representation Leakage | Main found by `equalsIgnoreCase(name, "Main")` | the entry is decided twice | low | `tir::Program::entry` (step 1) | F6-5 |
| `codegen/AstQueries.cpp`, the walkers of `FunctionEmitterStrings.cpp` | Representation Leakage | `forEachExpressionOf` visits every AST node, callee names and encodings included, and asks their types | why the migration index must answer for names (design, "Step 2") | medium | walk the TIR's statements | F6-5 |
| `tools/codegen_decisions.py` | (tooling) | does not count `types().lookup(...)`, `rfind("__with")` or `signatures_.find(normalize(...))` | `--zero` at step 5 could pass with these left | medium | extend its patterns before step 3 relies on it | F6-3 |
| `semantic/SemanticResult.h` | Parallel Data Structures | 16 side tables keyed by AST node (inventory), plus the call meanings and declared symbols of step 1 | the backend reads several tables for one node | high | the TIR carries them; tables that serve only codegen leave | F6-5 |

## Risks (founded, not demonstrated)

- `codegen/FunctionEmitter.h` (563 lines, 29 members): one class holds
  control flow, cleanup, ownership, temporaries and exception state
  (Large Class, Shared Mutable State). Splitting it before F6 ends would
  mix two migrations. After F6.
- `semantic/SemanticAnalyzer.h` (502 lines): symbol tables, type
  contexts, generics, ownership and flow state in one class (God Object,
  Divergent Change). After F7, by responsibility, without new global
  contexts.
- `parser/Parser.cpp` (2866 lines): one class for the grammar. Splitting
  it by families of the grammar must keep the recursion limits
  (`tools/stack_frames.py`). After F7.
- `semantic/ModuleResolution.cpp` (944 lines): resolution and the AST
  rewriting it implies in one file. After F7.
- `main/main.cpp` (919 lines): CLI, module loading and the toolchain.
  Low priority.

## Stack

Frames are measured, not presumed (`tools/stack_frames.py`, clang Debug):
the largest frames of codegen's recursion are unchanged by step 2
(`emitExpressionValue` 6408 bytes, `emitTry` 5960, `emitIntrinsicCall`
5000); `FunctionEmitter::typeOf` keeps its 24 bytes. Whether steps 3-5
reduce them is to be shown by the same tool at each step; the Windows
budgets (963 KiB and 451 KiB) are checked by `windows-stack`.
