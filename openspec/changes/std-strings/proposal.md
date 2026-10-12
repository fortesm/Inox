## Why

The operators compare Strings (`=` `#` `<` `>` `<=` `>=`, byte order), but a
program cannot trim, change case, search, slice or compare without regard to
case. Marcelo Fortes, 2026-10-12: "Registre essas funções de biblioteca no
futuro -> Std.Strings", and Inox must not be tied to one human language:
"Não queria ficar preso a um idioma específico e deveria ser mais genérico
acho que como moderno Delphi, FreePascal, Ada2005 em diante".

Status: PLANNED. Registered in canon v3.57 (CANON-18). Not scheduled yet: it
comes after the compiler modularization and Std.Files. Design approval is
required before code (a new runtime surface).

## How other languages separate the operator from human-language order

| Language | `<` on strings | Case-insensitive | Alphabetical order of a language |
|---|---|---|---|
| Delphi / Free Pascal | ordinal (UTF-16 code units) | `CompareText`, `SameText` (ASCII); `AnsiCompareText` (OS locale) | `AnsiCompareStr` (OS locale) |
| Ada | `Character'Pos` order | `Ada.Strings.Equal_Case_Insensitive` (Ada 2012) | not in the standard |
| Python | code points | `str.casefold()` | `locale.strcoll` |
| Java | UTF-16 code units | `compareToIgnoreCase` | `java.text.Collator` |
| Go | bytes (= code points) | `strings.EqualFold` | `golang.org/x/text/collate` |
| Rust | bytes (= code points) | `eq_ignore_ascii_case` | external crates (ICU4X) |
| Swift | Unicode canonical equivalence, not locale | `caseInsensitiveCompare` | `localizedCompare` (Foundation) |
| Ruby | bytes | `casecmp?` | external gems |

None of them makes the operator depend on a human language: the operator is
an exact, portable order, and the language-aware orders are library calls.
Inox follows that (canon v3.56). Language-neutral does not have to mean
"bytes only": Unicode defines rules that belong to no single language —
default case mapping and case folding, White_Space, normalization, and the
Unicode Collation Algorithm with its root (default) order, which sorts
"água" next to "agua" for every language. A specific language's order
(Swedish, Spanish ...) is an explicit, later option.

## What Changes (draft; names decided in the design)

- Lengths: `ByteLength(S)`, `CharLength(S)` (code points), `GraphemeLength(S)`.
- Trimming: `Trim`, `TrimLeft`, `TrimRight` (Unicode White_Space).
- Case: `Upper`, `Lower` (Unicode default case mapping), `CaseFold`.
- Comparison: `SameText(A, B)` (case-insensitive equality by case folding),
  `CompareText(A, B)` and `CompareStr(A, B)` (-1, 0, 1), `Collate(A, B)`
  (Unicode Collation Algorithm, root order).
- Normalization: `Normalize` to NFC / NFD.
- Search: `Pos`, `Contains`, `StartsWith`, `EndsWith`.
- Building: `Copy` (slice by characters), `Replace`, `Split`, `Join`,
  `Repeat`, `PadLeft`, `PadRight`, `Reverse`.
- The String concatenation operator (reserved by Q3 of `string-runtime`).

Dependencies: `Split`/`Join` need `Vector<String>`; character access needs
`Char` lowering; Unicode tables (case, White_Space, normalization,
collation) must be generated into libinoxrt, portable and without C interop
in `stdlib/`.

## Capabilities

### New Capabilities
- `stdlib/strings`: the `Std.Strings` module.

### Modified Capabilities
(none)

## Impact

stdlib/Std.Strings.inox, libinoxrt (Unicode tables and algorithms), the
emitter (intrinsics), canon CANON-14/18, manual.
