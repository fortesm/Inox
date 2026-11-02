## Why

Deviation D5 of `docs/ARCHITECTURE.md`: code generation reads the raw AST
beside the side tables of `SemanticResult` (16 maps and lists, most keyed
by AST node addresses), and still takes semantic decisions of its own. Measured on
main (r3.119):

- 91 name comparisons in `codegen/` decide what a call is (`Low`, `High`,
  `Free`, `New`, `Move`, `Length`, `Ord`, `Succ`, the math functions, `Put`,
  `GetLn` ...), work the analysis already did (DECISION P-C of E11: later
  phases consume the analysis's decisions, never rediscover them);
- the AST encodes accesses as calls (`__member`, `__index`, `__slice`,
  `__set`, `__check`, `__pure`) that codegen recognizes by name;
- a `with` binding is recognized by its name's prefix (F5 debt);
- `typeOf` is looked up 78 times by node address.

E11 lists the migration in five steps; steps 1 and 2 are done. F6 does
steps 3-5: typed nodes, an emitter that stops reinterpreting the AST, a
dedicated lowered representation. It is the second of F5 → F6 → F7, before
0.4.1 and any 0.5 item; 1.0 does not ship with codegen taking semantic
decisions (decision of 2026-11-01).

Approval of the plan: Marcelo Fortes, 2026-10-31 and 2026-11-01. This
change is documents only; F6 is high risk and incremental: no code before
the design's dated approval, then one pull request per step.

Touches: E11 (no text change; its steps 3-5). Compiler phases: a new phase
5b, lowering to a typed IR (TIR), between analysis (5) and codegen (6);
codegen (6) migrates to read it; the analysis (5) records the decisions
codegen still takes. `docs/ARCHITECTURE.md` updated at each step.

Risk: high (rewrites how codegen reads the program).

Status: **design awaiting approval**. No code before it.

## What Changes

1. A typed IR (`tir/`): per routine, statements and expressions whose
   every node carries its type and its resolved meaning (which routine or
   builtin is called, which place is read or written, which checks run),
   built once from the analyzed AST and `SemanticResult`.
2. Codegen reads the TIR instead of the AST and the side tables, step by
   step; it stops matching names.
3. The F5 debt is paid: a `with` binding is a TIR place of kind
   `WithBinding`, not a name with a prefix.
4. No output changes at any step (IR snapshot without exceptions); no
   frame grows on any recursion path, codegen's included.

## Decisions

See `design.md`, "Decisions asked of the maintainer" (Q1-Q5).
