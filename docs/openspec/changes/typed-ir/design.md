Design of foundation step F6 (change `typed-ir`). Risk: high. Status:
**approved** by Marcelo Fortes, 2026-11-02, with Q1-Q5 and five technical
conditions (see "Approval"); step 0 (inventory) done, steps 1-5 one pull
request each.

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
   (since step 1, `SemanticResult::callMeaning`, condition 1); codegen
   dispatches on `Builtin::op`. The 91 name comparisons go.
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

Approved by Marcelo Fortes, 2026-11-02. The approval covers the
incremental execution of steps 0-5, each validated on its own; it does not
cover unrelated changes, any relaxing of the gates, or any change of the
LLVM IR.

- **Q1.** A separate TIR: it keeps E11 and the layering of AST, types and
  codegen.
- **Q2.** A tree-level TIR: no CFG, basic blocks or SSA in F6.
- **Q3.** Six pull requests in sequence, each passing every gate and merged
  before the next.
- **Q4.** `--dump-tir`: deterministic and tested.
- **Q5.** 0.4.1 requires steps 0-5: D5 closed, no partial migration
  shipped.

Technical conditions:

1. **5b is not a second analyzer.** `tir::Lower` carries decisions the
   analysis took: symbol identity, overload choice, conversions, checks and
   the classification of builtins are resolved before it; it resolves no
   name and infers nothing. What the analysis does not record yet, it
   records first (in `SemanticResult`), and the lowering copies it.
2. **References and effects.** `Place` nodes keep addressability,
   mutability, pass modes and lifetime; the order of evaluation of
   operands, the checks and the exception semantics are kept; a
   `WithBinding` is identified by its semantic origin, never by a naming
   convention.
3. **Ownership and lifetime.** The TIR is owned by its program (an arena);
   the `const types::Type*` it holds stay valid during the whole code
   generation (the type context outlives it); pointers back to the AST are
   tolerated during the migration and are gone from codegen's interface at
   the end.
4. **Two snapshots.** The LLVM IR snapshot stays strictly identical at all
   six steps, without any allow list. `--dump-tir` has its own tests and
   its own snapshot, deterministic (no addresses, no map order), so an
   intended change of the TIR's structure never touches the LLVM IR rule.
5. **Stack and memory at every step.** Windows Debug keeps 963 and 451 KiB;
   the recursive paths of codegen and of the lowering join
   `tools/stack_frames.py`, and no protected frame grows; the peak of the
   whole compilation (analysis, TIR, codegen) is the `windows-stack`
   measure; the extra memory of the TIR is measured on the largest test
   program.

Adopted from the same review round:

- The inventory is generated and checked by a tool
  (`tools/codegen_decisions.py`), not kept by hand; its `--zero` mode is
  part of the exit criterion of step 5.
- The closed set of `Place` kinds and the builtin enumeration are defined
  at step 1, the enumeration in one header the analysis and the TIR share.
- **Exit criterion of step 5:** `tools/codegen_decisions.py --zero` (no
  name comparison, no lookup by AST node, no AST shape test in
  `codegen/`), no `__with` test anywhere, and the LLVM IR snapshot
  identical.
- The count of side tables is reconciled at step 0: 16 (`inventory.md`).

## Step 2: the migration index (approved 2026-11-04)

Approved by Marcelo Fortes, 2026-11-04, as a refinement of the migration
strategy of step 2; the other decisions above are unchanged.

- **Strategy A, a transitional index.** The lowering records, by node
  identity, which TIR node each AST node became
  (`tir/MigrationIndex.h`); code generation's wrappers (`typeOf`,
  `symbolOf`, `resolvedConstant`, `declaredTypeOf`, the declared type of a
  State variable) answer from the TIR through it. The signatures of the
  recursive `emit*` functions do not change. Strategy B (passing AST and
  TIR nodes together through about 70 recursive functions) is not
  authorized in this step.
- **No fallback.** A node without its TIR node is an internal error in
  every build; a wrapper never asks the `SemanticResult` instead. A Debug
  compiler compares each answer with the analysis's record; a
  disagreement is an internal error.
- **Lifetime.** The TIR and the index are built for every command that
  generates code, in Debug and Release, and live until code generation
  ends (`main.cpp`).
- **Temporary.** The index resolves no name, infers nothing and is not
  part of the TIR's API; it leaves at step 5.
