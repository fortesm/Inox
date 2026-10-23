## Why

Fourth change of 0.3 (`docs/ROADMAP.md`): Currency and Crypto, the exact
decimals of CANON-8, had names and no values. Money and quantities of coins
need exact arithmetic and a rounding rule written down.

Request (Marcelo Fortes, 2026-10-21): "Implemente: Versão 0.3, números,
texto e biblioteca básica: Currency e Crypto; [...]".

Approval (Marcelo Fortes, 2026-10-21, repeated on 2026-10-22), answering
the decisions proposed to him: "A Acatado todos / B Acatado todos / C
Acatado todos". This change carries A1-A5, and B1, B2 and C2 for Currency
and Crypto.

Risk: normal (new values and runtime functions; no change to exception
lowering or memory ownership). A Crypto is an i128 inside generated code;
multiplication and division go through libinoxrt, so no 128-bit division
helper (compiler-rt, absent from the MSVC link) is ever needed.

Status: **implemented** (canon r3.91).

## What Changes

1. **Values.** Currency is an i64 scaled by 10^6, Crypto an i128 scaled by
   10^18. They live in locals, parameters, results, struct fields (literal
   defaults), arrays, State, Consts (`Const Fee := Currency(2.5)`) and IIf,
   and compare with `= # < <= > >=`.
2. **Literals (CANON-8, A2).** A literal where a decimal is expected, or next
   to one in an operator (`Price * 3`, `3 * Price`), is read exactly; more
   places than the type holds or a value beyond it is a compile error. A
   variable of another type is converted explicitly: `Price *
   Currency(Count)`; the error shows it.
3. **Arithmetic (A1).** `+ -` exact and checked inline; `* /` exact in
   libinoxrt, then rounded once half to even to the type's places. Beyond
   the type: new trap "decimal overflow" (kind 20); `/` by zero "division by
   zero". `^`, `div`, `mod` do not apply.
4. **Conversions (A3, A4).** `Currency(X)`/`Crypto(X)` of an integer (exact),
   a Float (its exact binary value rounded half to even), the other decimal
   (`Crypto(C)` exact, `Currency(Cr)` rounded half to even, no error), a
   String (exact; ConversionError otherwise). `Integer(C)` and every integer
   type truncate toward zero; `Float(C)`/`Float32(C)` the nearest real.
   Beyond the target: "integer conversion out of range".
5. **Std.Math `Round(X)` and `Trunc(X)` (A3)** of a Float, Float32, Currency
   or Crypto give an Integer: Round half to even, Trunc toward zero; visible
   with `Use Std.Math` (the names stay free otherwise).
6. **Text (A5, B1, B2, C2).** Written with at least two decimals and no
   trailing zeros after them (`19.99`, `5.00`, `0.123456`, `-0.50`) by Put,
   Write and `String(C)`; `Get` reads one strictly (at most the type's
   decimals, else "invalid number input").
7. **Fix found on the way:** a negated real literal was emitted as written,
   so a Float32 `-0.1`, `-1_000.5` or an exponent made invalid IR; it is now
   read in its type and negated.

## Capabilities

### New Capabilities
- `language/decimals`: Currency and Crypto.

### Modified Capabilities
- Constitution CANON-8 (decimal arithmetic, decimal conversions), CANON-17
  (how decimals are written), CANON-18 (Std.Math Round and Trunc), CANON-19
  (decimal overflow; input trap categories split out).

## Impact

- libinoxrt: `runtime/DecimalRuntime.cpp`, `runtime/Decimal.h`
  (multiplication, division, rescaling, conversions, text), console, file
  and String wrappers; runtime unit tests.
- Compiler: `support/DecimalLiteral.h` (exact literals); Type
  `isDecimal()`/`decimalPlaces()`; the analyzer (literal context, operators,
  conversions, Round/Trunc, field defaults, decimal Consts, arrays) and the
  emitter (`emitDecimalArithmetic`, `emitDecimalConversion`, `emitRounding`,
  I/O).
- `array-element-type-unsupported` now uses an exception type.
