## 0. Decisions

- [x] 0.1 Record Marcelo's answers to Q1 (GetLn into String), Q2 (non-atomic counts), Q3 (keep concatenation reserved) and Q4 (String without initializer)

- [x] 0.2 Marcelo approves design.md (Risk: high) before any code (2026-10-12, "Então tudo aprovado.")

## 1. Runtime

- [ ] 1.1 `runtime/StringRuntime.cpp`: block layout, immortal literals, retain, release, equality, output
- [ ] 1.2 Line input into a String (`GetLn(S)`, Q1), with UTF-8 validation
- [ ] 1.3 C++ unit tests for the runtime, run by the suite on Linux and Windows

## 2. Lowering

- [ ] 2.1 String slots for locals, parameters, results, fields, elements, State, ref parameters
- [ ] 2.2 Retain/release at stores and temporaries; release owned slots at every routine exit
- [ ] 2.3 Exception cleanup pad per routine owning Strings (capture, release, rethrow; both ABIs)
- [ ] 2.4 `=`/`#` and Put/PutLn through the runtime

## 3. Tests

- [ ] 3.1 tests/runtime/strings.inox and strings-exception-cleanup.inox
- [ ] 3.2 Sanitizer gate runs the String tests with `detect_leaks=1`
- [ ] 3.3 Flip backend_gaps probe `string-local` to OK; add probes for String fields and State

## 4. Documentation

- [ ] 4.1 Canon: change log, CANON-14/17/19 Layer B, pointer from CANON-14 to openspec/specs/language/strings on archive
- [ ] 4.2 docs/BACKEND_GAPS.md, docs/site/index.html, CHANGELOG.md

## 5. Gates (E19)

- [ ] 5.1 `bash scripts/run-tests.sh` passes
- [ ] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [ ] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [ ] 5.4 `tools/backend_gaps.py` reports 0 BUG
- [ ] 5.5 `tools/eh_state_balance.py` and `tools/grammar_consistency.py` pass
- [ ] 5.6 Linux and Windows CI green
