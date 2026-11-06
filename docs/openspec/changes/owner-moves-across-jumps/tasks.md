## 0. Approval

- [x] 0.1 Q1-Q3 answered (maintainer, 2026-11-05)
- [x] 0.2 Approval of the revised `design.md` recorded (maintainer, 2026-11-05)

## 1. Analysis (phase 5)

- [x] 1.1 Reachability; `leave`, `continue`, `Return`, `Raise`, `Exit` and `Retry` end the path, recording their set first
- [x] 1.2 Merges of if, unless, case and try unite the reachable sets
- [x] 1.3 Loop frames: exit set (`leave`, true `until`), next-round set (`continue`, false `until`); end of round and after loop; inner-scope owners dropped
- [x] 1.4 Try frames: accumulation of every owner given away in the body; handlers start from it
- [x] 1.5 `ensure` applied to every recorded departure (`out(S)`), forwarded to its target
- [x] 1.6 `Retry` checked on the set after `ensure` against the try's start

## 2. Tests and records

- [x] 2.1 Negative and positive tests of design.md; `pointer-moved-owner` moved to diagnostics; `pointers-owner-freed-by-callee` added (trap 17)
- [x] 2.2 CANON-25 rule 3 scenarios, STATUS, the "As implemented" note of `safe-pointers`, item 2.1 of `safe-pointers-followups`, CHANGELOG, revision line

## 3. Gates

- [x] 3.1 E19 gates of design.md, run locally on commit adfb557 (unidentified
  cloud container, no .inox-machine): Debug and Release builds and suites
  1156/1156, compiler unit tests, ASan+UBSan with a 1 MiB stack 1156/1156,
  string_leak_check 69/69, owner_safety_check 40/40, stack_frames (0 grown
  or new against main 5b5a0a4), mutation fuzz seeds 1 and 7, differential
  (arithmetic oracle), backend_gaps, grammar, openspec, constitution_check,
  version_check. LLVM IR and TIR snapshots: recorded with the compiler of
  main 5b5a0a4 and compared locally (Debug and Release IR, TIR); only the
  18 added or moved test programs differ. This comparison is local
  evidence only: the CI job "IR snapshot against the base (refactor)" runs
  only on PRs labelled `refactor` and was skipped.
- [x] 3.2 CI on PR #143 (head adfb557), all completed: success for Linux
  (clang, Debug), Windows (clang, MSVC ABI, Debug), Windows Debug stack
  budget, Suite under ASan+UBSan 1 MiB stack, mutation fuzz seeds 1 and 7,
  arithmetic oracle, Version and tag, Linux package, Windows package,
  CodeQL (actions, c-cpp, python), SonarCloud Code Analysis; skipped by
  their conditions: IR snapshot against the base (refactor), Tag and
  publish. Merged into main as fa1a37c by the maintainer.
