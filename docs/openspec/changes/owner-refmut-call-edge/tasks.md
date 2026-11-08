## 0. Approval

- [x] 0.1 Q1-Q3 answered and the design approved (maintainer, 2026-11-06; G7)

## 1. Baseline

- [x] 1.1 Suite state and LLVM IR/TIR snapshots of main recorded before the change

## 2. Analysis (phase 5)

- [x] 2.1 `ref mut` owner parameters known while a routine is analyzed; checked at the end of the body, `Return` and `Exit`
- [x] 2.2 `Return` and `Exit` cross `ensure` as departures of the try frames
- [x] 2.3 A `ref mut` owner argument joins the accumulation of the enclosing try frames

## 3. Tests and records

- [x] 3.1 Negative and positive tests of design.md; `pointers-owner-freed-by-callee` moved to diagnostics; trap 17 runtime test added
- [x] 3.2 CANON-25 sentence (if Q1), pointers spec, STATUS, ARCHITECTURE, item 2.4 of `safe-pointers-followups`, CHANGELOG, revision line

## 4. Gates

- [x] 4.1 E19 gates of design.md (local, unidentified cloud container): Debug and Release suites 1176/1176, unit tests, ASan+UBSan 1 MiB 1176/1176, string_leak_check 72/72, owner_safety_check 43/43, stack_frames 0 grown, fuzz 1 and 7, oracle, backend_gaps, grammar, openspec, constitution_check, version_check; LLVM IR (Debug, Release) and TIR against main fa1a37c: only the 16 newly rejected test programs differ (local evidence; the CI snapshot job runs only with the `refactor` label)
- [x] 4.2 CI on PR #146 (head 36ad0b4), all completed: success for Linux
  (clang, Debug), Windows (clang, MSVC ABI, Debug), Windows Debug stack
  budget, Suite under ASan+UBSan 1 MiB stack, mutation fuzz seeds 1 and 7,
  arithmetic oracle, Version and tag, Linux package, Windows package,
  CodeQL (actions, c-cpp, python), SonarCloud Code Analysis; skipped by
  their conditions: IR snapshot against the base (refactor), Tag and
  publish. Merged into main as 7401b1a by the maintainer.
