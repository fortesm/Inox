## 1. Baseline

- [x] 1.1 ir_snapshot recorded on main (633 programs)

## 2. Type module

- [x] 2.1 `types::Type`, `types::TypeContext` (interning, aliases, structs), `types::TypeSyntax`
- [x] 2.2 Compiler library and `inox-compiler-tests` (unit tests of the type module), run by both runners

## 3. Analyzer

- [x] 3.1 Analyzer works with `const types::Type*`; predicates on type objects; support/ArrayType and PointerType removed
- [x] 3.2 `SemanticResult` exports expression and declaration types, signatures and struct types

## 4. Code generator

- [x] 4.1 LLVM types by kind; struct layouts and signatures from the analysis
- [x] 4.2 Locals, parameters and expressions carry their Type

## 5. Statement cloning

- [x] 5.1 `ast::cloneStatement` with unit tests

## 6. Gates

- [x] 6.1 ir_snapshot identical; suite, ASan+UBSan, leak check, fuzz, backend_gaps, eh_state_balance, grammar, openspec, constitution, MSVC IR
- [ ] 6.2 AGENTS file map, STATUS (debt), CHANGELOG; PR; CI on Linux and Windows
