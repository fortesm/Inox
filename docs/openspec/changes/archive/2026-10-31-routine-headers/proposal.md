## Why

Deviation D3 of `docs/ARCHITECTURE.md`: the parser kept each routine
header as a list of text tokens (`signatureTokens`), and four later phases
parsed it again, each in its own way: the analyzer
(`registerFunctionSignatureIn`), contract lowering (`hasResult`,
`parameterList`, `parameterNames`), generics (`writtenParameters`) and
module resolution, plus `inox interface`. Header syntax errors were
semantic errors without the token's position, and two of those scanners
read parameter names as if they were types (the `Result` parameter check
and the private-type check of a Public routine). Foundation step F3.

Approval: Marcelo Fortes, 2026-10-31: the foundation plan F0-F7 with the
E11 contract; F3 proceeds as planned, without anticipating concurrency.

Touches: E11 (no text change). Compiler phases: 2 parser (builds the
header, reports its syntax errors), 3 module resolution, 4 contract
lowering, 5 semantic analysis (signatures, generic instances), and the
interface printer of `ast/`. `docs/ARCHITECTURE.md` updated (D3 closed,
the parser's guarantees).

Risk: normal (a refactor of one AST node and its readers; the generated
code of every test program is unchanged, checked with `tools/ir_snapshot.py`).

Status: **implemented** (canon r3.114).

## What Changes

1. `ast::RoutineHeader` replaces `signatureTokens`: its
   `ParameterDeclaration`s (name, mode Value/Ref/RefMut, `isReceiver` for
   `Self`/`Self mut`, the written type) and the result type. Types stay
   text until F4.
2. `Parser::parseRoutineHeader` builds it and raises the header's syntax
   errors as parse errors at the token, with the messages they had:
   `mut X T`, `ref` on a receiver, missing name or type, missing `,` or
   `)`, an invalid result type.
3. The analyzer keeps the checks that need types or other declarations:
   the receiver's type, slices by reference or as result, `File`,
   duplicate parameters, methods of built-in types.
4. Contract lowering, generic instantiation, module resolution and
   `inox interface` read the structure. The interface prints the same
   text as before.

Changed output, reported in the pull request:
- Header syntax errors are parse errors at the token
  (`mut-parameter-reserved`: `10:1: semantic error` becomes
  `10:6: parse error`, same message).
- A trailing comma (`F(X Integer,)`) and a reserved word as a parameter
  name (`F(Do Integer)`), outside the grammar but accepted before, are
  parse errors; an unknown word after the result type is "invalid return
  type" rather than "unknown type".
- The `Result` check reads parameter names only; the private-type check
  of a Public routine reads types only.

CI (E19, no change of its text): the Gates job `ir-snapshot` could accept
only the programs a pull request adds, so an intended difference in an
existing program failed it. It now also accepts the paths that the
OpenSpec change lists in `ir-snapshot-allow.txt` (here
`mut-parameter-reserved`). Approved by Marcelo Fortes, 2026-10-31, as a
condition of F3, restricted to CI.

## Out of scope

Written types as structure and generics on that structure (F4); the
desugaring the parser still does (F5).
