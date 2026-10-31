## Why

Deviation D4 of `docs/ARCHITECTURE.md`: every type a program writes is
text in the AST. The parser reads `Array[1..N] TPair<String, Int64>` into
structure and then spells it back into a string (`parseTypeName`,
`spellArrayType`); the analyzer reads that string again
(`types::readTypeSyntax`, about 20 shape tests with `isArraySpelling` and
`isGenericSpelling`); generic instances replace type parameters by
rewriting text (`substituteTypeNames`, the clone's `TypeNameRewriter`);
module resolution renames types inside strings (`rewriteText`) and checks
private types by scanning words (`privateTypeIn`); contract lowering tests
`Own`/`Option`/arrays on text. Each reader can disagree with the parser,
and the coming work (methods of generic structs in 0.5, routine types,
concurrency's typed tasks) multiplies the places that read types.
Foundation step F4.

Approval of the plan: Marcelo Fortes, 2026-10-31 (F0-F7, E11 contract).
This step is high risk: the design waits for his dated approval before
any code is written.

Touches: E11 (no text change). Compiler phases: 2 parser (builds the
written type), 3 module resolution (renames and visibility on the tree),
4 contract lowering (owner and array tests on the tree), 5 semantic
analysis (resolution and generic instantiation on the tree), the AST
(`ast/`, the clone, the interface printer) and `types/` (loses the text
reader). Codegen only tests whether a type was written. Updates
`docs/ARCHITECTURE.md` (D4 closed, parser and AST contracts).

Risk: high (rewrites how phases 2-5 represent written types).

Status: **design awaiting approval**. No code before it.

## What Changes

1. `ast::TypeExpr`, the written form of a type, replaces the `typeName`
   strings of the AST (fields, Const/State, parameters, results, typed
   locals) and moves the tree `types::TypeSyntax` into the AST, where the
   written form belongs (`ast` keeps not knowing `types`).
2. The parser builds it; the text spelling exists only for messages, the
   interface and the names of generic instances, through one function.
3. The analyzer resolves a `TypeExpr`; generic instances substitute type
   parameters on the tree; module resolution and contract lowering work on
   the tree.
4. The text path goes: `readTypeSyntax`, `substituteTypeNames`,
   `spellArrayType`, `isArraySpelling`, `isGenericSpelling`,
   `TypeNameRewriter`.

No language change: every program keeps its IR, types, diagnostics and
exit codes (IR snapshot, without an allow list).

## Out of scope

Exception handler types (names until F7); type names inside expressions
(casts, constructors, `New(T)`), which are names the analyzer resolves;
desugaring (F5); the typed IR (F6).

## Open questions

See `design.md`, "Decisions asked of the maintainer" (Q1-Q4).
