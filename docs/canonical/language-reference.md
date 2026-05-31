# Inox Language Manual 0.1 pre-alpha

This document is the consolidated human and AI reference for Inox 0.1 pre-alpha.
It is written as a tutorial and as a guardrail for future work by human contributors,
Codex, and other coding agents.

The canonical source tree remains:

- `AGENTS.md`
- `docs/canonical/vision.md`
- `docs/canonical/syntax.md`
- `docs/canonical/semantics.md`
- `docs/canonical/type-system.md`
- `docs/canonical/runtime.md`
- `docs/canonical/llvm-backend.md`
- `grammar/grammar.ebnf`

When this manual and a more specific canonical document disagree, treat the
specific canonical document as needing correction. Do not silently invent a third
meaning.

---

## Table of contents

1. [Purpose](#purpose)
2. [Design philosophy](#design-philosophy)
3. [Non-goals](#non-goals)
4. [Lexical rules](#lexical-rules)
5. [Blocks and semicolon](#blocks-and-semicolon)
6. [Modules and Use](#modules-and-use)
7. [Declarations](#declarations)
8. [Types](#types)
9. [Structs](#structs)
10. [Associated methods](#associated-methods)
11. [Variables, constants, and state](#variables-constants-and-state)
12. [Expressions and operators](#expressions-and-operators)
13. [Control flow](#control-flow)
14. [Subroutines and functions](#subroutines-and-functions)
15. [Prelude and runtime](#prelude-and-runtime)
16. [Strings](#strings)
17. [Arrays, vectors, ranges, and sets](#arrays-vectors-ranges-and-sets)
18. [Contracts, protocols, and behaviors](#contracts-protocols-and-behaviors)
19. [Visibility and modules](#visibility-and-modules)
20. [LLVM backend status](#llvm-backend-status)
21. [Portability rules](#portability-rules)
22. [Style conventions](#style-conventions)
23. [Canonical examples](#canonical-examples)
24. [Deferred features](#deferred-features)

---

## Purpose

Inox is a compiled, strongly typed, case-insensitive language designed to recover
some of the clarity and low-friction usability of Pascal-family languages while
avoiding the complexity of classical object orientation.

The practical 0.1 goal is not to compete immediately with mature industrial
languages. The practical goal is a small, coherent compiler that can compile useful
programs with a short edit-build-run cycle and a readable language surface.

A production-quality 0.1 should feel closer to the simplicity of Turbo Pascal 7
than to a sprawling framework ecosystem, but with a modern type system direction,
LLVM backend, structs, composition, and associated methods instead of classes and
inheritance.

---

## Design philosophy

Inox is post-object-oriented.

The language explicitly rejects the idea that every software problem must be
modeled as a class hierarchy. Not every relation is taxonomic. Reuse should not
require inheriting a base class or building stacks of interfaces.

Inox therefore uses:

- nominal static typing;
- structs as data;
- associated methods declared outside structs;
- composition over inheritance;
- future contracts/protocols/behaviors for behavior reuse;
- explicit, strongly checked semantics;
- minimal boilerplate;
- a deliberately DRY style.

The ergonomic goal is to keep useful call syntax:

```inox
P.Move(3, 7)
```

without introducing classes, inheritance, Java-style interfaces, mixins, duck
typing, or hidden runtime taxonomies.

---

## Non-goals

Inox 0.1 does not have:

- classes;
- classical inheritance;
- Java-style interfaces;
- mixins;
- duck typing;
- mandatory class taxonomies;
- duplicated method signatures inside structs;
- heavy RTTI;
- reflection;
- a heavy garbage collector;
- property syntax;
- virtual/abstract method dispatch;
- a complete module/package manager;
- a complete string/Unicode runtime;
- a complete exception ABI.

Future work may add advanced behavior reuse, but it must not reintroduce classical
OO under another name.

---

## Lexical rules

Inox is case-insensitive. The following spellings denote the same keyword or name:

```inox
Main
main
MAIN
```

The compiler must preserve original lexemes where useful for diagnostics, but
semantic lookup is case-insensitive.

Line comments use `==`:

```inox
== This is a comment.
```

Block comments are not part of Inox 0.1.

String literals use double quotes. Character literals use single quotes.
Hexadecimal integer literals may use Pascal-style `$FF` or C-style `0xFF` where
supported by the lexer.

---

## Blocks and semicolon

The semicolon `;` closes blocks. It is not a general statement terminator.

Named subroutine blocks use `:`:

```inox
Compute() Integer :
    Return 10
;
```

The `;` closes the subroutine block. It does not terminate the `Return` statement.

Conditional branches are not named blocks. `if`, `elif`, and `else` do not use
`then` and do not use `:`. The end of the line after the condition opens the branch:

```inox
if A > B
    Return A
else
    Return B
;
```

There is exactly one `;` for the whole `if`/`elif`/`else` structure. This is wrong:

```inox
if A > B
    Return A
;
else
    Return B
;
```

Indentation is visual only. It is not Python-style block syntax.

---

## Modules and Use

A source file begins with a module declaration:

```inox
Module Demo
```

`Module` closes at EOF. There is no final `;` for the module.

`Use` is reserved for module imports. In 0.1, the complete multi-file module system
is not implemented. The automatic prelude is always visible.

Future `Use` semantics must avoid uncontrolled name pollution. The preferred
future direction is explicit modules and predictable conflict handling.

---

## Declarations

Top-level declarations include:

- `Use`
- `Type`
- `Const`
- `State`
- functions/subroutines
- `Main`

`Type` is a section/declarator, inspired by Go. It is not a block and it has no
closing semicolon.

Canonical type section:

```inox
Type
    TPoint Struct
        FX Integer
        FY Integer
    ;

    TUser Struct
        FName String
        FAge Integer
    ;
```

The `;` closes each `Struct`, not the `Type` section.

---

## Types

Inox is strongly and nominally typed.

Canonical built-in scalar type names include:

- `Bool`
- `Int8`, `Int16`, `Int32`, `Int64`
- `UInt8`, `UInt16`, `UInt32`, `UInt64`
- `Integer` = `Int64`
- `UInteger` = `UInt64`
- `Natural`
- `Float32`, `Float64`
- `Float` = `Float64`
- `Currency`
- `Crypto`
- `Char`
- `String`

`Bool` is the canonical boolean type. `Boolean` is not a built-in type.

`Currency` is an exact monetary decimal type and is never `Float64`.
`Crypto` is an exact high-precision decimal type and is never `Float64`.

Implicit conversions are allowed only for safe widening. Casts use:

```inox
Integer(X)
Currency("12.34")
```

---

## Structs

A `Struct` declares fields only. It never declares methods and never repeats method
signatures.

Canonical syntax:

```inox
Type
    TPoint Struct
        FX Integer
        FY Integer
    ;
```

`Type` has no `:` and no closing `;`.
`Struct` opens the struct body and `;` closes the struct.

Struct type names conventionally begin with `T`. Struct fields conventionally
begin with `F`. These are style conventions in 0.1, not compiler errors.

### Field defaults

Integer and Bool fields may have literal defaults in the current subset:

```inox
Type
    TConfig Struct
        FPort Integer := 8080
        FEnabled Bool := true
    ;
```

A local struct variable is default-initialized. The current LLVM backend uses zero
initialization and then emits explicit stores for literal defaults.

### Field access

Field access uses dot syntax:

```inox
P.FX := 10
P.FY := 20
Return P.FX + P.FY
```

Field lookup is case-insensitive.

### Value semantics

Structs are value types. Assignment and parameter passing are intended to copy by
value unless a future explicit reference/borrow mechanism says otherwise.

The current backend already passes associated-method receivers by pointer for
implementation convenience when the receiver is local storage. That does not make
structs classes and does not introduce inheritance.

### Not in the first struct subset

The following are deferred:

- variant structs;
- tags such as JSON/DB metadata;
- embedding/composition promotion;
- general struct parameters and returns where not implemented;
- struct comparison;
- heap allocation;
- reference/borrow semantics.

---

## Associated methods

Associated methods are declared outside structs. This is a core Inox rule.

Canonical method declaration:

```inox
TPoint.Move(Self TPoint, DX Integer, DY Integer) :
    Self.FX := Self.FX + DX
    Self.FY := Self.FY + DY
;
```

Canonical call-site sugar:

```inox
P.Move(3, 7)
```

The call is lowered as an associated function call with the receiver supplied
explicitly by the compiler.

Rules for 0.1:

- `Self` is explicit in the method declaration.
- The receiver parameter is the first parameter.
- Struct declarations do not list or repeat method signatures.
- No virtual dispatch is introduced.
- No inheritance relation is introduced.
- No Java-style interface is introduced.
- Method lookup is nominal and case-insensitive.

Mutability of `Self` is intentionally simple in the current subset: local struct
storage may be mutated by associated methods. A future `mut Self` spelling may be
introduced only if it improves static checking without adding boilerplate.

---

## Variables, constants, and state

Local variables are declared with inline `var` or a `Var` block.

Examples:

```inox
var X := 10
```

```inox
Var :
    X := 10
    Y Integer := 20
;
```

Local variables are mutable in the current 0.1 subset. Constants belong in `Const`.
Mutable global state must use `State` explicitly.

Shadowing is forbidden in all scopes. Because the language is case-insensitive,
`Value`, `value`, and `VALUE` are the same name.

---

## Expressions and operators

Assignment uses `:=`.

Logical Bool operators:

- `and`
- `or`
- `xor`
- `not`

Integer bitwise operators:

- `bitand`
- `bitor`
- `bitxor`
- `bitnot`
- `shr`
- `shl`

Do not use `&`, `|`, `~`, `<<`, or `>>` as bitwise operators in Inox 0.1.

`^` is exponentiation and is never XOR.

Precedence, from highest to lowest:

1. parentheses
2. calls, indexing, member access
3. exponentiation `^`
4. unary `+`, `-`, `not`, `bitnot`
5. `*`, `/`, `div`, `mod`
6. `+`, `-`
7. `shl`, `shr`
8. `bitand`
9. `bitxor`
10. `bitor`
11. range `..`
12. membership `in`
13. relational `=`, `#`, `<`, `>`, `<=`, `>=`
14. logical `and`
15. logical `xor`
16. logical `or`
17. assignment `:=`

`^` is right-associative. Assignment is right-associative. Assignment inside a
boolean expression is forbidden.

---

## Control flow

### if / elif / else

Canonical form:

```inox
if A > B
    Return A
elif A = B
    Return 0
else
    Return B
;
```

No `then`. No `:`. No `;` between branches.

### while

```inox
while I > 0
    I := I - 1
;
```

### repeat / until

Inox `repeat` is a general loop, not merely Pascal `repeat until`.

`until Condition` is a statement inside `repeat`. It exits the nearest enclosing
`repeat` when the condition is true.

```inox
repeat
    Work()
    until Done
    ContinueWork()
;
```

`until` may appear at the start, middle, or end of the repeat body, and may appear
multiple times.

```inox
repeat
    until Done
    Work()
;
```

A final `until` gives Pascal-like behavior:

```inox
repeat
    Work()
until Done
;
```

`repeat` without `until` is an explicit infinite loop and must rely on `break`,
`Exit`, or `Return` to terminate.

### for in range

Counted range loops use:

```inox
for I in 1..N
    Total := Total + I
;
```

Stepped counted range loops use:

```inox
for I in 2..N(2)
    Total := Total + I
;
```

The current LLVM subset supports inclusive increasing Integer ranges with positive
step.

### break and continue

`break` exits the innermost loop.
`continue` continues the innermost loop.

Current loop constructs are `while`, `repeat`, and `for`.

### case

`case` follows Ada/SPARK style:

- no fallthrough;
- no `break`;
- supports individual values and ranges;
- `otherwise` is required unless the compiler can prove exhaustiveness.

---

## Subroutines and functions

A function has a return type:

```inox
Sum(A Integer, B Integer) Integer :
    Return A + B
;
```

A subroutine omits the return type:

```inox
Report(Value Integer) :
    PutLn(Value)
;
```

`Return Expression` returns a value from a function.

`Exit` exits the current subroutine without an expression.

There is no implicit `Result`. There is no `Return := Expression`.

Function and subroutine names are case-insensitive.

---

## Prelude and runtime

The automatic prelude exposes:

- `Sys.IO`: `Put`, `PutLn`, `ReadLn`
- `Sys.Math`: `Sin`, `Cos`, `Sqrt`, `Abs`
- `Sys.Std`: `Length`, `Ord`

The current backend implements a temporary `printf`-based lowering for output.
This is a smoke-test mechanism, not the final runtime ABI.

---

## Strings

Canonical direction:

- `String` is a UTF-8 runtime type.
- String values are immutable unless a future mutable string/buffer type is
  explicitly introduced.
- String literals are supported by the current output backend.
- Full Unicode indexing semantics are deferred.

Future runtime should distinguish byte length, Unicode scalar length, and possibly
grapheme-aware operations rather than overloading one ambiguous `Length` meaning.

---

## Arrays, vectors, ranges, and sets

Canonical direction:

- arrays have explicit index ranges;
- vectors are dynamic and 0-based;
- ranges are finite ordinal intervals where applicable;
- sets follow Pascal/Ada style with a finite ordinal base.

The detailed concrete syntax for arrays, vectors, and set types remains future
work. Agents must not invent final syntax for these without explicit approval.

---

## Contracts, protocols, and behaviors

This is the future behavior-reuse mechanism of Inox. It is deliberately not
Java-style interfaces and not duck typing.

Canonical constraints for future design:

- no classical inheritance;
- no implicit duck typing;
- no mixins;
- no mandatory class/interface hierarchies;
- no duplicated method signatures inside structs;
- contract/protocol satisfaction should be explicit and statically checked;
- behavior reuse must remain DRY and composition-oriented.

Detailed syntax is not implemented in 0.1 and must not be invented ad hoc by
agents.

---

## Visibility and modules

0.1 has no `public`, `private`, `protected`, or `published` keywords.

The future visibility model should be module/export based rather than class based.
Until that model is designed, agents must not introduce visibility keywords.

---

## LLVM backend status

The textual LLVM backend is an incremental prototype. It currently supports:

- `Integer` and `Bool` expressions;
- arithmetic, bitwise, comparison, and simple Bool operators;
- functions and subroutines;
- `Main` with a body;
- local variables;
- assignments;
- `if`, `elif`, `while`, flexible `repeat`, and `for` range subsets;
- `break` and `continue` in implemented loop forms;
- temporary output through `Put`/`PutLn`;
- simple structs with Integer/Bool fields;
- literal field defaults for Integer/Bool;
- associated methods in a restricted subset.

It is not yet the final runtime ABI.

---

## Portability rules

The compiler implementation is portable C++20.

Supported development hosts:

- Windows with MSVC and CMake;
- Linux with GCC or Clang and CMake.

Future desired hosts include FreeBSD, Solaris/Illumos, AIX, HP-UX, UnixWare, and
other Unix systems where feasible.

Engineering rules:

- prefer standard C++;
- isolate platform differences in CMake or scripts;
- avoid platform `#ifdef`s unless there is a real platform API boundary;
- do not make Windows-only path assumptions in compiler code;
- keep textual LLVM backend host-independent.

---

## Style conventions

Canonical spelling in documentation:

- `Module`
- `Type`
- `Struct`
- `Var`
- `Const`
- `State`
- `Main`
- `Return`
- `Exit`

Types and structs conventionally start with `T`:

```inox
TPoint
TUser
TConfig
```

Struct fields conventionally start with `F`:

```inox
FX
FY
FName
FAge
```

These are style rules in 0.1, not hard compiler errors.

---

## Canonical examples

### Hello

```inox
Module Hello

Main() :
    PutLn("Hello, Inox")
;
```

### Function

```inox
Module Functions

Sum(A Integer, B Integer) Integer :
    Return A + B
;

Main() :
    PutLn(Sum(3, 4))
;
```

### Struct

```inox
Module StructBasic

Type
    TPoint Struct
        FX Integer
        FY Integer
    ;

SumPoint() Integer :
    Var :
        P TPoint
    ;

    P.FX := 10
    P.FY := 20

    Return P.FX + P.FY
;

Main() :
    PutLn(SumPoint())
;
```

### Associated method

```inox
Module AssociatedMethods

Type
    TPoint Struct
        FX Integer
        FY Integer
    ;

TPoint.Move(Self TPoint, DX Integer, DY Integer) :
    Self.FX := Self.FX + DX
    Self.FY := Self.FY + DY
;

TPoint.Sum(Self TPoint) Integer :
    Return Self.FX + Self.FY
;

Main() :
    Var :
        P TPoint
    ;

    P.FX := 10
    P.FY := 20
    P.Move(3, 7)
    PutLn(P.Sum())
;
```

---

## Deferred features

Deferred, but directionally accepted:

- struct embedding as composition with controlled promotion;
- variant structs;
- field metadata tags for JSON/DB;
- full arrays/vectors/sets;
- complete module system;
- contracts/protocols/behaviors;
- final runtime ABI;
- complete strings and Unicode operations;
- exceptions lowering;
- package manager;
- optional linting for naming conventions.

Explicitly rejected:

- classes;
- inheritance;
- Java-style interfaces;
- duck typing;
- mixins;
- heavy RTTI as a core requirement.
