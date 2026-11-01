## Why

`docs/STATUS.md` listed one over-acceptance item: `TPoint.M(Self TPoint)`
was rejected with a generic parse message ("expected ',' or ')' in
function"), not one naming CANON-9, whose scenario said "test to add".
Since F3 the parser builds routine headers, so the rule can be stated
where the mistake is.

Approval: Marcelo Fortes, 2026-11-01.

Touches: CANON-9 (the scenario names its tests; constitution relocked).
Compiler phases: 2 parser (`Parser::parseRoutineHeader`). Not changed:
`docs/ARCHITECTURE.md` (no boundary, owner or deviation changes).

Risk: normal.

Status: **implemented** (canon r3.116).

## What Changes

1. After the receiver `Self` or `Self mut`, a name is a parse error at that
   name: "the receiver of a method is Self or Self mut, without a type: its
   type is TPoint, the prefix of TPoint.M (CANON-9)"; `owned` there says
   "'Self owned' is reserved for a future Inox version (CANON-9)". The
   message is built out of line (`rejectReceiverType`).
2. Tests `receiver-with-type`, `receiver-owned-reserved`; CANON-9's
   scenario names them; STATUS has no over-acceptance item left; the site
   says the receiver has no written type.

## Out of scope

`Self owned` itself (reserved).
