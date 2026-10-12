## 1. Semantic analysis

- [ ] 1.1 Accept literal defaults for every lowered scalar field type (Integer family, Natural, Float, Float32, Bool), range-checking constants
- [ ] 1.2 Reject a struct that contains itself ("struct contains itself: T"), with tests/diagnostics/struct-recursive.inox
- [ ] 1.3 Accept struct element types in `Array[L..H] T`

## 2. Lowering

- [ ] 2.1 Struct layout for all scalar field types and nested structs, emitted in dependency order
- [ ] 2.2 `emitStructDefaults` shared by declarations, construction and array elements
- [ ] 2.3 Named construction expression
- [ ] 2.4 Struct initializers, including grouped declarations
- [ ] 2.5 Arrays of struct: element field access and defaults

## 3. Tests

- [ ] 3.1 tests/runtime/struct-defaults.inox, struct-construction.inox, struct-initializers.inox, struct-nested.inox, array-of-struct.inox
- [ ] 3.2 Trap tests/runtime/array-of-struct-out-of-bounds.inox
- [ ] 3.3 Flip backend_gaps probes `grouped-decl-struct` and `named-struct-construction` to OK; add probes for nested structs and arrays of struct

## 4. Documentation

- [ ] 4.1 Canon: change-log entry, CANON-20 backend matrix, pointer from CANON-9 to openspec/specs/language/structs
- [ ] 4.2 docs/BACKEND_GAPS.md, docs/site/index.html, CHANGELOG.md

## 5. Gates (E19)

- [ ] 5.1 `bash scripts/run-tests.sh` passes
- [ ] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [ ] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [ ] 5.4 `tools/backend_gaps.py` reports 0 BUG
- [ ] 5.5 `tools/eh_state_balance.py` and `tools/grammar_consistency.py` pass
- [ ] 5.6 Linux and Windows CI green
