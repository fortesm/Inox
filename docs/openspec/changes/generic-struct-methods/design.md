Design of the change `generic-struct-methods` (0.5, item 1). Risk: normal.
Status: **awaiting the maintainer's approval**; no code before it is
recorded here with its date.

## Today

- A generic struct is a template (`TPair<K, V> Struct`); `TPair<String,
  Int64>` is an instance, made on first use by `instantiateStruct`, named
  by its arguments and recorded in `structInstanceOf_` (type → template
  and arguments).
- A generic routine (`Max<T>(A T, B T) T`) is a template too: an instance
  is a copy of its header and body with T replaced (F4: on the tree),
  registered and analyzed like any routine (`instantiateRoutine`,
  `cloneInstanceBody`, `analyzePendingInstances`). Codegen emits instances
  like any routine.
- A method `TPoint.Move(Self mut, D Integer)` is a routine named
  `TPoint.Move`; a call `P.Move(3)` looks up `<type name>.Move` by the
  receiver's type (`analyzeMethodCallExpression`; a call without
  arguments, `P.Sum`, in `analyzeMemberExpression`).
- The parser rejects `TPair<K, V>.Swap(...)`
  (`tests/diagnostics/generics-method.inox`); it also rejects invariants
  and reused parts in a generic struct.

## The language

```inox
Type
    TPair<K, V> Struct Public
        FKey K
        FValue V
    ;

TPair<K, V>.Key(Self) K Public :
    Return Self.FKey
;

TPair<K, V>.SetValue(Self mut, X V) :
    pre X != Self.FValue
    Self.FValue := X
;

TPair<K, V>.Swapped(Self) TPair<V, K> :
    Return TPair(FKey := Self.FValue, FValue := Self.FKey)
;

Main :
    P := TPair(FKey := "a", FValue := 1)     == TPair<String, Int64>
    PutLn(P.Key)                             == a
    P.SetValue(2)
    Q := P.Swapped                           == TPair<Int64, String>
    PutLn(Q.Key)                             == 2
;
```

Rules:

