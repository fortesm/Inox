Design of foundation step F5 (change `desugaring-out-of-parser`). Risk:
high. Status: **approved** by Marcelo Fortes, 2026-11-01, with Q1-Q4 and
five conditions (see "Approval"); implemented (canon r3.119, see "As
implemented").

## Today (D2)

| Construct | What the parser builds | Parser state |
|---|---|---|
| `A := B := C := X` | `C := X` returned; `B := C'` and `A := B'` pushed after it (C', B' are copies of the targets made by `cloneChainTarget`, without positions) | `pendingStatements_` |
| `with P.Address` … `.City := "X"` … `;` | `WithStatement(target, body, "__with_N")`; `.City` is `__member(__with_N, City)` | `withTargetStack_`, `withCounter_` (pre-order: N is taken when the header is read) |
| `while C` / `repeat` / `for I in A..B` with `invariant I` and `variant V` lines | at the top of the body: `__check(4, I, "loop invariant violated ...")`; for a variant, `__variant_K := __pure(Integer(V))` and two `__check(5, ...)`, then `__variant_previous_K := __variant_K`; before the loop `__variant_previous_K := -1`; after a `while` or `repeat`, the invariant once more `(at the end)` | `precedingStatements_`, `pendingStatements_`, `contractCounter_` (post-order: K is taken when the loop's body is done), `currentRoutine_` (the routine's name in messages) |

`finishStatement` puts the preceding statements before, and the pending
ones after, the statement being finished; `stampStatement` gives them its
position. The top-of-body checks have no position.

Behaviour that must not change (from the code and the suite):

- **Chain.** X is evaluated once; the stores go right to left (C, then B,
  then A), each from the target stored just before; a target is a
  variable or a field path (a parse error otherwise, at the operator).
- **with.** The target is evaluated once; `.Member` reaches its fields,
  also through nested `with` (the innermost); writing through it follows
  the mutability of the target (a value parameter, `Self`, owners: r3.66,
  r3.109); the binding is never a name a program can write.
- **Loop contracts.** A `while` or `repeat` takes invariants and one
  variant; a `for` takes invariants only (it always ends: a `variant` there
  is a parse error, `contract-variant-in-for`). At the start of every
  iteration: the invariant, then the variant (>= 0 and smaller than at the previous start). `continue`
  goes to the next start, so the checks run. `leave` and `until` (in a
  `repeat`) leave the loop; the statement after the loop, the final
  invariant check of a `while` or `repeat`, runs. A `for` has no final
  check. An exception leaves without any check. The messages name the
  routine and the clause as written.

## The design

### The parser records what was written

New AST forms, so the parser has no lowering state:

```cpp
// A := B := C := X (CANON-5): targets left to right, the value last.
class ChainedAssignmentStatement final : public Statement {
    std::vector<ExpressionPtr> targets;  // at least 2
    ExpressionPtr value;
};

// `.Member` inside a `with` body: the target of the enclosing `with`,
// `depth` levels out (0 = innermost).
class WithTargetExpression final : public Expression {
    std::size_t depth;
};
```

- `WithStatement` is built with an empty binding name; `.City` becomes
  `__member(WithTargetExpression{0}, City)` (the member call is the AST's
  representation of a field access today, unchanged).
- `WhileStatement`, `RepeatStatement` and `ForInStatement` get
  `std::vector<ContractClause> contracts` (the `LoopInvariant` and
  `LoopVariant` kinds exist already) and keep their body as written.
- The parse errors of these constructs stay where they are (they are
  syntax: a chained target that is not a variable or field, `invariant`
  in the wrong place).

### Phase 2b: desugaring

A new pass, `lowering/Desugar.{h,cpp}`, run by the driver on each module
right after it is parsed (`parseSource`), before module resolution:

- **Chains:** the statement becomes `C := X` in place, followed by `B :=
  C'`, `A := B'`, with the copies made exactly as `cloneChainTarget` makes
  them today (moved from the parser into the pass), and the chain's
  position on each.
- **with:** numbers each `with` of the module in pre-order (the order the
  parser numbers them today), names its binding `__with_N`, and replaces
  each `WithTargetExpression{d}` by the identifier of the `with` `d` levels
  out.
- **Loop contracts:** numbers loops with contracts in post-order (the
  parser's order today), and inserts the same statements in the same
  places, with the same positions (the loop's for the statements before
  and after it, none for the top-of-body checks), and the same messages,
  naming the routine as the parser named it (the declaration's name as
  written; the pass runs before module resolution renames it).

Its contract (for `docs/ARCHITECTURE.md`):

- **In:** the AST of one module, as written. **Out:** the same module
  with no `ChainedAssignmentStatement`, no `WithTargetExpression`, no loop
  contracts, exactly as the parser produced it before F5.
- **Guarantees:** runs once per module; synthetic names unique per module,
  numbered as before; no node shared.
- **Never:** resolves names, reads types, reports a syntax error.
- **Owner:** `lowering/` (new directory; the pass knows only `ast/`).

Why right after parsing: every later phase then receives the AST it
receives today, so module resolution (which renames identifiers),
contract lowering, the analyzer and codegen do not change, and the IR,
types and diagnostics stay identical. The alternatives were weighed:

- *In contract lowering (phase 4)*: the loop contracts fit there by name,
  but module resolution would then see the written forms, and every
  phase between would need to know them.
- *In the analyzer or codegen*: that is lowering with types, which is F6's
  business (below).

### Relation to F6

F6 introduces a typed IR between analysis and codegen. With F5 done, the
written forms exist in the AST; F6 may then choose to lower chains,
`with` and loop contracts in the IR instead of in phase 2b (the analyzer
would analyze the written forms directly). F5 does not decide that: it
keeps the lowered AST every later phase expects, so F6 starts from a
parser that says what was written and one place, phase 2b, that knows
each lowering. The analyzer's checks of the `__with` prefix
(`SemanticExpressions`, `SemanticStatements`) stay as they are in F5 (Q3).

### Stack budget

The parser's recursive functions lose work: `finishStatement` no longer
moves preceding and pending statements, `parseExpressionStatement` (840
bytes on main) no longer builds the chain, and `parsePrimary` (712) no
longer reads `withTargetStack_`. They must not grow; they should shrink
(`tools/stack_frames.py`).

Phase 2b is a new recursion, over nested statements and expressions: it
runs after the parser has returned and before the analyzer starts, so its
deepest stack does not add to theirs. Its frames must be small (the walk
in one function per level, no `std::string` or vector locals in the
recursive function; the work in out-of-line helpers), and
`limits-boundary-ok` must keep 963 KiB and 451 KiB on Windows Debug
(`windows-stack`); the new walk is added to the table of
`tools/stack_frames.py` as a recursion path.

### Order of work

1. AST forms and the clone (`AstClone` names every kind); the parser
   builds them; phase 2b lowers them; the parser's lowering state is
   deleted. One change, as F4: the IR snapshot decides.
2. Unit tests: the parser's AST for each construct (written form), and the
   pass's output for nested `with`, chains of three and four, nested loops
   with contracts (the numbering), compared with the AST the parser built
   before (recorded once as text from main).
3. `docs/ARCHITECTURE.md`: phase 2b, D2 closed; `AGENTS.md` pitfall.

### Verification

- IR snapshot against main with **no exception** (IR, types, diagnostics,
  exit codes of every program).
- The suite, the sanitized suite (1 MiB), fuzz seeds 1 and 7, the
  differential test, backend_gaps, openspec, constitution_check,
  version_check, `tools/stack_frames.py`, `windows-stack`.
- Tests that already cover the behaviour above (chains, `with` nesting
  and mutability, loop contracts with `leave`, `continue`, `until` and
  exceptions) are listed in the pull request; any behaviour above without
  a test gets one before the code changes.

## Decisions asked of the maintainer

- **Q1. Where the lowering runs.** Phase 2b right after parsing
  (recommended), or in contract lowering (phase 4) for the loop contracts
  only.
- **Q2. Compound assignment.** `L += R` is also rewritten by the parser
  into `L := L + R` (ADR-0012, flagged `isCompoundAssignment`). D2 did not
  list it. Recommended: move it to phase 2b in this change too, with the
  same guarantees (it is the same kind of work, and the parser would then
  build only written forms); or leave it, listed as a deviation.
- **Q3. The analyzer's knowledge of `__with`.** Two analyzer checks
  recognize a `with` binding by its name's prefix. Recommended: keep them
  in F5 (the AST after phase 2b is unchanged) and replace them in F6 by a
  property of the typed IR.
- **Q4. Encodings that are not desugaring.** `__member`, `__index`,
  `__slice` and `__set` are how the AST spells a field access, an index, a
  slice and a set literal. Recommended: out of F5 (they change no
  meaning); they are reviewed with F6's typed IR.

## Approval

Approved by Marcelo Fortes, 2026-11-01:

- **Q1.** Phase 2b right after each module is parsed, before module
  resolution.
- **Q2.** The whole compound family (`+=`, `-=`, `*=`, `/=`, `^=`) leaves
  the parser too.
- **Q3.** The analyzer's `__with` prefix checks stay during F5, as a
  recorded debt of F6: F6 gives the analysis an explicit property of the
  binding's origin instead of a string.
- **Q4.** `__member`, `__index`, `__slice`, `__set` stay; they are not part
  of F5.

Conditions:

1. A compound assignment keeps its target's identity: the targets the
   parser accepts today and no others, no target evaluated twice, and the
   `isCompoundAssignment` mark kept.
2. The `__with` prefix recognition is marked as F6 debt in
   `docs/ARCHITECTURE.md`.
3. Flow-control tests come before the refactoring: separate tests for
   `continue`, `leave`, `until`, exceptions and nested loops with
   contracts, which fix the order and the number of checks.
4. Phase 2b runs exactly once per module, on every driver path that
   analyzes a program, imported modules and cloned ASTs included; no form
   of phase 2b reaches the analyzer or codegen.
5. No IR or stack regression: the snapshot identical to the base with no
   exception, and the stack compared in the same build configuration,
   with the new walk included.

Scope: phase 2b only turns the AST as written into the normalized AST the
later phases already understand; it is not a general optimization or
transformation framework.

## As implemented

- **Forms.** `ChainedAssignmentStatement`, `CompoundAssignmentStatement`
  (Q2) and `WithTargetExpression`; loops carry `contracts`. A
  `WithTargetExpression` holds the nesting level of its `with` counted from
  the outermost of its routine (0), not the depth from the innermost: the
  parser knows the level when it reads `.Member`, and the pass keeps a
  stack of names.
- **Limits.** The nesting limit of expressions applies to the forms the
  pass will build, as before (`requireLoweredDepth`): the parser still
  rejects a chain or compound assignment whose lowered form would be too
  deep, with the same message.
- **Names in messages.** A loop contract's message names the routine as
  the parser named it (the declaration's name as written).
- **Driver paths (condition 4).** The pass runs in
  `ModuleLoader::loadModule`, the only path that analyzes: the entry
  module and every module a `Use` loads, once each; Debug builds then
  check that no written form is left. `--parse-only`, `--dump-tokens` and
  `inox interface` do not analyze and do not run it. ASTs cloned later
  (contract wrappers, generic instances) are clones of the normalized AST.
- **Clone.** `AstClone` copies the written forms (the pass uses it, with a
  statement rewriter); a loop's contracts are attached after the copy, out
  of line, so `cloneStatementNode` keeps its 1512 bytes.
- **Stack** (`tools/stack_frames.py`, clang Debug, against main): no
  recursion-path function grows; `parseExpressionStatement` 840 → 728,
  `cloneStatement` 120 → 88, `cloneInstanceBody` 264 → 184. The pass's own
  recursion stays under 512 bytes a frame (`lowerChain` 328, `lowerLoop`
  312, `rebuildLoop` 216, `lowerWith` 168). The tool now keeps the
  `(anonymous namespace)` in names, so the clone's functions are measured
  (they were folded together before).
