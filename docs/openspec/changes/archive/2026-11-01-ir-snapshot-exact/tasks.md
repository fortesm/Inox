## 1. Tool

- [x] 1.1 `expect` command; `compare --expect` exact; `--allow` only for added or removed programs; unused exceptions fail
- [x] 1.2 `tools/ir_snapshot_selftest.py` (12 checks; a weakened rule makes 3 fail)

## 2. CI and documents

- [x] 2.1 Gates `ir-snapshot` reads `ir-snapshot-expect.json`; CI runs the self-test
- [x] 2.2 `docs/BUILD.md`, `AGENTS.md`, revision r3.115, kCanonRevision, CHANGELOG

## 3. Gates

- [x] 3.1 F3's real difference (`mut-parameter-reserved`, pre-F3 snapshot against main) declared with `expect` and accepted exactly; openspec, constitution_check, version_check
- [x] 3.2 CI
