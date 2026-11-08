## 1. Implementation

- [x] 1.1 Through-pointer invariant places in phase 5 and the TIR
- [x] 1.2 `Value(P)` free of side effects in a checked place

## 2. Tests and records

- [x] 2.1 Trap, positive, diagnostic, TIR and module tests
- [x] 2.2 Contracts spec, STATUS, item 3.1, CHANGELOG, revision line

## 3. Gates

- [x] 3.1 E19 gates (local): Debug and Release suites 1213/1213, unit tests, ASan+UBSan 1 MiB 1213/1213, string_leak_check 75/75, owner_safety_check 54/54, stack_frames 0 grown, fuzz 1 and 7, oracle, backend_gaps, grammar, openspec, constitution_check, version_check; LLVM IR (Debug, Release) and TIR against the compiler of main 8f300f6: only the 10 new tests that main compiles differently differ (local evidence)
- [ ] 3.2 CI
