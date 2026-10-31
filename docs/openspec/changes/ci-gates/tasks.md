## 1. Workflows and tools

- [x] 1.1 Gates workflow: sanitized suite (1 MiB stack), fuzz seeds 1 and 7, differential test, IR snapshot on `refactor`
- [x] 1.2 `tools/version_check.py` in the CI workflow; checked against a drifted README (it fails)
- [x] 1.3 `ir_snapshot.py --allow` for added programs; checked against the D8 baseline

## 2. Law and documents

- [x] 2.1 E19 says where each gate runs; relock; D7 closed in `docs/ARCHITECTURE.md`
- [x] 2.2 Revision r3.111, kCanonRevision, CHANGELOG

## 3. Gates

- [x] 3.1 The Gates workflow itself green on this pull request; suite, openspec, constitution_check
