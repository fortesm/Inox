## Why

Deviation D2 of `docs/ARCHITECTURE.md`: the parser does not only say what
was written, it rewrites three constructs (E11: the parser never lowers or
desugars):

- a chained assignment `A := B := C := X` becomes three statements, two of
  them pushed after the current one (`pendingStatements_`);
- a `with` statement invents a binding `__with_N`, and `.Member` in its
  body becomes `__member(__with_N, Member)`;
- a loop's `invariant` and `variant` lines become checks inserted into the
  body, before the loop (`__variant_previous_N := -1`) and after it
  (`lowerLoopContracts`, `precedingStatements_`).

The AST therefore does not say what the program wrote, and the parser
keeps state (`pendingStatements_`, `precedingStatements_`, counters, a
stack of `with` targets) that belongs to a lowering. Foundation step F5,
the first of F5 → F6 → F7, before any 0.5 item.

Approval of the plan and order: Marcelo Fortes, 2026-10-31 (F0-F7) and
2026-11-01 (F5, F6, F7 before 0.5; 0.4.1 pre-alpha closes the
foundation). This change starts as documents only; no code before the
design's dated approval.

Touches: E11 (no text change). Compiler phases: 2 parser (builds the
written forms), a new phase 2b desugaring (proposed), the AST (new nodes
and fields, the clone). Phases 3-6 receive exactly the AST they receive
today. `docs/ARCHITECTURE.md` updated when implemented (D2 closed, the new
phase's contract).

Risk: high (moves work out of a compiler phase and adds one; the design
waits for approval).

Status: **implemented** (canon r3.119). Design approved by Marcelo Fortes,
2026-11-01.

## What Changes

1. The parser records what was written: a chained assignment node, a
   `with` statement whose `.Member` names its target without an invented
   name, and loops that carry their `invariant` and `variant` clauses.
2. A desugaring pass, right after each module is parsed, turns these into
   the statements the parser makes today, with the same synthetic names,
   order of evaluation, positions and messages.
3. The parser loses `pendingStatements_`, `precedingStatements_`,
   `withTargetStack_`, `withCounter_`, `contractCounter_` and
   `lowerLoopContracts`.
4. No output changes: IR snapshot with no exception, no frame on a
   recursion path grows.

## Decisions

See `design.md`, "Decisions asked of the maintainer" (Q1-Q4).
