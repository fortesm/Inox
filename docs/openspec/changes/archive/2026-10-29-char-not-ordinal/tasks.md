## 1. Analyzer

- [x] 1.1 Remove the prelude signatures `Length(String)` and `Ord(Char)`; explicit rejection messages
- [x] 1.2 Tests: diagnostics `string-length-reserved`, `ord-of-char`; `tests/invalid/invalid-004` and `-005` keep failing with the new messages

## 2. Documentation and gates

- [x] 2.1 Constitution CANON-14; relock; docs/STATUS.md; CHANGELOG
- [x] 2.2 Suite; constitution_check
- [x] 2.3 Linux and Windows CI green (merged into main; main green on Linux and Windows, 2026-10-29)
