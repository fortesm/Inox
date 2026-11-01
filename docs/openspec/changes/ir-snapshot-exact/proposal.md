## Why

`tools/ir_snapshot.py compare --allow PATH` accepted any difference in the
listed program (IR, types, diagnostics, exit codes), and an exception that
was no longer needed did not fail. Since r3.114 the Gates job also took
the programs a change listed in `ir-snapshot-allow.txt`. That is weaker
than E19 asks ("every difference is listed as intended") before the
extensive refactors F4-F6.

Approval: Marcelo Fortes, 2026-11-01: question Q1 of the F4 design
(`structured-types`): exact IR-snapshot exceptions, merged before any code
of F4.

Touches: E19 (no text change: this is how its rule is checked). Compiler
phases: none. Tools: `tools/ir_snapshot.py`, `tools/ir_snapshot_selftest.py`,
`.github/workflows/gates.yml`, `.github/workflows/ci.yml`; documents
`docs/BUILD.md`, `AGENTS.md`.

Risk: normal (CI and a tool; no compiler change).

Status: **implemented** (canon r3.115).

## What Changes

1. `ir_snapshot.py expect <inox> <snapshot> PATH...` writes, for each
   program and each mode that changes, the exit codes and diagnostics
   before and after and a SHA-256 of each whole output (exit, IR or types,
   diagnostics). The file is reviewed with the change.
2. `compare --expect FILE` accepts a difference only when it is exactly the
   declared one. It fails on any other difference, on a declared mode that
   does not change, on a declared program that no longer differs, and on a
   program declared twice.
3. `--allow` names only programs the change adds or removes; an `--allow`
   on any other program fails, and it never excuses a difference.
4. The Gates job `ir-snapshot` passes `--allow` for the programs a pull
   request adds or removes and `--expect` for the
   `docs/openspec/changes/<name>/ir-snapshot-expect.json` it adds or edits
   (never one under `archive/`). `ir-snapshot-allow.txt` is no longer read.
5. `tools/ir_snapshot_selftest.py` checks these rules with made-up
   snapshots, in the CI workflow.

## Out of scope

What the snapshot records (the same programs and modes); F4 itself.
