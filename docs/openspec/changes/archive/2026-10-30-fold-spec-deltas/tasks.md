## 1. Review

- [x] 1.1 Compare the four deltas with the constitution and the compiler; check every named test exists
- [x] 1.2 Update the Option<X> wording (pointers) and add the r3.99 invariant rule (contracts)

## 2. Fold

- [x] 2.1 Archive debug-assert, design-by-contract, method-reuse and safe-pointers with their deltas applied
- [x] 2.2 Purpose of stdlib/debug, language/contracts, language/pointers; Purpose of language/structs

## 3. Records

- [x] 3.1 Revision r3.107, kCanonRevision, CHANGELOG

## 4. Gates (E19)

- [x] 4.1 Suite, ASan+UBSan with a 1 MiB stack, mutation fuzz seeds 1 and 7, backend_gaps 0 BUG, eh_state_balance, string_leak_check, grammar_consistency, openspec validate, constitution_check

Results (2026-10-30, unidentified cloud container, no .inox-machine): suite 1053/1053, also under ASan+UBSan (g++) with a 1 MiB stack; fuzz seed 1 OK=287 REJECTED=3713, seed 7 OK=296 REJECTED=3704, no crash; backend_gaps OK=54; eh_state_balance 2/2; string_leak_check 62/62; grammar_consistency 5/5; openspec 35/35; constitution_check 190 units, 0 problems.
