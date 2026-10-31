## 1. Positions

- [x] 1.1 `ast::SourcePosition` on nodes and declarations; `ast::startOf`
- [x] 1.2 Parser stamps leaves, statements (and their desugaring), items, declarations; recursive frames unchanged (`-fstack-usage`)
- [x] 1.3 `AstClone` keeps positions; contract checks start at their clause

## 2. Diagnostics

- [x] 2.1 `SemanticError` position and module; analyzer `notePosition` and one attach point
- [x] 2.2 Module resolution and contract lowering attach their item
- [x] 2.3 Driver prints `path:line:column: kind: message`

## 3. Tests

- [x] 3.1 New: `position-statement`, `position-generic-body`, `position-declaration`, `position-parse`, `position-contract` (tests/diagnostics), `position-in-used-module` (tests/modules); `ambiguous-import` checks its position
- [x] 3.2 The ten parse `.err` files follow the new prefix

## 4. Documents

- [x] 4.1 `docs/ARCHITECTURE.md` (Diagnostics; D1 closed), revision r3.112, kCanonRevision, CHANGELOG

## 5. Gates (E19)

- [x] 5.1 Suite, ASan+UBSan with a 1 MiB stack, fuzz seeds 1 and 7, backend_gaps, eh_state_balance, string_leak_check, grammar_consistency, openspec, constitution_check, version_check; IR snapshot against main

Results (2026-10-31, unidentified cloud container, no .inox-machine):
suite 1078/1078, also under ASan+UBSan (g++) with a 1 MiB stack; fuzz seed
1 OK=269 REJECTED=3731, seed 7 OK=292 REJECTED=3708, no crash; backend_gaps
OK=54; eh_state_balance 2/2; string_leak_check 62/62; grammar_consistency
5/5. IR snapshot against main: of 1047 programs, the IR, the dumped types
and the exit code of every one are identical; 1122 diagnostic outputs
changed, each only by the new `path:line:column:` prefix (checked by
rewriting it back to the old form); one program added. Stack needed by the
boundary tests (Linux Debug): runtime 899 KiB on main, 896 KiB here; parser
896 KiB on both.

Windows: the first CI run overflowed the 1 MiB stack in the boundary tests
(Windows Debug), although every recursive frame was unchanged on Linux.
The frames below the recursion had grown: `main` (+496 bytes, the new
printing code in its catch blocks), `parseSource` (+16), and
`SemanticAnalyzer::analyze` (+64 with a try, +72 with a wrapper). The
printing moved to `printParseError`/`printSemanticError`, the file is noted
before parsing, and the analyzer no longer catches: a `SemanticError` takes
the place the analyzer keeps in `diagnosticPlace()` when it is made. Every
frame on the path is now at or below its size on main (`-fstack-usage`);
`docs/ARCHITECTURE.md` records the stack budget rule.
