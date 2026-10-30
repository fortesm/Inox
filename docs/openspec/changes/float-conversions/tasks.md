## 1. Decisions

- [x] 1.1 Record the approval of 2026-10-19 and r3.84; list the choices the law does not spell out as open question 3

## 2. Compiler

- [x] 2.1 Analyzer: real literals in a Float32 context; the argument of Float32(X) is a Float64
- [x] 2.2 Analyzer: conversions into a number take a number; constant real conversions checked
- [x] 2.3 Analyzer: real Consts folded (literals, Consts, unary minus, + - * /, Float/Float32 conversions)
- [x] 2.4 Emitter: integer to real, real between widths (Float32 overflow trap), real to integer (truncation, NaN and range trap)
- [x] 2.5 Emitter: real constants as exact hexadecimal doubles; Float32 printed and written
- [x] 2.6 Std.Math: Pi, Tau and E as Consts

## 3. Tests

- [x] 3.1 Runtime: float-conversions; traps float-to-integer-range, float-to-integer-nan, float-to-int64-range, float32-overflow
- [x] 3.2 Diagnostics: float-conversion-constant, float32-constant-range, conversion-of-bool, conversion-of-string, float-enum-conversion, float-implicit-integer, float32-float64-mix
- [x] 3.3 backend_gaps probe float-const

## 4. Documentation

- [x] 4.1 Constitution CANON-8, relocked; `language/reals`; revisions.md r3.84
- [x] 4.2 docs/site/index.html, CHANGELOG.md, docs/STATUS.md, docs/ROADMAP.md

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG; `tools/string_leak_check.py` passes
- [x] 5.5 `tools/eh_state_balance.py`, `tools/grammar_consistency.py`, calc differential, `openspec validate --all --strict` and `tools/constitution_check.py check` pass
- [ ] 5.6 Linux and Windows CI green
