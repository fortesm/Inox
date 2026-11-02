Design of foundation step F6 (change `typed-ir`). Risk: high. Status:
**awaiting the maintainer's approval**; no code before it is recorded here
with its date.

## Today (D5)

```text
5 analysis ──▶ AST (as lowered by 2b and 4)  +  SemanticResult
                                                 signatures_, instances_, structInstances_,
                                                 constructions_, stringCalls_, declaredTypes_,
                                                 invariantBefore_/After_, newValueInvariants_,
                                                 naturalChecks_, rangeChecks_, fillUnits_,
                                                 expressionTypes_, identifierSymbols_,
                                                 callSymbols_, constantValues_
6 codegen  ──▶ walks the AST; asks the maps by node address; decides by name
```

What codegen still decides itself (to be confirmed and completed by the
inventory of step 0):

| Decision | Where | How today |
|---|---|---|
| Which builtin a call is | `MathBuiltins.cpp` (31), `FunctionEmitterExpressions.cpp` (24), `FunctionEmitterPointers.cpp` (13), arrays, enums, sets, statements | `equalsIgnoreCase(callee, "Low")`, ... (91 comparisons) |
| A field, an element, a slice, a set literal, a check, a pure read | expressions | callee named `__member`, `__index`, `__slice`, `__set`, `__check`, `__pure` |
| A `with` binding | statements, pointers | the analyzer marks it by the `__with` prefix; codegen follows the symbol |
| Whether a local has a written type | statements | `VarStatement::type()` |
| Constants | identifiers | `constantValueOf(symbol)` |

The largest frames of codegen's recursion (clang Debug):
`emitExpressionValue` 6408 bytes, `emitTry` 5960, `emitIntrinsicCall`
5000, `emitStringCall` 3080, `emitFileCall` 3048, `emitForIn` 2808: on
`limits-boundary-ok`, codegen is among the deepest parts of the 963 KiB.

## The typed IR

```text
5 analysis ─▶ 5b TIR lowering (tir/Lower) ─▶ tir::Program ─▶ 6 codegen
```

- **One program, owned by an arena.** `tir::Program` holds the routines
  that codegen emits (instances and contract wrappers included), the
  struct types to declare, the State variables and Consts. Nodes are
  allocated in an arena owned by the program and refer to each other by
  pointer; nothing is shared with the AST except a `SourcePosition` (and,
  during the migration, a pointer back to the AST node it came from).
- **Every expression has a type** (`const types::Type*`, never null) and a
  kind that says what it is, not how it was written:

  | TIR expression | Meaning |
  |---|---|
  | `Literal` | a value of its type (integer, real, decimal, string, char, bool, enum literal, set) |
  | `Read(place)` | the value of a place |
  | `Call(routine, arguments)` | a call of a routine of the program, its signature resolved, each argument's pass mode known |
  | `Builtin(op, arguments)` | one of an enumeration of the language's builtins and intrinsics (`Low`, `High`, `Length`, `Ord`, `Succ`, `New`, `Move`, `Free`, `Sqrt`, `Put`, `GetLn`, the String and File functions, ...): no names |
  | `Unary`, `Binary` | the operation and its operand types, conversions explicit |
  | `Convert(to, value)` | an explicit conversion (Integer to Float, a Range check, Decimal scale) |
  | `Construct(struct, fields)` | a construction, fields in declaration order with defaults filled |
  | `Conditional` | `IIf` |

- **Places** are what can be read, written or passed by reference:
  `Local`, `Parameter(mode)`, `State`, `Receiver(mutable)`,
  `WithBinding(target)` (the F5 debt: the origin is a kind, not a name),
  `Field(place, index)`, `Element(place, indices, checked)`,
  `Through(owner or link)`.
- **Checks are nodes**: `RangeCheck(type)`, `NaturalCheck`,
  `InvariantCheck(before/after, struct)`, `ContractCheck(kind, message)`
  (from `__check`), so codegen emits what the analysis decided.
- **Statements** mirror the AST's after 2b and 4 (assign, call, if, loops,
  case, try, raise, retry, return, exit, leave, continue, with,
  declarations), with places and expressions as above.

Codegen keeps all the LLVM knowledge (layouts, the runtime ABI, exception
ABIs, temporaries, cleanup): the TIR says what to compute, codegen says how.

## Order of work: steps that each pass every gate

0. **Inventory.** A document listing every decision codegen takes today
   (each name comparison, each side-table lookup, each AST shape test),
   with the TIR node that will carry it. Reviewed before step 1.
