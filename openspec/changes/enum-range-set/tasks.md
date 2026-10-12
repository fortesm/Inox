## 0. Decisions

- [x] 0.1 Record Marcelo's answers to Q1, Q2, Q4–Q7 (canon v3.50)
- [ ] 0.2 Q3 (enum printing) still open; Phase A leaves enum printing out until it is decided

## 1. Phase A — Enum (own PR)

- [ ] 1.1 Enum types and literal constants in the analyzer; strict initialization; no implicit Integer conversion
- [ ] 1.2 Lowering: i32 ordinals, `Ord`, checked `TEnum(I)`, comparisons, printing only after Q3 is decided
- [ ] 1.3 Enums in fields, elements, State, parameters, results; `for` over enum ranges
- [ ] 1.4 case coverage for enums
- [ ] 1.5 Tests: runtime enums, enum-case, enum-conversion-range; diagnostics enum-implicit-integer, enum-without-initializer, enum-case-incomplete
- [ ] 1.6 Gates and docs for Phase A

## 2. Phase B — Range (own PR)

- [ ] 2.1 Range subtypes over the Integer family and Char; compile-time and run-time checks (Q1)
- [ ] 2.2 Low/High on range and enum types; checked Succ/Pred (Q5)
- [ ] 2.3 case coverage for ranges
- [ ] 2.4 Tests: runtime ranges, range-runtime-out-of-range; diagnostics range-constant-out-of-range, range-empty
- [ ] 2.5 Gates and docs for Phase B

## 3. Phase C — Set (own PR)

- [ ] 3.1 `Set<T>` type with base checks (enum or finite range, Q7 size limit)
- [ ] 3.2 Lowering: bit vector, `in`, comparisons, the six operations (Q6 syntax), literal in typed context
- [ ] 3.3 Tests: runtime sets; diagnostics set-invalid-base
- [ ] 3.4 grammar.ebnf for the set literal; gates and docs for Phase C

## 4. Documentation (each phase)

- [ ] 4.1 Canon change log and Layer B matrix; pointers from CANON-13 to the new specs on archive
- [ ] 4.2 docs/BACKEND_GAPS.md, docs/site/index.html, CHANGELOG.md

## 5. Gates (E19, each phase)

- [ ] 5.1 `bash scripts/run-tests.sh` passes
- [ ] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [ ] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [ ] 5.4 `tools/backend_gaps.py` reports 0 BUG
- [ ] 5.5 `tools/eh_state_balance.py` and `tools/grammar_consistency.py` pass
- [ ] 5.6 Linux and Windows CI green
