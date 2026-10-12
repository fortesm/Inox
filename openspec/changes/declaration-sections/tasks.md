## 1. Baseline

- [ ] 1.1 Record `--emit-llvm` and `--dump-types` for every test and example before the switch

## 2. Parser and AST

- [ ] 2.1 Add the Struct, Range, Enum, Const and State declaration nodes
- [ ] 2.2 Parse the three sections into the nodes, reusing `parseTypeName` and the expression parser; diagnostics name the line
- [ ] 2.3 Update parser dump output and its expectation files

## 3. Consumers

- [ ] 3.1 Semantic analyzer reads the nodes; delete its section token scanners
- [ ] 3.2 Emitter reads the nodes; delete `collectStructDefinitions`' token walk and the State token walk
- [ ] 3.3 Compare against the 1.1 baseline: identical IR and types

## 4. New behavior

- [ ] 4.1 Array struct fields: layout, element access, defaults, copy
- [ ] 4.2 State of struct and array type, with and without initializer

## 5. Tests

- [ ] 5.1 tests/runtime/struct-array-field.inox, state-aggregates.inox; trap struct-array-field-out-of-bounds.inox
- [ ] 5.2 tests/diagnostics/struct-field-without-type.inox
- [ ] 5.3 backend_gaps probes for array fields and aggregate State

## 6. Documentation

- [ ] 6.1 Canon: change-log entry, Layer B backend matrix
- [ ] 6.2 docs/BACKEND_GAPS.md, docs/site/index.html, CHANGELOG.md

## 7. Gates (E19)

- [ ] 7.1 `bash scripts/run-tests.sh` passes
- [ ] 7.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [ ] 7.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [ ] 7.4 `tools/backend_gaps.py` reports 0 BUG
- [ ] 7.5 `tools/eh_state_balance.py` and `tools/grammar_consistency.py` pass
- [ ] 7.6 Linux and Windows CI green
