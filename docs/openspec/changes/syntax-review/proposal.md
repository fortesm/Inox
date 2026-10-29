## Why

A second review of the syntax, after `keyword-review` (r3.74).
Marcelo Fortes, 2026-10-16:

- "Estou seriamente inclinado remover Module da sintaxe seguida de um nome e
  resolver o nome do módulo pelo nome do seu arquivo"; then "Tirar o
  `Module` e usar o nome do arquivo simplifica também o parser".
- "Estou propenso a remover then da sintaxe também, e o seu if se tornar
  iif"; then "`IIf(C, A, B)` com parênteses no lugar de `if C then A else B`
  aprovado".
- "Leave e continue deixam de ser palavras reservadas ficam como o break
  continue do Pascal".
- "with não tem do a palavra reservada do é para paralelismo como o go de
  golang. Se var não é mais usado deixa de ser palavra reservada e fica
  palavra comum para mensagem de migração".
- "Reservadas pre, pos, old, invariant, override"; then "`variant` entra
  nas reservadas sim".

Status: implemented (canon r3.76). Risk: medium (every program had a
`Module` header; the conditional expression changes spelling).

## What Changes

- **No `Module` header** (CANON-3). A file is a module named after the
  file: `Use Calc.Core` loads `Calc.Core.inox` or `Calc/Core.inox`, which is
  the module `Calc.Core`; the entry file may have any name (its stem names
  it). The header was redundant: the compiler already required the declared
  name to match the file ("module name mismatch"). A file that still starts
  with `Module Name` gets "Inox has no 'Module' header: a file is a module
  named after the file (r3.76); remove this line". `Module` is an ordinary
  name. A package (a distributable library, a folder with a manifest) is
  left to the FUTURE package manager.
- **`IIf(C, A, B)`** (CANON-12) replaces `if C then A else B`: three
  arguments, only the chosen branch evaluated (unlike Visual Basic's IIf,
  the reason ADR-0013 had chosen `if then else`), nested for chains.
  `IIf` is a predefined name. `if` in expression position gets a message
  naming IIf. `then` is no longer reserved; it keeps its meaning in
  `pos then` (CANON-24).
- **`leave` and `continue` are predefined names**, as Pascal's `Break` and
  `Continue`: unchanged statements, never declared.
- **`Var` is an ordinary name**; `Var` alone on its line, or before a
  declaration, still gets the migration message of CANON-5.
- **Reserved**: `pre`, `pos`, `old`, `invariant`, `variant`, `Override`
  (CANON-24, CANON-9). Declaring a reserved word as a variable gets "X is a
  reserved word of Inox and cannot be declared".

Result: 42 reserved words (Use Type Const State Struct Enum if elif else
unless while repeat until for in step case try except On do ensure raise
retry with mut ref div mod and xor or not shr shl Return pre pos old
invariant variant Override) and 10 predefined names (True False Exit leave
continue IIf bitand bitor bitxor bitnot).

## Impact

Every `.inox` file loses its `Module` line (629 files, line numbers in
expected diagnostics adjusted); the conditional expressions of the tests
and examples are written with IIf; the tools that write programs
(`backend_gaps.py`, `calc_differential_test.py`, `mutation_fuzz.py`) no
longer write a header.
