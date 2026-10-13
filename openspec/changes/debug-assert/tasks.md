## 1. Assert

- [x] 1.1 Semantic: Assert visible with `Use Std.Debug`; one or two arguments, Bool and String (`SemanticDebug.cpp`)
- [x] 1.2 Codegen: branch to fault kind 14 or to `__inox_assert_message` (`FunctionEmitterDebug.cpp`, `RuntimeHelpersIr`)
- [x] 1.3 Tests: runtime `debug-assert-holds`, `assert-name-free`; traps `debug-assert-fails` (not catchable, ensure does not run), `debug-assert-message`; diagnostics `assert-without-use`, `assert-condition-bool`

## 2. Documentation and gates

- [x] 2.1 Constitution CANON-18, CANON-19; relock; spec `stdlib/debug`; Std.Debug.inox; manual; STATUS; CHANGELOG
- [x] 2.2 Suite; ASan+UBSan with a 1 MiB stack; mutation fuzz seeds 1 and 7; backend_gaps 0 BUG; openspec validate; constitution_check
- [ ] 2.3 Linux and Windows CI green
