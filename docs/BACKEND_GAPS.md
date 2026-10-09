<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Backend gaps (semantic analysis accepts, LLVM backend does not lower)

Status: measured on 2026-10-09 against v3.18, re-measured against v3.20
(keyword rename only; results unchanged) and against v3.21 (compositional
statement lowering: 7 gaps closed, 3 probes added, none of them a gap). Layer B — describes the
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

The tool exits with status 1 if any probe is a BUG (a crash, a timeout, a
codegen failure not marked as unsupported, or — when `clang` is on `PATH` —
emitted LLVM IR that clang rejects). The IR check was added in v3.21: it found
that a `Float` local declared without an initializer emitted `store double 0`,
which is invalid IR (fixed in v3.21; probe `float-uninit`).

## Current measurement

| Probe | Construct | Result | Detail |
|---|---|---|---|
| `nested-for` | for inside for | **OK** |  |
| `while-in-for` | while inside for | **OK** |  |
| `for-in-while` | for inside while | **OK** |  |
| `repeat-in-for` | repeat inside for | **OK** |  |
| `loop-if-elif` | if/elif inside a loop body | **OK** |  |
| `loop-if-else` | if/else inside a loop body | **OK** |  |
| `loop-local-var` | local declaration inside a loop body | **OK** |  |
| `until-in-if` | until inside if within repeat | **OK** |  |
| `until-across-loop` | until with a loop between it and its repeat | **OK** |  |
| `float-uninit` | Float local declared without initializer | **OK** |  |
| `nested-if` | if inside if (straight-line code) | **OK** |  |
| `const-use` | module Const used in an expression | **OK** |  |
| `case` | case statement | **GAP** | LLVM emission does not lower case statements yet |
| `unless` | unless statement | **GAP** | LLVM emission does not lower unless statements yet |
| `string-local` | String local variable | **GAP** | unsupported expression in function: Main |
| `bool-local` | Boolean local variable | **OK** |  |
| `float-arith` | Float arithmetic | **OK** |  |
| `recursion` | recursive function | **OK** |  |
| `loop-in-function` | while loop in an Integer function | **OK** |  |
| `struct-local` | struct local with field assignment | **OK** |  |
| `with` | with statement on a struct local | **OK** |  |
| `try-in-for` | try/except inside a loop body | **OK** |  |
| `state-global` | State section variable | **GAP** | unsupported expression in function: Main |

Summary: GAP=4, OK=19

## Reading the table

* **GAP** entries are legal Inox the backend does not lower yet. The count
  refers to the probes in this table, not to every limitation of the backend
  (for example, `Const` values are still limited to Integer and Bool literals).
* v3.21 closed the loop-body group (`nested-for`, `while-in-for`,
  `for-in-while`, `repeat-in-for`, `loop-if-elif`, `loop-if-else`,
  `loop-local-var`): loop bodies and `if`/`elif`/`else` branches are lowered by
  the same statement dispatcher as any other block, so a construct that lowers in
  one block lowers in every block context where semantic analysis accepts it
  (context rules such as `until` only inside a repeat stay in the analyzer).
* `until` is a transfer to an explicit target, its nearest repeat
  (`until-in-if`, `until-across-loop`): it may appear anywhere in the repeat
  body, including inside `if`, `try` and loops nested in the repeat, and it runs
  every `ensure` between it and the repeat, innermost first.
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
