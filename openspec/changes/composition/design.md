## Embedding: reuse of routines

```
Type
    TPoint Struct
        FX Integer
        FY Integer
    ;
    TCircle Struct
        TPoint                 == embedded: TCircle has FX, FY and TPoint's routines
        FRadius Integer
    ;

TPoint.Move(Self mut, DX Integer, DY Integer) :
    Self.FX += DX
    Self.FY += DY
;

Main :
    C TCircle
    C.Move(1, 2)               == TPoint.Move on the TPoint part of C
    PutLn(C.FX, " ", C.FRadius)
    P TPoint := C.TPoint       == the embedded part, copied as a value
    PutLn(P.FY)
;
```

Rules proposed:
- Promotion is resolved by the compiler: `C.Move(1, 2)` is
  `C.TPoint.Move(1, 2)`; nothing happens at run time.
- A name reachable twice (two embedded structs with `Move`, or an own field
  or routine with an embedded name) is a compile error, as shadowing is
  (CANON-10); the program calls `C.TPoint.Move` explicitly. There is no
  hidden overriding.
- A TCircle is not a TPoint: a routine that takes a TPoint receives
  `C.TPoint`, never `C` (no subtyping).
- Embedding is by value and recursive embedding is an error, as for fields.

## Protocols: what a type can do

```
Type
    TShape Protocol
        Area(Self) Float
        Name(Self) String
    ;
    TCircle Struct (TShape)    == TCircle declares that it satisfies TShape
        TPoint
        FRadius Float
    ;

TCircle.Area(Self) Float :
    Return 3.14159 * Self.FRadius * Self.FRadius
;
TCircle.Name(Self) String :
    Return "circle"
;

Report<T TShape>(Shape T) :
    PutLn(Shape.Name, ": ", Shape.Area)
;
```

- A struct satisfies a protocol only by declaring it, and the compiler
  checks that every listed routine exists with that signature (explicit,
  never duck typing; FUTURE: "not Java interfaces, not OO subtyping").
- An embedded struct's routines count: TCircle satisfies a protocol whose
  routines TPoint provides.
- Protocols are the constraints of generics; `Equatable`, `Ordered` and
  `Numeric` are built-in protocols.

## Decisions for the maintainer

1. **Embedding syntax**: a bare type on its own line (recommended; Go's
   form, the least to write) — or `Use TPoint` inside the struct, or a
   keyword such as `Includes TPoint`.
2. **Name collisions are errors** (recommended), or the outer struct's own
   routine replaces the embedded one (Go's rule; it is a hidden override).
3. **Declaring protocols** `TCircle Struct (TShape)` (recommended) — or a
   separate line `TCircle Satisfies TShape`, or structural satisfaction
   (duck typing at compile time, which CANON-1 rejects). The FUTURE
   requirement "Contracts, protocols and generics" says a protocol "is
   satisfied by providing its operations … without repeating signatures in
   the struct"; the recommended form keeps that (only the protocol's name
   is written, never its signatures) and adds the declaration, so the
   requirement's text changes with your approval.
4. **Protocol values** (a `TShape` variable holding any satisfying struct,
   calls through a table): now, later, or never. Recommended: later, after
   generics and pointers; generics cover most uses without run-time cost.
5. **Default routines in a protocol** (a body shared by every type that
   satisfies it): recommended later.
