## 1. Exception types

- [x] 1.1 `types::Type` holds an exception type's parent and id; `isExceptionOf`; `TypeContext::declareException`, `setExceptionParent`, `exceptionTypes`
- [x] 1.2 The analysis declares the standard types with their hierarchy, resolves `Raise` and `On` types, checks handlers by type, records the types (`SemanticResult::exceptionType`)
- [x] 1.3 The TIR carries `Stmt::type` (Raise) and `Handler::type`; verified and printed by name
- [x] 1.4 Codegen takes ids from the types; `exceptionTypeId` and the name-based helpers of `ExceptionTypes.h` removed
- [x] 1.5 D6 closed in `docs/ARCHITECTURE.md`; STATUS, CHANGELOG, revision line

## 2. Gates

- [x] 2.1 Suite (Debug, Release, ASan+UBSan), LLVM IR and typed IR snapshots identical, `tools/stack_frames.py`, fuzz 1 and 7, differential, backend_gaps, owner_safety_check, openspec, constitution_check, version_check
- [ ] 2.2 CI (label `refactor`)
