## Why

Deviation D6 of `docs/ARCHITECTURE.md`: exception handlers match the
written type name, and the hierarchy of the exception types is not in
`types::Type`. The analysis tests `On` handlers and `Raise` by name
(`exceptions::isSubtypeOf`), the typed IR carries the names as written,
and code generation finds a type's id and the ids an `On E` catches by
name (`exceptionTypeId`, `matchingTypeIds`). F7 is the last foundation
step, after F6 (ROADMAP, "Foundation").

Approval: Marcelo Fortes, 2026-11-04 (F7 after F6, in the design and
scope of the foundation; one pull request). Risk: medium; the lowering
of exceptions, the runtime ABI and the ids do not change.

## What Changes

- An exception type of `types::Type` holds its parent and its runtime id;
  `Type::isExceptionOf(ancestor)` is the hierarchy. The analysis declares
  the standard exception types from `exceptions/ExceptionTypes.h` and
  links their parents.
- The analysis resolves the type of each `Raise E` and `On E` handler and
  records it; the checks of handlers (unknown, duplicate, unreachable,
  `On Exception` last) compare types.
- The typed IR carries the type: `Stmt::type` of a Raise, `Handler::type`.
- Code generation takes the id from the type and the ids an `On E`
  catches from the declared exception types; it reads no exception name.
- No program changes: the language, the diagnostics, the LLVM IR and the
  typed IR of every program are identical.

## Impact

- Compiler: `types/`, `semantic/` (builtins, statements, result),
  `tir/`, `codegen/` (exceptions). libinoxrt keeps finding an id by name
  in the table.
- Closes D6; `docs/ARCHITECTURE.md`, `docs/STATUS.md`, CHANGELOG, a
  revision line. User-declared exception types remain out of scope (no
  rule of the law yet).
