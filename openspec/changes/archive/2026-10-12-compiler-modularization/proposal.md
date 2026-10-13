## Why

Three files hold most of the compiler: `codegen/LlvmIrEmitter.cpp` (5,154
lines, one class `FunctionEmitter` of ~3,700), `semantic/SemanticAnalyzer.cpp`
(3,352) and `parser/Parser.cpp` (2,168). No human or model reads 5,000 lines
at once; work happens by searching fragments, and coupling turns into bugs.
Two of the String-runtime bugs came from that coupling: a second, duplicated
call path that forgot `invoke`, and a driver that guessed from the IR text
whether the runtime library was needed.

Approval (Marcelo Fortes, 2026-10-12): "Aprovo a modularização antes dos
Arquivos, nesses moldes que você descreveu." The molds, as sent in chat:
modules by responsibility; a Strategy only for the exception ABI (Itanium /
MSVC); one table of the libinoxrt functions, from which declarations are
emitted and which tells the driver which runtime is needed; the analyzer
split into declarations/sections, scopes/statements, expressions/types,
builtins and constant evaluation; the parser left as it is for now; no
abstract factories, plugins or interface hierarchies; no new IR yet; pure
refactors proven by `tools/ir_snapshot.py` (identical IR, types,
diagnostics and exit codes for every program), in small pull requests.

Risk: high (rewrites the structure of two compiler phases). The design was
approved in chat before this change was written (quote above).

## What Changes

No Inox program changes behavior. Code moves into modules:
- Step 1 — codegen support: AST walkers, LLVM names and literals, type
  layout, runtime-fault IR and IR helpers leave `LlvmIrEmitter.cpp`. A stray
  `LlvmIrEmitter.cpp.diff`, tracked since an old commit, is deleted.
- Step 2 — `FunctionEmitter` gets a header and one source file per
  responsibility (statements, control flow, exceptions, expressions,
  places and arrays, I/O, String ownership).
- Step 3 — exception-ABI Strategy; runtime-ABI table; the emitter reports
  which runtime the program needs (the driver stops scanning IR text).
- Step 4 — `SemanticAnalyzer` split the same way.

Out of scope: the parser; a new intermediate representation; any language
change. Behavior that must not change: everything (`ir_snapshot compare`
with no allowed difference, except where a step lists one).

## Capabilities

### New Capabilities
(none)

### Modified Capabilities
(none)

## Impact

src/compiler/codegen/*, src/compiler/semantic/*, src/compiler/main/main.cpp,
CMakeLists.txt, canon (Layer B note on the source layout, change log),
AGENTS.md (where things live).
