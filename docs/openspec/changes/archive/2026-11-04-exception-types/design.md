## Decisions

- **The hierarchy is the types'.** `types::Type` of kind Exception has
  `exceptionParent()` and `exceptionId()`; `TypeContext::declareException`
  makes one with its id, `setExceptionParent` links it once every standard
  type exists (the table lists children before parents), and
  `exceptionTypes()` gives them in declaration order.
- **The table stays the source of the standard types.**
  `exceptions/ExceptionTypes.h` keeps the names, ids and parents, which
  the analysis declares; libinoxrt finds the id of a fault by name there.
  Its name-based hierarchy helpers are removed.
- **The analysis records the resolved type** of a `RaiseStatement` and of
  an `ExceptionHandler` (`SemanticResult::exceptionType`), as it records
  declared symbols; phase 5b copies it into the TIR.
- **Code generation matches by type.** `On E` compares the raised id with
  the id of every declared exception type that is E or a descendant, in
  declaration order: the comparisons, and the IR, are those of the table
  walk it replaces.
- **Diagnostics unchanged:** messages keep the names as written.
