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

- [x] 4.1 E19 gates (local, unidentified cloud container): Debug and Release suites 1199/1199, unit tests, ASan+UBSan 1 MiB 1199/1199, string_leak_check 74/74, owner_safety_check 45/45, stack_frames 0 grown (new recursion functions within budget: emitLoopBody 504, collectBodyLocals 280), fuzz 1 and 7, oracle, backend_gaps, grammar, openspec, constitution_check, version_check; against the compiler of PR #149 (0663469) over these programs: LLVM IR differs for 4 programs (the 2 new tests and pointers-moves-across-jumps, pointers-moves-across-until, whose loops declare owners), TIR for 38, only by their `ends` lines (69 lines; checked by stripping them); local evidence
- [ ] 4.2 CI
