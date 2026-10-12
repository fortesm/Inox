## 1. Step 1 — codegen support modules

- [x] 1.1 `tools/ir_snapshot.py record` with the compiler before the step (491 programs)
- [x] 1.2 AstQueries, LlvmText, TypeLayout, Operators, MathBuiltins, ExceptionSupport, RuntimeHelpersIr extracted; stray .diff deleted
- [x] 1.3 `ir_snapshot compare`: no difference (491 programs)

## 2. Step 2 — FunctionEmitter split by responsibility

- [ ] 2.1 FunctionEmitter.h and FunctionEmitter*.cpp; `ir_snapshot compare`: no difference

## 3. Step 3 — exception ABI Strategy, runtime ABI table

- [ ] 3.1 ExceptionAbi (Itanium, MSVC); RuntimeAbi table; RuntimeNeeds reported to the driver; `ir_snapshot compare`: no difference

## 4. Step 4 — SemanticAnalyzer split

- [ ] 4.1 Declarations, statements, expressions, builtins, constant evaluator; `ir_snapshot compare`: no difference

## 5. Gates (E19) and documentation, every step

- [ ] 5.1 Suite (CI preset, libstdc++ 14), ASan+UBSan with a 1 MiB stack, mutation fuzz seeds 1 and 7, backend_gaps 0 BUG, eh_state_balance, string_leak_check, grammar_consistency, openspec validate, canon_trace
- [ ] 5.2 Linux and Windows CI green
- [ ] 5.3 Canon (Layer B source layout, change log), AGENTS.md (where things live), CHANGELOG.md
