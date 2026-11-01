## 1. AST and parser

- [x] 1.1 `ast::RoutineHeader`, `ParameterDeclaration`; `signatureTokens` removed
- [x] 1.2 `Parser::parseRoutineHeader`: header syntax errors as parse errors at the token
- [x] 1.3 Unit test `RoutineHeaderTests`; diagnostics tests `routine-header-*`

## 2. Readers

- [x] 2.1 Analyzer `registerFunctionSignatureIn`: semantic checks only
- [x] 2.2 Contract lowering, generics (instances substitute types out of line), module resolution, `inox interface`

## 3. Law and documents

- [x] 3.1 `language/ref-parameters` requirement, `docs/ARCHITECTURE.md` (D3 closed), site, revision r3.114, kCanonRevision, CHANGELOG

## 4. Gates

- [x] 4.1 Suite; IR snapshot against main (only `mut-parameter-reserved` differs, as intended); `-fstack-usage` against main (no frame on the recursion path grows: `parseFunctionDeclaration` 1480→1240, `instantiateRoutine` 1288→1192, `registerFunctionSignatureIn` 3096→2056 bytes)
- [x] 4.2 Fuzz seeds 1 and 7, differential seeds 1 and 7, backend_gaps, openspec, constitution_check, version_check
- [x] 4.3 Gates job `ir-snapshot` reads `ir-snapshot-allow.txt` of the change (intended differences); `docs/BUILD.md`
- [x] 4.4 CI: sanitized suite, Windows suite, `windows-stack`, `ir-snapshot` (label `refactor`)
