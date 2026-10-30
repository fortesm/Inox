## 1. Decisions

- [x] 1.1 Record Marcelo's answers of 2026-10-21 (D1-D4, F3) as r3.92

## 2. Library, runtime and compiler

- [x] 2.1 Std.DateTime in Inox (calendar algorithms, preconditions) with the NowUTC and UTCOffset intrinsics
- [x] 2.2 Std.DateUtils in Inox: ISO 8601 both ways, FormatDateTime with Delphi codes
- [x] 2.3 Std.SystemUtils intrinsics and runtime/SystemRuntime.cpp (POSIX and Windows, UTF-8)
- [x] 2.4 Main receives argc/argv; the driver passes program arguments and keeps the exit status
- [x] 2.5 Intrinsic table by module (StdIntrinsics.h)
- [x] 2.6 examples/edit.inox, the micro-editor

## 3. Tests

- [x] 3.1 Runtime: datetime, datetime-invalid (trap), dateutils, system-utils, system-halt-range (trap), system-paramstr-range (trap); examples/edit.inox with keys (tests/examples/edit.in)
- [x] 3.2 Diagnostics: datetime-without-use, system-utils-without-use, getenv-argument; module-not-found replaces module-planned-not-found
- [x] 3.3 Driver: "inox run passes arguments and keeps Halt's status" (both runners)
- [x] 3.4 By hand in a pseudo-terminal: the editor loads a file, edits it and saves it

## 4. Documentation

- [x] 4.1 Constitution CANON-18, CANON-23, relocked; capability specs stdlib/datetime and stdlib/systemutils; revisions.md r3.92
- [x] 4.2 docs/site/index.html, CHANGELOG.md, docs/STATUS.md, docs/ROADMAP.md, stdlib/README.md

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG; `tools/string_leak_check.py` passes
- [x] 5.5 `tools/eh_state_balance.py`, `tools/calc_differential_test.py`, `tools/grammar_consistency.py`, `openspec validate --all --strict` and `tools/constitution_check.py check` pass
- [x] 5.6 Linux and Windows CI green (merged into main; main green on Linux and Windows, 2026-10-29)
