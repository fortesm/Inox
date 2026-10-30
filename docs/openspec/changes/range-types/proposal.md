## Why

An Integer Range was a subtype, as Ada's `subtype`: values of two Ranges
mixed freely, the result was an Integer, and a Range passed where another
was expected compiled. With Std.DateTime this turned unit mistakes into
wrong values: a TDate passed for a TDateTime gave an instant near year 1,
`T + D` added days to microseconds, and `Meters + Seconds` or
`Meters := Seconds` compiled.

Decided by Marcelo Fortes, 2026-10-30 (open question 3, CANON-13): Ranges
follow Ada's `type T is range`, with these rules:

- a Range is a type of its own; its values combine with values of the same
  Range and with constants, and the result of its arithmetic belongs to the
  Range (`TMeters op TMeters` is a TMeters);
- two Ranges, or a Range and an Integer value, mix only through a written
  conversion, `Integer(X)` or `T(X)`;
- Integer parameters are strict: a call writes `F(Integer(D))`; only the
  built-ins that take any value (Put, PutLn, `String(X)`) take a Range
  value as it is;
- a Range value indexes an array, as any integer-like index.

Risk: normal (semantic checks; the generated code keeps Int64 values and the
existing range checks).

Status: **implemented** (canon r3.106).

## What Changes

1. Arithmetic (`+ - * / ^ mod rem`, shifts, bit operators), unary minus and
   `bitnot` on a Range give a value of the Range, checked when stored; a
   declaration `N := M + 1` declares a value of the Range.
2. Arithmetic, comparisons, `..`, `in` and the branches of IIf reject two
   Ranges, or a Range and an Integer that is not a constant, with a message
   that names the conversion.
3. Assignments, initializers, arguments, results and set membership reject
   a Range value where an Integer or another Range is expected, and an
   Integer value where a Range is expected, unless it is a constant inside
   the bounds.
4. Std.DateTime converts with `Integer(T)` internally; its interface does not
   change. `examples/cards.inox` and the Range tests write the conversions.
5. CANON-13 Range, `language/ranges`, `stdlib/datetime`, the manual, the
   CHANGELOG and r3.106 (closing open question 3).

## Out of scope

- Char Ranges keep their rules (a value of one is a Char).
- No conversion is implicit and no new syntax is added; `Integer(X)` and
  `T(X)` already existed.
- The run-time checks and their messages do not change.

## Capabilities

### Modified Capabilities
- `constitution` (CANON-13 Range), `language/ranges`, `stdlib/datetime`:
  edited in place (skip_specs).
