Design of foundation step F4 (change `structured-types`). Risk: high.
Status: **awaiting the maintainer's approval**; no code is written before
it is recorded here with its date.

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

**6 Codegen.** Tests `has_value()` where it tests `empty()`. Nothing else.

**AST services.** `AstClone` copies trees; `inox interface` prints
`spell(...)`; `RoutineHeader` holds `TypeExpr`.

## Order of work: steps that each pass every gate

1. `ast::TypeExpr`, `spell`, unit tests: for every type spelled in the test
   corpus, `spell(parse(text)) == text`.
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
+5%). F4 touches functions on the recursion path of the parser:
`parseTypedLocalStatement` (488 bytes today) is reached from the statement
recursion, and `parseTypeName` (1064 bytes) recurses on nested types.

- A `TypeExpr` is larger than a `std::string` (about 150 bytes against 32).
  It must never be a local of a function on the statement or expression
  recursion: the node is built in an out-of-line helper and moved into the
  heap node it belongs to.
- `parseTypeName` is rewritten to keep its frame at or below 1064 bytes
  (it shrinks if the text concatenations go, which is expected).
- On the analyzer side, `analyzeStatement` must not hold a `TypeExpr`:
  `resolveWrittenType` takes a reference to the node's tree.
- Every step compares `-fstack-usage` with main and lists the frames that
  changed in the pull request; `windows-stack` must show no growth.

## What could go wrong, and the guard

| Risk | Guard |
|---|---|
| A message quotes a type differently | `spell` is tested against every spelling of the corpus (step 1); IR snapshot covers every diagnostic |
| A generic instance substitutes differently from text substitution (a parameter named like part of another name) | The tree substitutes only whole Named nodes, which is what text substitution meant; snapshot plus the generics tests; any difference is reported, never allowed silently |
| Module resolution renames a type the text path did not, or misses one | Snapshot of every module test; the walk resolves exactly the words `rewriteText` resolved |
| A frame on the recursion path grows | `-fstack-usage` per step, `windows-stack` job |
| A half-migrated state ships | Steps 2-5 keep both forms only inside the branch; one pull request, merged after step 6 |

## Decisions asked of the maintainer

- **Q1. IR snapshot exactness first.** `--allow` accepts any difference in a
  listed program, and an exception no longer needed does not fail. Proposal:
  before F4's code, a small CI change (`ir-snapshot-exact`) where the allow
  list records the expected difference per program (the before and after
  lines) and the job fails on any other difference and on an unused
  exception. F4 itself should need no exception at all. Recommended: yes,
  first.
- **Q2. Positions of types.** `TypeExpr` records where a type starts.
  Proposal: record only, and use it for diagnostics later (F6), so F4
  changes no output. The alternative points "unknown type" errors at the
  type instead of the declaration: a diagnostic change, declared as
  intended. Recommended: record only.
- **Q3. Exception handler types** stay names until F7 (the exception
  hierarchy). Recommended: yes.
- **Q4. Type names inside expressions** (`Integer(D)`, `TPoint(FX := 1)`,
  `New(T)`) stay identifiers the analyzer resolves; they are names, not
  written types. Recommended: yes.

## Approval

(To be recorded: who approved, the date, and the answers to Q1-Q4.)
