## 1. Step 1 — codegen support modules

- [x] 1.1 `tools/ir_snapshot.py record` with the compiler before the step (491 programs)
- [x] 1.2 AstQueries, LlvmText, TypeLayout, Operators, MathBuiltins, ExceptionSupport, RuntimeHelpersIr extracted; stray .diff deleted
- [x] 1.3 `ir_snapshot compare`: no difference (491 programs)

## 2. Step 2 — FunctionEmitter split by responsibility

- [x] 2.1 FunctionEmitter.h and FunctionEmitter*.cpp; `ir_snapshot compare`: no difference (491 programs)

## 3. Step 3 — exception ABI Strategy, runtime ABI table

- [x] 3.1 ExceptionAbi (Itanium, MSVC); RuntimeAbi table; `LlvmModule::needsInoxRuntime` reported to the driver; `ir_snapshot compare`: no difference (491 programs); MSVC-ABI IR compared with the step 2 compiler forced to the Windows ABI: no difference (491 programs)

## 4. Step 4 — SemanticAnalyzer split

- [x] 4.1 Declarations, statements, expressions, builtins, constant evaluator (plus calls, types and SemanticSupport); every code line of the old file kept exactly once (checked); `ir_snapshot compare`: no difference (491 programs)

## 5. Gates (E19) and documentation, every step

- [x] 5.1 Suite (CI preset, libstdc++ 14), ASan+UBSan with a 1 MiB stack, mutation fuzz seeds 1 and 7, backend_gaps 0 BUG, eh_state_balance, string_leak_check, grammar_consistency, openspec validate, canon_trace
- [ ] 5.2 Linux and Windows CI green
- [x] 5.3 AGENTS.md (where things live), CHANGELOG.md. No canon entry: no language rule or implementation status changes (refactor only)
