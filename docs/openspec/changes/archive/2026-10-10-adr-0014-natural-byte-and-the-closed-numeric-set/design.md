# ADR-0014 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1953-1980) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Rationale and consequences

Rationale: Ada's `Natural` is a subtype, not a new unsigned type, so mixing it
with Integer needs no casts in the safe direction and a negative value is
caught where it enters. Keeping the Int64 representation makes the subtype
free at run time and keeps case coverage provable with Int64 constants.
Consequence: `N += 1` on a Natural is `N := N + 1`, an Integer result, so it
is written `N := Natural(N + 1)`. The ADR-0011 sentence "UInt64 and Natural
are never provable" is superseded for Natural (UInt64 is unchanged); the
locked ADR-0011 text is not edited.
