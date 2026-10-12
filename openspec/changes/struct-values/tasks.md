## 1. Semantic analysis

- [x] 1.1 Accept literal defaults for every lowered scalar field type (Integer family, Natural, Float, Float32, Bool), range-checking constants
- [x] 1.2 Reject a struct that contains itself ("struct contains itself: T"), with tests/diagnostics/struct-recursive.inox
- [x] 1.3 Accept struct element types in `Array[L..H] T`

## 2. Lowering

- [x] 2.1 Struct layout for all scalar field types and nested structs (forward references, no ordering needed)
- [x] 2.2 `emitStructDefaults` shared by declarations, construction and array elements
- [x] 2.3 Named construction expression
- [x] 2.4 Struct initializers, including grouped declarations
- [x] 2.5 Arrays of struct: element field access and defaults

## 3. Tests

- [x] 3.1 tests/runtime/struct-defaults.inox, struct-construction.inox, struct-initializers.inox, struct-nested.inox, array-of-struct.inox
- [x] 3.2 Trap tests/runtime/array-of-struct-out-of-bounds.inox
- [x] 3.3 Flip backend_gaps probes `grouped-decl-struct` and `named-struct-construction` to OK; add probes for nested structs and arrays of struct

## 4. Documentation

- [x] 4.1 Canon: change-log entry v3.52 and Layer B (the pointer from CANON-9 comes when Marcelo approves the capability, G7)
- [x] 4.2 docs/BACKEND_GAPS.md, docs/site/index.html, CHANGELOG.md

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG
- [x] 5.5 `tools/eh_state_balance.py` and `tools/grammar_consistency.py` pass
- [ ] 5.6 Linux and Windows CI green
