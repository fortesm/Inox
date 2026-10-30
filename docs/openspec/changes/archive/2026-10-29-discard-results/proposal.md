## Why

E2 (r3.88, a function is not a statement, as in Ada) met OPEN-4 (a local
must be read): a function called only for its effect could not be called at
all. `F(X)` alone broke E2, and `R := F(X)` without reading R broke OPEN-4.
The open point of change `console-abi-terminal` asked the maintainer what
to do; three options were set out (keep it, `_ :=`, OPEN-4 as a warning).

Approval (Marcelo Fortes, 2026-10-22): option 2: an explicit discard `_ := Expression`; E2 and OPEN-4 stay in full, no error becomes a warning; `_` is no special variable but a syntactic discard target, without identity, type, storage or scope

His wording for the constitution: E2, explicit discard of results: a function call that produces a value is not a statement alone; its result is consumed by a valid expression or discarded explicitly with `_ := FunctionCall`; `_` is a discard target, not an identifier

Decision: final decision: option 2, `_ :=`, at first only for calls with a result; E2 and OPEN-4 stay compile errors; a separate PR with tests of a lone call, an explicit discard, an unread variable and invalid discards

Risk: low (a new statement form; programs without `_` get the same IR).

Status: **implemented** (canon r3.93).

## What Changes

1. **Lexer.** `_` alone is the token Underscore, never an identifier
   (`_x` and `A_B` are still names).
2. **Parser.** `_ := Expression` at the start of a statement is a discard
   statement. `_` anywhere else (an expression, a declaration, a parameter,
   a routine name, `_ +=`, a chain) is "'_' is not a name: it only discards
   the result of a call, _ := F(X) (r3.93)".
3. **Analyzer.** The expression must be a call of a function or a method
   with a result. A literal, a variable (`_ := N` would pass for a read of
   N), a field, an element, an operation, a conversion (`Integer(S)`), a
   construction, or a call without a result is rejected. The E2 message
   now shows both ways: `X := F(...)` and `_ := F(...)`.
4. **Emitter.** The call is emitted as any call whose value is used, once;
   an owned result (String, owner, struct with Strings) is released with
   the statement's temporaries.
5. **Found on the way.** A line at module level that is no declaration
   (`+ Integer : ... ;`, `_ Integer : ... ;`) was parsed as a raw
   declaration and ignored; it is now "expected a declaration: Use, Type,
   Const, State or a routine, Name :".

## Open point

The right of `_ :=` is restricted to calls, as approved; a conversion such
as `Integer(S)` is not a call, so validating a text by discarding its
conversion (`_ := Integer(S)` inside try) is rejected. It can be widened
later if needed.
