## 1. Decisions

- [x] 1.1 Record Marcelo's approval of 2026-10-19 as r3.85

## 2. Compiler

- [x] 2.1 Analyzer: String selectors, static String choices, no ranges, overlaps, else required
- [x] 2.2 Emitter: String choices compared with `__inox_str_equal`

## 3. Tests

- [x] 3.1 Runtime: case-strings (with .in); diagnostics case-string-without-else, case-string-range, case-string-duplicate, case-string-not-static, case-string-integer-choice, case-float-selector (replaces case-string-selector)

## 4. Documentation

- [x] 4.1 Constitution CANON-12, relocked; `language/case-statement`; revisions.md r3.85
- [x] 4.2 docs/site/index.html, CHANGELOG.md, docs/STATUS.md, docs/ROADMAP.md

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG; `tools/string_leak_check.py` passes
- [x] 5.5 `tools/grammar_consistency.py`, `openspec validate --all --strict` and `tools/constitution_check.py check` pass
- [x] 5.6 Linux and Windows CI green (merged into main; main green on Linux and Windows, 2026-10-29)
