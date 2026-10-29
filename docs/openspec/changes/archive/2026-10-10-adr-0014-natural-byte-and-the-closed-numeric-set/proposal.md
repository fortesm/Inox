## ADR-0014 — Natural, Byte and the closed numeric set  (Status: Accepted, 2026-10-10)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1953-1980) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Decision

Decision (Marcelo Fortes, 2026-10-10, accepting the Claude/ChatGPT consensus
"like Ada 2005, without negatives"):
1. `Natural` is the non-negative subtype of `Integer`: range
   0..Int64.Max, representation i64, the same as Integer. It is not UInt64;
   `UInt64` remains a separate type.
2. A Natural is usable wherever an Integer is (implicit Natural -> Integer).
3. An Integer becomes a Natural only through `Natural(X)`, which checks
   X >= 0. A negative value at run time is a deterministic runtime fault
   (CANON-19, "value out of range for Natural"); a constant >= 0 needs no
   conversion, and a negative constant is a compile-time error.
4. Arithmetic involving Natural is done in the Integer family and yields
   Integer; storing the result in a Natural needs `Natural(...)`. Negation and
   `bitnot` of a Natural yield Integer.
5. `Byte` is the friendly name of `UInt8` (0..255), the same type.
6. Every integer conversion `T(X)` checks the range of T; a constant outside
   it is a compile-time error.
7. The numeric set is the CANON-8 table: no `Double` (use `Float`, which is
   Float64) and no `Variant`/`OleVariant`.
