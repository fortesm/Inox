## 1. Concatenation without copying again and again

`S := S + X` in a loop is the classic trap: Delphi (and Java with `+`)
allocates a new block and copies all of S every time, so N appends cost
O(N²). Inox avoids it without a StringBuilder:

1. **One allocation per expression.** The emitter flattens a `+` chain of
   String operands (`A + B + C + D`) into one runtime call,
   `__inox_str_concat(i64 n, ptr parts)`, which sums the lengths, allocates
   once and copies each part once. No intermediate String exists.
2. **In place when nobody else sees it.** For `S := S + X ...` and `S += X`
   (S a local, State variable, field, element or `ref mut` parameter), the
   emitter calls `__inox_str_append(ptr slot, i64 n, ptr parts)`. When the
   block in the slot has count 1 (no other variable, field or temporary
   holds it) and enough spare capacity, the bytes are appended in place.
   Otherwise a new block is allocated with room to grow (capacity doubled,
   at least the new length) and the old one released. N appends then cost
   O(N) amortized, like a StringBuilder, while every String stays immutable
   for the program: a block is changed only when it has exactly one owner,
   the variable being assigned, so no other value can observe the change.
   Literals and `""` are immortal (count -1) and are never changed.
3. **Aliasing.** `S := S + S` reads S while appending to it: the runtime
   takes the source lengths first and copies with `memmove`; when it must
   grow, it copies into the new block before releasing the old one.

So no StringBuilder type is needed; if a case appears that this cannot
cover, it is reported and a StringBuilder is designed then (maintainer's
condition).

## 2. Block layout

Generated code keeps seeing `{ i64 count, i64 length, bytes }` at offsets 0,
8 and 16 (the printing code and the Assert helper depend on it). Blocks
allocated by libinoxrt get a hidden header **before** the count:

```
[ i64 capacity ][ i64 characters ][ i64 count ][ i64 length ][ bytes ... NUL ]
                                   ^ the String pointer
```

- `capacity`: bytes available for the text (in-place append).
- `characters`: the number of code points, computed when the block is
  built; `Length(S)` is then O(1), and `characters = length` means the text
  is ASCII, so `Substr` and `Pos` find positions in O(1) per character.
- Immortal blocks (literals from the compiler, `""`) have no header; the
  runtime recognizes them by count -1 and scans them when needed (they are
  short and never appended).

## 3. Positions and ranges

Positions count code points and start at 1. A range outside the String is a
bug, not a value: `Substr(S, Start, Count)` needs `1 <= Start <=
Length(S) + 1`, `Count >= 0` and `Start + Count - 1 <= Length(S)`;
otherwise the program stops with the new trap **"string index out of
range"** (fault kind 15), as an array index does. The same holds for
`Left`/`Right` (`0 <= N <= Length(S)`), `PadLeft`/`PadRight` (`Width >= 0`;
a String already longer is returned unchanged) and `Replicate` (`N >= 0`).
The runtime returns null for an invalid range and the emitted code branches
to the trap, so every fault goes through the one fault function of the
program (CANON-19). `Pos(Sub, S)` returns 0 when `Sub` is absent or empty.

## 4. Unicode data

`tools/gen_unicode_tables.py` generates `runtime/UnicodeTables.inc` from the
Unicode Character Database shipped with Python (`unicodedata`; the version
is written in the file): simple uppercase, lowercase and case-folding
mappings (one code point to one, so lengths in characters never change) and
the White_Space property (the 25 code points of PropList.txt). The tables
are compact range lists searched by binary search. Full mappings (`ß` →
`SS`), normalization and collation come with the later changes.

## 5. Runtime ABI (libinoxrt, `runtime/StringFunctions.cpp`)

| Function | Notes |
|---|---|
| `ptr __inox_str_concat(i64 n, ptr parts)` | owned result |
| `void __inox_str_append(ptr slot, i64 n, ptr parts)` | replaces `*slot` |
| `i64 __inox_str_length(ptr)` | code points; `ByteLength` reads offset 8 in IR |
| `ptr __inox_str_substr(ptr, i64 start, i64 count)` | null: out of range |
| `i64 __inox_str_pos(ptr sub, ptr s)` | |
| `ptr __inox_str_case(ptr, i32 mode)` | upper, lower |
| `ptr __inox_str_trim(ptr, i32 sides)` | |
| `ptr __inox_str_pad(ptr, i64 width, i32 side)` | null: width < 0 |
| `ptr __inox_str_replicate(ptr, i64 n)` | null: n < 0 |
| `ptr __inox_str_replace(ptr, ptr old, ptr new)` | |
| `i1 __inox_str_find(ptr s, ptr sub, i32 where)` | contains, starts, ends |
| `i32 __inox_str_compare_text(ptr, ptr)` | case-insensitive |

Every result String is owned (+1) and joins the existing ownership paths
(statement temporaries, owned slots, cleanup pad), so the leak check covers
it. Out of memory ends the program with "out of memory" as today. All
functions are `nounwind`. Names are visible only with `Use Std.Strings` /
`Use Std.StrUtils` (the File pattern); `+` is always available.

## 6. Verification

Tests per function (normal cases, empty Strings, multi-byte text, each
trap), `S += X` in a loop of 100 000 steps (time bound and memory balance),
`S := S + S`, appends to a shared block (the other variable unchanged), the
leak check over every new program, runtime unit tests for the tables and
the append paths, the MSVC IR check, and every E19 gate.

Design approval: **approved** (Marcelo Fortes, 2026-10-13: "Std.Strings / Std.StrUtils: o design espera a sua aprovação (aprovado)"; canon r3.65).
