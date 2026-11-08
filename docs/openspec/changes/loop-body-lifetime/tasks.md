## 0. Approval

- [x] 0.1 Q1-Q5 answered and the design approved (maintainer, 2026-11-08; G7)

## 1. Baseline

- [x] 1.1 Suite state and LLVM IR/TIR snapshots of main recorded before the change

## 2. Lowering

- [x] 2.1 The locals each loop body ends (decision 1, Q5)
- [x] 2.2 Their release at the end of a round, `continue`, `leave`, true `until`, `Return`, `Exit` and the exception path, in the order of decision 3

## 3. Tests and records

- [x] 3.1 Tests of design.md
- [x] 3.2 Pointers spec, STATUS, item 2.2 of `safe-pointers-followups`, CHANGELOG, revision line (if approved)

## 4. Gates

- [x] 4.1 E19 gates (local, unidentified cloud container, after the conditional-locals tests): Debug and Release suites 1201/1201, unit tests, ASan+UBSan 1 MiB 1201/1201, string_leak_check 75/75, owner_safety_check 46/46, stack_frames 0 grown (new recursion functions within budget), fuzz 1 and 7, oracle, backend_gaps, grammar, openspec, constitution_check, version_check; against the compiler of PR #149 over these programs: LLVM IR differs for the 4 programs listed in the PR and the 2 added conditional-locals tests, TIR only by `ends` lines; local evidence
- [ ] 4.2 CI
