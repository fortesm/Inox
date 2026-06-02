# Inox Language Reference 0.1 Pre-alpha

This is the consolidated human and AI reference for Inox 0.1 pre-alpha. It is the primary tutorial-style source for people and coding agents. Topic-specific canonical files under `docs/canonical/`, ADRs under `docs/decisions/`, `grammar/grammar.ebnf`, and `AGENTS.md` must stay aligned with this document.

## Table of Contents

1. [Vision](#vision)
2. [Lexical rules](#lexical-rules)
3. [Modules and Use](#modules-and-use)
4. [Blocks and statement syntax](#blocks-and-statement-syntax)
5. [Declarations](#declarations)
6. [Functions, subroutines, Return, and Exit](#functions-subroutines-return-and-exit)
7. [Types](#types)
8. [Structs and associated methods](#structs-and-associated-methods)
9. [Mutability and ownership](#mutability-and-ownership)
10. [Control flow](#control-flow)
11. [Operators and numeric semantics](#operators-and-numeric-semantics)
12. [Arrays, Vector, Range, Enum, Set](#arrays-vector-range-enum-set)
13. [Strings and Char](#strings-and-char)
14. [Errors, exceptions, null, unsafe](#errors-exceptions-null-unsafe)
15. [Contracts, protocols, behaviors](#contracts-protocols-behaviors)
16. [Implementation status and conformance gaps](#implementation-status-and-conformance-gaps)

## Vision

Inox is a compiled, strongly typed, post-object-oriented systems language for mission-critical and high-integrity software. It combines lessons from Ada/SPARK, Modula/Oberon/Component Pascal/Zonnon, Eiffel/Sather, Rust, Go, Chapel, modern Pascal, Swift, Kotlin, C#, Java, Julia, and the expressive interpreted languages, while rejecting unsafe or obsolete defaults.

The language is designed for systems where silent failure is unacceptable: aviation, finance, crypto, industry, medical equipment, energy, aerospace, scientific computing, high-performance parallel computation, and critical infrastructure.

Inox prefers explicit safety over convenience when the two conflict.

## Lexical rules

Inox is case-insensitive for keywords and identifiers. Documentation uses canonical spelling, but the compiler treats these as equivalent:

```inox
PutLn
putln
PUTLN
```

Line comments use `==`:

```inox
X := 10  == comment until end of line
```

Block comments do not exist in 0.1.

String literals use double quotes. Character literals use single quotes.

## Modules and Use

A source file starts with a module declaration:

```inox
Module Calc.Core
```

Rules:

- `Module` is the first declaration in a file.
- `Module` has no `;`.
- EOF terminates the module.
- One `.inox` file corresponds to one logical module.

`Use` declares semantic dependencies:

```inox
Module Calc.Core

Use Sys.IO
Use Math.Basic
Use Calc.Types
```

A compact form is also allowed:

```inox
Module Calc.Core Use Sys.IO Use Math.Basic Use Calc.Types
```

`Use` is not textual inclusion. It is not `#include`, not source copying, and not manual linking. It tells the compiler to load imported module types and signatures. Multi-file compilation is required for the 0.1 direction, even if the current implementation is incremental.

In 0.1 all module symbols are public by default. `Export`, interface/body separation, aliases, selective imports, and visibility controls are reserved for future versions.

The minimum 0.1 driver resolves imported modules relative to the entry file
directory and through the configured standard-library directory. `Use Math.Basic`
checks `Math.Basic.inox`, then `Math/Basic.inox`, in each search root.
Dependencies are loaded recursively, cycles are rejected, and imported
signatures participate in semantic analysis before textual LLVM IR is emitted.

Standard-library discovery uses this order: `INOX_STDLIB`, the prebuilt release
layout (`bin/` next to `stdlib/`), `stdlib/` next to the executable, `stdlib/`
under the current working directory, and `stdlib/` in the source file directory
or one of its parents.

## Standard library

The initial portable 0.1 standard library lives under `stdlib/`:

- `Std.Core` is the conceptual prelude/core module. It anchors fundamental
  names and compiler intrinsics such as future array bounds operations.
- `Std.IO` documents the canonical `Put` and `PutLn` I/O facade.
- `Std.Math` contains pure Integer helpers implemented in Inox source,
  including `Min`, `Max`, `Clamp`, `IsEven`, and `IsOdd`.
- `Std.Debug` documents the future `Assert` facade. `Assert` remains
  intentionally unavailable until trap/abort behavior is canonical.

`Std.Core` is conceptually implicit. The other modules are available through
explicit `Use`, such as `Use Std.Math` and `Use Std.IO`. The standard library
must remain portable across Windows and Linux and must not depend on GC,
unsafe features, C interop, or a complex runtime.

The current `stdlib/` directory is not a complete standalone runtime library.
It is an early standard-library layer and documentation anchor. The future
runtime ABI, startup model, traps, allocation strategy, and platform services
remain separate design work.

## Build and run driver

The temporary native driver uses Clang as an external toolchain:

```text
inox --build file.inox
inox --run file.inox
```

`--build` emits textual LLVM IR and a native executable under `build/inox-artifacts/`.
`--run` builds the same controlled artifacts and executes the resulting native
program. Override this artifact directory with `INOX_OUTPUT_DIR`.

Clang must be installed and available in `PATH` for `--build` and `--run` in the
current implementation. Running a prebuilt `inox.exe` for parsing, semantic
checking, dumps, or `--emit-llvm` does not require Clang, LLVM, or a C++ compiler.

## Blocks and statement syntax

Inox uses `;` to close blocks. It is not a general statement terminator.

`End`/`end` is not part of Inox syntax. It is not a keyword and must not be accepted as a block closer; only `;` closes Inox blocks.

Named subroutine/function blocks use `:`:

```inox
Sum(A Integer, B Integer) Integer :
    Return A + B
;
```

Many control structures do not use `:`. The newline after the header opens the body.

## Declarations

### Type

`Type` is a section/declarator, not a block. It has no `:` and no closing `;` of its own.

```inox
Type
    TPoint Struct
        FX Integer
        FY Integer
    ;

    TMonthRange Range 1..12

    TCardSuit (Club, Diamond, Heart, Spade)
```

`Struct` and multi-line `Enum` open blocks and therefore close with `;`. `Range` is a simple line declaration and does not use `;`.

### Local variables and Var

A local declaration introduces a new symbol. The canonical typed inline form is:

```inox
X Integer := 10
```

`Var` opens a local variable declaration block and closes with `;`. It does not use `:`.

```inox
Var
    X Integer := 10
;

X := X + 1
```

Declarations and assignments are deliberately distinct:

```inox
X Integer := 2  == declaration of a new local X
X := 2          == assignment to an existing X
```

Local variables are mutable by default. A declaration is visible only from its declaration point to the end of the current block.

Shadowing is forbidden. A declaration must not reuse a name already visible in the current scope or in an outer scope, including names that differ only by case.

Valid assignment to an outer variable:

```inox
Main :
    X Integer := 1

    if true
        X := 2
    ;
;
```

Invalid shadowing:

```inox
Main :
    X Integer := 1

    if true
        X Integer := 2
    ;
;
```

Variables declared inside `if`, `elif`, `else`, `while`, `repeat`, `for`, a `case` arm, `try`, `except`, or `finally` do not escape that block. Use before declaration is an error.

### Const and State

`Const` declares constants. Mutable global state must be explicit via `State`. Global mutable state should be rare and visible.

## Functions, subroutines, Return, and Exit

A function has a return type:

```inox
Sum(A Integer, B Integer) Integer :
    Return A + B
;
```

A subroutine has no return type:

```inox
PrintValue(X Integer) :
    PutLn(X)
;
```

`Main` is a subroutine at language level:

```inox
Main :
    PutLn("hello")
;
```

Rules:

- `Return Expression` is required in functions with return values.
- `Return Expression` is forbidden in subroutines without return values.
- `Exit` terminates the current subroutine without a value.
- `Exit` is allowed only in subroutines without return values and in `Main`.
- `Exit` is forbidden in functions with return types.
- Falling through the end of a subroutine is allowed.
- Functions must not fall through without returning a value.

## Types

Canonical built-in names include:

- `Integer` = `Int64`;
- `UInteger` = `UInt64`;
- `Float` = `Float64`;
- `Bool` is the boolean type; `Boolean` is not canonical;
- `Char` is a Unicode scalar value;
- `String` is UTF-8, immutable, non-null;
- `Currency` and `Crypto` are exact decimal domains reserved for precise finance and cryptoasset work.

`Decimal` is not a built-in 0.1 type.

Generics use square brackets:

```inox
Vector[Integer]
Set[TCardSuit]
Array[1..10] Integer
```

## Structs and associated methods

Struct syntax is:

```inox
Type
    TPoint Struct
        FX Integer
        FY Integer
    ;
```

Rules:

- `Struct` is a reserved word.
- `Struct` opens the struct body.
- `;` closes the struct.
- Struct declarations contain fields only.
- Structs do not declare methods.
- Structs do not repeat method signatures.
- Struct type names conventionally begin with `T`.
- Struct fields conventionally begin with `F`.
- Those naming conventions are style rules in 0.1, not fatal errors.

Fields may have literal defaults for supported types:

```inox
Type
    TConfig Struct
        FPort Integer := 8080
        FEnabled Bool := true
    ;
```

Structs are value types. Assignment, ordinary parameter passing, and ordinary return values copy the struct value. The backend may use pointers internally for associated receivers, but that does not change the language semantics.

Associated methods are declared outside the struct:

```inox
TPoint.Move(Self mut, DX Integer, DY Integer) :
    Self.FX := Self.FX + DX
    Self.FY := Self.FY + DY
;

TPoint.Sum(Self) Integer :
    Return Self.FX + Self.FY
;
```

The receiver type is implied by the `TPoint.` prefix. Repeating `TPoint` in the `Self` parameter is redundant and not canonical.

Call-site sugar:

```inox
P.Move(3, 7)
PutLn(P.Sum)
```

Conceptually lowers to static associated calls. This is not virtual dispatch and does not create classes, inheritance, interfaces, or subtyping.

`Self` means read-only receiver. `Self mut` means mutable receiver. `Self owned` is reserved for future ownership-consuming methods.

## Mutability and ownership

Rules:

- Parameters are immutable by default.
- Local variables declared inline or in `Var` are mutable.
- Assignment uses `Name := Expression`; declaration uses `Name Type := Expression`.
- Shadowing is forbidden in all local scopes.
- Receiver mutation requires `Self mut`.
- `mut X Integer` for ordinary mutable parameters is reserved for a future version and must be a clear error in 0.1.

Future ownership work includes `Self owned`, `ref X T`, and `ref mut X T`. These are not part of the 0.1 executable subset.

## Control flow

### if / elif / else

```inox
if A > B
    Return A
elif A = B
    Return 0
else
    Return B
;
```

Rules:

- no `then`;
- no `:`;
- one final `;` closes the whole structure;
- no `;` between branches.

### while

```inox
while I > 0
    I := I - 1
;
```

`break` exits the nearest loop. `continue` proceeds to the next iteration.

### repeat / until

`repeat` is a general loop. `until` is an internal exit statement, not the terminator.

```inox
repeat
    Work
    until Done
    MoreWork
;
```

`until Condition` exits the nearest repeat when the condition is true. It may appear at the beginning, middle, or end, and may appear more than once.

### for in range

```inox
for I in A..B
    ...
;

for I in A..B (S)
    ...
;
```

Rules:

- range endpoints are inclusive;
- direction comes from `A..B`;
- `A < B` is ascending;
- `A > B` is descending;
- `A = B` executes once;
- step is always positive;
- step zero or negative is an error if constant, or a runtime trap if dynamic;
- `continue` goes to the step/next iteration;
- `break` exits the loop;
- the iterator is declared implicitly by the `for`;
- the iterator is read-only;
- the iterator is visible only inside the loop body;
- the iterator must not conflict with any already visible symbol;
- two sequential `for` loops may reuse the same iterator name after the first loop has ended;
- a nested `for` must not reuse the same iterator name as an outer loop.

### case

```inox
case Suit
    Club
        PutLn("club")
    Diamond
        PutLn("diamond")
    otherwise
        PutLn("other")
;
```

Single-line arms are allowed:

```inox
case Suit
    Club PutLn("club")
    Diamond PutLn("diamond")
    otherwise PutLn("other")
;
```

Rules:

- no `of`, `when`, `=>`, `:`, or `do`;
- no fall-through;
- `otherwise` is optional;
- for `Enum`, a `case` without `otherwise` must be exhaustive;
- ranges per arm, multi-values with `|`, and `case` as expression are reserved for future versions.

## Operators and numeric semantics

Boolean operators:

```text
and  or  xor  not
```

Integer bitwise operators:

```text
bitand  bitor  bitxor  bitnot  shl  shr
```

`^` is exponentiation and never XOR.

Integer division:

```inox
A div B
A mod B
```

`/` is not integer division. For `Integer` operands, `/` is a compile-time error with a message directing the programmer to use `div`. Future `Float` division uses `/` and lowers to LLVM `fdiv`.

Integer overflow is invalid language behavior. It is not wraparound and not saturation. Constant overflow is always a compile-time error. Debug/checking mode should trap at runtime. Release 0.1 does not promise wraparound. The compiler should not emit LLVM `nsw`/`nuw` until checks and optimization policy are mature.

Implicit conversions are allowed only for safe widening explicitly defined by Inox. Narrowing is never implicit. Explicit conversion uses `TypeName(Expression)`. `Integer(FloatExpr)` truncates toward zero and traps/errors if out of range. Constant narrowing with loss is a compile-time error.

## Arrays, Vector, Range, Enum, Set

### Array

Fixed arrays use explicit ranges:

```inox
Var
    Values Array[1..10] Integer
;
```

Multidimensional:

```inox
Var
    Matrix Array[1..10, 1..10] Integer
;

Matrix[I, J] := 42
```

Arrays are value types. Bounds checking is on by default. `Low(A)`, `High(A)`, and `Length(A)` are compile-time constants for fixed arrays. Array literals are reserved for later.

### Vector

Future dynamic vectors use:

```inox
Var
    Items Vector[Integer]
;
```

`Vector[T]` is dynamic, 0-based, heap/runtime-managed, bounds-checked, and distinct from `Array`. The selected semantic direction is ownership/move: assignment and by-value passing move the vector O(1) and invalidate the source. Deep copy requires `Clone`. There is no implicit aliasing.

### Range

```inox
Type
    TMonthRange Range 1..12
    TLetterRange Range 'A'..'Z'
```

`Range` does not open a block and does not close with `;`.

### Enum

Short form:

```inox
Type
    TCardSuit (Club, Diamond, Heart, Spade)
```

Multi-line form:

```inox
Type
    TDayOfWeek Enum
        Monday
        Tuesday
        Wednesday
        Thursday
        Friday
        Saturday
        Sunday
    ;
```

Enums are nominal and ordinal. There is no implicit conversion to or from `Integer`. Values start at 0 by default. `Ord(E)` returns the ordinal. `TEnum(I)` converts explicitly with bounds check/trap. Enum ranges are valid in `for`.

### Set

```inox
Var
    Suits Set[TCardSuit]
;
```

`Set[T]` is a finite mathematical set over a nominal ordinal base. `T` must be an `Enum` or finite `Range`. `Set[Integer]`, `Set[Float]`, and `Set[String]` are invalid. Sets are value types. Default is empty. Membership uses `in`. Equality uses `=` and `#`. Subset/superset may use `<=` and `>=`. Canonical operations are `Union`, `Intersection`, `Difference`, `SymmetricDifference`, `With`, and `Without`. Literal `[A, B, C]` is planned and contextual, but may be deferred.

## Strings and Char

`String` is UTF-8, immutable, non-null, and has `""` as its zero/default value. There is no null string. Absence is future `Option[String]`.

0.1 string operations:

- string literal;
- local variable `S String := "..."`;
- parameter and return type `String`;
- `Put` and `PutLn` for strings/literals;
- byte-by-byte equality and inequality with `=` and `#`.

`Put` and `PutLn` accept one or more arguments, Delphi/Object Pascal style. Arguments are emitted sequentially; this is not string concatenation and does not allocate a combined intermediate string. `PutLn` appends exactly one newline after the final argument:

```inox
Put("J=", J)
PutLn("Ciclo numero ", J)
PutLn("A", 10, "B", true)
```

Reserved/not implemented in 0.1:

- `S[I]` indexing;
- string concatenation;
- `ByteLength`, `CharLength`, `GraphemeLength`.

`Char` is a Unicode scalar value, not a byte, not an integer, and not a grapheme cluster. It is conceptually 32-bit. Valid values are U+0000..U+10FFFF excluding UTF-16 surrogate range U+D800..U+DFFF. Literals use single quotes: `'a'`, `'é'`, `'😀'`. Surrogate literals are compile-time errors. Conversions among `Char`, `Byte`, and `Integer` are always explicit.

## Errors, exceptions, null, unsafe

Inox has no universal `null` or `nil`. Normal types are non-null by default. Future absence is `Option[T]`. Future recoverable failure is `Result[T, E]`.

Exceptions exist in 0.1 syntax:

```inox
try
    ...
except
    ...
finally
    ...
;
```

`raise` throws or re-raises. Full exception lowering is incremental.

The safe 0.1 core has no raw pointers, no `Pointer[T]`, no `unsafe`, and no direct C interop. Those belong behind explicit future unsafe/interop boundaries.

## Contracts, protocols, behaviors

Contracts/protocols/behaviors are future static capability checks. They are not Java interfaces, not abstract classes, not mixins, not duck typing, and not copied Rust traits. A type satisfies a contract by providing required operations, likely associated methods. Satisfaction may be explicit or derivable later, but struct declarations must not duplicate method signatures.

## Implementation status and conformance gaps

This manual is the canonical design target. The current compiler is a pre-alpha implementation and may lag the spec. Known conformance work includes:

- full `case` lowering and enum exhaustiveness checks;
- module exports, visibility, and package search beyond local `Module`/`Use`;
- full enum, range, set, array, vector, and char implementation;
- checking-mode integer overflow traps;
- final runtime ABI instead of temporary `printf` lowering;
- formal ownership/borrow/arena/unsafe design for 0.2+.

When implementing a conformance item, update code, tests, canonical docs, manual HTML, and ADRs together.

## Empty parentheses are forbidden

Inox does not use C/Java-style empty parentheses. Parentheses exist only when
there is at least one parameter or argument.

Canonical declarations without parameters:

```inox
Main :
    PutLn("Hello")
;

PrintReport :
    PutLn("report")
;
```

Canonical calls without arguments:

```inox
PrintReport
Account.Print
```

Invalid forms:

```inox
Main() :
;

PrintReport() :
;

Account.Print()
```

Calls and declarations with parameters still use parentheses:

```inox
Add(A Integer, B Integer) Integer :
    Return A + B
;

PutLn(42)
Account.Deposit(100)
```

## License

The Inox compiler, standard library, runtime components, official examples,
tests and tools are licensed under the Mozilla Public License Version 2.0
(MPL-2.0). The project intentionally does not use the MPL 2.0 "Incompatible With
Secondary Licenses" notice.

See `LICENSE`, `NOTICE.md`, `AUTHORS.md`, `CONTRIBUTING.md` and `TRADEMARK.md` at
the repository root.
