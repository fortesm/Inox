## 1. Decisions

- [x] 1.1 Record Marcelo's answers A1-A5 (and B1, B2, C2 for decimals) of 2026-10-21 as r3.91

## 2. Runtime and compiler

- [x] 2.1 libinoxrt DecimalRuntime.cpp: 32-bit limb arithmetic, * / rounded half to even, rescaling, conversions with integers and reals, text in and out
- [x] 2.2 Analyzer: decimal types, exact literals in context, operators and mixing errors, conversions, field defaults, Consts, arrays, Put/Get/Write/String
- [x] 2.3 Emitter: i64/i128 values, literals, inline + - and comparisons, runtime * /, conversions, I/O
- [x] 2.4 Std.Math Round and Trunc (with Use Std.Math)
- [x] 2.5 Negated real literals read in their type (fix)

## 3. Tests

- [x] 3.1 Runtime: decimals, decimals-input (trap), decimal-overflow, decimal-division-by-zero, decimal-conversion-range, negative-real-literals; runtime unit tests for decimals
- [x] 3.2 Diagnostics: decimal-mix-integer, decimal-mix-float, decimal-mix-crypto, decimal-too-many-places, decimal-literal-out-of-range, decimal-conversion-constant, decimal-power, decimal-div, decimal-of-bool, decimal-field-places, round-without-math, round-of-integer; array-element-type-unsupported retargeted
- [x] 3.3 backend_gaps probe decimals; compiler unit test of the LLVM types

## 4. Documentation

- [x] 4.1 Constitution CANON-8, CANON-17, CANON-18, CANON-19, relocked; capability spec language/decimals; revisions.md r3.91
- [x] 4.2 docs/site/index.html, CHANGELOG.md, docs/STATUS.md, docs/ROADMAP.md, docs/BACKEND_GAPS.md, stdlib/Std.Math.inox

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG; `tools/string_leak_check.py` passes
- [x] 5.5 `tools/eh_state_balance.py`, `tools/calc_differential_test.py`, `tools/grammar_consistency.py`, `openspec validate --all --strict` and `tools/constitution_check.py check` pass
- [ ] 5.6 Linux and Windows CI green
