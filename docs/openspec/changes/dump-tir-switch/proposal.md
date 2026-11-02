## Why

Q4 of the design of F6 (change `typed-ir`) approved `--dump-tir`, a driver
mode that prints the typed IR; step 1 (r3.120) implemented it and
documented it in `docs/STATUS.md` and the manual. CANON-23 still listed
only `--parse-only`, `--dump-tokens` and `--dump-types` as developer
switches, so the constitution did not say what the driver accepts.

Approval: Marcelo Fortes, 2026-11-02: CANON-23 lists `--dump-tir` as a
developer switch, not as a public compilation command, with the normal
governance (revision line, constitution relocked, gates). The public
commands, their syntax and the grammar of the language do not change.

Touches: CANON-23 (the sentence on developer switches moves into its own
requirement, "CANON-23 Developer switches", since the driver commands'
text would pass the 500 characters OpenSpec allows; constitution
relocked). Compiler phases:
none (the driver already accepts the switch). Not changed:
`docs/ARCHITECTURE.md` (no boundary, owner or deviation).

Risk: normal.

Status: **implemented** (canon r3.121).

## What Changes

1. CANON-23, new requirement "Developer switches": "`--parse-only`,
   `--dump-tokens`, `--dump-types` and `--dump-tir` are developer
   switches", with a scenario naming the tests of each switch.
2. Revision r3.121, kCanonRevision, CHANGELOG.

## Out of scope

The public commands, the help text's public part, the grammar.
