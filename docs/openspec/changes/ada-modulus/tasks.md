## 1. Decision

- [x] 1.1 Record Marcelo's answer of 2026-10-23 as r3.96

## 2. Compiler

- [x] 2.1 Lexer/parser: `rem`; AST BinaryOperator::Remainder
- [x] 2.2 Analyzer and constant folding: `mod` with the divisor's sign, `rem` with the dividend's
- [x] 2.3 Emitter: `__inox_rem_i64` (srem) and `__inox_mod_i64` corrected; UInt64 urem for both

## 3. Tests

- [x] 3.1 div-mod-signs, integer-slash; new mod-rem-edges, rem-by-zero, const-rem-zero, decimal-rem, rem-reserved
- [x] 3.2 calc_differential and mutation_fuzz know `rem`

## 4. Documentation

- [x] 4.1 Constitution and capability specs, relocked; revisions.md r3.96
- [x] 4.2 grammar.ebnf, site, README, CHANGELOG, STATUS

## 5. Gates (E19)

- [x] 5.1 run-tests, ASan, mutation_fuzz 1 and 7, backend_gaps, string_leak_check, eh_state_balance, calc_differential, grammar_consistency, openspec validate, constitution_check
- [x] 5.2 ir_snapshot against `integer-slash`: the mod helper changes in every program that has the arithmetic helpers
