## 0. Approval

- [ ] 0.1 Q1-Q3 answered and the design approved (maintainer, date; G7)

## 1. Baseline

- [ ] 1.1 Suite state and LLVM IR/TIR snapshots of main recorded before the change

## 2. Analysis (phase 5)

- [ ] 2.1 Raising points named: Raise, bare Raise, Retry, user calls, raising built-ins from the built-in table
- [ ] 2.2 A raising point with a `ref mut` owner given away departs as an exception exit through the try frames (catch-all handler, `ensure`)
- [ ] 2.3 The r3.135 check at the routine's edge

## 3. Tests and records

- [ ] 3.1 Negative and positive tests of design.md; the runtime test moved; the trap 17 emission test
- [ ] 3.2 CANON-25 text (if Q1), pointers spec, STATUS, ARCHITECTURE, item 2.5 of `safe-pointers-followups`, CHANGELOG, revision line

## 4. Gates

- [ ] 4.1 E19 gates
- [ ] 4.2 CI
