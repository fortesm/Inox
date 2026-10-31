Nothing below starts before the design's approval is recorded with its date.

## 0. Before the code

- [ ] 0.1 Approval of `design.md` recorded (who, date, answers to Q1-Q4)
- [ ] 0.2 If Q1 is yes: change `ir-snapshot-exact` merged first
- [ ] 0.3 IR snapshot recorded on main; `-fstack-usage` of main kept for comparison

## 1. Written type in the AST

- [ ] 1.1 `ast::TypeExpr`, `ArrayBound`, `spell`; unit test: `spell` reproduces every type spelling of the corpus

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

- [ ] 7.1 Suite, IR snapshot without allow list, `-fstack-usage` per step, sanitized suite (1 MiB), fuzz 1 and 7, differential, backend_gaps, openspec, constitution_check, version_check
- [ ] 7.2 CI, `windows-stack` with no growth, `ir-snapshot` (label `refactor`)
