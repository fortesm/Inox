## 1. Decision

- [x] 1.1 Analysis and decisions (Marcelo Fortes, 2026-10-16)

## 2. Compiler

- [x] 2.1 Lexer: 41 reserved words, predefined names, `{` `}` reserved
- [x] 2.2 Parser: `else` as the last case arm, migration message for `otherwise`, predefined names at statement start
- [x] 2.3 Analyzer: a predefined name is never declared

## 3. Tests, law and documents

- [x] 3.1 Diagnostics and runtime tests; existing tests and examples use `else`
- [x] 3.2 Constitution (CANON-2, CANON-12, FUTURE), case-statement spec, grammar
- [x] 3.3 Manual, STATUS, CHANGELOG, revisions, AGENTS

## 4. Gates

- [x] 4.1 Suite, ASan+UBSan, fuzz, leak check, grammar, openspec, constitution, MSVC IR
- [ ] 4.2 CI on Linux and Windows
