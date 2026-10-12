## 1. Process

- [x] 1.1 Known pitfalls section in AGENTS.md
- [x] 1.2 Risk levels and design approval for high-risk changes (config.yaml, AGENTS.md); mark string-runtime high-risk
- [x] 1.3 Archive changes right after merge (config.yaml)

## 2. Tooling

- [x] 2.1 tools/ir_snapshot.py record/compare with --allow; INOX_STDLIB set
- [x] 2.2 Validated by replaying the item 2 check (recorded with the pre-v3.53 compiler, compared with v3.53: the three intended differences plus the new tests)

## 3. Documentation

- [x] 3.1 Canon v3.54: E19 refactor gate; no type alias form
- [x] 3.2 CHANGELOG.md

## 4. Gates

- [x] 4.1 openspec validate, canon_trace, grammar_consistency
- [ ] 4.2 Linux and Windows CI green
