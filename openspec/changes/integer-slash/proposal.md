## Why

Inox had two division operators: `/` for reals and decimals, `div` for
integers, and integer `/` was a compile error naming `div`. Ada uses `/` for
both and lets the operands' type decide; Inox already never mixes an
integer and a real without a conversion, so the type always decides
without ambiguity.

Approval (Marcelo Fortes, 2026-10-23): "valtando ao div e / vamos eliminar
a palavra resrvada div / Ada usa o caractere `/` tanto para a divisão
inteira quanto para a divisão real (ponto flutuante). O compilador decide
qual operação realizar com base no tipo dos operandos envolvidos. Divisão
Inteira (`Integer / Integer`): Se os dois valores envolvidos forem do tipo
`Integer` (inteiros), o operador `/` realiza a divisão inteira, truncando
qualquer parte decimal em direção ao zero. Divisão Real (`Float / Float`):
Se os valores forem decimais (`Float` ou outro tipo de ponto
flutuante/fixo), o operador `/` realiza a divisão aritmética real
convencional. Nota de segurança: Por ser uma linguagem com tipagem
extremamente forte, Ada não permite misturar tipos diretamente (como `5 /
2.0`) sem realizar uma conversão de tipo explícita / o mesmo pode ser
imiplementado em Inox?"

Risk: normal (every integer division in the repository is rewritten; the
generated IR of each is unchanged).

Status: **implemented** (canon r3.95).

## What Changes

1. **Lexer.** `div` leaves the reserved words (41 remain); it is an
   ordinary name.
2. **Parser and AST.** The multiplicative level is `*`, `/`, `mod`; the
   operator IntegerDivide is gone.
3. **Analyzer.** `/` of two integers of one type (Integer, a fixed width,
   UInt64, Natural, a Range) is the integer division, truncated toward zero,
   of that type; of two reals or two decimals, theirs; mixed operands are
   rejected as before. `/=` follows. Constant folding and the constant zero
   divisor (now "constant division by zero in '/'") apply to integer `/`
   only; `7 / 2` where a Currency is expected stays a decimal division.
4. **Emitter.** Integer `/` uses the checked helpers `div` used (sdiv, udiv
   for UInt64; traps on zero and Int64.Min / -1). The IR of every program
   that wrote `div` is unchanged.
5. **Code.** All `div` in stdlib, examples and tests become `/`; tools
   (calc_differential, backend_gaps, mutation_fuzz) generate `/`.
6. **Tests.** New: runtime integer-slash; diagnostics slash-mixed-types,
   div-is-a-name. Removed: semantic/invalid/integer-slash and
   diagnostics/compound-integer-divide (both now valid); decimal-div
   becomes decimal-mod.
7. **Law and docs.** Constitution CANON-2, CANON-5, CANON-8, CANON-19,
   CANON-20, CANON-23; capability specs decimals and fixed-width-integers;
   grammar.ebnf; site, README, CHANGELOG, STATUS, BACKEND_GAPS.

## Open point

`mod` keeps the sign of the dividend (`-7 mod 2` is -1): Ada's `rem` and
Delphi's `mod`. Ada's `mod` follows the divisor (`-7 mod 2` is 1). Left as
it is; the maintainer decides whether to follow Ada here too.
