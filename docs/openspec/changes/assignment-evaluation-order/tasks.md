Nothing below starts before the design's approval is recorded with its date.

## 1. Design

- [x] 1.1 Change opened before F6 step 4 (Marcelo Fortes, 2026-11-04); direction: R before the target, compound assignment as its own operation
- [ ] 1.2 Approval of `design.md` (the CANON-5 text, Q1-Q3) recorded with its date

## 2. Implementation

- [ ] 2.1 CANON-5 text; constitution relocked
- [ ] 2.2 Code generation: every assignment form evaluates R, then the place once, then stores; String append; R's value kept alive
- [ ] 2.3 Compound assignment: R, the place once, its read, op, the store; one arithmetic entry point on values; the typed IR's compound `Assign`
- [ ] 2.4 `repro/` as tests in `tests/runtime/`; Q1 as decided
- [ ] 2.5 Checking runtime (`INOX_OWN_QUARANTINE`) and `tools/owner_safety_check.py`; the reproductions report nothing
- [ ] 2.6 Revision line, kCanonRevision, CHANGELOG (changed behavior), STATUS, ARCHITECTURE

## 3. Gates

- [ ] 3.1 Suite, owner safety check, IR snapshot with every difference declared and classified, TIR snapshot, `tools/stack_frames.py`, fuzz 1 and 7, differential, backend_gaps, openspec, constitution_check, version_check
- [ ] 3.2 CI: sanitized suite (1 MiB), Windows, `windows-stack`, `ir-snapshot`
