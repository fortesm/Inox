## 1. The constitution

- [x] 1.1 Read the whole canon (r3.60) and its change log; list duplicates, stale status and superseded text (design.md)
- [x] 1.2 `openspec/specs/constitution/spec.md`: G1–G8, CANON-1 to CANON-23, VERSIONING, FUTURE, each with test-named scenarios
- [x] 1.3 `engineering.md`: E1–E23
- [x] 1.4 `revisions.md`: version, revision r3.61, open questions, revision log
- [x] 1.5 Three diagnostics for rules without a test: `mut-parameter-reserved`, `do-reserved`, `double-is-not-inox`

## 2. The lock

- [x] 2.1 `tools/constitution_check.py` (`check`, `relock --approval`, `list`), with the test-reference check for every spec
- [x] 2.2 `constitution.lock` written with `relock --approval constitution`
- [x] 2.3 Negative controls: an edited requirement, a removed one, an unknown approval and a missing test file each fail `check`
- [x] 2.4 CI runs `check` instead of `canon_trace.py check`

## 3. Retiring the canon

- [x] 3.1 Final `canon_trace.py` map: 268 units, none `planned`; frozen in `docs/history/CANON_MAP.md`
- [x] 3.2 Change log frozen verbatim in `docs/history/canon-changelog.md`
- [x] 3.3 `docs/INOX_CANONICAL.md`, the stale `docs/index.html` and `tools/canon_trace.py` removed
- [ ] 3.4 After the merge: tag the last commit that holds the canon as `canon-final` (`git tag canon-final b517f90`) and push the tag

## 4. Everything that pointed at the canon

- [x] 4.1 Capability specs become law under the constitution (Purpose texts; stale Purpose notes fixed)
- [x] 4.2 `AGENTS.md` (start here; no duplicated language rules), `README.md`, `openspec/README.md`, `openspec/config.yaml`, `CHANGELOG.md`, manual, grammar header and conformance gaps, `ci/windows-known-failures.txt` comment
- [x] 4.3 `docs/STATUS.md` rewritten from the code; `docs/ROADMAP.md`; `BUILD_AND_TEST_INSTRUCTIONS.md` with toolchain, packages and the release checklist
- [x] 4.4 Version: `kCanonRevision` r3.61; both runners read `revisions.md`; `tools/grammar_consistency.py` reads the CANON-20 table from the constitution
- [x] 4.5 Code comments citing `SECTION NN` now cite CANON anchors
- [x] 4.6 Release scripts: ship the manual, the constitution and libinoxrt (they referenced a missing file)

## 5. Gates

- [x] 5.1 `bash scripts/run-tests.sh` (Linux)
- [x] 5.2 `openspec validate --all --strict`; `tools/constitution_check.py check`; `tools/grammar_consistency.py` (with a negative control on the CANON-20 table)
- [x] 5.3 `scripts/package-release.sh` builds a package with the manual, the constitution and libinoxrt
- [ ] 5.4 CI green on Linux and Windows
