## Why

Open question 3 (change `float-conversions`, r3.84) listed three choices the
law did not spell out. The maintainer compared Ada 2022 (ISO/IEC
8652:2023), SPARK, Rust, Go and Delphi and decided a floating-point policy
that joins Ada/SPARK's safety, Rust and Go's contextual inference and IEEE
754's predictability.

Approval (Marcelo Fortes, 2026-10-21):
- `Float32(F)`: a checked model for `Float32(F)`, inspired by Ada, with more deterministic rules: a constant that does not fit
  is a compile error; at run time, a controlled failure when a finite value
  would overflow to an infinity; otherwise IEEE 754 rounding, ties to even;
  an ordinary loss of precision is no error; `Float32(F)` does not saturate at the largest finite number `Float32Unchecked(F)` is
  named as an idea, not an approved syntax.
- Literals: confirmed without change; an untyped real literal takes the expected type, while a typed expression never changes precision implicitly; the compiler converts the decimal literal straight to the final type, never necessarily through Float64.
- Constants: the Ada/Go model: Pi, Tau and E untyped
  until the context resolves them, kept with many digits.
- Also: NaN and the infinities are preserved by a conversion; underflow gives
  a subnormal or a signed zero; Float32 expressions are not evaluated in
  Float64.

Risk: normal.

Status: **implemented** (canon r3.86).

## What Changes

- A Float32 literal is read with `strtof` from its digits (one rounding);
  before, it was read as a double and then rounded (two roundings, wrong
  near ties, e.g. `1.000000059604644775390626`).
- A Const whose value is a real literal, or its negation, is universal (Ada
  named number, Go untyped constant): a Float32 where a Float32 is expected,
  rounded once from its digits; a computed Const keeps its Float64 type.
- Std.Math writes Pi, Tau and E with 37 significant digits.
- `Float32(F)`: already rounding to nearest even, trapping on a finite
  overflow, keeping NaN and the infinities, flushing underflow to a signed
  zero or subnormal; now covered by tests and stated in the law. Float32
  arithmetic is emitted as `float` operations (checked in the IR).

Out of scope: `Float32Unchecked(F)` (an idea, not approved).

## Capabilities

### Modified Capabilities
- `language/reals`: literals rounded once, special values, universal Consts.

## Impact

- `support/RealLiteral.h` (`parseRealLiteral32`), the analyzer (Float32 context
  for universal Consts, constant folding), the emitter (constants), Std.Math,
  `language/reals`, revisions.md, docs.
