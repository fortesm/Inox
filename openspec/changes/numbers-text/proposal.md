## Why

Third change of 0.3 (`docs/ROADMAP.md`): numbers and text. A program reads a
line and needs the number in it, builds a line with numbers in it, splits a
line into fields, and prints reals that say exactly what they hold
(`printf("%f")` showed six decimals: `0.1` as `0.100000`, `1e-7` as
`0.000000`).

Request (Marcelo Fortes, 2026-10-21): "Implemente: Versão 0.3, números,
texto e biblioteca básica: [...] Conversão entre número e texto, Split/Join
e literais com expoente [...]".

Approval (Marcelo Fortes, 2026-10-21), answering the decisions proposed to
him, of which this change carries B1 (for integers and reals), B2 (for
integers and reals), B3 and B4: "B Acatado todos". B4 moves Split/Join to
0.5, with the collections, and gives FieldCount/Field now.

Risk: normal (new runtime functions; every Float output changes text, so
the expected outputs of the suite change; no change to exception lowering
or memory ownership).

Status: **implemented** (canon r3.90). `String(C)` and `Currency(S)` of a
Currency come with the change that gives Currency values.

## What Changes

1. **B1, numbers as text.** `String(N)` of any integer type (UInt64
   unsigned) and `String(F)` of a Float or Float32 are the text Put writes.
   `FormatFloat(F, Decimals)` (Std.Strings): exactly Decimals digits after
   the point, 0..20 (else "string index out of range"), rounded to nearest
   from the exact binary value (ties to even, `std::to_chars`, the same on
   every C++ library); no point for 0.
2. **B2, text as numbers.** `Integer(S)` and every integer type (Natural,
   Byte, a Range: the type's limits), `UInt64(S)`, `Float(S)`, `Float32(S)`
   read the whole String: an optional `-` and digits, for a real an
   optional fraction and exponent. Spaces, `+`, `_`, a value outside the
   type or nothing raise the new exception `ConversionError` (taxonomy:
   under Exception), which a handler catches (`TryInteger` comes with
   Result in 0.5). Text is never an enum value.
3. **B3, how reals are written.** `Put`, `PutLn`, `Write`, `WriteLn` and
   `String(F)` write the shortest text that reads back as the same value of
   the real's own type: fixed notation for decimal exponents -4..15 with at
   least one digit after the point (`2.5`, `1500.0`,
   `0.30000000000000004`), else an exponent in Inox literal form (`1.0e16`,
   `1.5e-5`); `-0.0`, `NaN`, `Inf`, `-Inf`. A Float32 prints its own
   shortest text (`0.1`, not the Float64 it widens to). The expected outputs
   of the suite are updated. Every printed real is a valid Inox literal and
   reads back with `Get` and `Float(S)`.
4. **B4, fields.** `FieldCount(S, Sep)` and `Field(S, Sep, I)`
   (Std.StrUtils): fields between non-overlapping separators, from 1, empty
   fields included; one field for `""` or an empty separator; a field
   outside 1..FieldCount stops with "string index out of range".

## Capabilities

### Modified Capabilities
- `language/reals`: conversions take numbers or text; numbers as text.
- `language/chars`: String(X) of a number.
- `stdlib/strings`: FormatFloat; fields.
- Constitution CANON-8 (numbers and text), CANON-14 (String functions split
  into Std.Strings and the conveniences), CANON-15 (ConversionError),
  CANON-17 (how reals are written), CANON-18 (planned modules).

## Impact

- libinoxrt: `runtime/NumberText.cpp` (formatting and parsing shared by the
  console, files and Strings); `__inox_con_put_f32`, `__inox_file_write_f32`,
  `__inox_str_from_*`, `__inox_str_format_float`, `__inox_str_to_*` (these
  raise), `__inox_str_field_count`, `__inox_str_field`; runtime unit tests.
- Compiler: `ConversionError` in the taxonomy (`ExceptionTypes.h`); String(X)
  and T(S) in the analyzer and emitter; three Std.Strings/StrUtils entries.
- Tests: about twenty expected outputs change from `2.500000` to `2.5`;
  `conversion-of-string` is gone (now valid), `string-of-integer` became
  `string-of-bool`.
- `tools/calc_differential_test.py` help text.
