Nothing below starts before the design's approval is recorded with its date.

## 0. Before the code

- [x] 0.1 Answers to Q1-Q4 recorded (Marcelo Fortes, 2026-11-01); conditions written into the design
- [x] 0.2 Guard test `generic-type-parameter-expressions` (`New(T)`, `T(...)` in generic bodies)
- [ ] 0.3 Final approval of `design.md` recorded (who, date)
- [ ] 0.4 Change `ir-snapshot-exact` merged
- [ ] 0.5 IR snapshot recorded on main; `-fstack-usage` of main kept; `tools/stack_frames.py`

## 1. Written type in the AST

- [ ] 1.1 `ast::TypeExpr`, `ArrayBound`, `spell`, `checkTypeExpr` (invariants 1-6)
- [ ] 1.2 Unit test of the canonical spelling table and deep nestings; corpus test against today's parser

## 2. Parser

- [ ] 2.1 `parseTypeName` builds `TypeExpr` (no larger frame); text kept beside it, made by `spell`

## 3. Analyzer

- [ ] 3.1 `resolveWrittenType(const TypeExpr&)`; shape tests on the tree

## 4. Generics

- [ ] 4.1 Substitution and unification on the tree; clone rewriter `TypeExpr ↦ TypeExpr`

## 5. Module resolution, contract lowering, interface

- [ ] 5.1 Renaming, private-type check, name collection on the tree
- [ ] 5.2 Owner and array tests, `old` copies on the tree; interface prints `spell`

## 6. Text path removed

- [ ] 6.1 Text fields, `readTypeSyntax`, `substituteTypeNames`, `spellArrayType`, `isArraySpelling`, `isGenericSpelling`, `TypeNameRewriter`, `types/TypeSyntax.*` deleted
- [ ] 6.2 `docs/ARCHITECTURE.md` (D4 closed), revision, kCanonRevision, CHANGELOG

## 7. Gates

- [ ] 7.1 Suite, IR snapshot without any exception, `tools/stack_frames.py` per step (no frame of the table grows), sanitized suite (1 MiB), fuzz 1 and 7, differential, backend_gaps, openspec, constitution_check, version_check
- [ ] 7.2 CI, `windows-stack` with no growth, `ir-snapshot` (label `refactor`)
