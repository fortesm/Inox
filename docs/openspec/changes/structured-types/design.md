Design of foundation step F4 (change `structured-types`). Risk: high.
Status: **awaiting the maintainer's final approval**. Q1-Q4 answered
2026-11-01 (see "Approval"); the three conditions of that review are
written into this design (invariants of `TypeExpr`, canonical spelling,
type parameters inside expressions, stack proof). No code of F4 before the
final approval and the merge of `ir-snapshot-exact`.

## Today (D4)

The parser already reads a written type into structure, then throws the
structure away:

```text
parser:   tokens ─▶ parseTypeName ─▶ "Array[1..N] TPair<String, Int64>"   (text in the AST)
module:   rewriteText(text)            renames words inside the string
          privateTypeIn(text)          scans words for private types
lowering: isGenericSpelling(text, "Own"), readTypeSyntax(text)
analyzer: readTypeSyntax(text) ─▶ types::TypeSyntax ─▶ resolveTypeSyntax ─▶ TypeRef
generics: substituteTypeNames(text, T ↦ Int64)   on headers, fields, locals
          readTypeSyntax(text) to unify a parameter's written type
clone:    CloneRewriters::typeName (text ↦ text)
interface: prints the text
codegen:  only asks whether a local had a written type
```

Text appears in: `StructFieldDeclaration::typeName`,
`VariableDeclaration::typeName` (Const, State), `ParameterDeclaration::typeName`
and `RoutineHeader::resultTypeName` (F3), `VarStatement::typeName` (typed
locals, also from `A, B T` lines), and `ExceptionHandler::typeName` (out of
scope, F7). About 70 reads of these fields, about 20 shape tests on text, 11
calls of `readTypeSyntax`, 6 of `substituteTypeNames`.

## The written type in the AST

```cpp
namespace inox::compiler::ast {

// One array bound as written: an integer literal (with its sign) or a
// Const name (CANON-13). Module resolution renames a name.
struct ArrayBound {
    std::string text;
    bool isName = false;
};

// The written form of a type (F4, r3.NNN): what the program wrote, not what
// it means. The analyzer resolves it to a types::Type.
struct TypeExpr {
    enum class Form { Named, Array, Generic };

    Form form = Form::Named;
    std::string name;               // Named: the name, possibly qualified
                                    // (`Bank.Accounts.TAccount`); Generic:
                                    // Slice, Own, Link, Option, Set or a
                                    // generic struct (`TPair`)
    ArrayBound low;                 // Array only
    ArrayBound high;
    std::vector<TypeExpr> arguments;  // Array: the element; Generic: the
                                      // type arguments, in order
    SourcePosition position;        // where the type starts

    const TypeExpr& element() const { return arguments.front(); }
};

// The canonical text: `Array[1..3] Int64`, `TPair<String, Int64>`. The only
// way back to text; used by messages, `inox interface` and the names of
// generic instances. Identical to today's spelling, byte for byte.
std::string spell(const TypeExpr& type);

}
```

- **Value type, deep copy.** A `TypeExpr` is copied with the node that
  holds it (contract wrappers, generic instances, clones); nothing shares a
  tree, so a rewrite never changes another declaration. `std::vector` of an
  incomplete element type is valid since C++17.
- **Optional where a type may be absent:** `std::optional<TypeExpr>` for an
  inferred Const (`Name := Expr`), an untyped local and a procedure's
  result. A receiver `Self` keeps no type (F3).
- **`ast` still does not know `types`.** The tree moves from
  `types/TypeSyntax.*` into `ast/TypeExpr.*`; `types/` loses its only
  text reader and keeps only meanings.
- **Multi-dimensional arrays** stay nested, as the parser spells them
  today: `Array[1..2, 1..3] T` is an Array whose element is an Array.

### Invariants of `TypeExpr`

Every `TypeExpr` in the AST satisfies these, from the parser on; a phase
that builds or rewrites one keeps them, and a debug check
(`ast::checkTypeExpr`, asserting in Debug builds) verifies them where a
tree is made or changed (parser, substitution, renaming, clone).

1. **Named:** `name` is not empty; it is one identifier or a qualified name
   (`A.B.TName`, identifiers joined by single dots); `low` and `high` are
   empty; `arguments` is empty.
2. **Array:** `name` is empty; `low` and `high` are not empty, each an
   integer literal (with an optional leading `-`) or one identifier
   (`isName`); `arguments` has exactly one element, the element type.
   A multi-dimensional array is an Array whose element is an Array, one
   level per dimension, in the written order.
3. **Generic:** `name` is not empty; `arguments` has one element or more;
   `low` and `high` are empty. Slice, Own, Link and Option have exactly
   one argument (the parser requires it, as today); the count of a generic
   struct's or a Set's arguments is the analyzer's check, with today's
   messages.
4. **Position:** every node has a known position (`line != 0`), its first
   token; a node made by substitution takes the position of the node it
   replaces, so a message about an instance still points at the generic
   body (F2).
