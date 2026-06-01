# Inox Type System Canonical Reference

## Built-ins

- `Integer` = `Int64`.
- `UInteger` = `UInt64`.
- `Float` = `Float64`.
- `Bool` is canonical; `Boolean` is not.
- `Char` is Unicode scalar value.
- `String` is UTF-8, immutable, non-null.
- `Currency` and `Crypto` are exact decimal domains for future precise monetary/crypto work.

## Conversions

Implicit conversions are allowed only for widening conversions explicitly approved by the language. Narrowing is never implicit. Explicit conversion uses `TypeName(Expression)`.

`Integer(FloatExpr)` truncates toward zero and traps/errors if out of range. Constant narrowing with loss is a compile-time error.

## Struct

```inox
Type
    TPoint Struct
        FX Integer
        FY Integer
    ;
```

Structs are nominal value types. Field defaults for literals are allowed for supported field types.

## Enum

Enums are nominal and ordinal. Short form and multiline form are equivalent. There is no implicit conversion to or from `Integer`. Use `Ord(E)` or `TEnum(I)` explicitly.

## Range

Range type declarations are simple line declarations:

```inox
Type
    TMonthRange Range 1..12
```

## Array

`Array[Low..High] Type` is fixed-size, bounds-checked, and value type. Low and High are part of the type.

## Vector

`Vector[T]` is future dynamic, 0-based, bounds-checked, heap-managed, and ownership/move based. Assignment moves and invalidates the source. `Clone` performs deep copy.

## Set

`Set[T]` requires a finite ordinal nominal base: an Enum or finite Range. It is not a generic hash set.
