## Why

The maintainer decided on 2026-11-08 that the numbering of the
documentation follows the numbering of the release: a canon revision is
labelled by the compiler version and the documentation revision.

Approval: Marcelo Fortes, 2026-11-08 (G2: constitution text, VERSIONING).
Risk: low (labels and checks; no rule of the language changes).

## What Changes

- VERSIONING "Canon revision": the label is `MAJOR.MINOR.RELEASE.S.N`, the
  compiler version of Version.h then the documentation revision (series
  3); `N` grows by one with every recorded decision and is never reset; a
  new compiler version changes the prefix and keeps `S.N`. r3.137 is
  0.4.1.3.137; the next decision is 0.4.1.3.138. Earlier labels stay.
- `kCanonRevision` is "0.4.1.3.137"; `inox --version` prints `inox
  0.4.1.BUILD pre-alpha (canon 0.4.1.3.137)`.
- The suites read the new label; `tools/version_check.py` also checks that
  the label starts with the version.

## Impact

Version.h, revisions.md, spec.md (VERSIONING), run-tests.sh,
run-tests.ps1, version_check.py, AGENTS.md, CHANGELOG. No compiler
behavior other than the `--version` text.
