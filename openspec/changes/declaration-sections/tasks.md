## 1. Baseline

- [x] 1.1 Record `--emit-llvm` and `--dump-types` for every test, example and stdlib file before the switch (462 programs)

## 2. Parser and AST

- [x] 2.1 Typed declarations for Struct, Range, Enum, Const and State in the section node
- [x] 2.2 Parse the three sections into the nodes, reusing `parseTypeName` and the expression parser; diagnostics name the line
- [x] 2.3 Parser dumps: none prints section tokens, nothing to update

## 3. Consumers

- [x] 3.1 Semantic analyzer reads the nodes; delete its section token scanners
- [x] 3.2 Emitter reads the nodes; delete `collectStructDefinitions`' token walk and the State token walk
- [x] 3.3 Compare against the 1.1 baseline: identical IR and types except the three intended changes (`Type :` message, two State struct programs now compile)

## 4. New behavior

- [x] 4.1 Array struct fields: layout, element access, defaults, copy
- [x] 4.2 State of struct and array type, with and without initializer

## 5. Tests

- [x] 5.1 tests/runtime/struct-array-field.inox, state-aggregates.inox; trap struct-array-field-out-of-bounds.inox
- [x] 5.2 Diagnostics: struct-field-without-type, struct-field-slice, state-slice, struct-array-field-default, struct-field-default-expression, state-grouped, type-assignment-form, range-without-bounds, enum-literals-per-line; semantic-valid type-section-forms
- [x] 5.3 backend_gaps probes for array fields and aggregate State

## 6. Documentation

- [x] 6.1 Canon: change-log entry, Layer B backend matrix
- [x] 6.2 docs/BACKEND_GAPS.md, docs/site/index.html, CHANGELOG.md

## 7. Gates (E19)

- [x] 7.1 `bash scripts/run-tests.sh` passes
- [x] 7.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 7.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 7.4 `tools/backend_gaps.py` reports 0 BUG
- [x] 7.5 `tools/eh_state_balance.py` and `tools/grammar_consistency.py` pass
- [ ] 7.6 Linux and Windows CI green
