## Why

Object-oriented languages let a library accept "anything that knows how to
Draw" and call types it does not know (frameworks, plugins, GUI events).
Inox rejects interfaces and protocols as nominal entities (FUTURE "Generics
and static proof"), and variant structs are closed. What fills the gap
without a new kind of type is a **routine type**: a routine kept in a
variable, a field or a parameter, as Pascal's procedural types
(`TComparison = function(A, B: Integer): Boolean`,
`Button1.OnClick := Button1Click`).

Recorded at the request of Marcelo Fortes, 2026-10-14.

Status: **direction recorded** (FUTURE "Encapsulation, routine types and
lifecycle", r3.70). No code before the design is approved.

## Illustrations (syntax NOT decided)

One sort, several orders:

```
Type
    TComparison Routine (A Integer, B Integer) Bool

Ascending(A Integer, B Integer) Bool :
    Return A < B
;
Descending(A Integer, B Integer) Bool :
    Return A > B
;

Sort(ref mut V Array[1..5] Integer, Before TComparison) :
    for I in 1..4
        for J in I + 1..5
            if Before(V[J], V[I])
                T := V[I]
                V[I] := V[J]
                V[J] := T
            ;
        ;
    ;
;

Main :
    Grades Array[1..5] Integer
    Sort(Grades, Ascending)          == the routine itself, not its result
    Sort(Grades, Descending)
;
```

Behavior per value, not per type (a game):

```
Type
    TReacao Routine (Self mut TSoldier)
    TSoldier Struct
        FLife Integer := 100
        FOnSeeEnemy TReacao := Attack        == every soldier keeps its own reaction
    ;

if S.FLife < 20
    S.FOnSeeEnemy := Flee                   == this wounded soldier changes behavior
;
S.FOnSeeEnemy(S)
```

| | `Override` (CANON-9, implemented) | Routine type |
|---|---|---|
| behavior varies per | type (every knight runs alike) | value (each soldier reacts its own way) |
| chosen at | compile time | run time |
| used for | specializing a type | callbacks, events, strategies, sorting, game AI |

Combined with generics, routine types cover most of what interfaces do in
Go and Java, without a nominal interface type.

## Recommendation recorded (to confirm in the design)

1. First form: **named routines only, no captured variables** (classic
   Pascal procedural types): simple, no memory questions.
2. Closures (anonymous routines that capture locals, Delphi `reference to`,
   Rust/Go closures) only after change `safe-pointers`, because a capture
   asks who owns the captured variable.

## Open questions (not decided)

1. Syntax of the type (`Routine (...)` above is only an illustration).
2. Methods as values (Delphi `procedure of object`): a routine bound to a
   receiver.
3. Default value (CANON-5: every variable is born with a value, and Inox
   has no null): a routine-type field needs an initializer, or the type
   names its default.
4. Equality of routine values; routine values in contracts.
5. Cost: an indirect call that cannot be inlined (acceptable; document it).
