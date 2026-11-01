Nothing below starts before the design's approval is recorded with its date.

## 0. Before the code

- [x] 0.1 Answers to Q1-Q4 recorded (Marcelo Fortes, 2026-11-01); conditions written into the design
- [x] 0.2 Guard test `generic-type-parameter-expressions` (`New(T)`, `T(...)` in generic bodies)
- [x] 0.3 Final approval of `design.md` recorded (Marcelo Fortes, 2026-11-01)
- [x] 0.4 Change `ir-snapshot-exact` merged
- [x] 0.5 IR snapshot recorded on main; `-fstack-usage` of main kept; `tools/stack_frames.py`

## 1. Written type in the AST

- [x] 1.1 `ast::TypeExpr`, `ArrayBound`, `spell`, `checkTypeExpr` (invariants 1-3; 4 and 6 by the unit test)
- [x] 1.2 `TypeExprTests`: the canonical spelling table and deep nestings, the invariant checks, and the round trip of every written type of the corpus; equality with the old spelling proven by the IR snapshot (every diagnostic and interface)

## 2. Parser

- [x] 2.1 `parseTypeInto` builds `TypeExpr` (one recursion level 448 bytes against 1064); the dual-form step was not needed (design, "As implemented")

## 3. Analyzer

- [x] 3.1 `resolveWrittenType(const TypeExpr&)`; shape tests on the tree

## 4. Generics

- [x] 4.1 Substitution and unification on the tree; clone rewriter `TypeExpr ↦ TypeExpr`

## 5. Module resolution, contract lowering, interface

- [x] 5.1 Renaming, private-type check, name collection on the tree
- [x] 5.2 Owner and array tests, `old` copies on the tree; interface prints `spell`

## 6. Text path removed

- [x] 6.1 Text fields, `readTypeSyntax`, `substituteTypeNames`, `spellArrayType`, `isArraySpelling`, `isGenericSpelling`, `TypeNameRewriter`, `types/TypeSyntax.*` deleted
- [x] 6.2 `docs/ARCHITECTURE.md` (D4 closed), revision, kCanonRevision, CHANGELOG

## 7. Gates

- [x] 7.1 Suite, IR snapshot without any exception (1053 programs, 0 differences), `tools/stack_frames.py` (47 functions, none grows), fuzz 1 and 7, differential, backend_gaps, openspec, constitution_check, version_check
- [ ] 7.2 CI: sanitized suite (1 MiB), `windows-stack` with no growth, `ir-snapshot` (label `refactor`)
