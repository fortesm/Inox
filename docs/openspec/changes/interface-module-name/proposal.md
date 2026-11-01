## Why

Conformance gap of `docs/STATUS.md`: `inox interface` named a module by
its file's stem, so the nested file `Bank/Accounts.inox` showed as
`Accounts`, while every `Use` and every diagnostic names it
`Bank.Accounts` (CANON-3, r3.99).

Approval: Marcelo Fortes, 2026-11-01: risk normal and narrow scope. Only
the name `inox interface` shows changes; `Use`, module resolution, the AST
and generated code stay as they are. No new command-line option and no
root-discovery machinery. The full name depends on the root chosen, not
on the file alone; that is acceptable when documented.

Touches: CANON-23 (one sentence and the scenario's tests; constitution
relocked). Compiler phases: 7 driver (`main.cpp`, `interfaceModuleName`).
Not changed: `docs/ARCHITECTURE.md` (no boundary, owner or deviation).

Risk: normal.

Status: **implemented** (canon r3.118).

## What Changes

1. `inox interface` names the module by its path from the first root that
   holds the file, directories joined by dots: each `--path`, each
   INOX_PATH entry, the standard library, then the current directory (the
   roots a `Use` already searches, plus the current directory). A root
   whose path to the file has a directory that is not an identifier is
   skipped; outside every root the file's stem is kept.
2. Tests: the B4 cases pass `--path examples/bank` and expect
   `Bank.Accounts` and `Bank`; a new case shows the name from the
   repository's root, `examples.bank.Bank.Accounts`.
3. CANON-23 states the rule and that the name depends on the root; the
   manual says it; STATUS loses the gap.

## Out of scope

How `Use` finds modules; a `Module` line in files.
