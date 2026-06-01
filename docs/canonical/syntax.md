# Inox Syntax Canonical Reference

This document mirrors the syntax rules in `docs/canonical/language-reference.md`. The language reference is the tutorial; this file is the concise syntax checklist.

## Blocks

- `;` closes blocks.
- `;` is not a statement terminator.
- `End`/`end` is not a keyword and is not a block closer.
- Function and subroutine declarations use `:` to open their body.
- `if`, `elif`, `else`, `while`, `for`, `case`, `repeat`, `until`, `Type`, and `Var` do not use `then`, `do`, or Pascal-like `of` unless explicitly stated here.

## Module and Use

```inox
Module My.App
Use Other.Module
```

`Module` has no `;`. EOF terminates the module. `Use` is a semantic dependency, not textual inclusion.

## Type

`Type` has no `:` and no closing `;`.

```inox
Type
    TPoint Struct
        FX Integer
    ;

    TRange Range 1..10

    TEnum (A, B, C)
```

## Var

`Var` has no `:` and closes with `;`.

```inox
Var
    X Integer := 10
;
```

## Struct

```inox
TName Struct
    FField Type
    FOther Type := DefaultLiteral
;
```

## Associated method

```inox
TPoint.Move(Self mut, DX Integer, DY Integer) :
    ...
;

TPoint.Sum(Self) Integer :
    Return ...
;
```

`Self TPoint` and `Self mut TPoint` are not canonical.

## if / elif / else

```inox
if Condition
    ...
elif Other
    ...
else
    ...
;
```

## repeat / until

```inox
repeat
    ...
    until Done
    ...
;
```

## for

```inox
for I in A..B
    ...
;

for I in A..B (S)
    ...
;
```

## case

```inox
case Expression
    Value
        ...
    otherwise
        ...
;
```

Single-line arms are allowed: `Value Statement`.

## Enum

```inox
Type
    TCardSuit (Club, Diamond, Heart, Spade)

    TDayOfWeek Enum
        Monday
        Tuesday
    ;
```

## Range

```inox
Type
    TMonthRange Range 1..12
```

Range declarations are line declarations and do not close with `;`.

## Empty parentheses

Empty parentheses are not valid Inox syntax. If a declaration has no parameters,
omit parentheses. If a call has no arguments, omit parentheses.

```inox
Main :
    PutLn("Hello")
;

Report :
    PutLn("report")
;

Report
```

Invalid:

```inox
Main() :
;

Report()
```
