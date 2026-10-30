## ADR-0013 — Conditional expression  (Status: Accepted, 2026-10-10)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1981-2006) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Decision

Decision (Marcelo Fortes, design chat with ChatGPT):

    Result :=
        if X < 0
        then -X
        else if X > Limit
             then Limit
             else X

1. The conditional expression is a language construct, not a function (unlike
   Delphi `IfThen`/`IIF`, whose arguments are all evaluated).
2. `then` and `else` are mandatory.
3. Only the chosen branch is evaluated (`if X # 0 then 100 div X else 0` is
   safe).
4. The condition is Bool; both branches have compatible types, which is the
   type of the expression.
5. Each branch is an expression, never a block: Inox does not make blocks
   produce values. Choosing behavior is the if statement; choosing a value is
   the expression.
