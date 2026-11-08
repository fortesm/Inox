## 0. Approval

- [x] 0.1 Q1-Q4 answered and the design approved (maintainer, 2026-11-08; G7)

## 1. Baseline

- [x] 1.1 Suite state and LLVM IR/TIR snapshots of main recorded before the change

## 2. Analysis (phase 5)

- [x] 2.1 Raising points named: Raise, bare Raise, Retry, user calls, raising built-ins from the built-in table
- [x] 2.2 A raising point with a `ref mut` owner given away departs as an exception exit through the try frames (catch-all handler, `ensure`)
- [x] 2.3 The r3.135 check at the routine's edge

## 3. Tests and records

- [x] 3.1 Negative and positive tests of design.md; the runtime test moved; the trap 17 emission test
- [x] 3.2 CANON-25 text (if Q1), pointers spec, STATUS, ARCHITECTURE, item 2.5 of `safe-pointers-followups`, CHANGELOG, revision line

## 4. Gates

- [x] 4.1 E19 gates (local, unidentified cloud container): Debug and Release suites 1197/1197, unit tests (with RaisingCallsTests), ASan+UBSan 1 MiB 1197/1197, string_leak_check 72/72, owner_safety_check 43/43, stack_frames 0 grown, fuzz 1 and 7, oracle, backend_gaps, grammar, openspec, constitution_check, version_check; LLVM IR (Debug, Release) and TIR against the compiler of main 7401b1a over these programs: only the 18 newly rejected tests differ (local evidence; the CI snapshot job runs only with the `refactor` label)
- [ ] 4.2 CI
