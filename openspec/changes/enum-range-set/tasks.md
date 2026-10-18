## 0. Decisions

- [x] 0.1 Record Marcelo's answers to Q1, Q2, Q4–Q7 (canon v3.50)
- [x] 0.2 Q3 decided: `PutLn(E)` prints the literal's name (canon v3.51)

## 1. Phase A — Enum (own PR)

- [x] 1.1 Enum types and literal constants in the analyzer; strict initialization; no implicit Integer conversion
- [x] 1.2 Lowering: i64 ordinals (see design), `Ord`, checked `TEnum(I)`, comparisons, printing the literal's name (Q3)
- [x] 1.3 Enums in fields, elements, State, parameters, results; `for` over enum ranges
- [x] 1.4 case coverage for enums
- [x] 1.5 Tests: runtime enums, enum-case, enum-conversion-range; diagnostics enum-implicit-integer, enum-without-initializer, enum-case-incomplete
- [x] 1.6 Gates and docs for Phase A

## 2. Phase B — Range (own PR)

- [x] 2.1 Range subtypes over the Integer family; compile-time and run-time checks (Q1); a Char Range is declared and waits for Char values
- [x] 2.2 Low/High on range and enum types; checked Succ/Pred (Q5)
- [x] 2.3 case coverage for ranges
- [x] 2.4 Tests: runtime ranges, range-runtime-out-of-range; diagnostics range-constant-out-of-range, range-empty
- [x] 2.5 Gates and docs for Phase B

## 3. Phase C — Set (own PR)

- [x] 3.1 `Set<T>` type with base checks (enum or finite range, Q7 size limit)
- [x] 3.2 Lowering: bit vector, `in`, comparisons, the six operations (Q6 syntax), literal in typed context
- [x] 3.3 Tests: runtime sets; diagnostics set-invalid-base
- [x] 3.4 grammar.ebnf for the set literal; gates and docs for Phase C

## 4. Documentation (each phase)

- [x] 4.1 Canon change log and Layer B matrix; pointers from CANON-13 to the new specs on archive
- [x] 4.2 docs/BACKEND_GAPS.md, docs/site/index.html, CHANGELOG.md

## 5. Gates (E19, each phase)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG
- [x] 5.5 `tools/eh_state_balance.py` and `tools/grammar_consistency.py` pass
- [ ] 5.6 Linux and Windows CI green
