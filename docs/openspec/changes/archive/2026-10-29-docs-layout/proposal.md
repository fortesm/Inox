## Why

The maintainer wants the documentation of the language in one place:
`grammar/` and `openspec/` move under `docs/`.

Marcelo Fortes, 2026-10-29: move the grammar and OpenSpec directories under docs. OpenSpec 1.14 looks for a fixed `openspec/`
directory at its project root, so three ways were offered: (A) move it and
make `docs/` OpenSpec's root, (B) move it and leave a symbolic link, (C)
move only `grammar/`. Approval: "A".

Risk: normal (no compiler, runtime or language change; paths only).

Status: **implemented** (canon r3.100).

## What Changes

1. `grammar/` becomes `docs/grammar/`; `openspec/` becomes `docs/openspec/`.
2. `docs/` is OpenSpec's project root: every `openspec` command runs from
   `docs/` (CI's OpenSpec step has `working-directory: docs`). From the
   repository root OpenSpec finds no root and creates nothing.
3. The tools (`constitution_check.py`, `grammar_consistency.py`), both test
   runners, the release script, the README files, AGENTS.md,
   BUILD_AND_TEST_INSTRUCTIONS.md, CONTRIBUTING.md, the site, STATUS,
   ROADMAP, the capability specs and the `/opsx` skills and commands name
   the new paths; each skill and command starts with a note on where
   OpenSpec runs.
4. The constitution and engineering.md name the new paths (relocked); a
   quotation of the maintainer keeps the path he wrote.
5. `docs/openspec/config.yaml` was invalid YAML (an unquoted `Risk: high`),
   so OpenSpec ignored the project's rules and context; every rule is now a
   block scalar, and the context no longer says that `/` between integers
   is invalid (r3.95).

## Not changed

History is not rewritten: `docs/history`, the revision log, CHANGELOG
entries before this one and the texts of earlier changes keep the paths
they were written with.
