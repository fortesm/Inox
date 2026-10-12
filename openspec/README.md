<!-- SPDX-License-Identifier: MPL-2.0 -->
# Inox specifications (OpenSpec)

This folder holds the Inox language and compiler specifications in the
OpenSpec format (OpenSpec 1.14.1, adopted 2026-10-12, canon v3.48, G7).

## Layout

```
openspec/
├── config.yaml        Inox context and the rules every proposal follows
├── specs/             the current truth, one capability per folder
│   └── language/<capability>/spec.md
├── changes/           proposed changes, one folder each
│   ├── <name>/        proposal.md, design.md, tasks.md, specs/ (deltas)
│   └── archive/       finished changes, dated; ADR-0001..0017 are here
└── migration/
    └── CANON_MAP.md   every unit of docs/INOX_CANONICAL.md and where it goes
```

- A **spec** states requirements (`### Requirement:`, with SHALL) and
  scenarios (`#### Scenario:`, WHEN/THEN). Every Inox scenario names the test
  that proves it, or says "test to add".
- A **change** holds only the delta (`ADDED`, `MODIFIED`, `REMOVED`, `RENAMED`
  requirements). Archiving it merges the delta into `specs/` and moves the
  folder to `archive/` with its date.

## Workflow

1. `/opsx:propose` (or `openspec new change <name>`): write the proposal.
   A Layer A change quotes Marcelo Fortes's dated approval; open questions
   stay listed until he answers.
2. Marcelo reviews the proposal.
3. `/opsx:apply`: implement, ticking `tasks.md`; the E19 gates come last.
4. Merge the pull request.
5. `/opsx:archive` (or `openspec archive <name> -y`).

`openspec validate --all --strict` must pass; CI runs it. Set
`OPENSPEC_TELEMETRY=0`.

## Migration from the canon

`docs/INOX_CANONICAL.md` is still the law (G7). It is being migrated in phases
(plan approved by Marcelo Fortes on 2026-10-12):

1. Inventory: `tools/canon_trace.py` splits the canon into units covering
   every line; `migration/CANON_MAP.md` gives each unit a status and a
   destination. CI runs `tools/canon_trace.py check`.
2. Capabilities move to `specs/` one by one; each becomes law when Marcelo
   approves it.
3. ADRs become archived changes (done: `changes/archive/*-adr-*`, verbatim,
   checked by the tool; ADR-0005 and ADR-0006 also keep their original files
   from before 2026-06-15). Governance and engineering rules move to
   `AGENTS.md` and `docs/ENGINEERING.md`; the change log is frozen in
   `docs/history/`.
4. When no unit is `planned`, the canon is tagged `canon-final` in git and
   removed.

After editing the canon, run `python3 tools/canon_trace.py update`; if a
mirrored unit changed, update its spec and run
`python3 tools/canon_trace.py accept <unit>`.
