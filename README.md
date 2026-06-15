# Correção do __inox_read_i64 — leitor de inteiros seguro (fail-fast)

## Problema corrigido
O leitor de inteiros do Get/GetLn acumulava dígitos com `mul i64 %acc, 10` +
`add` SEM checagem de overflow — violando a regra central da constituição Inox
(CANON-2 / ADR-0006: "no silent integer overflow"). Além disso, entrada inválida
e EOF viravam 0 silenciosamente.

## Solução (sem Result[T,E], conforme decidido para 0.1)
Comportamento fail-fast via `llvm.trap`:
- overflow (positivo ou negativo)  -> trap
- entrada inválida (nenhum dígito)  -> trap
- EOF antes de número               -> trap
- número válido                     -> armazena o valor

Implementação:
- magnitude acumulada como UNSIGNED i64 com deteccao de overflow via
  `@llvm.umul.with.overflow.i64` e `@llvm.uadd.with.overflow.i64`;
- sinal aplicado no fim, com validacao de range conforme o sinal:
  negativo permite magnitude ate 9223372036854775808 (Int64 min);
  positivo permite ate 9223372036854775807 (Int64 max);
- exige ao menos um digito (senao trap);
- EOF inicial -> trap.

O caso dificil `-9223372036854775808` (Int64 min, cuja magnitude nao cabe como
i64 positivo) e tratado corretamente porque a magnitude e acumulada unsigned.

## Critérios de aceite — TODOS PASSARAM (verificado por execução nativa)
    "42"                      -> 42
    "-42"                     -> -42
    "9223372036854775807"     -> OK (Int64 max)
    "-9223372036854775808"    -> OK (Int64 min)
    "9223372036854775808"     -> trap
    "-9223372036854775809"    -> trap
    "abc"                     -> trap
    "" / EOF                  -> trap

## Regressão
- parse+semantic: 39/39 exemplos (inalterado).
- IR válido (LLVM 21 verify): 31 antes, 31 depois — ZERO regressão.
- Os 2 exemplos com IR inválido (llvm-bool-comparisons, llvm-bool-operators) já
  eram inválidos ANTES desta mudança (bug pré-existente: PutLn de Bool passa i1
  onde printf espera i64). NÃO relacionado a esta correção.

## Arquivos
- LlvmIrEmitter.cpp  -> substitui o seu (helper reescrito + intrinsics declarados)
- tests/  -> 6 casos novos de integração:
    get-int64-max (.in/.out)   valor limite positivo
    get-int64-min (.in/.out)   valor limite negativo
    get-overflow-pos (.in/.trap)  deve trapar
    get-overflow-neg (.in/.trap)  deve trapar
    get-invalid (.in/.trap)       deve trapar
    get-eof (.in/.trap)           deve trapar
  (.trap marca os casos que devem abortar; o runner precisa reconhecer essa
   convenção — ver nota abaixo.)

## Nota para o runner de testes
Os casos .trap esperam SIGILL/SIGTRAP (codigo de saida negativo / 132-136). O
script run-tests precisa tratar "trap esperado" como sucesso para esses 4.
