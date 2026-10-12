## 0. Decisions

- [x] 0.1 Record Marcelo's answers to Q1 (GetLn into String), Q2 (non-atomic counts), Q3 (keep concatenation reserved) and Q4 (String without initializer)

- [x] 0.2 Marcelo approves design.md (Risk: high) before any code (2026-10-12, "Então tudo aprovado.")

## 1. Runtime

- [x] 1.1 `runtime/StringRuntime.cpp`: block layout, immortal literals, retain, release, equality (output stays in the program: CRT split on Windows)
- [x] 1.2 Line input into a String (`GetLn(S)`, Q1), with UTF-8 validation
- [x] 1.3 C++ unit tests for the runtime (`inoxrt-tests`), run by the suite on Linux and Windows

## 2. Lowering

- [x] 2.1 String slots for locals, parameters, results, fields, elements, State, ref parameters
- [x] 2.2 Retain/release at stores and temporaries; release owned slots at every routine exit
- [x] 2.3 Exception cleanup pad per routine owning Strings (capture, release, rethrow; both ABIs)
- [x] 2.4 `=`/`#` and Put/PutLn through the runtime

## 3. Tests

- [x] 3.1 tests/runtime/strings, strings-getln, strings-aggregates, strings-exception-cleanup, strings-literal-compare, string-local, strings-invalid-utf8 (trap); diagnostics string-without-initializer, get-string, struct-default-string-on-integer, struct-default-number-on-string; eh-lifetime string-cleanup
- [x] 3.2 `tools/string_leak_check.py` runs the String tests under ASan with `detect_leaks=1` (Linux CI step)
- [x] 3.3 Flip backend_gaps probe `string-local` to OK; add probes `string-aggregates` and `string-getln`

## 4. Documentation

- [x] 4.1 Canon: change log, CANON-14/17/19 Layer B, pointer from CANON-14 to openspec/specs/language/strings on archive
- [x] 4.2 docs/BACKEND_GAPS.md, docs/site/index.html, CHANGELOG.md

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG
- [x] 5.5 `tools/eh_state_balance.py` and `tools/grammar_consistency.py` pass
- [x] 5.7 `tools/ir_snapshot.py` against main: only intended differences (fault table kinds 12/13 in every program with checked arithmetic; programs with String locals now compile; clearer `Get` on String message; the unsupported-element test moved to Char)
- [ ] 5.6 Linux and Windows CI green
