# Inox Language Tutorial and Reference

Current 0.1 pre-alpha tutorial for users, compiler contributors, and release testers.

Updated for block-closing semicolons, no empty parentheses, variadic output, local scope, no shadowing, implicit read-only for iterators, release packages, `INOX_STDLIB`, and `INOX_OUTPUT_DIR`.

**Contents**

1.  [Overview](#overview)
2.  [Quick start: Windows prebuilt package](#quick-start-windows)
3.  [Quick start: Linux prebuilt package](#quick-start-linux)
4.  [Prebuilt package layout](#release-layout)
5.  [Toolchain profiles](#toolchain-profiles)
6.  [Compiler modes](#compiler-modes)
7.  [Standard library discovery and output directory](#stdlib-output)
8.  [Lexical rules](#lexical-rules)
9.  [Modules and Use](#modules)
10. [Blocks and statement syntax](#blocks)
11. [No empty parentheses](#no-empty-parens)
12. [Declarations, assignment, and local scope](#declarations)
13. [Functions, subroutines, Return, and Exit](#functions)
14. [Types and numeric semantics](#types)
15. [Structs and associated methods](#structs-methods)
16. [Control flow](#control-flow)
17. [Std.IO and variadic output](#output)
18. [Arrays, vectors, ranges, enums, and sets](#arrays-sets-future)
19. [Runtime and implementation status](#runtime-status)
20. [Design stance](#design-stance)
21. [Building the compiler from source](#build-from-source)

## Overview

Inox is a strongly typed, compiled programming language for high-integrity software. It is designed around explicit syntax, strong static typing, predictable semantics, composition-first design, and LLVM-oriented compilation.

Inox is post-object-oriented: it does not use classes, inheritance trees, Java-style interfaces, duck typing, or mixins as its foundation. The language direction is based on structs as data, associated methods as behavior, composition over inheritance, explicit mutability, and future contracts/protocols/behaviors for statically checked capabilities.

The current compiler is a pre-alpha implementation of the 0.1 language direction. It supports a restricted executable subset, textual LLVM IR emission, a Clang-backed native build/run path, regression tests, standard-library discovery, and prebuilt release packages for Windows and Linux.

## Quick start: Windows prebuilt package

Download the latest Windows package:

<https://github.com/fortesm/Inox/releases/latest/download/inox-windows-x64.zip>

    Expand-Archive .\inox-windows-x64.zip -DestinationPath C:\Tools
    cd C:\Tools\inox-windows-x64

    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
    .\set-inox-env.ps1

    inox .\examples\hello.inox
    inox --emit-llvm .\examples\llvm-put-output-basic.inox

The setup script configures `PATH`, `INOX_STDLIB`, and `INOX_OUTPUT_DIR` for the current PowerShell session.

## Quick start: Linux prebuilt package

Download the latest Linux package:

<https://github.com/fortesm/Inox/releases/latest/download/inox-linux-x64.zip>

    unzip inox-linux-x64.zip -d "$HOME/tools"
    cd "$HOME/tools/inox-linux-x64"

    source ./set-inox-env.sh

    inox ./examples/hello.inox
    inox --emit-llvm ./examples/llvm-put-output-basic.inox

The setup script configures `PATH`, `INOX_STDLIB`, and `INOX_OUTPUT_DIR` for the current shell session.

## Prebuilt package layout

Windows packages use this layout:

    inox-windows-x64/
        README.md
        set-inox-env.ps1
        bin/
            inox.exe
        stdlib/
        examples/
        output/
        manual/
            index.html
        licenses/

Linux packages use this layout:

    inox-linux-x64/
        README.md
        set-inox-env.sh
        bin/
            inox
        stdlib/
        examples/
        output/
        manual/
            index.html
        licenses/

The `stdlib/` directory must stay with the compiler. The `manual/index.html` file is this tutorial.

## Toolchain profiles

| Profile                                              | Needs Clang/LLVM? | Needs a C++ compiler?       | Notes                                                                                              |
|------------------------------------------------------|-------------------|-----------------------------|----------------------------------------------------------------------------------------------------|
| Inox compiler developer                              | Yes               | Yes                         | Builds `inox.exe` or `inox` from the C++ source code.                                              |
| User who only wants to run a prebuilt compiler       | No                | No                          | Needs only the Release executable and the normal operating-system/runtime libraries.               |
| User who wants to use `inox --build` or `inox --run` | Yes, for now      | Not to develop the compiler | The current driver delegates final native executable generation to external tools such as `clang`. |

## Compiler modes

The current executable supports these user-visible modes:

    inox file.inox
    inox --parse-only file.inox
    inox --dump-tokens file.inox
    inox --dump-types file.inox
    inox --emit-llvm file.inox
    inox --build file.inox
    inox --run file.inox

Parsing, semantic checking, dumps, and LLVM IR emission do not require Clang when using a prebuilt compiler. `--build` and `--run` currently require `clang` in `PATH`.

## Standard library discovery and output directory

The compiler searches the standard library in this order:

1.  `INOX_STDLIB`, when set.
2.  `stdlib/` next to the release package root when the executable is under `bin/`.
3.  `stdlib/` next to the executable.
4.  `stdlib/` under the current working directory.
5.  `stdlib/` in the source file directory or one of its parent directories.

The native build/run driver writes artifacts to `INOX_OUTPUT_DIR` when set. Otherwise it uses `build/inox-artifacts/` relative to the current working directory.

    # Windows
    $env:INOX_STDLIB = "C:\Tools\inox-windows-x64\stdlib"
    $env:INOX_OUTPUT_DIR = "C:\Tools\inox-windows-x64\output"

    # Linux
    export INOX_STDLIB="$HOME/tools/inox-linux-x64/stdlib"
    export INOX_OUTPUT_DIR="$HOME/tools/inox-linux-x64/output"

## Lexical rules

Inox is case-insensitive for keywords and identifiers. Documentation uses canonical spelling, but the compiler treats these as equivalent:

    PutLn
    putln
    PUTLN

Line comments use `==`:

    X := 10  == comment until end of line

String literals use double quotes. Character literals use single quotes. `Char` is a Unicode scalar value, not a byte and not a grapheme cluster.

## Modules and Use

A source file may start with a module declaration:

    Module Calc.Core

    Use Std.IO
    Use Std.Math

`Use` declares semantic dependencies. It is not textual inclusion and not a C-style `#include`. Standard modules are resolved through the standard-library search path.

## Blocks and statement syntax

Inox uses `;` to close blocks. It is not a general statement terminator.

    Main :
        if true
            PutLn("yes")
        ;
    ;

Named subroutine/function blocks use `:`. Control structures such as `if`, `while`, `repeat`, and `for` do not use `:` or `do`.

## No empty parentheses

Empty parentheses are forbidden for no-argument declarations and no-argument calls.

Correct:

    Main :
        PutLn("hello")
    ;

    Account.Print

Incorrect:

    Main() :
        PutLn("hello")
    ;

    Account.Print()

## Declarations, assignment, and local scope

A typed local declaration introduces a new local symbol:

    X Integer := 10

An assignment updates an existing symbol:

    X := 20

These are different operations. A declaration cannot hide an already visible symbol. Shadowing is forbidden.

Invalid:

    Main :
        X Integer := 1

        if true
            X Integer := 2
        ;
    ;

Correct:

    Main :
        X Integer := 1

        if true
            X := 2
        ;
    ;

Symbols declared inside a block are local to that block. Use before declaration is an error.

## Functions, subroutines, Return, and Exit

A function has an explicit return type and returns a value with `Return Expression`:

    Add(A Integer, B Integer) Integer :
        Return A + B
    ;

A subroutine has no return type:

    PrintReport :
        PutLn("report")
    ;

`Exit` is reserved for early return from subroutines without a value. Functions with a return type must use `Return Expression`.

## Types and numeric semantics

Current and planned canonical scalar types include `Integer`, `Float`, `Bool`, `String`, `Char`, `Natural`, `Currency`, and future domain-oriented numeric types.

- `Integer` is intended as Int64.
- `Float` is intended as Float64.
- `String` is a non-null UTF-8 value type direction; the empty string is the base/default value, not absence.
- `Char` is a Unicode scalar value.

Integer `/` is rejected. Use `div` for integer division and `mod` for signed remainder. Integer overflow is not a promised wraparound semantic.

## Structs and associated methods

Inox uses structs for data:

    Type
        TPoint Struct
            X Integer
            Y Integer
        ;

Associated methods are declared outside the struct. The receiver uses `Self`; mutable receivers use `Self mut`:

    TPoint.Sum(Self) Integer :
        Return Self.X + Self.Y
    ;

    TPoint.Move(Self mut, DX Integer, DY Integer) :
        Self.X := Self.X + DX
        Self.Y := Self.Y + DY
    ;

This preserves `Object.Method` ergonomics without classes or inheritance.

## Control flow

`if` uses no `then` and no `:`:

    if X > 0
        PutLn("positive")
    elif X < 0
        PutLn("negative")
    else
        PutLn("zero")
    ;

`while` uses no `do` and no `:`:

    while X > 0
        X := X - 1
    ;

`repeat` is flexible; `until` is an internal conditional exit:

    repeat
        PutLn(X)
        X := X + 1
    until X > 10
    ;

`for` ranges are inclusive. The iterator is implicit, read-only, local to the loop body, and cannot shadow a visible symbol:

    for I in 1..10
        PutLn(I)
    ;

    for I in 10..1 (2)
        PutLn(I)
    ;

Sequential loops may reuse the same iterator name because their scopes do not overlap.

## Std.IO and variadic output

`Put` and `PutLn` accept one or more arguments. Arguments are emitted sequentially; this is not string concatenation and does not allocate a combined intermediate string. `PutLn` appends exactly one newline after the final argument.

    Put("J=", J)
    PutLn("Ciclo numero ", J)
    PutLn("A", 10, "B", true)

The current backend lowers these facilities through the runtime/compiler path used by the LLVM smoke-test backend. This is not the final language ABI.

## Arrays, vectors, ranges, enums, and sets

The 0.1 direction defines or reserves several Pascal/Ada-inspired aggregate concepts:

- Fixed arrays use explicit ranges: `Array[Low..High] Type`.
- Vectors are planned as dynamic, 0-based, bounds-checked collections.
- Ranges are finite ordinal declarations.
- Enums are nominal and ordinal, with no implicit conversion to/from `Integer`.
- Sets are planned as finite sets over ordinal base types, inspired by Pascal and Ada discipline.

Some of these features are language direction rather than complete current compiler implementation.

## Runtime and implementation status

Inox does not yet provide a complete standalone language runtime. The `stdlib/Std.*.inox` files are early standard-library modules and documentation anchors, not a complete native runtime such as `libinoxrt.a`, `inoxrt.lib`, or `inox_runtime.dll`.

The current compiler executable is a native C++ program. The current native build/run path emits textual LLVM IR and delegates final executable generation to external Clang.

## Design stance

Inox rejects unsafe defaults:

- universal null;
- implicit narrowing;
- integer wraparound guarantees;
- unchecked bounds;
- implicit aliasing;
- classes;
- inheritance;
- Java-style interfaces.

Future work includes modules, arrays, vectors, sets, contracts, protocols, behaviors, arenas, borrowing, unsafe boundaries, and structured parallelism.

## Building the compiler from source

Windows development build:

    cmake -S . -B build -G "Ninja Multi-Config" -DCMAKE_CXX_COMPILER=clang++
    cmake --build build --config Debug
    pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1

Linux development build:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
    cmake --build build
    bash scripts/run-tests.sh

Expected test result:

    Summary: 142 passed, 0 failed, 142 total
