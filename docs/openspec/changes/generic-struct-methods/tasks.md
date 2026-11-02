Nothing below starts before the design's approval is recorded with its date.

## 0. Before the code

- [ ] 0.1 Approval of `design.md` recorded (who, date, answers to Q1-Q5)
- [ ] 0.2 IR snapshot of main recorded; `-fstack-usage` of main kept

## 1. Parser and AST

- [ ] 1.1 `Name<P...>.Method` header accepted (type parameters on the declaration); `Name<P...>.Method<U>` still rejected
- [ ] 1.2 `ast::methodDot`: the last dot outside angle brackets and after `::`; unit test on the corpus's names
- [ ] 1.3 `inox interface` prints `TPair<K, V>.Key(...)`

## 2. Analysis

- [ ] 2.1 `registerGenericTemplates` indexes generic methods; rule 7 checks and their diagnostics
- [ ] 2.2 `genericMethodFor`: lookup by the receiver's instance, out of line; used by method calls with and without arguments
- [ ] 2.3 `instantiateRoutine` takes the instance's name; a method instance is named `<instance type>.<method>`
- [ ] 2.4 Contracts: generic wrapper and body of a generic method (check, no new code expected)

## 3. Tests, example, documents

- [ ] 3.1 `generic-methods`, `generic-methods-contracts`, `tests/modules/generic-methods/`, diagnostics of the table
- [ ] 3.2 `examples/generics.inox` (Q5)
- [ ] 3.3 `language/generics` spec (the "not yet" requirement becomes the rule), the FUTURE line of the constitution (relock), site, STATUS, `docs/ARCHITECTURE.md`, revision, kCanonRevision, CHANGELOG

## 4. Gates

- [ ] 4.1 Suite, IR snapshot (only the declared differences, `ir-snapshot-expect.json`), `tools/stack_frames.py` (no growth), fuzz 1 and 7, differential, backend_gaps, openspec, constitution_check, version_check
- [ ] 4.2 CI: sanitized suite (1 MiB), Windows, `windows-stack`, `ir-snapshot` (label `refactor` not used: this is a feature; the snapshot is run locally and its result reported)
