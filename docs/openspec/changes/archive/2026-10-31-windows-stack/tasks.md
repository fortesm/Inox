## 1. Stack

- [x] 1.1 `/STACK:8388608` for `inox` on Windows only
- [x] 1.2 Gates job `windows-stack`: head and base measured, half-reserve and 5% limits, numbers reported

## 2. Error places

- [x] 2.1 `thread_local` places; every SemanticError handler checked to rethrow
- [x] 2.2 Test `position-nested-generic`: the innermost body's line through two instances

## 3. Law and documents

- [x] 3.1 E19 (relock), `docs/ARCHITECTURE.md`, revision r3.113, kCanonRevision, CHANGELOG

## 4. Gates

- [x] 4.1 Suite, openspec, constitution_check, version_check; the Windows CI suite and the new job on this pull request
