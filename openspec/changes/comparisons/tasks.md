## 1. Implementation

- [x] 1.1 `__inox_str_compare` in libinoxrt, with unit tests in `inoxrt-tests`
- [x] 1.2 Lower String `<` `>` `<=` `>=` through the runtime; Bool `=`/`#` as `i1`
- [x] 1.3 Analyzer: struct comparison is a compile error
- [x] 1.4 Bool ordering false < true (v3.57), lowered with unsigned i1 predicates

## 2. Tests

- [x] 2.1 tests/runtime/strings-ordering (input lines; leak-checked by tools/string_leak_check.py)
- [x] 2.2 diagnostics struct-equality, struct-ordering; runtime bool-ordering
- [x] 2.3 backend_gaps probes string-ordering and bool-comparison (OK; bool-equality renamed in v3.57)

## 3. Documentation

- [x] 3.1 Canon v3.56 and v3.57 (change log, CANON-14, CANON-8 Bool, CANON-9 structs)
- [x] 3.2 docs/site/index.html, CHANGELOG.md

## 4. Gates (E19)

- [x] 4.1 Suite, ASan+UBSan suite, string_leak_check, backend_gaps 0 BUG, eh_state_balance, grammar, openspec, canon_trace
- [ ] 4.2 Linux and Windows CI green
