## Why

`Char` is law (CANON-14: a Unicode scalar value, never a byte or an
integer, no `Ord`, every conversion explicit), and a Range over Char is
declared since `enum-range-set`, but the backend lowers no Char value: a
Char local or comparison is "not yet implemented", `PutLn` of a Char is
rejected, and arrays of Char are refused. Char values are the next item of
0.2 in `docs/ROADMAP.md`.

Approval (Marcelo Fortes, 2026-10-19), of the eight recommendations below as
they were proposed to him: all approved; proceed as usual

Risk: normal (two new functions in libinoxrt's String table, the same
shape as Substr; no change to an existing runtime function, exception
lowering or memory ownership).

Status: **implemented** (canon r3.83).

## What Changes

The eight decisions:

1. **Conversions.** `Char(I)` from an integer is checked: outside
   0..$10FFFF, or a surrogate ($D800..$DFFF), the program stops with
   "integer conversion out of range" (kind 8); a constant outside is a
   compile error. `Integer(C)` gives the code point (other integer types
   convert from it as usual). No other conversion exists, and `Ord(C)`
   stays rejected.
2. **Char and String.** `String(C)` is the one-character String; there is no
   implicit conversion, so `S + String(C)`. `CharAt(S, I)` (Std.Strings) is
   the character at position I, counted from 1 in characters; outside the
   String the program stops with "string index out of range" (kind 15).
   `S[I]` stays reserved.
3. **Comparison.** `=`, `#`, `<`, `<=`, `>`, `>=` by code point, as Strings
   are ordered.
4. **Ranges and loops.** `for Letter in 'A'..'Z'`, `case` with Char ranges,
   `Letter in 'a'..'z'`; Char Range types (`TLetter Range 'A'..'Z'`) hold
   Chars, check stores ("value out of range", kind 11), have `Low` and
   `High`, and are the base of a `Set<TLetter>` (at most 256 values). No
   arithmetic and no `Succ`/`Pred`: a Char is not an ordinal.
5. **Escapes.** A Char literal takes the escapes of Strings and its quote:
   `'\n'`, `'\r'`, `'\t'`, `'\0'`, `'\\'`, `'\''`; any other escape, an
   empty literal, two characters or a surrogate is a compile error.
6. **Char fields.** A struct field of type Char has a literal default or is
   given in the construction (as an enum field, r3.82): U+0000 is never an
   implicit value.
7. **Output.** `Put`, `PutLn` and `Write`/`WriteLn` to a file write the
   character in UTF-8. Reading one Char (`Get(C)`) comes later.
8. **Where a Char lives.** Locals, parameters (also `ref`), results, struct
   fields, State, array elements (`Fill('A')`), generic instances, `IIf`.

Representation: as the CANON-8 table says, a Char is stored in 32 bits (an
array element is an i32) and computed as an i64 holding its code point, the
way narrow integers are; so, like them, a Char array element cannot yet be
passed by `ref` (a recorded gap).

Out of scope: `Get(C)`, `S[I]`, Char classification (IsLetter, IsDigit) and
case mapping of a single Char, conversions between Byte and Char other than
through Integer.

## Capabilities

### New Capabilities
- `language/chars`: Char values.

### Modified Capabilities
- `language/ranges`: Char Ranges hold values.
- `language/sets`: sets over a Char Range.
- `language/structs`: Char fields have no implicit value.
- `stdlib/strings`: CharAt.

## Impact

- Lexer: escapes in Char literals.
- Analyzer: Char literals checked once (escapes, one scalar, no surrogate),
  conversions, comparisons, `String(C)`, `CharAt`, Char Ranges and their
  sets, `for` over Char ranges, required Char fields.
- Emitter: Char computed as i64 and stored as i32; printing and file
  writing through `String(C)`.
- libinoxrt: `__inox_str_from_char`, `__inox_str_char_at`.
- Constitution CANON-14 (Char), the capability specs, grammar.ebnf,
  docs/site/index.html, CHANGELOG.md, docs/STATUS.md, docs/ROADMAP.md.
