## Why

Asked whether Inox computes any mathematical expression in any order
(`4*4(5*(2+2))-1`), the answer showed a bug: `4(...)` was accepted by the
analyzer as a call of the number 4 and failed only in code generation,
saying the program was "valid Inox". The differential test of arithmetic
wrapped every operation in parentheses, so it never tested precedence.

Approval (Marcelo Fortes, 2026-10-23), answering whether to fix it, and
improve the message of a chain of comparisons, in a small PR: "sim porém
faça muito mais testes com expressões matemáticas para testar."

Risk: low (diagnostics and tests; valid programs get the same IR).

Status: **implemented** (no canon revision: the law is unchanged).

## What Changes

1. **Analyzer.** A call whose callee is a literal or an expression is
   rejected: "4(...) is not a multiplication: a number is not called; write
   4 * (...)", "(...)(...) is not a multiplication". A variable called is
   still "'X' is a value", now with "to multiply, write X * (...)".
2. **Analyzer.** `1 < X < 10` (a comparison whose left side is a
   comparison, against a number) gets "comparisons are not chained: ...
   write A < B and B < C, or B in A..C" instead of "requires matching
   operand types".
3. **tools/calc_differential_test.py, rewritten.** Random expression trees
   are rendered with only the parentheses CANON-20 requires (and a few
   redundant ones); an independent recursive-descent parser written from
   the grammar evaluates them with Inox's semantics (checked Int64, `/`,
   `mod`, `rem`, shifts, bitwise, reals, Std.Math, IIf, Bool). Batches of
   250 per program, half folded at compile time and half through `I(...)`,
   `F(...)` at run time; expressions the oracle says overflow or divide by
   zero must stop (run time) or be rejected (compile time). Default 2000
   expressions and 60 traps; 100 000 expressions over ten seeds and 1 500
   traps passed.
4. **Tests.** `tests/runtime/math-expressions.inox`: 152 written
   expressions in 13 groups, expected values from the oracle; diagnostics
   implicit-product-number, -real, -group, -variable, literal-called,
   comparison-chained. New capability spec `language/expressions`.

## Open point (found by the new test)

`shl` on a signed integer drops the bits that leave Int64 silently (`(2 ^
62) shl 2` is 0), at run time and in constant folding, while on UInt64 it
stops with "integer overflow", and CANON-8 says overflow is never silent.
The test does not generate that case until the maintainer decides: trap as
UInt64 does, or define `shl` as a bit operation that drops bits.
