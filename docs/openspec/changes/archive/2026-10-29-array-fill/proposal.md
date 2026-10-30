## Why

Open question 2 of `revisions.md` (change `enum-range-set`, r3.80): a struct
field of an enum type without a default, and an array element of an enum
type, started at the first literal, an implicit value that the strict rule
for enum variables (CANON-5) forbids everywhere else. Answering it, the
maintainer chose the strict rule for every array, with a minimal operation
to fill one, and renamed the set functions `With`/`Without`, since `with`
is a reserved word.

Approval (Marcelo Fortes, 2026-10-18):

- On enums in aggregates: of the three original alternatives, the strict one is the final rule, with a simple mechanism to initialize arrays. The mechanism: a minimal uniform fill, inspired by Ada's `(others => value)` and Rust's `[value; size]`, without all array literals at once
- The law, in his words: every Inox array, whatever its element type, is initialized explicitly where it is declared; `Fill(Value)` initializes the whole target with the value, following the rules of types, copy and construction; there is no implicit initialization with zero, false or empty
- On Fill: `Fill(X)` is a contextual initialization expression whose type and dimensions come from the target array; `Fill(X)` produces a fully initialized array and never depends on an existing array without a defined value; the rule also works for multidimensional arrays and composite types, following the copy and construction rules of the elements
- On structs: the existing rule of struct field defaults stays separate, which keeps `P TPoint`.
- On the set functions: the function `With` is renamed; `Include(S, X)` and `Exclude(S, X)` replace `With`/`Without`. With a reservation: in Pascal and Delphi, Include and Exclude change the set they receive; in Inox they are pure functions returning a new value, and the contract says so. The contract is
  expressed by the compiler: a call that drops the new set is an error that
  shows `S := Include(S, X)` (proposed in the same conversation, not
  objected to).

Risk: normal (an initialization rule, a new expression lowered with the
existing element loop, and a rename; no exception lowering, runtime ABI or
memory ownership change).

Status: **implemented** (canon r3.81 and r3.82, one PR).

Approval of r3.82 (Marcelo Fortes, 2026-10-18), answering the question this
change first left open: both questions are closed by one principle: explicit initialization cannot be bypassed because a value sits inside a Struct, an Array or a generic type
For array fields: the field requires `:= Fill(...)` or the array is given in the construction; for type parameters: a field whose type is a type parameter T has no implicit default, whatever the concrete type of the instance; and nested structs are built from defaults only when every component can be; a missing required field is a compile error; no generic instance silently introduces a default the generic type does not guarantee. Decision: both corrections in one PR, with tests of nested structs and generic instances. His examples write `TBox<TStatus>(FValue := Pending)`; type
arguments are never written in an expression (r3.77), so the tests write
`A TBox<TStatus> := TBox(FValue := Pending)`.

## What Changes

- **Every local and State array is initialized where it is declared**,
  whatever its element type. `A Array[1..3] Integer` is "an array needs an
  initializer (CANON-5, r3.81): A Array[1..3] Integer := Fill(0)"; the
  message suggests the Fill for the element type (`Fill(Club)`,
  `Fill(False)`, `Fill("")`, `Fill([])`, `Fill(None)`).
- **`Fill(Value)`** is an array whose type and bounds come from its target:
  a declaration, a State initializer, an assignment, an argument or a
  returned value. Without one (`X := Fill(1)`) it is an error. Value is evaluated once and copied into
  every element: a Range element checks it ("value out of range"), a String
  element holds its own reference, a struct element is a copy of a struct
  value (`Fill(Origin)` with `Origin TPoint`, or `Fill(TPoint(...))`). In a
  multi-dimensional array Value is an element (`Fill(0)`) or a whole row
  (`Fill(Row)`). An array of owners is filled only with None. `Fill` is a
  predefined name (CANON-2), like IIf: it cannot be declared.
- **Initialization is transitive (r3.82).** A struct field of an enum,
  array or type-parameter type has no implicit value: it has a default
  (`FSuit TSuit := Club`, `FData Array[1..4] Byte := Fill(0)`, a literal of
  the innermost element, `None` or `[]`) or is given in the construction.
  A struct takes its type defaults (`P TPoint`, State, `New(TName)`) only
  when every component has one, nested structs and parts included;
  otherwise it is an error naming the field. A field whose type uses a type
  parameter (`FValue T`, `FItems Array[1..3] T`) never has a default, in
  any instance (`D TBox<Integer>` is an error). Scalar, String, Set and
  pointer fields without default keep their zero, "", empty and None.
- **`Include(S, X)` and `Exclude(S, X)`** replace `With` and `Without`.
  `With(S, X)` and `Without(S, X)` are rejected with the new names. The six
  set functions never change their arguments; calling one as a statement is
  an error ("Include returns a new set and does not change its argument;
  write S := Include(S, ...)").

Migrated: every program of the suite and the examples (arrays filled; three
test routines named `Fill` renamed).

Both questions this change first left open (array fields of structs, and
fields of a type parameter instantiated with an enum) are answered by r3.82.

## Capabilities

### Modified Capabilities
- `language/fixed-arrays`: arrays are initialized where they are declared;
  Fill.
- `language/enums`: enums in aggregates.
- `language/sets`: Include and Exclude; set functions as statements.
- `language/module-state`: State arrays are initialized.
- `language/structs`: type defaults only when complete; required fields.
- `language/generics`: a field of a type parameter has no default.

## Impact

- Lexer: `Fill` is a predefined name.
- Parser: `with(` in an expression is the old set function, rejected with
  the new name.
- Parser: an array field default `:= Fill(Literal)`.
- Analyzer: `SemanticArrays.cpp` (Fill, the array diagnostic); the
  initialization rule for locals and State; required fields
  (`requiredFieldWithoutDefault`, used by locals, State, New and
  construction); fields of a type parameter in generic instances; the set
  functions renamed and rejected as statements.
- Emitter: `FunctionEmitter::emitFill` (FunctionEmitterArrays.cpp) and
  `emitArrayFillDefault` (FunctionEmitterAggregates.cpp).
- Constitution (CANON-2, CANON-5, CANON-13), the four capability specs,
  grammar.ebnf, docs/site/index.html, CHANGELOG.md, docs/STATUS.md,
  `tools/string_leak_check.py` (array-fill tests).
