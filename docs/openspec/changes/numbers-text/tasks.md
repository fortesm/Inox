## 1. Decisions

- [x] 1.1 Record Marcelo's answers of 2026-10-21 (B1-B4 for integers and reals) as r3.90

## 2. Runtime and compiler

- [x] 2.1 runtime/NumberText.cpp: shortest round-trip formatting, fixed formatting, strict integer and real parsing
- [x] 2.2 Console and file output of Float and Float32 through it
- [x] 2.3 String(N), String(F), FormatFloat; T(S) raising ConversionError (new taxonomy entry)
- [x] 2.4 FieldCount and Field in Std.StrUtils

## 3. Tests

- [x] 3.1 Runtime: float-printing, number-text, format-float (trap), fields (trap); runtime unit tests for the new functions and the Char classes
- [x] 3.2 Diagnostics: string-of-bool (was string-of-integer), string-of-enum, text-to-enum, format-float-argument, field-needs-strutils; conversion-of-bool message; conversion-of-string removed
- [x] 3.3 Expected outputs of every program that prints a real updated (B3), each diff reviewed

## 4. Documentation

- [x] 4.1 Constitution CANON-8, CANON-14, CANON-15, CANON-17, CANON-18, relocked; capability specs; revisions.md r3.90
- [x] 4.2 docs/site/index.html, CHANGELOG.md, docs/STATUS.md, docs/ROADMAP.md, stdlib/Std.Errors.inox

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG; `tools/string_leak_check.py` passes
- [x] 5.5 `tools/eh_state_balance.py`, `tools/calc_differential_test.py`, `tools/grammar_consistency.py`, `openspec validate --all --strict` and `tools/constitution_check.py check` pass
- [ ] 5.6 Linux and Windows CI green