5. **Ownership:** a tree is a value; no node is shared between two trees,
   and no phase keeps a pointer into a tree it does not own beyond one
   call.
6. **Spelling round trip:** `spell(t)` is the canonical text below, and
   reading that text gives back a tree equal to `t` except for positions.

### Canonical spelling

`spell` must give, byte for byte, the text the parser makes today; every
message, `inox interface` line and instance name depends on it. The rules
(the current `parseTypeName`, `spellArrayType` and `TypeSyntax::spelling`):

| Written | Spelled |
|---|---|
| `Integer`, `TPoint` | as written (the name's own capitals) |
| `Bank.Accounts.TAccount` | as written, dots without spaces |
| `Array[1..3] Int64` | `Array[1..3] Int64` (no spaces inside the brackets) |
| `Array[ -1 .. N ] T` | `Array[-1..N] T` |
| `Array[1..2, 1..3] T` | `Array[1..2] Array[1..3] T` (nested, one per dimension) |
| `Array[$10..$1F] T`, `Array[1_000..1_009] T` | the literal as `tokenText` gives it (`$10`, `1000`) |
| `slice<int8>`, `own<T>` | `Slice<int8>`, `Own<T>`: Slice, Own, Link and Option get their canonical capitals; the argument as written |
| `TPair<String,Own<TNode>>` | `TPair<String, Own<TNode>>` (`, ` between arguments, no other spaces) |
| `Option<Own<Array[1..2, 1..3] TItem>>` | `Option<Own<Array[1..2] Array[1..3] TItem>>` |
| `Set<TColor>` | `Set<TColor>` |

Step 1 proves it in two ways: a unit test with each row above plus deep
nestings (generic inside array inside generic, three dimensions, qualified
names as arguments), and a corpus test that parses every written type of
`tests/`, `examples/` and `stdlib/` with the new parser and compares
`spell` with the text today's parser produces for the same source.

## Phase by phase

**2 Parser.** `parseTypeName` returns a `TypeExpr` and stamps its position.
The errors it raises keep their messages and places. `spellArrayType`
leaves the parser.

**3 Module resolution.** A walk of the tree replaces `rewriteText` for
types: each Named name and each bound that is a name is resolved as a
qualified or bare name, exactly as `rewriteText` resolves a word today.
`privateTypeIn` walks the tree. The name collector reads names from the
tree.

**4 Contract lowering.** `movesOnly` and the `old` copies test the tree's
form (`Own`, `Option`, Array) and copy the parameter's `TypeExpr` into the
local they declare.

**5 Semantic analysis.**
- `resolveWrittenType(const TypeExpr&)` takes the tree; the current
  `resolveTypeSyntax` family switches from `types::TypeSyntax` to
  `ast::TypeExpr` without changing its rules or messages (a message that
  quotes the written type uses `spell`).
- The shape tests on text become tests of `form` and `name`.
- **Generics on the tree.** `substitute(TypeExpr, T ↦ type)` replaces a Named
  node that is a type parameter by the written form of its argument (the
  argument's resolved type turned back into a `TypeExpr` by one function,
  `writtenFormOf(TypeRef)`, which today's code does through
  `Type::name()` text). It applies to the header, the fields of a generic
  struct and the typed locals of the body (the clone's rewriter becomes
  `TypeExpr ↦ TypeExpr`). Unification of a parameter's written type with an
  argument's type walks the tree it already walks after `readTypeSyntax`.
- The names of instances (`Max<Array[1..3] Int64>`) keep their text: they
  come from `Type::name()`, unchanged.
- **Type parameters inside expressions are kept.** In a generic body,
  `New(T)` and `T(X + X)` name a type parameter as an identifier in an
  expression, not as a written type. Today the instance's clone replaces
  such an identifier by one naming the argument's type (the clone's
  expression rewriter in `instantiateRoutine`). F4 leaves that rewriter as
  it is and changes only the written-type rewriter beside it; both run on
  every clone of a body. Guard: `tests/runtime/generic-type-parameter-expressions.inox`
  (`New(T)` with a struct, `T(...)` with Int8 and Float), added before F4 so
  the snapshot covers it.

**6 Codegen.** Tests `has_value()` where it tests `empty()`. Nothing else.

**AST services.** `AstClone` copies trees; `inox interface` prints
`spell(...)`; `RoutineHeader` holds `TypeExpr`.

## Order of work: steps that each pass every gate

0. Before the code: `ir-snapshot-exact` merged; the snapshot and the
   `-fstack-usage` of main recorded; `tools/stack_frames.py`.
1. `ast::TypeExpr`, `spell`, `checkTypeExpr`; the unit test of the
   canonical table and the corpus test (above).
2. The parser fills `TypeExpr` beside the text (both kept, the text made by
   `spell`): consumers still read text. IR snapshot: no difference.
3. The analyzer reads the tree (resolution, shape tests).
4. Generics on the tree (headers, generic struct fields, locals, clone).
5. Module resolution and contract lowering on the tree; interface.
6. The text fields and the text path are deleted (`readTypeSyntax`,
   `substituteTypeNames`, `spellArrayType`, `isArraySpelling`,
   `isGenericSpelling`, `TypeNameRewriter`, `types/TypeSyntax.*`); D4 closed.

