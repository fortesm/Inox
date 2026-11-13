---
paths:
  - "src/compiler/codegen/**"
  - "src/runtime/**"
---

# Inox textual LLVM backend and runtime

- Authority: E11/E12/E15/E16/E19/E22 and CANON-19/-23/-25; consult `docs/ARCHITECTURE.md` phases 6–8. Inox emits textual LLVM IR and calls **external clang**. Do NOT introduce `llvm::verifyModule`, libLLVM API usage or a new backend architecture by analogy.
- Codegen consumes TIR only. No AST reads, textual type inference, name resolution, rechecking of semantic rules, or missing decision silently defaulted.
- Use one statement dispatcher for every block context. Never rely on unordered-map iteration for LLVM IR emission; deterministic output across platforms is required.
- Observe checked arithmetic and applicable CANON-23 restrictions; no unapproved `nsw`/`nuw` changes. Use `FunctionEmitter::emitRuntimeCall` for throwing runtime calls and preserve Itanium/MSVC exception-state balance.
- `libinoxrt` owns console I/O, `StringBlock` allocation and freeing. Generated code must not directly call `printf`/`malloc`/`free` or pass incompatible C runtime ownership across Windows ABI boundaries.
- Reproduce bugs with runnable/IR/trap cases; verify with external clang, backend_gaps, exception/string/owner checks and Linux/Windows CI evidence as applicable.
