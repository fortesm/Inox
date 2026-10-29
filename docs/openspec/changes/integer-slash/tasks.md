## 1. Decision

- [x] 1.1 Record Marcelo's decision of 2026-10-23 as r3.95

## 2. Compiler

- [x] 2.1 Lexer: `div` is not a reserved word
- [x] 2.2 Parser/AST: no IntegerDivide; `/` only
- [x] 2.3 Analyzer: integer `/`, `/=`, literal contexts, constant folding and zero divisor
- [x] 2.4 Emitter: integer `/` through the checked helpers (signed and unsigned)

## 3. Code and tests

- [x] 3.1 Every `div` in stdlib, examples and tests becomes `/`
- [x] 3.2 Tools generate `/` (calc_differential, backend_gaps, mutation_fuzz)
- [x] 3.3 New tests integer-slash, slash-mixed-types, div-is-a-name; integer-slash (invalid) and compound-integer-divide removed; decimal-div becomes decimal-mod

## 4. Documentation

- [x] 4.1 Constitution and capability specs, relocked; revisions.md r3.95
- [x] 4.2 grammar.ebnf, site, README, CHANGELOG, STATUS, BACKEND_GAPS

## 5. Gates (E19)

- [x] 5.1 run-tests, ASan, mutation_fuzz 1 and 7, backend_gaps, string_leak_check, eh_state_balance, calc_differential, grammar_consistency, openspec validate, constitution_check
- [x] 5.2 ir_snapshot against `remove-var`: the IR of programs that wrote `div` is unchanged
