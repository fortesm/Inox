# Inox 0.1 pre-alpha - Runtime

This document is the canonical runtime specification for Inox 0.1 pre-alpha.

It is based on `AGENTS.md`, `docs/canonical/syntax.md`,
`docs/canonical/semantics.md`, and `docs/canonical/type-system.md`.

The runtime for 0.1 pre-alpha is intentionally small and compilable.

## Startup

Program startup calls `Main()`.

The detailed command-line argument model, process environment model, and exit
code model are not specified in this document.

## Automatic Prelude

The automatic prelude is available without explicit `Use`.

The prelude includes:

- `Sys.IO`
- `Sys.Math`
- `Sys.Std`

Names exposed by the automatic prelude participate in normal name resolution.
Inox is case-insensitive, so prelude names are also case-insensitive.

## Sys.IO

`Sys.IO` provides:

- `Put`
- `PutLn`
- `ReadLn`

`Put` writes output.

`PutLn` writes output followed by a line ending.

`ReadLn` reads one line of input.

The detailed stream model, encoding error behavior, buffering behavior, and
platform-specific line-ending behavior are not specified in this document.

I/O failures raise `IOError`.

## Sys.Math

`Sys.Math` provides:

- `Sin`
- `Cos`
- `Sqrt`
- `Abs`

The detailed numeric domains, precision rules, and exceptional math behavior
are not specified in this document.

## Sys.Std

`Sys.Std` provides:

- `Length`

`Length` returns the length of a supported value.

The exact set of supported value categories for `Length` is not specified in
this document.

## Exceptions

Exceptions exist in Inox 0.1 pre-alpha.

Exception syntax includes:

```inox
try
except
finally
raise
```

Exceptions are lightweight, without heavy RTTI.

The runtime must support raising exceptions with `raise`.

The runtime must support re-raise from an active exception handler.

The detailed syntax of re-raise is not specified in this document.

## Error

`Error` is a lightweight structure or lightweight view of the captured error.

`Error` must not require heavy reflection or heavy RTTI.

The exact fields, construction rules, and formatting rules of `Error` are not
specified in this document.

## Runtime Checks

The runtime supports the following check failures:

- `RangeError`
- `IndexError`
- `DivisionByZero`
- `OverflowError`
- `IOError`

`RangeError` is used for range-check failures.

`IndexError` is used for invalid indexing.

`DivisionByZero` is used for division by zero.

`OverflowError` is used for arithmetic overflow in safe mode.

`IOError` is used for input/output failures.

The detailed payload, hierarchy, and formatting of these errors are not
specified in this document.

## String Runtime

The runtime supports `String` as UTF-8.

The detailed indexing, slicing, normalization, and invalid-byte behavior for
UTF-8 strings are not specified in this document.

## Currency Runtime

The runtime supports `Currency` as an exact monetary decimal.

`Currency` is never `Float64`.

`Currency` is intended for international fiat money.

The detailed rounding semantics for `Currency` belong in this runtime
specification, but are not specified yet.

The internal representation of `Currency` is not specified in this document.

## Crypto Runtime

The runtime supports `Crypto` as an exact high-precision decimal.

`Crypto` is never `Float64`.

`Crypto` is intended for cryptoassets.

Detailed support for scales and networks belongs in this runtime specification,
but is not specified yet.

The internal representation of `Crypto` is not specified in this document.

## Memory Management

Inox 0.1 pre-alpha has no heavy garbage collector.

This document does not specify a full memory-management model.

## Reflection and RTTI

Inox 0.1 pre-alpha has no heavy reflection or heavy RTTI.

Exceptions and `Error` must remain lightweight and must not depend on heavy
RTTI.

## Out of Scope

This document does not define:

- a package manager runtime
- concurrency runtime
- heavy garbage collection
- heavy reflection
- heavy RTTI
- detailed `Currency` rounding rules
- detailed `Crypto` scale and network rules
- a complete I/O stream model
- a complete process or environment model
