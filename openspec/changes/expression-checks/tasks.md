## 1. Compiler

- [x] 1.1 Analyzer: a literal or an expression is not called (implicit product)
- [x] 1.2 Analyzer: the message of a chain of comparisons

## 2. Tests

- [x] 2.1 tools/calc_differential_test.py: precedence-aware generator, independent oracle, batches, run time and folded, traps
- [x] 2.2 tests/runtime/math-expressions.inox (152 expressions, oracle values)
- [x] 2.3 Diagnostics: implicit-product-number, -real, -group, -variable, literal-called, comparison-chained
- [x] 2.4 Capability spec language/expressions

## 3. Gates (E19)

- [x] 3.1 run-tests, ASan, mutation_fuzz 1 and 7, backend_gaps, string_leak_check, eh_state_balance, calc_differential (2000 and 10 × 10 000), grammar_consistency, openspec validate, constitution_check
- [x] 3.2 ir_snapshot against `ada-modulus`: only the new tests and diagnostics change