1. **TIR built, not used.** `tir/` (nodes, arena, a printer for tests) and
   `tir/Lower` from the analyzed AST and `SemanticResult`; a verifier that
   every node has a type and a meaning; built for every program in Debug
   and by a `--dump-tir` mode for tests. Codegen unchanged: the IR
   snapshot is identical by construction.
2. **Types and symbols from the TIR.** Codegen walks the TIR in lockstep
   with the AST and takes types, symbols and constants from it; the side
   tables of types and symbols are no longer read by codegen.
3. **Builtins by kind.** The analysis records the builtin of each call
   (it already resolves it); codegen dispatches on `Builtin::op`. The 91
   name comparisons go.
4. **Places and encodings.** `__member`, `__index`, `__slice`, `__set`,
   `__check`, `__pure` are TIR nodes; `with` bindings are `WithBinding`
   places; the analyzer's `__with` prefix test goes (F5 debt).
5. **Codegen walks only the TIR.** The AST is no longer read by codegen;
   the side tables that served only codegen leave `SemanticResult`; D5
   closed.

Each step is its own pull request with an IR snapshot against main and
no exception, `tools/stack_frames.py` (codegen's recursion added to its
table at step 1), `windows-stack`, the sanitized suite and the other
gates. A step that cannot keep the IR identical stops and is reported.

## Stack budget

Two recursions are added or changed:

- **TIR lowering** (new, between analysis and codegen): one small
  function per node kind level, no strings or vectors in recursive frames;
  added to `tools/stack_frames.py` as a new path with a budget per frame.
  It runs after the analysis returns and before codegen starts, so it does
  not stack on either.
- **Codegen** (changed): its recursive functions (`emitExpressionValue`,
  `emitStatement`, `emitTry`, the call emitters) are added to the table at
  step 1, with their frames on main; none may grow. Dispatching on an
  enumeration instead of strings should shrink them (`emitIntrinsicCall`
  and the name tests hold string temporaries today).

`windows-stack` must keep `limits-boundary-ok` at or below 963 KiB and
451 KiB at every step.

## What could go wrong, and the guard

| Risk | Guard |
|---|---|
| The TIR says something the AST path did not do | Step 2 walks both in lockstep and asserts they agree (Debug) before codegen relies on the TIR; IR snapshot at every step |
| A builtin is classified differently | Step 3 records the analysis's own decision; a unit test lists every builtin name and its kind |
| Memory: the TIR doubles the program in memory | An arena per program, freed after codegen; measured on the largest test |
| A gap: valid Inox the TIR cannot express | The lowering reports "valid Inox, not yet lowered" (E15), never crashes; backend_gaps stays at 0 BUG |
| The migration stalls half way | Each step is complete on its own and leaves no dual path behind except the lockstep check of step 2, removed at step 5 |

## Relation to the other steps

- **F5** left written forms in the AST and phase 2b; F6 does not move
  those lowerings into the TIR (they stay in 2b), but it removes the
  string test of `with` bindings.
- **F7** (exception hierarchy in the types) comes after: in F6, exception
  types in `Raise` and handlers are carried as they are (names), so F7
  changes only those nodes.
- **0.5** (methods of generic structs, `Option`, `Result`, `Vector`): their
  designs are reviewed against the TIR before approval.

## Decisions asked of the maintainer

- **Q1. A separate TIR, or annotating the AST.** A separate typed tree
  owned by `tir/` (recommended: `ast` keeps not knowing `types`, and codegen
  stops depending on the AST), or fields with types and meanings added to
  the AST nodes (less code, but the AST would know `types`, against E11's
  layering).
- **Q2. Granularity.** The TIR mirrors statements and expressions
  (recommended for F6), or goes further down to basic blocks and SSA
  values (an LLVM-like IR). Recommended: not now; codegen already makes
  the blocks, and a lower IR is not needed to remove the semantic
  decisions.
- **Q3. Steps and pull requests.** Six pull requests (0-5), each merged
  before the next. Recommended: yes.
- **Q4. `--dump-tir`.** A driver mode printing the TIR, for tests and
  debugging (like `--dump-types`). Recommended: yes, and the IR snapshot
  records it from step 1 so a TIR change is visible.
- **Q5. What 0.4.1 requires.** All of steps 0-5 before 0.4.1
  (recommended, D5 closed), or steps 0-3 (no name decisions left in
  codegen) with 4-5 after 0.4.1.

## Approval

(To be recorded: who approved, the date, and the answers to Q1-Q5.)
