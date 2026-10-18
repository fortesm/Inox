## Why

Inox Strings can be stored, compared and read, but a program cannot join,
measure, slice, search, trim or change the case of one. The maintainer
registered the modules (canon v3.57, r3.58) and decided their shape on
2026-10-13 (Marcelo Fortes):

- "Operador de concatenação é + mas sem repetir a recopia na memória da
  string que o delphi faz se não houver jeito use strigbuild igual do
  delphi".
- "posições começam com 1 contam caracteres code points como no delphi a
  diferença é que eu queria uma função chamada Substr() como no java ao
  invés de Copy e também quero Trim() , RTrim(), LTrim()."
- Std.Strings must hold usable functions, not only definitions ("deve ter
  funções utilizáveis pelo usuário"); "Std.StrUtils, com nomes no estilo
  Delphi Sim"; "Length(S) em vez de charlenghth (aprovado)"; "Concatenação
  com + (aprovado)". The module rule itself changed in change
  `module-pairs` (r3.64).
- Language-neutral, Unicode rules (2026-10-12): "Não queria ficar preso a um
  idioma específico".

Risk: high (new runtime ABI functions; in-place append changes how String
blocks are owned). Design approved 2026-10-13 (canon r3.65).

Status: **implemented** (canon r3.78). `Pos` became a reserved word in
r3.76, so the search function is named `Position`:
Marcelo Fortes, 2026-10-17: "Position(Sub, S) (Recommended)"

## What Changes

**Language (CANON-14):** `A + B` concatenates Strings (`+=` with it,
ADR-0012). No implicit conversion: `"N=" + 5` is an error. String
concatenation and `Length` stop being reserved; `S[I]` and
`GraphemeLength` stay reserved.

**Std.Strings** (`Use Std.Strings`) — what nearly every program needs:

| Function | Result |
|---|---|
| `Length(S)` | number of characters (code points) |
| `ByteLength(S)` | number of UTF-8 bytes |
| `Substr(S, Start, Count)` | `Count` characters from position `Start` (1 = first) |
| `Position(Sub, S)` | position of the first `Sub` in `S`, 0 when absent or empty |
| `UpperCase(S)`, `LowerCase(S)` | Unicode default case mapping, any language |

**Std.StrUtils** (`Use Std.StrUtils`, which gives everything of
Std.Strings) — Delphi-style conveniences:

| Function | Result |
|---|---|
| `Trim(S)`, `LTrim(S)`, `RTrim(S)` | without Unicode White_Space at both ends, the start, the end |
| `Left(S, N)`, `Right(S, N)` | the first / last N characters |
| `PadLeft(S, Width)`, `PadRight(S, Width)` | spaces added up to Width characters |
| `Replicate(S, N)` | S repeated N times (`Repeat` is a keyword) |
| `Replace(S, Old, New)` | every `Old`, left to right, replaced |
| `Contains(S, Sub)`, `StartsWith(S, P)`, `EndsWith(S, P)` | Bool |
| `SameText(A, B)` | equal ignoring case (Unicode simple case folding) |
| `CompareText(A, B)` | -1, 0 or 1 ignoring case |

**Later changes** (need other features first): `GraphemeLength`,
`Collate` (Unicode root order), `NormalizeNFC`/`NFD`, `Split`/`Join`
(Vector), `IsLetter`/`IsDigit`/`IsWhiteSpace` (Char values), conversions
between numbers and text.

Constitution: CANON-14 (concatenation, Length), CANON-18 (modules),
CANON-19 (new trap "string index out of range"), CANON-20 unchanged (`+`
keeps its level). Out of scope: locale-specific behavior. Behavior that must
not change: every existing String program and its memory balance.
