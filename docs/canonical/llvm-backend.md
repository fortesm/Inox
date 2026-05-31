# Inox 0.1 pre-alpha - LLVM Backend

This document is the canonical LLVM backend strategy for Inox 0.1 pre-alpha.

It is based on `AGENTS.md`, `docs/canonical/syntax.md`,
`docs/canonical/semantics.md`, `docs/canonical/type-system.md`, and
`docs/canonical/runtime.md`.

## Official Backend

LLVM is the official backend for Inox.

Inox 0.1 pre-alpha has no alternative canonical backend.

## Native Compilation

Inox 0.1 pre-alpha targets native compilation through LLVM.

The compiler lowers Inox programs to LLVM IR, then relies on LLVM for target
code generation.

The exact object format, linker invocation, and platform ABI details are not
specified in this document.

## Lowering Pipeline

The backend strategy is:

1. Parse Inox source.
2. Build an AST.
3. Perform semantic analysis and type checking.
4. Lower the checked AST to LLVM IR.
5. Use LLVM to produce native code.

The AST must already reflect canonical Inox semantics before LLVM IR is
generated.

## Primitive Type Mapping

The canonical primitive LLVM mappings for Inox 0.1 pre-alpha are:

- `Bool` -> `i1`
- `Int8` -> `i8`
- `Int16` -> `i16`
- `Int32` -> `i32`
- `Int64` -> `i64`
- `UInt8` -> `i8`
- `UInt16` -> `i16`
- `UInt32` -> `i32`
- `UInt64` -> `i64`
- `Float32` -> `float`
- `Float64` -> `double`
- `Char` -> `i32`

`Integer` maps as `Int64`.

`UInteger` maps as `UInt64`.

`Float` maps as `Float64`.

## Signed and Unsigned Operations

Signedness is defined by the LLVM operations selected by the backend, not by a
different integer storage type.

For example, signed and unsigned integer types of the same bit width may share
the same LLVM integer representation, while comparisons, division, remainder,
extension, and overflow checks use the signed or unsigned LLVM operation
appropriate to the Inox type.

## Runtime Types

Some Inox types are runtime types in 0.1 pre-alpha.

`String` is a runtime type.

The runtime supports `String` as UTF-8.

`Currency` is a runtime type.

The runtime supports `Currency` as an exact monetary decimal, never `Float64`.

`Crypto` is a runtime type.

The runtime supports `Crypto` as an exact high-precision decimal, never
`Float64`.

The LLVM-level representation of `String`, `Currency`, and `Crypto` is not
specified in this document.

## Exceptions

Inox exceptions are lightweight and must not depend on heavy RTTI.

The LLVM backend must support:

- `raise`
- re-raise from an active exception handler
- `try`
- `except`
- `finally`

The exact LLVM exception mechanism, personality function, landing pad strategy,
or alternative lowering strategy is not specified in this document.

## Runtime Checks

The LLVM backend must lower required runtime checks for:

- `RangeError`
- `IndexError`
- `DivisionByZero`
- `OverflowError`
- `IOError`

`OverflowError` is raised for arithmetic overflow in safe mode.

The exact check-lowering patterns are not specified in this document.

## Memory Management

Inox 0.1 pre-alpha has no heavy garbage collector.

The LLVM backend must not assume a heavy GC runtime.

The detailed ownership, allocation, and lifetime model is not specified in this
document.

## RTTI and Reflection

Inox 0.1 pre-alpha has no heavy RTTI.

Inox 0.1 pre-alpha has no reflection.

The backend must not require heavy RTTI or reflection metadata to implement the
0.1 runtime model.

## Optimization

LLVM passes may be used for future optimization.

The 0.1 pre-alpha backend should remain small and compilable.

No specific optimization pipeline is canonical yet.

## Out of Scope

This document does not define:

- an alternative backend
- a JIT backend
- a bytecode backend
- a complete ABI
- object file or linker rules
- exact runtime type layouts
- a heavy GC strategy
- heavy RTTI metadata
- reflection metadata
- a canonical LLVM optimization pipeline

## Host Portability

The Inox compiler implementation targets portable C++20. The current textual LLVM IR backend is host-independent and should build with MSVC on Windows and GCC or Clang on Linux through CMake. Host-specific behavior belongs in CMake or test scripts unless a future runtime feature genuinely requires a platform API boundary.

Validation commands:

Windows:

```powershell
cmake --build build --config Debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

Linux:

```bash
cmake --build build
./scripts/run-tests.sh
```


## Temporary textual runtime lowering

The current textual LLVM backend includes a deliberately small runtime lowering for early smoke tests:

- `PutLn(Integer)` lowers to an external C `printf` declaration and an internal integer newline format string.
- User-defined subroutines without return values lower to LLVM `void` functions.
- Statement calls to user-defined subroutines lower to LLVM `call void` instructions.
- This is a temporary backend mechanism for validating executable-style I/O paths.
- It does not yet define the final Inox runtime ABI.
- `ReadLn`, buffering, Unicode and platform-specific console behavior remain future runtime work.

### Temporary output lowering

- `PutLn(Integer)` and `Put(Integer)` currently lower through `printf`.
- `PutLn(Bool)` and `Put(Bool)` currently lower Bool values through a `select i1` to internal `true`/`false` string constants.
- `PutLn(String literal)` and `Put(String literal)` currently lower string literals to private LLVM string constants.
- User subroutines without return values currently lower to `define void @name(...)` and end with `ret void`.
- Calls to those subroutines are allowed as statements and lower to `call void @name(...)`.
- This is temporary smoke-test lowering, not the final Inox runtime ABI.

## Struct Lowering

The current textual LLVM backend supports a first simple struct subset. Canonical Inox:

```inox
Type
    TPoint Struct
        FX Integer
        FY Integer
    ;
```

lowers to an LLVM aggregate type similar to:

```llvm
%tpoint = type { i64, i64 }
```

Local struct variables are lowered with `alloca`, default zero initialization via `zeroinitializer`, and field access through `getelementptr`. The implemented backend subset supports `Integer` and `Bool` fields, field assignment, field reads, and restricted associated methods with an explicit struct receiver lowered as a pointer parameter. General struct parameters, struct returns, embedding, tags, variant structs, and defaults for individual fields remain out of scope for this milestone.


### Associated Method Lowering

The current textual LLVM backend supports a restricted associated-method form:

```inox
TPoint.Move(Self TPoint, DX Integer, DY Integer) :
    Self.FX := Self.FX + DX
    Self.FY := Self.FY + DY
;
```

A call such as `P.Move(3, 7)` is lowered as a direct call to `@tpoint.move`, with the local struct storage for `P` passed as the explicit receiver pointer:

```llvm
call void @tpoint.move(ptr %p, i64 3, i64 7)
```

A value-returning method such as `P.Sum()` is lowered similarly:

```llvm
%tmp = call i64 @tpoint.sum(ptr %p)
```

This is direct static dispatch for the current 0.1 subset. It is not virtual dispatch and does not imply classes, inheritance, interfaces, or subtyping.
