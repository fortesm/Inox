## 1. Invariants of generic structs

- [x] 1.1 The parser accepts `invariant` lines in a generic struct (parts stay rejected)
- [x] 1.2 Contract lowering makes `TName<T>.__invariant` a generic method; each struct instance gets its instance (`instantiateInvariant`)
- [x] 1.3 Tests: `generic-struct-invariants`, `generic-struct-invariant-construction`, `tests/diagnostics/generics-invariant-instance.inox`
- [x] 1.4 Specs (`language/generics`), site, STATUS, CHANGELOG, revision line

## 2. Gates

- [x] 2.1 Suite (Debug, Release, ASan+UBSan), snapshots against main (only added programs), stack_frames, fuzz, differential, backend_gaps, owner_safety_check, openspec, constitution_check, version_check
- [ ] 2.2 CI
