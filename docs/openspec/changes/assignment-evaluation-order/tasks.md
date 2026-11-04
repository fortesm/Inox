Nothing below starts before the design's approval is recorded with its date.

## 1. Design

- [x] 1.1 Change opened before F6 step 4 (Marcelo Fortes, 2026-11-04); direction: R before the target, compound assignment as its own operation
- [x] 1.2 `design.md` approved with amendments (Marcelo Fortes, 2026-11-04): the CANON-5 text and the CANON-25 invariant; Q1 a separate change before F6 step 4; Q2 checks in every build; Q3 approved; R preserved; the five kinds of IR difference
- [x] 1.3 Reproductions of an invalidation while the place is evaluated (a target and a read) and of Q3, in `repro/`

## 2. Implementation

- [x] 2.1 CANON-5 and CANON-25 text; constitution relocked
- [x] 2.2 Code generation: every assignment form evaluates R, then the place once, then stores; String append; R's value kept alive when L may run code
- [x] 2.2b Places reached through a pointer checked against their object's generation before an access that follows code able to free it (reads, compound reads, stores), every build; libinoxrt's block identity stated in ARCHITECTURE
- [x] 2.3 Compound assignment: R, the place once, its read, op, the store; one arithmetic entry point on values; the typed IR's compound `Assign`
- [x] 2.4 `repro/` as tests in `tests/runtime/` (output and trap); `reference-argument-owner-replaced.inox` to the change of Q1
- [x] 2.5 Checking runtime (`INOX_OWN_QUARANTINE`) and `tools/owner_safety_check.py`; the reproductions report nothing
- [x] 2.6 Revision line, kCanonRevision, CHANGELOG (changed behavior), STATUS, ARCHITECTURE

## 3. Gates

- [x] 3.1 Suite, owner safety check, IR snapshot with every difference declared and classified in the five kinds of `design.md`, TIR snapshot, `tools/stack_frames.py`, fuzz 1 and 7, differential, backend_gaps, openspec, constitution_check, version_check
- [ ] 3.2 CI: sanitized suite (1 MiB), Windows, `windows-stack`, `ir-snapshot`
