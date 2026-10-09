<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Backend gaps (semantic analysis accepts, LLVM backend does not lower)

Status: measured on 2026-10-09 against v3.18. Layer B — describes the
implementation, not the language.

## Why this file exists

CANON E11/E15: a later phase must not discover new language rules, and an
unsupported feature must stop with a clear diagnostic. Today the semantic
analyzer accepts some constructs the backend cannot lower. Until the backend
catches up, the rule is:

* the backend reports these as `codegen error: not yet implemented in the LLVM
  backend: ...` followed by `note: this program is valid Inox; the gap is in code
  generation` (C++ type `CodegenUnsupported`);
* every other codegen failure after semantic acceptance is a **bug**;
* the list below is produced by a tool, not written by hand, so it cannot drift.

Re-measure with (any OS):

    python tools/backend_gaps.py <path-to-inox> --markdown

The tool exits with status 1 if any probe is a BUG (a crash, a timeout, or a
codegen failure not marked as unsupported).

## Current measurement

| Probe | Construct | Result | Detail |
|---|---|---|---|
| `nested-for` | for inside for | **GAP** | LLVM emission currently supports only assignments, if, break, and continue in loop bodies |
| `while-in-for` | while inside for | **GAP** | LLVM emission currently supports only assignments, if, break, and continue in loop bodies |
| `for-in-while` | for inside while | **GAP** | LLVM emission currently supports only assignments, if, break, and continue in loop bodies |
| `repeat-in-for` | repeat inside for | **GAP** | LLVM emission currently supports only assignments, if, break, and continue in loop bodies |
| `loop-if-elif` | if/elif inside a loop body | **GAP** | LLVM emission currently supports loop if without elif or else |
| `loop-if-else` | if/else inside a loop body | **GAP** | LLVM emission currently supports loop if without elif or else |
| `loop-local-var` | local declaration inside a loop body | **GAP** | LLVM emission currently supports only assignments, if, break, and continue in loop bodies |
| `nested-if` | if inside if (straight-line code) | **OK** |  |
| `const-use` | module Const used in an expression | **OK** |  |
| `case` | case statement | **GAP** | LLVM emission currently supports only local variables, assignments, if, while, repeat, for, and with before Return |
| `unless` | unless statement | **GAP** | LLVM emission currently supports only local variables, assignments, if, while, repeat, for, and with before Return |
| `string-local` | String local variable | **GAP** | unsupported expression in function: Main |
| `bool-local` | Boolean local variable | **OK** |  |
| `float-arith` | Float arithmetic | **OK** |  |
| `recursion` | recursive function | **OK** |  |
| `loop-in-function` | while loop in an Integer function | **OK** |  |
| `struct-local` | struct local with field assignment | **OK** |  |
| `with` | with statement on a struct local | **OK** |  |
| `try-in-for` | try/except inside a loop body | **OK** |  |
| `state-global` | State section variable | **GAP** | unsupported expression in function: Main |

Summary: GAP=11, OK=9

## Reading the table

* **GAP** entries are legal Inox the backend does not lower yet. The biggest
  group is loop bodies: today a loop body may contain only assignments, calls,
  `if` without `elif`/`else`, `try`, `break` and `continue`.
* `state-global`: semantic analysis resolves module `State` names, but the
  emitter does not yet receive their storage. `const-use` was in the same
  situation and was closed by P-C stage 1 (below).
* `case` and `unless` are also listed in CANON B-PARSED.

## Direction (DECISION P-C, approved 2026-10-09)

Later phases consume semantically resolved information instead of re-deriving
it (CANON E11). Migration is incremental:

1. AST + semantic result -> LLVM emitter. **Done in v3.18**:
   `LlvmIrEmitter::emit(module, semanticResult)`; module `Const` values are
   resolved once by semantic analysis and read from the result.
2. The semantic result records more resolved decisions (next candidates:
   `State` storage, expression types, which callee a call resolves to).
3. Typed/semantic nodes where information is duplicated.
4. The emitter progressively stops reinterpreting the raw AST.
5. Eventually a dedicated lowered IR.

Each step should shrink this table or keep it unchanged, never add a BUG.