Each step: suite, IR snapshot against the snapshot recorded on main before
step 1 (no allow list: F4 changes no output), `-fstack-usage` against main,
sanitized suite with 1 MiB, fuzz, differential, and the Windows jobs.

## Stack budget

Windows Debug needs 963 KiB for `limits-boundary-ok` (1 MiB was the old
default; the reserve is 8 MiB since r3.113, the job fails above 4096 KiB or
+5%). The rule for F4 is stricter than the job: **no function on a
recursion path may have a larger frame than on main**, at every step.

**The recursion paths** (frames measured on main with clang Debug
`-fstack-usage`, bytes):

| Path | Functions F4 touches or may touch | Frame on main |
|---|---|---|
| Parser, statements | `parseStatement` | 808 |
| | `parseTypedLocalStatement` (typed locals: builds a type) | 488 |
| | `parseExpressionStatement`, `parseIfStatement`, `parseForInStatement`, `parseCaseStatement`, `parseTryStatement` | 840, 456, 456, 504, 632 |
| Parser, expressions | `parsePostfix`, `parsePrimary`, `parseUnary` | 904, 712, 328 |
| Parser, types | `parseTypeName` (recursive on nested types) | 1064 |
| Analyzer, statements | `analyzeStatement` (typed locals: resolves a type) | 5448 |
| Analyzer, expressions | `analyzeBinaryExpression`, `analyzeCallExpression`, `analyzeGenericCall`, `analyzeConstruction`, `analyzeGenericConstruction` | 4216, 2776, 1800, 3064, 1448 |
| Analyzer, types | `resolveWrittenType`, `resolveTypeSyntax` (recursive), `resolveArrayType`, `resolvePointerType`, `resolveGenericStructType`, `resolveSetType` | 488, 1160, 712, 920, 984, 600 |
| Generics | `instantiateRoutine` (nested generic calls) | 1192 |
| Clone | `cloneStatementNode`, `cloneExpressionNode` (recursive) | 1592, 376 |

**How it is kept.**
- A `TypeExpr` (about 150 bytes) is never a local, a temporary or a
  by-value parameter of a function in the table: the parser builds it in an
  out-of-line helper that returns it into the heap node that owns it, and
  the analyzer takes it by reference.
- `parseTypeName` returns into a caller-provided node (no `std::string`
  concatenation temporaries), so its frame is expected to shrink.
- Messages that spell a type are built in out-of-line helpers (as the F2
  and D8 fixes did), so a throw site does not add string temporaries to a
  recursive frame.

**How it is proven.** Step 0 adds `tools/stack_frames.py`: it reads the
`.su` files of two builds (main and the head) and fails when any function
of the table, or any new function called from them, has a larger frame on
the head; it prints the table with both numbers. Every step runs it, and
every pull request of F4 shows its output; `windows-stack` must report no
growth for `limits-boundary-ok` (963 KiB and 451 KiB on main).

## What could go wrong, and the guard

| Risk | Guard |
|---|---|
| A message quotes a type differently | `spell` is tested against every spelling of the corpus (step 1); IR snapshot covers every diagnostic |
| A generic instance substitutes differently from text substitution (a parameter named like part of another name) | The tree substitutes only whole Named nodes, which is what text substitution meant; snapshot plus the generics tests; any difference is reported, never allowed silently |
| Module resolution renames a type the text path did not, or misses one | Snapshot of every module test; the walk resolves exactly the words `rewriteText` resolved |
| A frame on the recursion path grows | `-fstack-usage` per step, `windows-stack` job |
| A half-migrated state ships | Steps 2-5 keep both forms only inside the branch; one pull request, merged after step 6 |

## Decisions of the maintainer

Answered by Marcelo Fortes, 2026-11-01:

- **Q1.** Exact IR-snapshot exceptions first: yes. Change
  `ir-snapshot-exact` (r3.115), merged before any code of F4. F4 itself
  declares no exception.
- **Q2.** Positions of types are recorded only; diagnostics do not change
  in F4 (positions serve F6).
- **Q3.** Exception handler types stay names until F7.
- **Q4.** Type names inside expressions (`Integer(D)`, `TPoint(...)`,
  `New(T)`) stay identifiers the analyzer resolves.

Conditions of the same review, written into this design: the
substitution of type parameters in expressions (`New(T)`, `T(...)`) is
preserved and guarded by a test; `spell` reproduces today's canonical
spelling, including multi-dimensional arrays and nested generics, and is
proven against a table and the corpus; the invariants of `TypeExpr` are
stated and checked; no frame on a recursion path grows, proven by a
comparison of `-fstack-usage` per step.

## Approval

Q1-Q4: Marcelo Fortes, 2026-11-01 (above). Final approval of this design:
pending, after these conditions are reviewed and `ir-snapshot-exact` is
merged.
