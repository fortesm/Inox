## Context

`SectionDeclaration` stores `tokens()`, `tokenLines()` and, for Const/State,
parsed initializers. `collectStructDefinitions` (emitter), the analyzer's
struct and State collectors, and the State global collector each walk the
tokens with their own rules. Multi-token types break every one of them.

## Goals / Non-Goals

Goals: one parse of each section into typed nodes; arrays in struct fields and
State; struct State. Non-goals: Enum/Range meaning (the nodes exist and are
checked for shape only until item 4), String storage (item 5).

## Decisions

- **Nodes.** `SectionDeclaration` keeps its kind and now holds typed
  declarations instead of tokens: `TypeDeclaration{kind Struct|Range|Enum,
  name, line, fields[{name, typeName, default literal, line}], bounds
  (the Range's `Low..High` expression), literals}` for Type, and
  `VariableDeclaration{name, typeName?, value?, line}` for Const and State.
  `typeName` uses the existing front-end spelling (`Array[L..H] T`,
  `Slice<T>`), produced by `parseTypeName`. One node class with typed members
  keeps the module item list unchanged.
- **Array types in sections.** A field or State array type is made canonical
  when the section is analyzed, after every Type and Const is declared: its
  bounds may name a Const and its element may be a struct declared later.
  The canonical name is recorded for the emitter like any other written type.
- **Migration in one step.** The three consumers switch to the nodes in the
  same change and the token scanners are deleted; keeping both paths would
  violate E13 (no valid-looking fallback).
- **Lowering.** A struct field of array type is an `[N x T]` member; element
  access is GEP(struct field) then the checked GEP of item 1/v3.45. State
  aggregates are `internal global %T zeroinitializer`; defaults and initializers
  are stored at the start of `main` in source order, like scalar State today.
- **Dumps.** No dump prints section tokens, so no expectation file changes.

## Risks / Trade-offs

- A broad parser change can regress section programs. Mitigation, applied:
  `--dump-types` and `--emit-llvm` were recorded for all 462 tests, examples
  and stdlib files before the switch; after it, only three outputs differ, all
  intended (`Type :` gets a clear parse error; two State struct programs now
  compile).

## Runtime faults

No new fault kinds; kind 9 for field array indices.
