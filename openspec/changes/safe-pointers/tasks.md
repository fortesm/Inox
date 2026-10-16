## 1. Decision

- [x] 1.1 Design discussed and decided (r3.71, r3.72)
- [x] 1.2 Design approved (Marcelo Fortes, 2026-10-16: "Tudo aprovado, prossiga como o usual")

## 2. Runtime

- [x] 2.1 libinoxrt owned blocks with a 64-bit generation, free lists never returned, queued (non-recursive) frees; unit tests

## 3. Owners

- [ ] 3.1 Types `Own<T>`, `Option<Own<T>>`; parser; never copied; `New`, `Free`, `Move`, `Value`; auto-deref; `None`
- [ ] 3.2 Move analysis (use after Move or Free is a compile error) with a run-time backstop (trap 17)
- [ ] 3.3 Owners released at every exit and on exceptions; drop functions per type
- [ ] 3.4 Runtime, trap and diagnostic tests; leak check

## 4. Links

- [ ] 4.1 `Link<T>`, `Link(X)` of owners, variables, fields and elements; tombstones for variables
- [ ] 4.2 Generation check (trap 16), `Alive`, `Same`, empty Link
- [ ] 4.3 Runtime, trap and diagnostic tests

## 5. Law and documents

- [ ] 5.1 Constitution (FUTURE Ownership becomes law; CANON-19 kinds), spec delta, grammar
- [ ] 5.2 Manual, STATUS, CHANGELOG, ROADMAP, revisions, AGENTS map; examples

## 6. Gates

- [ ] 6.1 Suite, ASan+UBSan, fuzz seeds 1 and 7, leak check, backend_gaps, eh_state_balance, grammar, openspec, constitution, MSVC IR
- [ ] 6.2 Linux and Windows CI green
