## 0. Decisions

- [x] 0.1 F1–F7 approved (2026-10-12, "Tudo aprovado"), with I/O errors usable in try/except On/ensure
- [x] 0.2 Design approval (Risk: high) before code (2026-10-12, "100% aprovado!"); string-runtime merged first

## 1. Runtime

- [x] 1.1 runtime/FileRuntime.cpp: open, close, readln, eof, write, exists, delete, rename; error mapping on Linux and Windows; unit tests in inoxrt-tests

## 2. Compiler

- [x] 2.1 `File` type, non-copyable rule, `ref mut` passing; visible only with `Use`
- [x] 2.2 Intrinsics, stdlib/Std.Files.inox and stdlib/Std.IOUtils.inox
- [x] 2.3 Owned File slots closed at every exit and in the cleanup pad

## 3. Tests

- [x] 3.1 runtime files-text, files-close-on-exception, files-not-found, files-system, file-names-free; 13 diagnostics; leak check of the file programs

## 4. Documentation and gates

- [x] 4.1 Canon r3.59 (CANON-17 "Files", CANON-18, change log), manual, CHANGELOG, AGENTS.md
- [ ] 4.2 E19 gates; Linux and Windows CI green
