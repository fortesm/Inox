# AGENTS.md

## Canonical Truth

The official Inox specification is under:

docs/canonical/

Historical discussions are reference material only.

## Specification Hierarchy

Use this hierarchy when modifying Inox:

1. `AGENTS.md` gives operational rules for agents.
2. `docs/canonical/language-reference.md` is the consolidated tutorial/reference for humans and agents.
3. Topic-specific canonical files under `docs/canonical/` refine the language by area.
4. `grammar/grammar.ebnf` is the human grammar mirror and must remain aligned with syntax decisions.
5. Examples and tests demonstrate implemented subsets.

If a proposed implementation requires a language decision not covered by these documents, stop and ask before changing code. Do not fill gaps by imitating Pascal, Go, Rust, Java, Python, or C++ unless that behavior is explicitly approved for Inox.

## Rules for AI Agents

- Do not invent semantics.
- Do not change syntax without explicit approval.
- Inox is case-insensitive.
- ; closes blocks and is equivalent to End.
- ; is not a statement terminator.
- : opens named blocks.
- Parentheses in conditions are optional.
- `if`, `elif`, and `else` do not use `then` or `:`.
- A line break after an `if` or `elif` condition opens its branch.
- A line break after `else` opens its branch.
- A single `;` closes the complete `if`/`elif`/`else` structure.
- `;` must not appear between `if`, `elif`, and `else` branches.
- Shadowing is forbidden.
- Inox is strongly typed, Ada/ObjectPascal-like.
- LLVM is the official backend.
- Keep 0.1 pre-alpha small and compilable.
- Keep `docs/site/index.html` and `docs/canonical/language-reference.md` aligned when resolving language questions.


## Cross-platform Engineering

- The compiler must remain portable C++20.
- Supported development hosts are Windows/MSVC and Linux/GCC or Clang.
- Prefer standard C++ and CMake over host-specific code.
- Do not add platform `#ifdef`s unless a real platform API boundary requires them.
- Keep platform differences in CMake or scripts where possible.
- Validate Windows changes with `cmake --build build --config Debug` and `pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1`.
- Validate Linux changes with `cmake --build build` and `./scripts/run-tests.sh`.
- When committing, add only task-scoped files explicitly and never use `git add .`.

## Inox 0.1 pre-alpha Consolidated Decisions

- Integer = Int64.
- Float = Float64.
- Currency and Crypto exist.
- Decimal does not exist.
- The canonical boolean type is Bool. Boolean is not a built-in type.
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
- `Type` is a section/declarator, not a block: it has no `:` and no closing `;`.
- Canonical struct syntax is `TName Struct ... ;`; the `;` closes the Struct itself.
- Struct declares fields only. Associated methods are declared outside Struct. Literal defaults for Integer and Bool fields are allowed. Simple structs are value types: ordinary struct assignment, ordinary struct parameters, and ordinary struct returns copy the struct value. Associated-method receivers may be lowered as pointers internally but do not create classes or inheritance.
- Associated method syntax is `TType.Method(Self TType, Args...) ReturnType : ... ;`. The receiver parameter is explicit; call-site sugar is `Value.Method(args)`.
- Struct type names conventionally begin with `T`; struct fields conventionally begin with `F`, but 0.1 does not reject other names.
- Boolean logical operators are and, or, xor, and not.
- Integer bitwise operators are bitand, bitor, bitxor, bitnot, shr, and shl.
- ^ is exponentiation and is never XOR.
- break and continue are loop statements.
- Return Expression returns a value. Exit takes no expression.
- repeat opens a general loop and is closed by ; or End.
- until Condition is a statement inside repeat, equivalent to a conditional
  exit from the nearest repeat.
- until may appear more than once and at any position inside repeat.
- repeat without until is an explicit infinite loop.
- until is invalid outside repeat and its condition must be Bool.

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
- Unary operators are +, -, not, and bitnot.
- Multiplicative operators are *, /, div, and mod.
- Additive operators are + and -.
- Shift operators are shl and shr.
- Integer bitwise operators, from highest to lowest precedence, are bitand,
  bitxor, and bitor.
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
- Sys.Std provides Length and Ord.
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


## Current LLVM backend I/O milestone

The textual LLVM backend currently supports temporary `printf`-based `Put`/`PutLn` lowering for `Integer`, `Bool`, and string literals. It also supports user-defined subroutines without return values and statement calls to those subroutines. This is not the final runtime ABI; preserve it only as an executable smoke-test path until the real runtime is introduced.
