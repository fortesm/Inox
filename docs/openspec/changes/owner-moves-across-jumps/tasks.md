## 0. Approval

- [ ] 0.1 Approval of `design.md` recorded (who, date, Q1-Q3)

## 1. Analysis

- [ ] 1.1 Reachability and loop frames; `leave`, `continue` (and Q1 exits) end the path
- [ ] 1.2 Merges of if, unless, case and try unite the reachable sets
- [ ] 1.3 End of a loop body with the `continue` set; after the loop with the `leave` set; inner-scope owners dropped; `ensure` added to the sets recorded inside its try
- [ ] 1.4 With Q2: `Retry` against the try's start

## 2. Tests and records

- [ ] 2.1 Diagnostics and runtime tests of design.md; `pointers-moved-owner` moved (Q3)
- [ ] 2.2 CANON-25 rule 3 scenario, STATUS, the "As implemented" note of `safe-pointers`, item 2.1 of `safe-pointers-followups`, CHANGELOG, revision line

## 3. Gates

- [ ] 3.1 E19 gates of design.md
- [ ] 3.2 CI
