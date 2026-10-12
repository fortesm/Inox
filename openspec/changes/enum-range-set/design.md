## Context

After `declaration-sections`, the parser produces `EnumDeclaration` and
`RangeDeclaration` nodes. Nothing gives them meaning yet: no type symbol, no
literal constants, no lowering. `case` coverage already proves Bool and
Integer-family choices (ADR-0011) and has a hook for Enum.

## Decisions

- **Enum symbols.** Each enum is a nominal type; each literal is a typed
  constant with its ordinal. Literal names share the module's single,
  case-insensitive namespace (Q4: a duplicate is the ordinary duplicate-symbol
  error).
- **Enum lowering.** i32 holding the ordinal (no narrower type: keeps the ABI
  simple and leaves room for future explicit values). `TEnum(I)` checks
  0 <= I < N with `__inox_range_i64` (kind per Q2). `Ord(E)` is a `zext` to
  i64. Comparisons are `icmp` on ordinals. Printing (Q3) uses a private
  constant table of literal names per enum, emitted once per module.
- **Range lowering.** A Range is its base type's representation (i64 for the
  Integer family, i32 for Char) plus bounds known to the analyzer. Stores of a
  non-constant value into a Range target call `__inox_range_i64` (kind 11
  under Q1); constants are checked at compile time. Arithmetic on Range values
  is done in the base type (like Natural, ADR-0015).
- **Set lowering.** `iN` with N = base cardinality rounded up to 8 (Q7: N <=
  256). `in` is shift-and-test; `With`/`Without` are or/and-not with a one-bit
  mask; `Union`/`Intersection`/`Difference`/`SymmetricDifference` are
  or/and/and-not/xor; `<=` is `(A and not B) = 0`. Element ordinal is taken
  relative to the base's Low. LLVM handles wide integers natively, so there is
  no runtime support.
- **Coverage.** Enum: the set of covered ordinals must equal 0..N-1. Range: the
  union of choices must equal Low..High (same interval logic as the Integer
  case).
- **for.** `for S in A..B` over enum literals iterates ordinals and converts
  back; the iterator has the enum type.

## Risks / Trade-offs

- A literal namespace shared with variables and functions can collide with
  existing user names; Q4 makes that an explicit error rather than silent
  shadowing.
- i32 for enums wastes space in large arrays; acceptable in 0.1 and invisible
  to the language.

## Runtime faults

Kind 8 (or a dedicated kind, per Q2) for `TEnum(I)`; new kind 11 "value out of
range" for Range stores (Q1); kind 9 unchanged.
