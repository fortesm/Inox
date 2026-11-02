Nothing below starts before the design's approval is recorded with its date.

## 0. Before the code

- [ ] 0.1 Approval of `design.md` recorded (who, date, answers to Q1-Q4)
- [ ] 0.2 Tests for the behaviour to keep where none exists: loop invariant and variant with `continue`, `leave`, `until` and an exception; nested `with` writing through the inner target; a chain of four with a field target and a call as value (evaluated once)
- [ ] 0.3 IR snapshot of main recorded; `-fstack-usage` of main kept

## 1. AST and parser

- [ ] 1.1 `ChainedAssignmentStatement`, `WithTargetExpression`, loop `contracts`; `AstClone` names them
- [ ] 1.2 The parser builds the written forms; `pendingStatements_`, `precedingStatements_`, `withTargetStack_`, `withCounter_`, `contractCounter_`, `lowerLoopContracts`, `cloneChainTarget` leave it (and compound assignment if Q2)

## 2. Phase 2b

- [ ] 2.1 `lowering/Desugar.{h,cpp}`: chains, `with` (pre-order numbering), loop contracts (post-order numbering, positions and messages as before); run by the driver after each parse
- [ ] 2.2 Unit tests: written forms from the parser; the pass's output equal to main's parser output for each construct

## 3. Documents

- [ ] 3.1 `docs/ARCHITECTURE.md` (phase 2b contract, D2 closed, source map), `AGENTS.md`, STATUS (the parser's duplicated clones), revision, kCanonRevision, CHANGELOG

## 4. Gates

- [ ] 4.1 Suite, IR snapshot without any exception, `tools/stack_frames.py` (the new walk added; nothing grows), fuzz 1 and 7, differential, backend_gaps, openspec, constitution_check, version_check
- [ ] 4.2 CI: sanitized suite (1 MiB), Windows, `windows-stack`, `ir-snapshot` (label `refactor`)
