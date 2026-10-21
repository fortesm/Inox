## 1. Decisions

- [x] 1.1 Record Marcelo's answers of 2026-10-21 (B5, C1, C2 for Float and Float32, C3) as r3.89

## 2. Compiler and runtime

- [x] 2.1 Lexer: exponents after the fraction; `1e3` and `1.5e` rejected with the way to write them; analyzer: real literals and Consts beyond Float64 rejected
- [x] 2.2 Get(C), GetLn(C), EndOfInput: runtime reader, analyzer, emitter (traps 18 and 13; Char Range checked)
- [x] 2.3 Get of Float and Float32: strict reader in libinoxrt, Float32 rounded once; traps 19 and 11
- [x] 2.4 Char classes and case: generated tables, `__inox_char_is`, `__inox_char_case`, Std.Strings entries and the Char overloads of UpperCase/LowerCase

## 3. Tests

- [x] 3.1 Runtime: get-char (with .in), get-char-end-of-input, get-char-invalid-utf8, get-char-range, get-real, get-real-invalid, get-real-range, real-exponent, char-classes
- [x] 3.2 Diagnostics: real-exponent-without-fraction, real-exponent-without-digits, real-literal-out-of-range, real-const-out-of-range, real-exponent-float32-range, char-class-argument, char-class-without-use, get-bool, end-of-input-statement; get-string message updated

## 4. Documentation

- [x] 4.1 Constitution CANON-2, CANON-14, CANON-17, CANON-19, relocked; capability specs; revisions.md r3.89
- [x] 4.2 grammar.ebnf (exponent); docs/site/index.html, CHANGELOG.md, docs/STATUS.md, docs/ROADMAP.md

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG; `tools/string_leak_check.py` passes
- [x] 5.5 `tools/eh_state_balance.py`, `tools/calc_differential_test.py`, `tools/grammar_consistency.py`, `openspec validate --all --strict` and `tools/constitution_check.py check` pass
- [ ] 5.6 Linux and Windows CI green
