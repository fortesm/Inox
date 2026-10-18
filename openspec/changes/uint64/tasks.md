## 0. Decisions

- [x] 0.1 Record Marcelo's answers to Q1 (literals above Int64.Max), Q2 (unary minus) and Q3 (mixing with Integer) in the proposal; all three approved

## 1. Semantic analysis

- [x] 1.1 UInt64/UInteger type rules: constants that fit, negative constants rejected, no mixing with Integer variables
- [x] 1.2 Unsigned constant folder with overflow detection
- [x] 1.3 Q2 diagnostic for unary minus

## 2. Lowering and runtime

- [x] 2.1 Unsigned arithmetic, power, div/mod, shifts, bitnot, comparisons
- [x] 2.2 Conversions in both directions with kind 8
- [x] 2.3 PutLn (`%llu`), Get (`__inox_read_u64`), WriteLn (libinoxrt `__inox_file_write_u64`); see design
- [x] 2.4 case and for over UInt64

## 3. Tests

- [x] 3.1 tests/runtime/uint64.inox (every operator with operands above Int64.Max)
- [x] 3.2 Traps uint64-overflow, uint64-underflow, uint64-conversion-range
- [x] 3.3 Diagnostics uint64-negative-literal, uint64-mixed-integer
- [x] 3.4 Flip backend_gaps probe `uint64-local` to OK

## 4. Documentation

- [x] 4.1 Constitution CANON-8 scenarios; spec `language/fixed-width-integers`; revision r3.79
- [x] 4.2 docs/BACKEND_GAPS.md, docs/site/index.html, CHANGELOG.md

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG
- [x] 5.5 `tools/eh_state_balance.py` and `tools/grammar_consistency.py` pass
- [ ] 5.6 Linux and Windows CI green