1. **Header.** A method of a generic struct writes the struct's name with
   type parameters, one per parameter of the struct, in order:
   `TPair<K, V>.Name(...)`. They are the method's names for the instance's
   type arguments (Q1: they must be the struct's own names).
2. **One method for every instance.** The method is a template; an
   instance of it exists for each instance of the struct it is called on,
   made on the first call, as a generic routine's instance is.
3. **Receiver.** `Self` and `Self mut` are the instance (`TPair<String,
   Int64>`), with the rules of CANON-9 (read-only through owners, r3.109).
4. **No written type arguments at the call** (CANON-8): `P.Key`, never
   `P.Key<String>`; the receiver's type gives them.
5. **Contracts.** `pre` and `pos` are checked in each instance, as in a
   generic routine (CANON-24); `Result` and `old` as usual.
6. **Modules.** As every method (r3.99): declared in the module of its
   struct; `Public` at the end of the header; a Public method of a Public
   struct is seen by the modules that use it, and an instance made in
   another module is still the struct's module's routine (visibility,
   private fields).
7. **A struct and its methods agree.** A method on a generic struct
   without type parameters (`TPair.Key(Self)`), or with the wrong count,
   is an error that writes the header expected; type parameters on a
   method of an ordinary struct are an error too.
8. **Errors in an instance** say which instance and which call made it,
   and point at the line of the generic body (F2): `in TPair<String,
   Int64>.SetValue, used in Main: ...`.

## Phase by phase

**2 Parser** (`parseFunctionDeclaration`). Today `Name<T>` followed by `.`
is an error. It becomes: `Name<P1, ..., Pn>.Method` builds a
FunctionDeclaration named `Name.Method` whose `typeParameters()` are
P1..Pn, exactly as a generic routine's. A method with its own type
parameters (`TPair<K, V>.Map<U>`) stays an error (Q2). The header is
built by the F3/F4 code unchanged.

**3 Module resolution.** No new rule: the name `TPair.Swap` resolves its
receiver `TPair` to the struct's internal name; the type parameters are
excluded from renaming (`excluded_`), as for generic routines; the
coherence check (a method in its struct's module, Public only on a
Public struct, no private type in a Public header) applies unchanged.

**4 Contract lowering.** A generic routine with contracts already gets a
generic wrapper and a generic body (`__Name_body`), with its type
parameters; a generic method gets the same (`TPair.Swap` and
`TPair.__Swap_body`), so each instance of the wrapper calls the instance
of the body.

**5 Semantic analysis.**
- **Registration.** A generic method is not registered (it is a
  template); `registerGenericTemplates` indexes it by its struct and name
  (`genericMethods_`: template struct → method name → declaration), and
  checks rule 7 (the struct exists and is generic; the counts agree; the
  names are the struct's).
- **Lookup.** When `<type name>.Method` is not found and the receiver's
  type is an instance of a generic struct (`structInstanceOf_`), the
  analyzer finds the template method of that struct and instantiates it
  with the instance's arguments; the instance's name is the instance
  type's name plus the method, `TPair<String, Int64>.SetValue`, so the
  next call finds it directly. This is one out-of-line function,
  `genericMethodFor(receiverType, name)`, called from the two places that
  look methods up; neither gains locals (stack, below).
- **Instantiation** reuses `instantiateRoutine` with the instance name
  given by the caller (a parameter, not a second copy of the function):
  header and body substituted on the tree (F4), `New(T)` and `T(...)` by
  the expression rewriter, the instance placed in the template's module,
  registered, then analyzed with the pending instances.
- **`ast::methodDot`.** Today the dot of a method is searched before the
  first `<` (so `Max<Array[1..3] Int64>` has none). An instance method's
  name has its dot after the type arguments
  (`TPair<Bank.TKey, Int64>.SetValue`). The rule becomes: the last dot
  outside angle brackets and after any `::`. Every current name keeps its
  answer (unit test over the names of the snapshot programs).
- **Reused methods.** A method reached through a reused part
  (`findReusedMember`) of an ordinary struct whose part is an instance
  of a generic struct would route to that instance's method; parts of a
  generic struct stay rejected (Q4).

**6 Codegen.** None: an instance is a FunctionDeclaration with a
signature, emitted like the instances of generic routines today (their
names already contain `<`, `>`, `,` and spaces).

**AST services.** `inox interface` prints a generic method's header as
written, `TPair<K, V>.Key(Self) K Public`, not `TPair.Key<K, V>`.

## Diagnostics

| Program | Message (proposed) |
|---|---|
| `TPair.Key(Self) K :` for a generic `TPair<K, V>` | `TPair is generic: write its type parameters, TPair<K, V>.Key(...) (CANON-8)` |
| `TPair<K>.Key(Self) K :` | `TPair takes 2 type parameters (K, V): write TPair<K, V>.Key(...)` |
| `TPair<A, B>.Key(Self) A :` (Q1) | `the type parameters of TPair are K, V: write TPair<K, V>.Key(...)` |
| `TPoint<T>.Move(...)` for an ordinary struct | `TPoint is not generic: write TPoint.Move(...)` |
| `TPair<K, V>.Map<U>(...)` (Q2) | `a method cannot have type parameters of its own yet (CANON-8)` (today's message) |
| `P.Key<String>` | today's "type arguments are not written in an expression (CANON-8)" |
| an operation the instance's type lacks | `in TPair<String, Int64>.SetValue, used in Main: ...`, at the line of the generic body |
| unknown method | today's `unknown method Swap for TPair<String, Int64>` |

## Stack budget

The functions on the analyzer's recursion that this change touches, with
their frames on main (clang Debug, `-fstack-usage`):
`analyzeMethodCallExpression` 2328, `analyzeMemberExpression` 1176,
`instantiateRoutine` 1032. The lookup of a generic method is one call to
an out-of-line function returning a pointer, and the instance name is
built inside that function; `instantiateRoutine` takes the name as a
`const std::string&`. `tools/stack_frames.py` must show no growth, and
`windows-stack` none for `limits-boundary-ok` (963 KiB and 451 KiB). A
generic method calling another of the same struct, or itself through a
bigger type, is bounded by the existing instance limits
(`requireBoundedNesting`, `kMaxInstances`).

## Tests (planned)

- `tests/runtime/generic-methods.inox`: the example above, plus `Self mut`
  through a `ref mut` and an owner (`Own<TPair<...>>`), a method calling
  another method and a generic routine, two instances of one struct, and
  a method whose result is another instance (`Swapped`).
- `tests/runtime/generic-methods-contracts.inox`: `pre` and `pos` (with
  `old` and `Result`) in a generic method, a broken one trapping with the
  instance's name.
- `tests/modules/generic-methods/`: the struct and its Public methods in
  one module, used from another; a private method not seen; a method
  declared outside its struct's module rejected.
- Diagnostics for each row of the table, with line and column.
- `examples/generics.inox` rewritten with methods (`S.Push(X)` instead of
  `Push(S, X)`), its output unchanged.
- A unit test of `methodDot` on the names the corpus produces.
- `inox interface` of a module with generic methods.

**IR snapshot.** The only intended differences in existing programs are
`tests/diagnostics/generics-method.inox` (now accepted or rewritten into a
diagnostic of rule 7) and `examples/generics.inox` if it is rewritten;
both are declared exactly in `ir-snapshot-expect.json`. Everything else
must be identical.

## Decisions asked of the maintainer

- **Q1. Names of the type parameters in a method's header.** (a) They must
  be the struct's own names (`TPair<K, V>.Swap`), so a reader of the method
  sees the same letters as in the struct; (b) any names, by position
  (`TPair<A, B>.Swap`), as Rust's `impl<A, B> TPair<A, B>`. Recommended:
  (a), simpler to read and to check; (b) can come later without breaking
  programs.
- **Q2. Type parameters of the method itself** (`TPair<K, V>.Map<U>(F
  ...)`): out of this change (they need routine types to be useful).
  Recommended: out.
- **Q3. Invariants of a generic struct.** Today rejected. With generic
  methods they could become a generic `__invariant` method checked in
  each instance. Recommended: a separate small change right after this
  one, so this one keeps its scope.
- **Q4. Reused parts in a generic struct, and Override of a generic
  method:** stay rejected. Recommended: out (no program asks yet).
- **Q5. `examples/generics.inox`:** rewrite it with methods to show the
  feature (its output unchanged, declared in the snapshot), or keep it as
  the routine form and add a new example. Recommended: rewrite.

## Approval

(To be recorded: who approved, the date, and the answers to Q1-Q5.)
