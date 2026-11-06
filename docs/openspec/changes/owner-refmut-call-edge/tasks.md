## 0. Approval

- [ ] 0.1 Q1-Q3 answered and the design approved (maintainer, date; G7)

## 1. Baseline

- [ ] 1.1 Suite state and LLVM IR/TIR snapshots of main recorded before the change

## 2. Analysis (phase 5)

- [ ] 2.1 `ref mut` owner parameters known while a routine is analyzed; checked at the end of the body, `Return` and `Exit`
- [ ] 2.2 `Return` and `Exit` cross `ensure` as departures of the try frames
- [ ] 2.3 A `ref mut` owner argument joins the accumulation of the enclosing try frames

## 3. Tests and records

- [ ] 3.1 Negative and positive tests of design.md; `pointers-owner-freed-by-callee` moved to diagnostics; trap 17 runtime test added
- [ ] 3.2 CANON-25 sentence (if Q1), pointers spec, STATUS, ARCHITECTURE, item 2.4 of `safe-pointers-followups`, CHANGELOG, revision line

## 4. Gates

- [ ] 4.1 E19 gates of design.md
- [ ] 4.2 CI
