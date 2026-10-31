## Why

Foundation step F2 (ROADMAP; `docs/ARCHITECTURE.md` deviation D1). E14
requires every diagnostic to give the source path, line and column. AST
nodes carried no position, so a semantic error had none, and a parse error
had a line and a column but no file: in a program of several modules the
programmer could not tell where an error was, and an error in a generic
instance named the instance but not the line of the generic body.

Approval: Marcelo Fortes, 2026-10-31 (foundation milestone F0-F7).

Touches: E14 (conformance, no law text change). Compiler phases (see
`docs/ARCHITECTURE.md`): 2 parser (stamps positions), 3 module resolution
and 4 contract lowering (attach the item), 5 semantic analysis (attaches
the statement or declaration), 7 driver (prints the file); the AST and its
copies (`AstClone`) carry positions.

Risk: normal (diagnostics only; the IR and the types of every accepted
program are unchanged).

Status: **implemented** (canon r3.112).

## What Changes

1. `ast::SourcePosition` on every AST node, and on Type and Const/State
   declarations; `ast::startOf` gives where an expression starts (its first
   operand, for operators, calls and members).
2. The parser stamps leaves (`leafLiteral`, `leafIdentifier`) and
   statements, with the statements its desugaring adds (`stampStatement`,
   the start kept on a member stack); module items and declarations get
   their first token.
3. Copies keep positions (`cloneExpression`, `cloneStatement`): contract
   wrappers and generic instances point at the source; a contract check
   starts where its clause does.
4. `SemanticError` carries a position and a module. The analyzer notes the
   statement or declaration it checks (`notePosition`) and attaches it at
   its single entry point; module resolution and contract lowering attach
   the item they were working on. No recursive analysis or parsing frame
   grows (checked with `-fstack-usage`; the boundary tests need no more
   stack than on main).
5. The driver prints `path:line:column: parse error: message` and
   `path:line:column: semantic error: message`, the path relative to the
   current directory when the file is below it.

## Behavior that changes

The text of every compile error: the ten `.err` files that matched the old
`parse error at L:C:` prefix now match `:L:C: parse error:`, and
`tests/modules/ambiguous-import/Main.err` now also checks the position.
Nothing else: the IR, the types and the exit code of every program are
unchanged (snapshot below).

## Out of scope

A column for each sub-expression (it comes with the typed IR, F6); runtime
traps (CANON-19) keep their messages; lexer errors already had positions.
