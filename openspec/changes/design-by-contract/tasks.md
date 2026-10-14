## 1. Decision

- [x] 1.1 Eiffel's documentation read; Ada/SPARK, C++26 and Java compared; LLVM primitives checked
- [x] 1.2 Marcelo asks for Eiffel's design with `pre` and `pos` (2026-10-14)

## 2. Compiler

- [x] 2.1 Parser: clauses of routines, structs and loops; `old`; contextual words
- [x] 2.2 Contract lowering pass: wrappers, old values, Result, Override inheritance, invariant methods
- [x] 2.3 Analyzer: `__check`, `__pure`, read-only contracts, invariant sites
- [x] 2.4 Codegen: checks with llvm.expect, contract trap, invariants with parts

## 3. Tests and documents

- [x] 3.1 Runtime tests (contracts, every violation), diagnostics, example
- [x] 3.2 Constitution CANON-19, CANON-24, FUTURE (relock); spec delta; grammar; manual; STATUS; CHANGELOG; ROADMAP; revisions r3.67

## 4. Monitoring levels

- [x] 4.1 Decision (r3.68): checks always on through 1.0; Eiffel-style monitoring levels per module are FUTURE, after 1.0
