## Context

`collectStructDefinitions` re-scans the Type section's raw token list and maps
each field type through `llvmTypeForScalar`, which today accepts only Integer
and Bool for fields. `emitTypedLocalVariable` stores `zeroinitializer` and the
literal defaults, and throws "struct initializers" for any `:=`. Named
construction reaches the emitter as an ordinary call and is unsupported.

## Goals / Non-Goals

Goals: every struct operation CANON-9 defines, for scalar and nested struct
fields, plus arrays of struct. Non-goals: array fields and struct State (item 2,
which also replaces the token re-scan), String fields (item 5).

## Decisions

- **Layout.** A struct is a named LLVM type `%T = type { ... }` in field order.
  Scalar fields use the type's value representation (narrow integers and
  Natural are i64, as for locals; only array elements are stored narrow).
  Nested structs are embedded by value. No ordering is needed: LLVM accepts
  forward references to named struct types; a cycle is reported by the
  semantic analyzer before codegen.
- **Defaults.** One routine `emitStructDefaults(ptr, T)` writes zero then each
  default, recursing into struct fields. Used by declarations, construction and
  array elements.
- **Construction.** `TPoint(FX := A, FY := B)` lowers to an `alloca %T`,
  defaults, then each named value stored in source order (arguments are
  evaluated left to right as written), then `load %T`. Each value follows the
  assignment rules: an Integer stored into a Natural field is checked at run
  time (kind 7, ADR-0015); a narrow field accepts a constant that fits or a
  value of its own type, as for a variable.
- **Initializers.** `Q T := E` evaluates E to a `%T` value and stores it; grouped
  declarations store into the first slot and copy into the next ones (the
  v3.26 rule, unchanged).
- **Places.** One routine computes the address of a variable, a field of a
  place or an element (`L.FB.FX`, `A[I].FX`, `M[I, J].F`); any other value is
  copied to a temporary. Field reads, writes, `ref mut` arguments, `with` and
  method receivers all use it, so a method can be called on any struct place.
- **Arrays of struct.** `[N x %T]`; `A[I].F` is a checked element address
  (fault kind 9) then the field GEP. Without an initializer the array is zeroed
  and, when the struct has a non-zero default, one loop over the flattened
  elements writes the defaults.

## Risks / Trade-offs

- Large structs copied by value: acceptable now (value semantics are law);
  copies use `load`/`store` of aggregates, which LLVM lowers to memcpy.
- Field-path assignment through calls (`F.X := 1` where F is a call) stays an
  error, unchanged.

## Runtime faults

No new fault kinds. Kind 9 for element indices; kind 7 for Natural fields;
narrow fields reuse the existing arithmetic range checks.
