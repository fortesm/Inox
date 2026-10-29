## 1. Decisions

- [x] 1.1 Record Marcelo's approvals of 2026-10-26 and 2026-10-29 as r3.99

## 2. Names and visibility (commit 1/5)

- [x] 2.1 Lexer: `Public` reserved; parser: `Public` in headers and structs, `Export Use`, qualified types
- [x] 2.2 Module resolution: own names, public names of used modules, ambiguity, qualified names
- [x] 2.3 Visibility rules; the standard library marks its public declarations; pairs with `Export Use`

## 3. Encapsulation (commit 2/5)

- [x] 3.1 Field and method visibility, construction (C1, C2), reuse (C3), invariant after public writes (C4)
- [x] 3.2 A type name alone on its header line; method arguments analyzed against the parameter type

## 4. Aliases and selective use (commit 3/5)

- [x] 4.1 Tests for `as` and `.( )`, with every error; a hint for a public name left out of the list

## 5. Search path, Std, interface, the bank (commit 4/5)

- [x] 5.1 `--path`, `INOX_PATH`, `Std` reserved
- [x] 5.2 `inox interface`
- [x] 5.3 `examples/bank` with its tests; driver cases in both runners

## 6. Law and documentation (commit 5/5)

- [x] 6.1 Constitution (CANON-1, 2, 3, 18, 23, 24, FUTURE), relocked; revisions.md r3.99; kCanonRevision
- [x] 6.2 Grammar, site, CHANGELOG, STATUS, ROADMAP, stdlib README, AGENTS.md

## 7. Gates (E19)

- [x] 7.1 run-tests, ASan, mutation_fuzz 1 and 7, backend_gaps, string_leak_check, eh_state_balance, calc_differential, grammar_consistency, openspec validate, constitution_check
- [x] 7.2 ir_snapshot against the base branch: only the intended differences
