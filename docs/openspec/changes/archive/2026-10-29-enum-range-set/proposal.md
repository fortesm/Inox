## Why

Enum, Range and Set are CANON-13 law (ADR-0006 #9-15) and the base of
mission-critical modelling: states, codes and flags without magic integers.
Today the parser accepts the declarations but `TSuit` is "unknown type", the
CANON-5 rule 5 strict initialization is not enforced, and `case` cannot check
enum coverage (ADR-0011). This is item 4 of the order approved on 2026-10-12.

Approval (Marcelo Fortes, 2026-10-12): "Todas as tomadas de decisão dos pontos
1 a 5 foram verificadas e eleitas para serem acatadas." Item 4: Enum, Range
and Set.

Risk: normal.

Status: **implemented** (canon r3.80), the three phases in one PR (the
maintainer prefers consolidated PRs); design.md records where the code
differs. A Range over Char is declared and waits for Char values.

## What Changes

Implemented in three phases, each its own PR and green gates:

- **Phase A, Enum.** Short and multi-line Enum types; values start at 0;
  `Ord(E)`; checked `TEnum(I)`; strict initialization (`Suit TSuit` is an
  error); `=`, `#`, `<`, `<=`, `>`, `>=` between values of one enum type; no
  implicit conversion to or from Integer; enum ranges in `for`; enum `case`
  without `else` must cover every literal; enums in fields, elements,
  State, parameters and results.
- **Phase B, Range.** `TMonth Range 1..12` (Integer family) and
  `TLetter Range 'A'..'Z'` (Char): a constrained subtype of its base;
  iteration with `for M in Low(TMonth)..High(TMonth)` (Q5);
  ranges as array index types are NOT part of this change.
- **Phase C, Set.** `Set<T>` over an Enum or finite Range, value type, empty by
  default; `in`, `=`, `#`, `<=`, `>=`; `Union`, `Intersection`, `Difference`,
  `SymmetricDifference`, `With`, `Without`.

Decisions (Marcelo Fortes answered on 2026-10-12, item by item, in his message "Decisões que dependem de mim"); they are now Layer A (canon v3.50; Q3 in v3.51):
- **Q1. Storing into a Range: implicit check, as for Natural (ADR-0015).** An
  Integer value stored into a Range variable is checked at run time (new fault
  kind 11, "value out of range"); a constant outside the range is a compile
  error; arithmetic on Range values yields the base type.
- **Q2. `TEnum(I)` fault: kind 8, "integer conversion out of range".**
- **Q3. Printing an enum: the literal's name ("Resposta (a) o nome Copas",
  2026-10-12, canon v3.51).** With `Naipe TNaipe := Copas`, `PutLn(Naipe)`
  prints `Copas`, spelled as in the declaration; `PutLn(Ord(Naipe))` prints
  `2`.
- **Q4. Same literal in two enums: forbidden.**
- **Q5. `Low(T)`/`High(T)` on enum and range types: yes; `Succ`/`Pred`,
  checked (trap past the ends): yes.**
- **Q6. Sets: functions returning a new set (`S := With(S, Club)`,
  `U := Union(A, B)`) plus the literal `[Club, Heart]` in a Set-typed context
  ("-> sim").**
- **Q7. Set base: at most 256 elements; above that is a compile error.**

Out of scope: enum literal values other than 0..N-1, Range or Enum as array
index types, sets over Char ranges larger than Q7's limit, Vector.

Behavior that must not change: Integer `case` coverage rules (ADR-0011);
Natural (ADR-0014/0015).

## Capabilities

### New Capabilities
- `language/enums`: enumeration types.
- `language/ranges`: range subtypes.
- `language/sets`: finite sets over ordinal bases.

### Modified Capabilities
- `language/case-statement`: enum and range coverage without `else`.

## Impact

- Parser: the item-2 Enum and Range nodes gain meaning; `Set<T>` type name;
  set literal in typed context (Q6).
- Semantic analyzer: enum and range types, literals as constants, strict
  initialization, coverage, conversions.
- Emitter: enums as i32, ranges as their base, sets as a bit vector.
- Canon (Q1–Q7 as Layer A text once approved), grammar.ebnf (Set literal),
  docs/site/index.html, CHANGELOG.md, docs/BACKEND_GAPS.md.

Depends on `declaration-sections` (item 2).
