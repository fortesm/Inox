## Why

The maintainer decided on 2026-11-08 that no feature is considered done on
the strength of an AI's report: the report is a statement; the
implementation, the tests and the results of running them are the
evidence. He asked for the rule to be canonical and entrenched.

Approval: Marcelo Fortes, 2026-11-08 (G2, governance; entrenched). Risk:
low (governance and checks; no language rule changes).

## What Changes

- Constitution: new requirement "G9 Evidence, not reports", with its
  entrenchment: no revision removes or weakens it.
- `tools/constitution_check.py`: G9's text is pinned by its digest beyond
  the lock, so a relock cannot change or remove it (changing it requires
  editing the tool, which the review sees).
- `docs/openspec/config.yaml` task rule and `AGENTS.md`: a task is marked
  done only with its tests, where they ran and their result.
- Canon revision 0.4.1.3.139.

## Impact

spec.md (new requirement, relocked), constitution_check.py, config.yaml,
AGENTS.md, revisions.md, Version.h, CHANGELOG. No compiler behavior other
than the `--version` text.
