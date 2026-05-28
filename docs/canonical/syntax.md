# Inox 0.1 pre-alpha - Syntax

This document is the canonical syntax specification for Inox 0.1 pre-alpha.

The scope of this version is intentionally small. Syntax not described here is
not part of the canonical language yet.

## Canonical Rules

- Inox source code is case-insensitive.
- `;` closes blocks.
- `;` is equivalent to `End` when closing a block.
- `;` is not a statement terminator.
- `:` opens named blocks.
- Parentheses in conditions are optional.
- Generics use `[]`.
- Casts use `TypeName(expr)`.
- Inline `var` and `Var` blocks coexist.
- Counted range loops use `for I in 1..10`.
- Stepped counted range loops use `for I in 1..10(2)`.
- `case` follows Ada/SPARK style.
- `case` accepts individual values and ranges.
- `otherwise` is used for non-exhaustive `case` constructs.
- Assignment uses `:=`.
- `Module` closes at EOF.
- Line comments use `==`.
- Block comments do not exist in 0.1.

## Case Insensitivity

All language syntax is case-insensitive.

The following spellings are syntactically equivalent wherever the same token is
expected:

```inox
End
end
END
```

The same rule applies to all reserved words and language-defined names.

## Blocks

Blocks are explicit syntactic regions.

A named block is opened with `:`.

```inox
Name :
    ...
;
```

The block may also be closed with `End`.

```inox
Name :
    ...
End
```

The following two forms are syntactically equivalent:

```inox
Name :
    ...
;
```

```inox
Name :
    ...
End
```

The semicolon closes the block itself. It does not terminate the previous
statement.

## Modules

A `Module` closes at EOF.

This means a module does not require an explicit final `;` or `End` solely to
close the module itself.

## Semicolon

In Inox 0.1 pre-alpha, `;` has exactly one canonical syntactic role:

- close the current block

It must not be interpreted as a general statement separator or statement
terminator.

Valid block-closing form:

```inox
Name :
    ...
;
```

Non-canonical interpretation:

```inox
Statement;
```

The second form must not be treated as "a statement followed by a terminator"
unless that `;` is closing a block according to the surrounding syntax.

## Named Blocks

The `:` token opens a named block.

The canonical shape is:

```inox
BlockName :
    ...
;
```

The syntax of the block name itself is not fully specified in this document.
Until a lexical grammar is canonical, compilers and tools must not invent extra
block-name syntax beyond what is explicitly approved.

## Conditions

When a syntactic construct contains a condition, parentheses around that
condition are optional.

These two forms are syntactically equivalent:

```inox
Construct condition :
    ...
;
```

```inox
Construct (condition) :
    ...
;
```

This rule only states the optionality of parentheses around conditions. It does
not define the set of conditional constructs or the expression grammar.

## Statements

The canonical restriction is that statements are not terminated by `;`.

Inox 0.1 pre-alpha includes exception statements using:

```inox
try
except
finally
raise
```

The full statement grammar for Inox 0.1 pre-alpha is not specified yet.

## Loops

Counted range loops use:

```inox
for I in 1..10
```

Stepped counted range loops use:

```inox
for I in 1..10(2)
```

The surrounding block form for loops is not specified yet.

## Case

`case` follows Ada/SPARK style.

`case` accepts individual values and ranges.

`otherwise` is used when a `case` is not exhaustive.

The detailed `case` grammar is not specified yet.

## Expressions

The complete expression grammar for Inox 0.1 pre-alpha is not specified yet.

Casts use:

```inox
TypeName(expr)
```

Operator precedence, from highest to lowest, is:

1. Parentheses
2. Calls, indexing, and member access
3. `^`
4. Unary `+`, unary `-`, and `not`
5. `*`, `/`, `div`, and `mod`
6. `+` and `-`
7. `..`
8. `in`
9. `=`, `#`, `<`, `>`, `<=`, and `>=`
10. `and`, `xor`, and `or`
11. `:=`

The `^` operator associates to the right.

Exponentiation has higher precedence than unary operators, so:

```inox
-x^2
```

means:

```inox
-(x^2)
```

Assignment `:=` is right-associative, and chained assignment is allowed.

Assignment inside a boolean expression is forbidden.

## Declarations

The complete declaration grammar for Inox 0.1 pre-alpha is not specified yet.

Inline `var` and `Var` blocks coexist.

`State` is used for mutable global state.

`Const` may be global.

## Generics

Generics use `[]`.

The detailed generic declaration and instantiation grammar is not specified
yet.

## Arrays and Vectors

Arrays have explicit ranges.

Vectors are dynamic and 0-based.

The detailed syntax for array and vector declarations, literals, indexing, and
operations is not specified yet.

## Sets

Sets follow Pascal/Ada style with a finite ordinal base.

The detailed set syntax is not specified yet.

## Lexical Grammar

Line comments use `==`.

Block comments do not exist in Inox 0.1.

At minimum, syntax recognition must respect case-insensitivity for language
tokens.

## Non-goals for This Version

This document does not define:

- import syntax
- concurrency syntax
- package syntax

Those topics require separate canonical decisions before implementation.
