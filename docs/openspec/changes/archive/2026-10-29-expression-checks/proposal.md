## Why

Asked whether Inox computes any mathematical expression in any order
(`4*4(5*(2+2))-1`), the answer showed a bug: `4(...)` was accepted by the
analyzer as a call of the number 4 and failed only in code generation,
saying the program was "valid Inox". The differential test of arithmetic
wrapped every operation in parentheses, so it never tested precedence.

Approval (Marcelo Fortes, 2026-10-23), answering whether to fix it, and
improve the message of a chain of comparisons, in a small PR: yes, with many more tests of mathematical expressions

Risk: low (diagnostics and tests; valid programs get the same IR).

Decision on the open point (Marcelo Fortes, 2026-10-23): option 1: `shl` detects overflow on Integer and UInt64 alike; no left shift loses information silently; a truncating shift, if it comes, has its own name and contract; fix Integer `shl` in the PR of the tests that found it.

Status: **implemented** (canon r3.97).

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

5. **Checked `shl` (r3.97).** Found by the new test: `shl` on an Integer
   dropped the bits that left Int64 (`(2 ^ 62) shl 2` was 0), at run time
   and in constant folding, while UInt64 stopped with "integer overflow".
   Now `A shl N` is A * 2^N on both: the helper shifts with a count already
   checked 0..63 (no LLVM poison), shifts back arithmetically and traps
   "integer overflow" unless it gets A; constant folding does the same and
   reports "constant integer overflow in shift". New CANON-8 requirement
   "Checked left shift", CANON-19 updated. Tests shl-checked, shl-overflow,
   shl-overflow-negative, shl-byte-overflow, const-shl-overflow; the oracle
   checks `shl` and generates counts up to 63.