- **No artificial nodes.** What the backend asks of an AST node that is no
  value in the TIR (the name of a routine, of an encoding) gets that
  answer: no type, the call's symbol; a
  property the backend needs is a property of a TIR node (a folded
  builtin keeps its operand; a Const's literal carries its value).

### The correspondence (matrix)

Measured on the 1065 programs of the snapshot (every query code generation
made, compared with the analysis's record); no other kind of node is asked.

| AST node | TIR representative | Type | Symbol | Constant |
|---|---|---|---|---|
| an expression lowered as a value (literal, operator, conditional, call, identifier) | its `Expr`, before the `RangeCheck` and `NaturalCheck` added on the way into a place | the `Expr`'s | a `Read`: its place's; a `Call`, `Builtin`, `Intrinsic`, `TypeValue`: the one the analysis bound | a `Literal` that reads a Const: the Const's value |
| an identifier that is a call (`GetLn`, `Report`, `ClearScreen`) | the `Call`, `Builtin` or `Intrinsic` | its result, `Void` without one | the routine's | none |
| `Low`, `High`, `Length` of a type (CANON-13) | a `Builtin` marked folded, with its value and its operand | Int64 or the enum | none | none |
| a written target, a `with` target, a reference argument | its `Place` (local, parameter, field, element, `Through`, ...) | the place's | the variable's | none |
| `L := V`; the `L` it declares | the `Assign` or `Declare` | the target's; the declared type | the declared variable's | none |
| a `VarStatement`; a State declaration | the `Declare`; the `StateVariable` | the declared type | | |
| the callee of a call of a routine, a builtin, an intrinsic; `__member` and the name of a method | the call | none (a name is no value) | the call's | none |
| the callee of `T(...)` (conversion, construction) | the `Convert` or `Construct` | T | T's | none |
| the field name of `P.F`; `F` and `F := V` of a construction | the `Field` place; the field of the `Construct` | the field's; none for `F` | none | none |
| the kind and text of a contract check | the check's literal operands (decoded in `checkKind` and `text`) | Int64, String | none | none |
| `__member`, `__index`, `Value` written in a place; `__low`, `__high` | names | none | none | none |

Two differences from the analysis's record are made on purpose, and the
Debug comparison accepts exactly these (`MigrationIndex::typeAgrees`),
both only where the analysis recorded no type: an operation without a
result (a call, a builtin, a string function, an intrinsic) is `Void`; a
written field or element has its place's type. A node the analysis typed
agrees only with that type. With the TIR's answers, the LLVM IR of every
program is identical.

### Two kinds of failure

Valid Inox the TIR cannot express yet is `tir::UnsupportedConstructError`,
a gap of the backend (E15): the driver reports it as code generation
reports its own, and `tools/backend_gaps.py` counts a GAP. A decision the
analysis must have recorded that is missing (a call without its meaning,
a variable without its origin, a routine without its signature), or a
structure the earlier phases guarantee that is broken, is
`tir::InvariantError`, an internal error of the compiler.

### Recorded by the analysis first (condition 1)

`Low(T)` and `High(T)` bind T to its type's symbol (the TIR's
`TypeValue` carries it); `--dump-types` shows the binding in 6 programs,
declared in `ir-snapshot-expect.json`.

### One diagnostic (approved by Marcelo Fortes, 2026-11-04)

Code generation now needs the TIR, so what the TIR cannot express yet
stops before code generation. It is a gap of the backend (E15) and is
reported as one: `codegen error: not yet implemented in the LLVM
backend: typed IR: ...` with the same note, so `tools/backend_gaps.py`
counts it as a GAP. The one program concerned,
`tests/semantic/valid/const-conversion.inox` (a Const whose value the
analysis does not compute, already a gap of codegen), now names that
reason instead of "unsupported expression in function: Main"; the
difference is declared in `ir-snapshot-expect.json`.

## Step 3: builtins by kind

Scope approved by Marcelo Fortes, 2026-11-04: extend the automated
inventory, and replace the textual classification of builtins by
`BuiltinOp`, the analysis remaining the source of every decision.

- **Where codegen reads the builtin.** `AstNodeMap::builtinOf(node)`
  answers from the TIR node the call became: the `op` of its `Builtin`
  (folded or not); `Value` for `Value(P)`, which the TIR builds as the
  object P points at (a `Through` place, the only one an AST node
  becomes, matrix above); `None` for any other node. A node without its
  TIR node is an `InvariantError`, as for every other query. The map
  gains no field and the TIR no node: the answer is the one the TIR
  already holds.
- **Agreement.** `FunctionEmitter::builtinOf` checks in a Debug compiler
  that the answer is the operation of the analysis's `CallMeaning`
  (`None` when the call is not a `Builtin`); a disagreement is an
  internal error, never a fallback.
- **What goes.** Every comparison of a callee name with a builtin's name,
  and every table of such names: the math functions (`MathBuiltins`, now
  on `BuiltinOp`), safe pointers, `Low`/`High`/`Length` of arrays and
  slices, `Ord`/`Succ`/`Pred`, sets, `Abs`, `Round`/`Trunc`, `EndOfInput`,
  `Put`/`PutLn`/`Get`/`GetLn`, `Assert` and the functions of Std.Files
  (codegen's own `FileCall` enumeration). With them go the tests that a
  routine of the program does not bear the builtin's name
  (`signatures_.find(normalize(name))`, `calledRoutine(...) == nullptr`):
  the analysis decided it.
- **What stays, and where it goes.** The encodings (`__member`,
  `__index`, `__slice`, `__set`, `__check`, `__pure`) and `Self`, step 4;
  `true` of a literal and `Main`, step 5. Outside this step's scope, as
  they are not `BuiltinOp`, and listed by the inventory: conversions
  (`CallKind::Conversion`: `String(X)` recognized by its name, and the
  target type resolved from the written name, three sites), the type
  named in `Low(T)` and `High(T)` (`enumTypeNamed`), the module
  intrinsics found by name (`intrinsicCallOf`), and the two types the
  module emitter resolves by name (`LlvmIrEmitter.cpp`). The audit
  proposed the conversions for step 3; their step is the maintainer's to
  set.
- **Inventory.** `tools/codegen_decisions.py` also counts the names
  codegen resolves itself (`resolutions`): a type by its name, a routine
  in the signature map, a module intrinsic by its name, an encoding by
  its prefix (`__with`), a name compared with a table of names.
