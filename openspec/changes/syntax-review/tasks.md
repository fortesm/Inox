## 1. Decision

- [x] 1.1 Decisions (Marcelo Fortes, 2026-10-16)

## 2. Compiler

- [x] 2.1 Lexer: 42 reserved words, 10 predefined names
- [x] 2.2 Parser: no Module header (name from the driver), migration message; IIf; `if` in an expression rejected; `then` only in `pos then`; `Var` migration without reserving it; reserved words as variables rejected
- [x] 2.3 Driver: module name from the Use path or the entry file's stem

## 3. Tests, law and documents

- [x] 3.1 Module lines removed from every program; IIf in tests and examples; new diagnostics
- [x] 3.2 Constitution (CANON-2, CANON-3, CANON-12), comparisons spec, grammar, tools
- [x] 3.3 Manual, STATUS, CHANGELOG, revisions, AGENTS, README

## 4. Gates

- [x] 4.1 Suite, ASan+UBSan, fuzz, leak check, grammar, openspec, constitution, calc differential, MSVC IR
- [ ] 4.2 CI on Linux and Windows
