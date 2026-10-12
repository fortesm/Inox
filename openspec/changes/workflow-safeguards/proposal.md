## Why

The article Marcelo shared on 2026-10-12 ("A Trindade Técnica", SDD with
agents) and tonight's own incidents point to three gaps in how changes are
made, not in the language:

- Lessons lived only in conversation memory. A session restart at 02:21 lost
  the memory of a finished but uncommitted change (item 1).
- A change approved as a proposal went straight to code even when its design
  was the risky part.
- The no-regression check of the item 2 refactor (IR and types of every
  program before and after) was an improvised script.

Approval (Marcelo Fortes, 2026-10-12): "Faça os três ajustes, adote o que
vale adotar, me chame atenção sobre mudanças, atualize o openspec sempre."
In the same message: "TNome := Integer deve ser erro".

Risk: normal (tooling and process; no compiler code).

## What Changes

- `AGENTS.md` gains "Known pitfalls (lessons already paid for)", one line per
  trap with where it bit, and the rule to add a line when a new trap costs time.
- High-risk changes (exception lowering, runtime ABI, memory ownership, a phase
  rewrite) wait for Marcelo's dated approval of `design.md` before code;
  proposals state `Risk: high` or `Risk: normal` (`openspec/config.yaml`,
  `AGENTS.md`). `string-runtime` is marked high-risk and awaits design approval.
- `tools/ir_snapshot.py` records the IR, types, diagnostics and exit codes of
  every program and compares them after a refactor; every intended difference
  is listed with `--allow`. It becomes an E19 gate for refactors (canon v3.54).
- Changes are archived right after their pull request is merged, so
  `openspec/specs` always matches `main`.
- Canon v3.54 records the gate and the decision that a Type line
  `TName := Type` is an error (there is no type alias form).

Not adopted, and why: GitHub Spec-Kit (duplicates OpenSpec's proposal and
tasks flow); "sdd-skills-ai" (cannot be verified; the `/opsx:*` skills already
cover it); the article's productivity numbers (unsourced and inconsistent).

## Capabilities

### New Capabilities
(none: no language or compiler behavior changes)

### Modified Capabilities
(none)

## Impact

`AGENTS.md`, `openspec/config.yaml`, `tools/ir_snapshot.py`, the canon change
log and E19, `openspec/changes/string-runtime` (risk marking), CHANGELOG.md.
