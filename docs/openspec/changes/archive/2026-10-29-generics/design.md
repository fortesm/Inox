Approved by Marcelo Fortes, 2026-10-17 (quoted in proposal.md): the recommendations for the three questions are approved. The decisions below are answered
with their recommendations; decision 3 is the recommendation given in the
conversation, broader than the first text of this design (see Decisions).

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

**Answered (2026-10-17):** 1. no constraints at first, `{ }` stays reserved;
2. inference only; 3. T may be **any type**, also one that is only moved
(an owner, a struct holding one) or a File: the instance is checked like
ordinary code, so copying an owner or passing a File by value is an error
of that instance (tests/diagnostics/generics-owner-copied.inox,
generics-file-by-value.inox).

## Lowering

The analyzer checks and records each instance (`Max<Int64>`, `Max<String>`);
the same instance machinery serves generic structs (one LLVM type per
instance). The emitter emits one LLVM function per instance
(`@inox_max$Int64`); String instances use the String ownership paths.

## Order of work after approval

1. Generic routines. 2. Generic structs and their methods. 3. `Option<T>`
and `Result<T, E>`. 4. `Vector<T>` (move semantics). Each step: tests,
gates, its own spec delta.

## Implementation notes (where the code differs from this design)

- **Order of work.** Steps 1 and 2 landed together: generic routines and
  generic structs; the methods of generic structs are rejected with a
  message that shows the generic routine doing the same job (`Push<T>(ref
  mut S TStack<T>, X T)`). Invariants and reused parts in a generic struct
  are rejected too ("not yet").
- **Where instances are checked.** After the program's own routines, as
  routines of the module (`analyzePendingInstances`): a generic body never
  sees the caller's locals. Checking an instance may make more. The
  analyzer copies the body with `ast::cloneStatements`, replacing the type
  parameters in written types (`types::substituteTypeNames`) and in
  expressions (`New(T)`).
- **Messages.** An error in an instance reads "in Max<TPoint>, used in Main:
  structs cannot be compared with '>'". Semantic messages in Inox carry no
  line numbers yet, so the "line of the generic body" of the example above
  is not printed.
- **Literals.** A literal argument takes the type the other arguments fix
  (`Max(B, 1)` with B Byte is Max<UInt8>); two arguments that give T two
  types are an error, literals included (`Max(1, "a")`).
- **Limits.** 1000 instances, and type arguments nested at most 32 levels
  deep, so a routine that calls itself with a bigger type each time
  (`Grow(N - 1, TBox(FItem := X))`) stops with a message in milliseconds.
- **Written type arguments.** `Max<Integer>(3, 7)` is rejected by the parser
  (the comma shows it is not a comparison); `Max<Integer>(3)` reads as
  comparisons and is rejected by the analyzer; both give the rule.
- **Codegen** compiles each instance like a routine of the program (a name
  such as `@"inox_max<int64>"`, a type such as `%"tpair<string, int64>"`),
  calls the routine the analysis bound to each call, and takes the struct
  of a construction from the analysis (no name lookups: the debt recorded
  by `type-representation` for constructions is paid).
- **Freshness of a construction.** "An owner is never copied" asked whether
  the callee's name was a type; a generic construction (`THolder(...)`) is
  now recognized from the analysis, so it can be returned or stored.
