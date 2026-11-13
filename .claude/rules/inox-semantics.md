---
paths:
  - "src/compiler/semantic/**"
  - "src/compiler/types/**"
  - "src/compiler/tir/**"
---

# Inox semantic and typed IR boundaries

- Authority: CANON-3/-8/-9/-24/-25, E11/E14/E16 and `docs/ARCHITECTURE.md` phases 3–5b; use the topic's approved capability spec.
- Mutability, `Self mut`, `ref`/`ref mut`, `Own<T>`, `Link<T>`, Move/Free, invariant checks and exception cleanup must agree across all paths. Never infer missing behavior from Rust.
- Canonical assignments evaluate the right-hand side first, then the target once (CANON-5); preserve the exact path and owner-generation semantics.
- Existing method reuse is **part delegation**, not receiver rebinding. `docs/MICROCONTRACTS.md` is an unapproved exploration, not permission to implement it.
- Every type, symbol, call meaning, overload resolution and field identity needed by codegen must be recorded in `SemanticResult`/TIR. Codegen must receive those decisions, never reconstruct them.
- Add focused valid and invalid cases, plus TIR tests when representation changes; report gaps under E15, not compiler crashes.
