## 1. Decisions

- [x] 1.1 Record Marcelo's answers of 2026-10-18 (strict rule for every array, Fill, enum field defaults, Include/Exclude) as r3.81; open question 2 leaves the list
- [x] 1.2 Record what stays open: array fields of structs; enum fields reached through a generic type parameter

## 2. Compiler

- [x] 2.1 `Fill` predefined name; parser rejects `with(` in an expression with the new set function names
- [x] 2.2 Analyzer: arrays need an initializer (locals and State), with the Fill the element type suggests
- [x] 2.3 Analyzer: `Fill(Value)` typed by its target, by element or by row, Range checks, owners only with None
- [x] 2.4 Analyzer: enum struct fields need a default
- [x] 2.5 Analyzer: Include/Exclude replace With/Without; a set function as a statement is an error
- [x] 2.6 Emitter: `emitFill`, Value evaluated once, copied into a temporary held to the end of the statement

## 3. Programs and tests

- [x] 3.1 Migrate the suite and the examples (arrays filled, routines named Fill renamed, Include/Exclude)
- [x] 3.2 Tests: runtime array-fill (with .in), array-fill-range; diagnostics array-without-initializer, enum-array-without-initializer, state-array-without-initializer, array-fill-without-target, array-fill-type, array-fill-owner, fill-predefined, enum-field-without-default, set-function-statement, set-with-renamed, set-without-renamed

## 4. Documentation

- [x] 4.1 Constitution (CANON-2, CANON-5, CANON-13), relocked; capability specs; revisions.md r3.81
- [x] 4.2 grammar.ebnf and tools/grammar_consistency.py; docs/site/index.html, CHANGELOG.md, docs/STATUS.md, AGENTS.md

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG; `tools/string_leak_check.py` passes
- [x] 5.5 `tools/eh_state_balance.py`, `tools/grammar_consistency.py`, `openspec validate --all --strict` and `tools/constitution_check.py check` pass
- [ ] 5.6 Linux and Windows CI green
