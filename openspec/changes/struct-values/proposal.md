## Why

Structs are the main data type of a post-object-oriented language, but the
backend lowers only the simplest case: a local `P TPoint` whose fields are
Integer or Bool. Named construction `TPoint(FX := 1)`, `Q TPoint := Expr`,
nested structs, other scalar field types and arrays of struct are legal Inox
(CANON-5, CANON-9, CANON-13) and stop at code generation or semantic analysis
today. This is item 1 of the order approved on 2026-10-12.

Approval (Marcelo Fortes, 2026-10-12): "Todas as tomadas de decisão dos pontos
1 a 5 foram verificadas e eleitas para serem acatadas." Item 1: struct
initializers and arrays of struct.

## What Changes

- Lower struct initializers: `Q TPoint := Expr` and grouped `P, Q TPoint := Expr`
  (closes the `grouped-decl-struct` GAP).
- Lower named construction `TPoint(FX := 1, FY := 2)` as an expression usable
  anywhere a value is expected (closes the `named-struct-construction` GAP).
- Struct fields of every lowered scalar type: the Integer family, Natural,
  Float, Float32 and Bool, with literal defaults for each.
- Nested struct fields (`FA TPoint` inside `TLine`), in any declaration order
  inside the `Type` section; a struct that contains itself is a compile error.
- Arrays whose element type is a struct: `A Array[1..3] TPoint`, `A[I].FX := 5`,
  arrays of struct as parameters and results.
- Struct values in `ref` / `ref mut` parameters keep working for the new field
  types (ADR-0016).

No Layer A rule changes: CANON-9 already defines named construction, defaults,
copy semantics and the errors; this change implements them.

Out of scope (later items): arrays as struct fields and struct State (item 2),
String fields and String elements (item 5), Char fields (Char output is not
lowered yet), struct comparison (`=`/`#` on structs stays unspecified; no
change), positional construction (forbidden by CANON-9).

Behavior that must not change: `P TPoint` without initializer still uses field
defaults then zero; struct assignment, parameters and returns copy (value type);
all CANON-9 construction diagnostics stay as they are.

## Capabilities

### New Capabilities
- `language/structs`: struct values — declaration with defaults, named
  construction, initializers, nesting, copy semantics.

### Modified Capabilities
- `language/fixed-arrays`: arrays may have struct elements.

## Impact

- `src/compiler/codegen/LlvmIrEmitter.cpp`: struct layout collection (all
  scalar field types, nested structs in dependency order), construction
  expression, struct initializers, element GEP through arrays of struct.
- `src/compiler/semantic/SemanticAnalyzer.cpp`: accept struct array elements;
  recursive-struct check; defaults for every scalar type.
- Tests: runtime, diagnostics, backend_gaps probes; docs/BACKEND_GAPS.md,
  canon Layer B (CANON-20 backend matrix), docs/site/index.html, CHANGELOG.md.
- Canon CANON-9 gains a pointer to `openspec/specs/language/structs` when the
  change is archived.
