## Why

Marcelo Fortes, 2026-10-16: "Quero resolver ainda a possibilidade de
reduzir palavras reservadas" and "Quero analisar a questão da divisão de
inteiros usando só div e não / pelas nossas regras pois estão me parecendo
tolice pois tanto div quanto / vão dividir exceto que haja uma diferença
da implementação de ambos". This change records the analysis. Nothing is
decided; each removal changes CANON-2, CANON-6, CANON-8 or CANON-20 and
needs approval and a relock.

Status: analysis, no design. Risk: medium (every program and test that
uses a removed word).

## The reserved words today (50, `src/compiler/lexer/Lexer.cpp`)

module use type const var state struct enum main if then elif else unless
while repeat until for in step case otherwise try except on do ensure raise
retry with mut ref div mod and xor or not bitand bitor bitxor bitnot shr
shl leave continue return exit true false

`pre`, `pos`, `old`, `invariant`, `variant`, `Result`, `Self`, `Override`,
`None` and the built-in routines are not reserved: they mean something
only where they appear.

## Candidates (50 → 37)

| Words | Proposal | Saves |
|---|---|---|
| `bitand` `bitor` `bitxor` `bitnot` | `and` `or` `xor` `not` on integers are bitwise, on Bool logical (Pascal, Ada for modular types); the type decides, and mixing is already a parenthesis or type error (CANON-20) | 4 |
| `true` `false` | predefined constants, as in Pascal's System unit | 2 |
| `main` | an ordinary name; the entry point is the routine named Main | 1 |
| `var` | `Var` blocks were removed; keep the migration message without reserving the word | 1 |
| `otherwise` | `else` as the last arm of `case` (Pascal, Delphi) | 1 |
| `exit` | `Return` without a value in a subroutine (today a parse error), as Pascal's `Exit` | 1 |
| `step` `on` `mut` | words only inside one construct (`for ... step`, `except On`, `ref mut`/`Self mut`): not reserved elsewhere | 3 |

Kept: the words that start a construct or an operator with no other
spelling. `do` stays reserved for concurrency.

## Integer division: `div` and `/`

They are two operations, not two spellings: `/` is the quotient in the
reals (`7.0 / 2.0` = 3.5), `div` the integer quotient truncated toward
zero (`7 div 2` = 3, `-7 div 2` = -3; `mod` takes the dividend's sign,
`-7 mod 2` = -1). The rule exists so that `/` never silently drops the
fraction (`Media := Soma / N`, the classic C bug).

Options:

- **A. Keep the rule** (Pascal, Modula-2, Oberon, Python 3's `//`). One
  keyword; the reader always knows that `/` is exact.
- **B. Ada's way**: `/` on two integers truncates, `/` on Floats is exact,
  no `div`. Safe enough in a language without implicit integer-to-Float
  conversion (Inox has none: `M Float := Soma / N` is a type error), but
  `PutLn(Soma / N)` prints 3 where 3.5 may be expected. Saves `div`.
- **C. C's way** without a type error: refused (silent truncation).
