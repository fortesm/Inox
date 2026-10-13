## 0. Decisions

- [x] 0.1 F1–F7 approved (2026-10-12, "Tudo aprovado"), with I/O errors usable in try/except On/ensure
- [ ] 0.2 Design approval (Risk: high) before code; string-runtime merged first

## 1. Runtime

- [ ] 1.1 runtime/FileRuntime.cpp: open, close, readln, eof, write, exists, delete, rename; error mapping on Linux and Windows

## 2. Compiler

- [ ] 2.1 `File` type, non-copyable rule, `ref mut` passing
- [ ] 2.2 Intrinsics, stdlib/Std.Files.inox and stdlib/Std.IOUtils.inox
- [ ] 2.3 Owned File slots closed at every exit and in the cleanup pad

## 3. Tests

- [ ] 3.1 runtime files-text, files-close-on-exception, files-not-found, files-system; diagnostics file-copy

## 4. Documentation and gates

- [ ] 4.1 Canon (CANON-17/18, change log), manual, CHANGELOG
- [ ] 4.2 E19 gates; Linux and Windows CI green
