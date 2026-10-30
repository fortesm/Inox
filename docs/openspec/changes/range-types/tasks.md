## 1. Decision

- [x] 1.1 Record Marcelo's decision of 2026-10-30 as r3.106 and close open question 3

## 2. Compiler

- [x] 2.1 Track the Range an expression's arithmetic belongs to; reject two Ranges, or a Range and an Integer value, in operators, `in` and IIf
- [x] 2.2 Assignments, arguments and results: no implicit Range to Integer, no Integer value into a Range; constants inside the bounds still enter
- [x] 2.3 `N := M + 1` declares a value of the Range, checked when stored

## 3. Library, tests and examples

- [x] 3.1 Std.DateTime conversions; ranges, sets, array-fill-range, range-runtime-out-of-range and cards updated
- [x] 3.2 New diagnostics (range-mix-two-ranges, range-mix-integer-variable, range-into-integer-parameter, range-from-other-range, range-from-integer-variable, range-compare-integer-variable, range-in-other-set, range-conditional-mix) and runtime tests (range-types, range-inferred-out-of-range)

## 4. Documents

- [x] 4.1 CANON-13 Range (relock), language/ranges, stdlib/datetime, manual, CHANGELOG

## 5. Gates (E19)

- [x] 5.1 Suite, ASan+UBSan with a 1 MiB stack, mutation fuzz seeds 1 and 7, backend_gaps 0 BUG, eh_state_balance, string_leak_check, grammar_consistency, openspec validate, constitution_check

Results (2026-10-30, unidentified cloud container, no .inox-machine): suite 1053/1053, also under ASan+UBSan (g++) with a 1 MiB stack; fuzz seed 1 OK=287 REJECTED=3713, seed 7 OK=296 REJECTED=3704, no crash; backend_gaps OK=54; eh_state_balance 2/2; string_leak_check 62/62; grammar_consistency 5/5; openspec 41/41; constitution_check 190 units, 0 problems.
