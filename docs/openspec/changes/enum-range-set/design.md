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
  i64. Comparisons are `icmp` on ordinals. Printing (Q3, decided) uses a
  private constant table of literal names per enum, spelled as declared,
  emitted once per module.
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

## Where the implementation differs

Recorded at implementation (canon r3.80); the decisions above stand
otherwise.

- **One PR.** The three phases went in one PR, each phase its own commit
  in the history of the branch.
- **Enums are i64.** An enum value is its ordinal in an i64, like every
  integer type, so no widening or narrowing happens at calls, fields or
  array elements; the language does not see the width.
- **Kind 11 for Succ and Pred.** Past the ends they stop with "value out of
  range", the Range fault (Q1), not kind 8 (Q2 is for `TEnum(I)`).
- **Aggregates.** An enum struct field may have a literal default; a field
  without one and an array element start at the first literal, the zero
  every aggregate starts from (CANON-5: aggregates take their type's
  defaults). Recorded as an open question for the maintainer.
- **Range bounds** are literals or Consts (as array bounds are); the grammar
  says so now.
- **A Range over Char** (`TLetter Range 'A'..'Z'`) is declared and its
  bounds checked, but Char values are not lowered yet, so its values and
  Sets over it come with that item of 0.2.
- **`With(S, X)`.** `with` is a reserved word; in an expression, followed by
  `(`, it can only be the set function (a `with` statement never starts an
  expression), so the parser reads it so. `Without` is an ordinary name.
- **Sets** are integers of whole bytes (`i8` to `i256`); a value outside a
  Range base is in no set (`101 in Seen` is false), and `X in A..B` is
  lowered too.
- **Messages.** An enum and an Integer never stand for each other, each way
  with a message that shows `Ord(E)` or `TEnum(I)`; enum arithmetic points
  to Succ, Pred and Ord.
