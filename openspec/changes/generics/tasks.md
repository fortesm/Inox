## 1. Approval

- [x] 1.1 Decisions answered by Marcelo Fortes (2026-10-17), quoted in proposal.md

## 2. Front end

- [x] 2.1 Parser: type parameters after a routine or struct name; generic types in type position; written type arguments in an expression rejected
- [x] 2.2 `TypeSyntax`: several arguments, generic structs of the program, `substituteTypeNames`

## 3. Analyzer (SemanticGenerics.cpp)

- [x] 3.1 Templates, inference from arguments and target, instances of routines and structs, checked after the program's routines
- [x] 3.2 Messages that name the instance; limits on instances and nesting

## 4. Code generator

- [x] 4.1 Instances compiled like routines of the program; struct instances; calls and constructions from the analysis

## 5. Tests and documents

- [x] 5.1 Runtime tests, diagnostics, a module test, unit tests, an example, the leak check
- [x] 5.2 Constitution (CANON-8, FUTURE), capability spec, revision r3.77, site, CHANGELOG, STATUS, AGENTS, grammar
- [x] 5.3 Gates: suite, ASan+UBSan, fuzz, backend_gaps, eh_state_balance, leak check, grammar, openspec, constitution, MSVC IR
