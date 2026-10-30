## Why

The law already gives Float conversions (CANON-8: `T(X)` converts
explicitly; `Integer(FloatExpr)` truncates toward zero and fails when out of
range) and Consts of any constant value (CANON-5), but the backend lowered
neither: `Float(I)`, `Integer(F)`, `Float32(X)` and a Float `Const` were
"not yet implemented", so Std.Math wrote `Pi`, `Tau` and `E` as functions.
They are the last item of 0.2 in `docs/ROADMAP.md` before the release.

Approval (Marcelo Fortes, 2026-10-19), of the plan to do the Float conversions and the Float Consts next, agreed. This closes a
conformance gap of existing law; the choices below that the law does not
spell out are implemented and listed as open question 3 for his
confirmation (G4).

Risk: normal (lowering of existing law; no runtime ABI, exception or
ownership change).

Status: **implemented** (canon r3.84).

## What Changes

- `Float(I)` and `Float32(I)` round an integer to the nearest real (a UInt64
  read unsigned).
- `Float(G)` widens a Float32 exactly; `Float32(F)` rounds to the nearest.
- `T(F)` into an integer type (Integer, the fixed widths, Natural, UInt64,
  a Range) truncates toward zero; NaN, an infinity or a value outside T
  stops the program with "integer conversion out of range" (kind 8); a
  constant outside T is a compile error.
- A conversion into a number takes a number: `Float(True)`, `Integer("7")`
  and `Float(Heart)` are errors with the way to write them.
- Real Consts: literals, other real Consts, unary minus, `+ - * /` and
  Float/Float32 conversions of constants, folded at compile time; a Const
  may follow its use. Std.Math's Pi, Tau and E become Consts (every use
  reads the same).
- Float32 values print and are written to files.

Choices awaiting confirmation (open question 3 in revisions.md):

1. `Float32(F)` of a finite F beyond Float32's range stops the program
   ("value out of range", kind 11) rather than giving an infinity, as every
   other narrowing conversion fails rather than lose the value.
2. A real literal is a Float32 where a Float32 is expected
   (`Small Float32 := 0.1`), as an integer literal is a Byte where a Byte
   is expected (CANON-8); Float32 and Float64 still never mix.
3. Pi, Tau and E are Consts.

Out of scope: real literals with exponents (`1.0e3`; the grammar has none),
`Float(Soma / N)` as an exact quotient (FUTURE), conversions between text
and numbers, decimals (0.3).

## Capabilities

### New Capabilities
- `language/reals`: real literals, conversions and Consts.

## Impact

- Analyzer: Float32 literal context; the numeric conversion checks
  (`checkNumericConversion`); real constant folding (`staticRealValue`);
  the argument of `Float32(X)` is a Float64.
- Emitter: `emitRealConversion` (sitofp/uitofp, fpext/fptrunc with the
  overflow check, fptosi/fptoui after an in-range check); real constants
  written as exact hexadecimal doubles (`llvmRealConstant`); Float32 printed
  and written through its Float64.
- `support/RealLiteral.h`, one parser of real literals for both.
- Std.Math: Pi, Tau and E as Consts.
- Constitution CANON-8, `language/reals`, revisions.md, docs/site,
  CHANGELOG.md, docs/STATUS.md, docs/ROADMAP.md, backend probes.
