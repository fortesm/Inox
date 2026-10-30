## Why

`#` is Inox's inequality (Pascal's `<>` replaced, CANON-2). Programmers from
C, Java, Rust, Go and Python write `!=`. The maintainer decided both are
valid, and that `!` alone stays out of the language.

Approval (Marcelo Fortes, 2026-10-21): "Deve ser adotado o operador !=
paralelo ao operador #, ambos são válidos — if a != b é válido; if ! (a = b)
é inválido deve ser o operador not".

Risk: low (one lexer rule). Status: **implemented** (canon r3.87).

## What Changes

- The lexer reads `!=` as the inequality token, the same as `#`: same
  operator, precedence and rules (structs and arrays still not compared).
- A lone `!` is an error: "'!' is not an operator: negation is 'not' (not
  (A = B)), inequality is '#' or '!='".

## Impact

- Lexer; constitution CANON-2 (relocked); grammar.ebnf `relational_op`;
  tests runtime not-equal-operator, diagnostic bang-negation; docs.
