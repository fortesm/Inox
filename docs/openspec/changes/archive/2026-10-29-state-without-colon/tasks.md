## 1. Parser

- [x] 1.1 `State` without `:`, closed by `;`; a section that reaches the next module item without `;` is reported (`atSectionBoundary`)
- [x] 1.2 Migration diagnostics: `State :`, a missing `;`, `Name : Type` in State and Const

## 2. Tests and sources

- [x] 2.1 All `.inox` files with `State :` migrated (19 files) and the three gap probes
- [x] 2.2 New: runtime `state-section-boundaries`; diagnostics `state-with-colon`, `state-without-semicolon`, `const-legacy-colon`; `state-legacy-scalar-without-initializer` becomes `state-legacy-colon`

## 3. Documentation

- [x] 3.1 Constitution CANON-4, CANON-5 State, "The idea of the syntax"; relock
- [x] 3.2 Grammar, manual, CHANGELOG, revisions.md r3.62

## 4. Gates

- [x] 4.1 Suite; ASan+UBSan with a 1 MiB stack; mutation fuzz seeds 1 and 7; backend_gaps 0 BUG; grammar_consistency; openspec validate; constitution_check
- [x] 4.2 Linux and Windows CI green (merged into main; main green on Linux and Windows, 2026-10-29)
