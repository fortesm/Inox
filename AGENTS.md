# AGENTS.md

## Canonical Truth

The official Inox specification is under:

docs/canonical/

Historical discussions are reference material only.

## Rules for AI Agents

- Do not invent semantics.
- Do not change syntax without explicit approval.
- Inox is case-insensitive.
- ; closes blocks and is equivalent to End.
- ; is not a statement terminator.
- : opens named blocks.
- Parentheses in conditions are optional.
- Shadowing is forbidden.
- Inox is strongly typed, Ada/ObjectPascal-like.
- LLVM is the official backend.
- Keep 0.1 pre-alpha small and compilable.

## Inox 0.1 pre-alpha Consolidated Decisions

- Integer = Int64.
- Float = Float64.
- Currency and Crypto exist.
- Decimal does not exist.
- Generics use [].
- Shadowing is forbidden.
- State is used for mutable global state.
- Inline var and Var blocks coexist.
- Arrays have explicit ranges.
- Vectors are dynamic and 0-based.
- Sets follow Pascal/Ada style with a finite ordinal base.
- Exceptions exist since 0.1.
- Exception syntax includes try, except, finally, and raise.
- Exceptions are lightweight, without heavy RTTI.
- Casts use TypeName(expr).
- Implicit conversions are allowed only for safe widening.
- Parentheses in conditions are optional.
- Counted range loops use for I in 1..10.
- Stepped counted range loops use for I in 1..10(2).
- case follows Ada/SPARK style.
- Module closes at EOF.
- The automatic prelude includes Sys.IO, Sys.Math, and Sys.Std.
- Line comments use ==.
- Block comments do not exist in 0.1.

## Case Semantics

- case follows Ada/SPARK style.
- case has no fallthrough.
- case does not use break.
- case accepts individual values and ranges.
- otherwise is mandatory when the case is not exhaustive.
- For enums, if all values are covered, otherwise is not mandatory.

## Operator Precedence

- Parentheses have maximum precedence.
- Calls, indexing, and member access come after parentheses.
- ^ is exponentiation and associates to the right.
- Exponentiation has higher precedence than unary operators.
- -x^2 means -(x^2).
- Unary operators are +, -, and not.
- Multiplicative operators are *, /, div, and mod.
- Additive operators are + and -.
- Range operator is ..
- Membership operator is in.
- Relational operators are =, #, <, >, <=, and >=.
- Logical operators are and, xor, and or.
- Assignment := is right-associative.
- Chained assignment is allowed.
- Assignment inside a boolean expression is forbidden.

## Scope

- Inline var declares in the current block.
- Var blocks declare in the current block.
- Names declared inside if, while, for, repeat, case, or try only exist inside
  that block.
- Shadowing is forbidden in all scopes.
- Global Const is allowed.
- State is the only mechanism for mutable global state.

## Prelude

- Sys.IO provides Put, PutLn, and ReadLn.
- Sys.Math provides Sin, Cos, Sqrt, and Abs.
- Sys.Std provides Length.
- The automatic prelude exposes these names without explicit Use.

## Currency

- Currency is exact monetary decimal.
- Currency is never Float64.
- Currency is intended for international fiat money.
- Detailed rounding semantics belong in runtime.md.

## Crypto

- Crypto is exact high-precision decimal.
- Crypto is never Float64.
- Crypto is intended for cryptoassets.
- Detailed support for scales and networks belongs in runtime.md.
