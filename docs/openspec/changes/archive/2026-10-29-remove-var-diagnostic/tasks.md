## 1. Decision

- [x] 1.1 Record Marcelo's decision of 2026-10-23 as r3.94

## 2. Compiler

- [x] 2.1 Remove kVarRemovedMessage and atRemovedVar (statement, module item, section boundary)

## 3. Tests

- [x] 3.1 Delete var-block-removed and module-var-removed; mut-var-removed becomes mut-reserved

## 4. Documentation

- [x] 4.1 Constitution CANON-4 and CANON-5, relocked; revisions.md r3.94
- [x] 4.2 grammar.ebnf, tests/invalid/README.md, CHANGELOG.md, docs/STATUS.md

## 5. Gates (E19)

- [x] 5.1 run-tests, ASan, mutation_fuzz 1 and 7, backend_gaps, string_leak_check, eh_state_balance, calc_differential, grammar_consistency, openspec validate, constitution_check
- [x] 5.2 ir_snapshot against `discard-results`: only Var diagnostics change
