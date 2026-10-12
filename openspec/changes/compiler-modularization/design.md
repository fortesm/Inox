## Context

`FunctionEmitter` emits one routine; it owns the output stream, temporary
and label counters, loop and cleanup stacks, the String owned slots, and the
lowering of every statement and expression. The code before it (lines
30–890) is free functions and plain structs: AST walkers, naming and
literal helpers, type layout, signatures; the code after it emits the
module (globals, declarations, runtime helpers, `main`).

## Decisions

- **Step 1 layout (codegen support).**
  - `codegen/AstQueries.{h,cpp}`: AST walkers (`forEachStatement`, ...) and
    the "does this use try / exceptions" queries.
  - `codegen/LlvmText.{h,cpp}`: names, literals, string constants, case-
    insensitive comparison.
  - `codegen/TypeLayout.{h,cpp}`: scalar and aggregate LLVM types, arrays,
    structs, function signatures, integer ranges.
  - `codegen/Operators.{h,cpp}`: the LLVM instruction or predicate of each
    operator (the former static members of `FunctionEmitter`).
  - `codegen/MathBuiltins.{h,cpp}`: Std.Math intrinsic and libm names.
  - `codegen/ExceptionSupport.{h,cpp}`: exception type ids and the ABI
    choice (grows into the ExceptionAbi strategy in step 3).
  - `codegen/RuntimeHelpersIr.{h,cpp}`: the IR text of the internal helpers
    (`__inox_arith_fault`, checked arithmetic, input helpers) and the
    runtime-fault table.
  - `LlvmIrEmitter.cpp` keeps `FunctionEmitter` (until step 2) and the module
    emission.
  - Everything stays in `namespace inox::compiler::codegen`; the helpers
    move out of the anonymous namespace into a `detail` namespace, declared
    in the headers.
- **Step 2.** `codegen/FunctionEmitter.h` declares the class; its member
  functions are defined in `FunctionEmitter*.cpp` files by responsibility.
  The class is still one class: this step only makes it readable.
- **Step 3.** `ExceptionAbi` interface with `ItaniumExceptionAbi` and
  `MsvcExceptionAbi`, chosen once per module (it replaces the
  `usesMsvcExceptionAbi()` tests scattered in the emitter). `RuntimeAbi`
  table: name, LLVM declaration, attributes; `requireRuntime(name)` emits the
  declaration once and records the need; `LlvmIrEmitter::emit` returns the
  IR and a `RuntimeNeeds` set to the driver.
- **Step 4.** `SemanticAnalyzer` keeps its public interface; its definitions
  move to `SemanticDeclarations.cpp`, `SemanticStatements.cpp`,
  `SemanticExpressions.cpp`, `SemanticBuiltins.cpp`, `ConstantEvaluator.cpp`.

## Verification

Each step: `tools/ir_snapshot.py record` with the compiler before the step,
`compare` after it, with no allowed difference; the full E19 gates; both CI
jobs. A step that cannot keep the IR identical is wrong, not the snapshot.

## Risks / Trade-offs

- Moving code can change the order of emitted declarations or numbering of
  temporaries; the snapshot catches it.
- Free functions leaving the anonymous namespace lose internal linkage; a
  `detail` namespace keeps them out of the public API.

## Design approval

Approved by Marcelo Fortes on 2026-10-12: "Aprovo a modularização antes dos
Arquivos, nesses moldes que você descreveu."
