## 1. Decisions

- [x] 1.1 Record Marcelo's approval of 2026-10-19 of the eight recommendations as r3.83

## 2. Compiler and runtime

- [x] 2.1 Lexer: escapes in Char literals; one shared decoder (support/CharLiteral.h) for the analyzer and the emitter
- [x] 2.2 Analyzer: Char literals checked (empty, two characters, unknown escape, surrogate); `Char(I)`, `TLetter(X)`, `Integer(C)`, `String(C)`; no arithmetic, Succ/Pred, or implicit integer conversion
- [x] 2.3 Analyzer: comparisons, Char ranges in `for`, `case` and `in`; Char Ranges hold Chars with checked stores, Low/High, sets over them
- [x] 2.4 Analyzer: Char in Put/PutLn/Write/WriteLn, array elements, field defaults (`'x'`, `Fill('#')`), Char fields required without default, Char Consts
- [x] 2.5 Std.Strings: `CharAt(S, I)`
- [x] 2.6 Emitter: Char computed as i64, stored as i32 in arrays; checked `Char(I)` (kind 8); `String(C)` and `CharAt` through libinoxrt (`__inox_str_from_char`, `__inox_str_char_at`), with runtime unit tests

## 3. Tests

- [x] 3.1 Runtime: chars; traps char-conversion-surrogate, char-conversion-range, char-at-out-of-range, char-range-store
- [x] 3.2 Diagnostics: char-arithmetic, char-from-integer, char-to-integer, char-conversion-constant, char-succ, char-field-without-default, string-of-integer, char-range-constant, char-literal-two, char-literal-empty, char-unknown-escape, char-float-conversion, char-surrogate-literal; array-element-type-unsupported now uses Currency
- [x] 3.3 backend_gaps probe char-values; string_leak_check covers char*.inox

## 4. Documentation

- [x] 4.1 Constitution CANON-14 (Char) and CANON-5 (Char fields), relocked; capability specs (chars, ranges, sets, structs, stdlib/strings); revisions.md r3.83
- [x] 4.2 grammar.ebnf; docs/site/index.html, CHANGELOG.md, docs/STATUS.md, docs/ROADMAP.md

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG; `tools/string_leak_check.py` passes
- [x] 5.5 `tools/eh_state_balance.py`, `tools/grammar_consistency.py`, `openspec validate --all --strict` and `tools/constitution_check.py check` pass
- [x] 5.6 Linux and Windows CI green (merged into main; main green on Linux and Windows, 2026-10-29)
