## Examples

```
Type
    TPair<K, V> Struct
        FKey K
        FValue V
    ;

Max<T>(A T, B T) T :
    Return IIf(A > B, A, B)
;

Swap<T>(ref mut A T, ref mut B T) :
    C := A
    A := B
    B := C
;

Main :
    P TPair<String, Integer> := TPair(FKey := "a", FValue := 1)
    X := 1
    Y := 2
    Swap(X, Y)
    PutLn(P.FKey, " ", Max(3, 7), " ", Max("b", "a"), " ", X)
;
```

`Max(3, 7)` compiles Max with T = Integer; `Max("b", "a")` with T = String.
`Max(P, P)` with a struct is an error at that call: "Max<TPair<String,
Integer>>: `>` is not defined for TPair (line 2 of Max)", because structs
are not ordered (CANON-9).

## Decisions for the maintainer

1. **No constraints at first** (recommended: the simplest; errors appear at
   the use, with the generic line), or a short written list now
   (`Max<T Ordered>`).
2. **Inference only** (recommended, what CANON-8 implies): no
   `Max<Integer>(3, 7)` in expressions; when nothing determines T, a typed
   target is needed (`Z Integer := Zero`).
3. **What T may be**: any type that can be copied (so not File); a struct,
   an array, a String.

## Lowering

The analyzer checks and records each instance (`Max<Int64>`, `Max<String>`);
the same instance machinery serves generic structs (one LLVM type per
instance). The emitter emits one LLVM function per instance
(`@inox_max$Int64`); String instances use the String ownership paths.

## Order of work after approval

1. Generic routines. 2. Generic structs and their methods. 3. `Option<T>`
and `Result<T, E>`. 4. `Vector<T>` (move semantics). Each step: tests,
gates, its own spec delta.
