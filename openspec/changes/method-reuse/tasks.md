## 1. Decisions

- [x] 1.1 Approaches compared (Go embedding, Rust traits, micro-contract, Object Pascal classes) and the ChatGPT discussion read
- [x] 1.2 Marcelo approves 1a, 2a, 3b, 4a+b (2026-10-13)

## 2. Compiler

- [x] 2.1 Parser: `TName Struct (TPart, ...)`, `Override` after a method signature
- [x] 2.2 Analyzer: parts as fields, offered members at every level, declaration errors, rewrite to the explicit path
- [x] 2.3 Methods of built-in types (`Self` copy, `Self mut` by reference), canonical names
- [x] 2.4 Mutability: read-only Self, value-parameter fields, `with` bindings, Self mut receivers, receiver aliasing
- [x] 2.5 Codegen: built-in receivers, method lookup by written or canonical type

## 3. Tests and documents

- [x] 3.1 Runtime: method-reuse, strings-method-reuse (leak-checked), builtin-type-methods; example method-reuse
- [x] 3.2 Diagnostics for every rule
- [x] 3.3 Constitution CANON-1, CANON-9, CANON-10, FUTURE (relock); spec delta language/structs; grammar; manual; STATUS; CHANGELOG; revisions r3.66
