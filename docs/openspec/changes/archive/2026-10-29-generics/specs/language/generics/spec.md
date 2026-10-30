## ADDED Requirements

### Requirement: A generic routine or struct is a template
A routine or a struct MAY declare type parameters after its name,
`Max<T>(A T, B T) T :` and `TPair<K, V> Struct`. A template SHALL NOT be
checked or compiled alone: each use with its types SHALL make an instance
(`Max<Int64>`, `TPair<String, Int64>`), a copy with the type parameters
replaced, checked and compiled like ordinary code, once per list of types.
(Marcelo Fortes, 2026-10-17: the recommendations are approved)

#### Scenario: Generic routines
- **WHEN** a program calls generic routines with Integers, Floats, Strings, Bools, arrays, slices and ref mut arguments, and one generic routine calls another
- **THEN** each call runs its own instance (tests/runtime/generics-routines.inox)

#### Scenario: Generic structs
- **WHEN** a program declares instances of generic structs, nested instances, arrays of instances, an array field of T, a State of an instance and an owner of T
- **THEN** each list of types is a struct of its own (tests/runtime/generics-structs.inox)

#### Scenario: Generics across modules
- **WHEN** a program uses generic routines and a generic struct of another module
- **THEN** the instances are made in the program (tests/integration/modules/generics-across-modules.inox)

#### Scenario: Contracts and exceptions in a generic routine
- **WHEN** a generic routine has `pre` and `pos`, another has `try`, and a method calls a generic routine
- **THEN** each instance checks its contracts and reaches its handler (tests/runtime/generics-contracts.inox)

### Requirement: Type parameters are distinct names that are not types
The type parameters of one template SHALL be distinct and SHALL NOT be the
name of a type. Among the Type declarations, only a Struct takes type
parameters.

#### Scenario: Names of type parameters
- **WHEN** a type parameter is the name of a type, or repeats another
- **THEN** it is rejected (tests/diagnostics/generics-parameter-is-type.inox, tests/diagnostics/generics-duplicate-parameter.inox)

#### Scenario: Only a Struct is generic
- **WHEN** a Range declares type parameters
- **THEN** it is rejected (tests/diagnostics/generics-range-parameters.inox)

### Requirement: Type arguments are inferred, never written in an expression
The type arguments of a call SHALL come from its arguments, matched with
the written types of the parameters (`Total(S Slice<T>)`, `KeyOf(P TPair<K,
V>)`); a literal argument SHALL take the type the other arguments fix
(`Max(B, 1)` with B Byte is Max<UInt8>). When the arguments do not fix them,
they SHALL come from the type of the target (`Z Integer := Zero`). Type
arguments written in an expression SHALL be an error.

#### Scenario: A literal takes the type of the other argument
- **WHEN** `Max(B, 1)` with B Byte gives a value that then leaves the Byte range
- **THEN** the program stops with "integer overflow" (tests/runtime/generics-instance-range.inox)

#### Scenario: Written type arguments
- **WHEN** a program writes `Max<Integer>(3, 7)` or `Max<Integer>(3)`
- **THEN** it is rejected with the rule (tests/diagnostics/generics-written-arguments.inox, tests/diagnostics/generics-written-argument.inox)

### Requirement: Every type argument has one type
Two arguments that give a parameter two types, and a parameter that nothing
fixes, SHALL be errors. The type arguments of a construction SHALL come from
the target or from the values of the fields, and a written type SHALL give
all the type arguments of a generic struct.

#### Scenario: Two types for one parameter
- **WHEN** a program calls `Max(1, "a")`
- **THEN** it is rejected (tests/diagnostics/generics-conflicting-arguments.inox)

#### Scenario: Nothing fixes the type
- **WHEN** a program calls `Zero` without a typed target, or builds `TPair(FKey := 1)` without FValue or a target
- **THEN** it is rejected with how to give the type (tests/diagnostics/generics-unknown-type.inox, tests/diagnostics/generics-construction-unknown.inox)

#### Scenario: Type arguments of a generic struct in a written type
- **WHEN** a written type gives a generic struct too few arguments, none, or gives arguments to an ordinary struct
- **THEN** it is rejected (tests/diagnostics/generics-struct-argument-count.inox, tests/diagnostics/generics-struct-without-arguments.inox, tests/diagnostics/generics-not-generic.inox)

### Requirement: No constraints: an instance reports what its types lack
A type parameter SHALL accept any type, also one that is only moved (an
owner, a struct holding one) or a File. What the body does with it SHALL be
checked in each instance like ordinary code; an error SHALL name the
instance and the routine that used it ("in Max<TPoint>, used in Main").

#### Scenario: An operation the type lacks
- **WHEN** a program calls `Max` with two structs
- **THEN** it is rejected in Max<TPoint> (tests/diagnostics/generics-no-operator.inox)

#### Scenario: An owner copied in a generic body
- **WHEN** a generic body copies its argument and the argument is an owner
- **THEN** it is rejected in that instance (tests/diagnostics/generics-owner-copied.inox)

#### Scenario: A File passed by value
- **WHEN** a generic routine takes T by value and is called with a File
- **THEN** it is rejected in that instance (tests/diagnostics/generics-file-by-value.inox)

#### Scenario: Endless instantiation
- **WHEN** a generic routine calls itself with a bigger type each time
- **THEN** it is rejected when the type arguments nest more than 32 levels deep (tests/diagnostics/generics-endless.inox)

### Requirement: Methods of generic structs are not yet in the language
A method of a generic struct, and invariants or reused parts in a generic
struct, SHALL be rejected until their own change; a generic routine does the
same job (`Push<T>(ref mut S TStack<T>, X T)`).

#### Scenario: A method of a generic struct
- **WHEN** a program declares `TPair<K, V>.Swap(Self mut)`
- **THEN** it is rejected with the generic routine to write (tests/diagnostics/generics-method.inox)

#### Scenario: A stack written once
- **WHEN** a generic stack holds Integers, Strings and pairs
- **THEN** each works through the same generic routines (examples/generics.inox)
