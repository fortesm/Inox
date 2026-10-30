## ADR-0015 — Storing into a Natural checks the range implicitly  (Status: Accepted, 2026-10-10)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1935-1952) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Decision

Decision (Marcelo Fortes, 2026-10-10): as in Ada, an assignment checks the range by itself, with no conversion written.
1. When an Integer value is stored into a Natural — initialization,
   assignment, compound assignment, argument, return, field — the range
   0..Int64.Max is checked at run time. No conversion is written:
       N Natural := 3
       N += 1          == checked
       N := N - 5      == runtime error: value out of range for Natural
2. A constant outside the range is a compile-time error (`N Natural := -1`).
3. Arithmetic on Natural still yields Integer (ADR-0014 point 4); the check
   happens where the value is stored.
4. A cast-like explicit form for this purpose is not part of the language;
   it is recorded only as a possible future form, which generics may make
   unnecessary.
This supersedes ADR-0014 points 3 and 4 where they require `Natural(X)`;
the locked ADR-0014 text is not edited.

This record has no separate rationale section.
