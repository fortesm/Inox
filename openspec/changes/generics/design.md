## Examples

```
Type
    TPair<K, V> Struct
        FKey K
        FValue V
    ;

Max<T Ordered>(A T, B T) T :
    Return if A > B then A else B
;

TPair<K, V>.Swapped(Self) TPair<V, K> :
    Return TPair(FKey := Self.FValue, FValue := Self.FKey)   == K and V from the result type
;

Main :
    P TPair<String, Integer> := TPair(FKey := "a", FValue := 1)
    PutLn(P.FKey, " ", Max(3, 7), " ", Max("b", "a"))   == T is Integer, then String
;
```

CANON-8 keeps `<...>` out of expressions: type arguments are written only
in type position (declarations, signatures, routine headers). A
construction or a call never writes them; they come from the arguments or
from the declared target (`P TPair<String, Integer> := TPair(...)`). Less
to write, and `<` in an expression is always a comparison.

## Decisions for the maintainer

1. **Constraint syntax** `<T Ordered>` (recommended: the Inox `Name Type`
   form) — or `<T: Ordered>` (Rust/Swift, but `:` only declares routines),
   or `<T is Ordered>`.
2. **Constraints required** (recommended): inside a generic body only the
   operations its constraints grant are allowed (`>` needs `Ordered`). The
   alternative, C++-style templates checked at each use, gives errors far
   from their cause.
3. **Built-in protocols** now: `Equatable` (`=`, `#`), `Ordered`
   (`Equatable` plus `<` `<=` `>` `>=`), `Numeric` (`Ordered` plus `+` `-`
   `*` and `div`/`mod` or `/` as the type allows). User protocols come with
   `composition`.
4. **Inference only** (recommended, and what CANON-8 already implies): no
   `Max<Integer>(3, 7)` or `TPair<String, Integer>(...)` in expressions;
   when nothing determines T, the compiler asks for a typed target
   (`Z Integer := Zero`).
5. **Which types may be T**: every type that can be copied (so not `File`,
   which is never copied); a later `Movable` rule for Vector.
6. **Monomorphization** (recommended) — or one shared body with run-time
   type descriptors (smaller code, slower, needs RTTI, which Inox avoids).

## Lowering

The analyzer records each instance (`Max<Int64>`, `Max<String>`); the
emitter emits one LLVM function per instance (`@inox_max$Int64`), with the
String ownership paths applying to String instances as to any routine.
Generic structs become one LLVM type per instance.

## Order of work after approval

1. Generic routines with built-in constraints. 2. Generic structs and their
methods. 3. `Option<T>` and `Result<T, E>`. 4. `Vector<T>` (move
semantics). Each step: tests, gates, its own spec delta.
