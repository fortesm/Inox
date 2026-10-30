## Why

`Var` stopped being reserved on 2026-10-16, but the parser kept a migration
diagnostic for the old `Var ... ;` block (`atRemovedVar`, the message "the
'Var' block and 'var'/'mut var' declarations were removed"), and CANON-4
still required it. Nobody but the maintainer writes Inox yet, so the
stable language is born without any trace of `var`.

Approval (Marcelo Fortes, 2026-10-23): remove it: nobody else uses Inox yet, the stable language is born without `var`; the word behaves as any unknown identifier, with a generic message; the rule leaves the canon and the OpenSpec documents.

Risk: low (only diagnostics change; valid programs get the same IR).

Status: **implemented** (canon r3.94).

## What Changes

1. **Parser.** `kVarRemovedMessage` and `atRemovedVar` are gone, in the
   statement, the module item and the section boundary. `Var` is read as
   any identifier: `Var := 1` declares a variable, `Var X := 1` is "unknown
   type: X", a module `Var` section is "expected a declaration".
2. **`mut`.** It stays reserved (CANON-5); `mut X := 1` now fails with the
   generic "expected expression".
3. **Constitution.** CANON-4 no longer names `Var` blocks; the scenario
   "Var is gone" is removed; the CANON-5 scenario "mut stays reserved" uses
   `tests/diagnostics/mut-reserved.inox`.
4. **Tests.** `var-block-removed` and `module-var-removed` are deleted;
   `mut-var-removed` becomes `mut-reserved`. `invalid-031` and
   `parser/invalid/var-colon` stay invalid through the generic errors.
5. **Docs.** grammar.ebnf notes, tests/invalid/README.md, CHANGELOG, STATUS.
