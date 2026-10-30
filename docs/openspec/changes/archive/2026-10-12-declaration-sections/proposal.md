## Why

`Type`, `Const` and `State` sections reach the analyzer and the emitter as flat
token lists that each phase re-scans with its own ad-hoc loop. A field type
that is more than one token (`Array[1..4] Byte`) is therefore "invalid struct
field declaration", diagnostics carry no line, and every new declaration form
(Enum, Range, String fields) would need three more scanners. This is item 2 of
the order approved on 2026-10-12, and it unblocks items 4 and 5.

Approval (Marcelo Fortes, 2026-10-12): decisions 1 to 5 approved. Item 2: structured
parser for the sections, arrays in struct and in State.

## What Changes

- The parser builds structured declarations for the three sections: struct
  (name, fields with type and optional default), Range, short and multi-line
  Enum (parsed only; meaning comes in item 4), Const (name, initializer) and
  State (names, type, optional initializer). The semantic analyzer and the
  emitter read these nodes; the token re-scanners are deleted (E11, E13).
- Diagnostics for malformed section entries name the source line.
- Arrays as struct fields: `FData Array[1..4] Byte`, `B.FData[I]` reads and
  writes, defaults are zero.
- State variables of struct and array type (today "State variables of type
  TPoint are not lowered yet"), with or without initializer.

No Layer A rule changes: CANON-5 sections and CANON-13 arrays already allow all
of this.

Out of scope: Enum/Range/Set meaning (item 4), String fields and String State
(item 5), array literals (reserved by CANON-13), Const of array or struct type
(Const stays scalar; no change), grouped declarations in State (reported as
not supported yet).

Removed by accident of the old scanner, never canonical: a Type line
`TName := Type` was silently skipped; it is now a parse error naming the
canonical forms. No test or example used it.

Behavior that must not change: every existing section program and diagnostic
keeps its result; the probe and suite counts only grow.

## Capabilities

### New Capabilities
- `language/declaration-sections`: the shape and diagnostics of `Type`,
  `Const` and `State` sections.

### Modified Capabilities
- `language/fixed-arrays`: arrays may be struct fields.
- `language/module-state`: State variables may have struct and array types.

## Impact

- `src/compiler/ast/Ast.h/.cpp`: typed declarations replace
  `SectionDeclaration::tokens()` for the three sections.
- `src/compiler/parser/Parser.cpp`: section parsing reuses `parseTypeName` and
  the expression parser.
- `src/compiler/semantic/SemanticAnalyzer.cpp`, `src/compiler/codegen/LlvmIrEmitter.cpp`:
  read the nodes; the State global path gains aggregate types.
- Canon Layer B, docs/BACKEND_GAPS.md, docs/site/index.html, CHANGELOG.md.
