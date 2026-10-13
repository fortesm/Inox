<!-- SPDX-License-Identifier: MPL-2.0 -->
# Inox specifications (OpenSpec)

The law of Inox and every change to it, in the OpenSpec format (OpenSpec
1.14.1, adopted 2026-10-12). Start with `specs/constitution/spec.md`.

## Layout

```
openspec/
├── config.yaml        Inox context and the rules every change follows
├── specs/             the law, as it is on main
│   ├── constitution/  spec.md (the law), engineering.md, revisions.md,
│   │                  constitution.lock (G8)
│   └── language/<capability>/spec.md   one topic of the constitution in detail
└── changes/           proposed changes, one folder each
    ├── <name>/        proposal.md, design.md, tasks.md, specs/ (deltas)
    └── archive/       finished changes, dated; ADR-0001..0017 are here
```

- A **spec** states requirements (`### Requirement:`, with SHALL) and
  scenarios (`#### Scenario:`, WHEN/THEN). Every scenario names the test that
  proves it, or says "test to add"; `tools/constitution_check.py` fails when
  a named test file does not exist.
- A **change** holds only the delta (`ADDED`, `MODIFIED`, `REMOVED`,
  `RENAMED` requirements). Archiving it merges the delta into `specs/` and
  moves the folder to `archive/` with its date.
- The constitution (`spec.md`, `engineering.md`) is locked: a change to its
  text is accepted only after `python3 tools/constitution_check.py relock
  --approval <change>`, where the change quotes Marcelo Fortes's approval.

## Workflow

1. `/opsx:propose` (or `openspec new change <name>`): write the proposal. A
   change to the law quotes Marcelo Fortes's dated approval; open questions
   stay listed until he answers.
2. Marcelo reviews the proposal (and, for `Risk: high`, the design).
3. `/opsx:apply`: implement, ticking `tasks.md`; the E19 gates come last.
4. Merge the pull request (only Marcelo merges).
5. `/opsx:archive` (or `openspec archive <name> -y`) in the next branch, and
   add the revision line to `specs/constitution/revisions.md`.

CI runs `openspec validate --all --strict` and `tools/constitution_check.py
check`. Set `OPENSPEC_TELEMETRY=0`.

## History

The canon `docs/INOX_CANONICAL.md` was the law until r3.60. At r3.61 it was
minimized into `specs/constitution/` (Marcelo Fortes, 2026-10-12) and
retired: its change log is in `docs/history/canon-changelog.md`, the place
of each of its units in `docs/history/CANON_MAP.md`, and its last text in the
git tag `canon-final`.
