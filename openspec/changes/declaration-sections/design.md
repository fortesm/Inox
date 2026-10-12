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

- **Nodes.** `StructDeclaration{name, line, fields[{name, typeName, default
  Expression?, line}]}`, `RangeDeclaration{name, low, high, line}`,
  `EnumDeclaration{name, literals[], line}`, `ConstDeclaration{name,
  Expression, line}`, `StateVariable{names[], typeName?, Expression?, line}`.
  `typeName` uses the existing canonical spelling (`Array[L..H] T`,
  `Slice<T>`), produced by `parseTypeName`.
- **Migration in one step.** The three consumers switch to the nodes in the
  same change and the token scanners are deleted; keeping both paths would
  violate E13 (no valid-looking fallback). `SectionDeclaration` is removed if
  nothing else uses it.
- **Lowering.** A struct field of array type is an `[N x T]` member; element
  access is GEP(struct field) then the checked GEP of item 1/v3.45. State
  aggregates are `internal global %T zeroinitializer`; defaults and initializers
  are stored at the start of `main` in source order, like scalar State today.
- **Dumps.** `--dump-*` output for sections is regenerated from the nodes; the
  parser expectation files are updated and reviewed by diff.

## Risks / Trade-offs

- A broad parser change can regress section programs. Mitigation: before the
  switch, record `--dump-types` and `--emit-llvm` for every test and example
  and require identical output after it, except the intended dump shape.

## Runtime faults

No new fault kinds; kind 9 for field array indices.
