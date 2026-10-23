## ADDED Requirements

### Requirement: Expressions in any order and nesting
An expression SHALL combine its operators in any order and nesting, with
the levels and associativity of the CANON-20 table and the mixing rules of
the grammar; its value SHALL be the one an independent evaluation of the
same law gives, folded at compile time or computed at run time alike.

#### Scenario: Curated and random expressions
- **WHEN** 152 written expressions and thousands of random ones mix every operator, depth and parenthesization, with literals and with numbers computed at run time
- **THEN** each prints the value of the oracle in tools/calc_differential_test.py (tests/runtime/math-expressions.inox), and those that overflow or divide by zero stop or are rejected

### Requirement: No implicit product
A product SHALL be written with `*`, as in Ada and Pascal: a number, a
group in parentheses or a variable followed by `(...)` is not a
multiplication, and is rejected naming the `*` to write.

#### Scenario: Implicit products
- **WHEN** a program writes `4(5 * (2 + 2))`, `2.5(2.0)`, `(2 + 3)(4)`, `X(2)` with X a variable, or a String literal followed by arguments
- **THEN** each is rejected (tests/diagnostics/implicit-product-number.inox, tests/diagnostics/implicit-product-real.inox, tests/diagnostics/implicit-product-group.inox, tests/diagnostics/implicit-product-variable.inox, tests/diagnostics/literal-called.inox)

### Requirement: Comparisons are not chained
`A < B < C` SHALL read as `(A < B) < C` (CANON-20, left associative); when
that compares a Bool with a number it is rejected with a message that shows
`A < B and B < C`.

#### Scenario: A chain of comparisons
- **WHEN** a program writes `1 < X < 10`
- **THEN** it is rejected naming the chain (tests/diagnostics/comparison-chained.inox)
