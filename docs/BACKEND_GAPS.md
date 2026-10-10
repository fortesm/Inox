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
emitted LLVM IR that clang rejects). The IR check was added in v3.21. (Its
first finding, invalid IR for a `Float` local without an initializer, turned out
to be a program CANON-5 forbids; since v3.23 semantic analysis rejects it and the
`float-uninit` probes were removed. Float32 keeps coverage through the valid
`float32-conversion` probe, an honest GAP: `Float32(0.0)` is accepted by semantic
analysis but not lowered yet.)

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
| `float32-conversion` | Float32 local initialized by explicit conversion | **GAP** | unsupported expression in function: Main |
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
| `grouped-decl-scalar` | grouped declaration A, B, C T := X (X evaluated once) | **OK** |  |
| `grouped-decl-struct` | grouped struct declaration P, Q TPoint := Base | **GAP** | LLVM emission does not support struct initializers yet |
| `chained-assignment` | chained assignment A := B := C := X | **OK** |  |
| `named-struct-construction` | named struct construction TPoint(X := 1, Y := 2) | **GAP** | unsupported expression in function: Main |
| `for-step-expression-bounds` | for I in 1..N + 1 step N div 2 | **OK** |  |
| `case-ada-choices` | case with \| alternatives and a static range | **GAP** | LLVM emission does not lower case statements yet |
| `digit-separators` | numeric literals with the digit separator _ | **OK** |  |
| `compound-assignment` | compound assignment += -= *= /= ^= | **OK** |  |
| `hex-0x` | hexadecimal literal written 0xFF | **OK** |  |
| `conditional-expression` | conditional expression if C then A else B | **GAP** | unsupported expression in function: Main |
| `state-global` | State section variable | **GAP** | unsupported expression in function: Main |

Summary: GAP=9, OK=24

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
* `grouped-decl-struct` (v3.26): the grouped form itself lowers; the GAP is the
  older one, a struct local initialized from another struct value
  (`P TPoint := Base`). It closes with struct initializers.

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
