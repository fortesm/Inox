## 1. Decision

- [x] 1.1 Record Marcelo's choice of 2026-10-22 (option 2, `_ :=`, calls only) as r3.93

## 2. Compiler

- [x] 2.1 Lexer: `_` alone is TokenKind::Underscore
- [x] 2.2 Parser: discard statement; `_` rejected in expressions, signatures and every declaration
- [x] 2.3 AST: ExpressionStatement::isDiscard (cloned)
- [x] 2.4 Analyzer: only a call with a result; E2 message shows `_ := F(...)`
- [x] 2.5 Emitter: the call as a value, owned results released
- [x] 2.6 A module-level line that is no declaration is an error (was ignored)

## 3. Tests

- [x] 3.1 Runtime: discard (every kind of call, owned results, exceptions), discard-contract (trap)
- [x] 3.2 Diagnostics: discard-literal, -variable, -operation, -field, -index, -conversion, -construction, -subroutine, -putln, -method-subroutine, -read, -declaration, -parameter, -routine-name, -compound, -chain; module-item-unknown; the E2 messages updated
- [x] 3.3 string_leak_check covers discard.inox

## 4. Documentation

- [x] 4.1 Constitution CANON-7 (Explicit discard, What is discarded), relocked; revisions.md r3.93
- [x] 4.2 grammar.ebnf discard_stmt; docs/site/index.html, CHANGELOG.md, docs/STATUS.md

## 5. Gates (E19)

- [x] 5.1 run-tests, ASan run-tests with `ulimit -s 1024`
- [x] 5.2 mutation_fuzz seeds 1 and 7; backend_gaps; string_leak_check; eh_state_balance; calc_differential; grammar_consistency
- [x] 5.3 `openspec validate --all --strict`; `constitution_check check`
- [x] 5.4 ir_snapshot against `datetime-systemutils-editor`: only programs that used `_` change
