## 0. Approval

- [x] 0.1 Q1-Q3 answered (maintainer, 2026-11-05)
- [ ] 0.2 Approval of the revised `design.md` recorded (who, date)

## 1. Analysis (phase 5)

- [ ] 1.1 Reachability; `leave`, `continue`, `Return`, `Raise`, `Exit` and `Retry` end the path, recording their set first
- [ ] 1.2 Merges of if, unless, case and try unite the reachable sets
- [ ] 1.3 Loop frames: exit set (`leave`, true `until`), next-round set (`continue`, false `until`); end of round and after loop; inner-scope owners dropped
- [ ] 1.4 Try frames: accumulation of every owner given away in the body; handlers start from it
- [ ] 1.5 `ensure` applied to every recorded departure (`out(S)`), forwarded to its target
- [ ] 1.6 `Retry` checked on the set after `ensure` against the try's start

## 2. Tests and records

- [ ] 2.1 Negative and positive tests of design.md; `pointer-moved-owner` moved to diagnostics; `pointers-owner-freed-by-callee` added (trap 17)
- [ ] 2.2 CANON-25 rule 3 scenarios, STATUS, the "As implemented" note of `safe-pointers`, item 2.1 of `safe-pointers-followups`, CHANGELOG, revision line

## 3. Gates

- [ ] 3.1 E19 gates of design.md
- [ ] 3.2 CI
