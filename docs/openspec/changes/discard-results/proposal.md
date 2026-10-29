## Why

E2 (r3.88, a function is not a statement, as in Ada) met OPEN-4 (a local
must be read): a function called only for its effect could not be called at
all. `F(X)` alone broke E2, and `R := F(X)` without reading R broke OPEN-4.
The open point of change `console-abi-terminal` asked the maintainer what
to do; three options were set out (keep it, `_ :=`, OPEN-4 as a warning).

Approval (Marcelo Fortes, 2026-10-22): "Minha escolha para a Inox é a
opção 2: descarte explícito com `_ := Expressão`. Eu manteria
integralmente a E2 e a OPEN-4, sem transformar erros em avisos. [...] Mas
faria uma distinção importante: `_` não deve ser uma variável especial.
Deve ser um destino sintático de descarte, sem identidade, tipo próprio,
armazenamento ou participação no escopo. [...] Para proteger ainda mais a
OPEN-4, inicialmente restringiria o descarte explícito a resultados de
chamadas de funções ou métodos. [...] Na Inox, eu estabeleceria desde já
que `_ :=` não suprime exceções, falhas de execução, verificações de
contratos ou outras obrigações semânticas."

His wording for the constitution: "E2 — Descarte explícito de resultados.
Uma chamada de função que produza valor não poderá ser utilizada
isoladamente como comando. O resultado deverá ser consumido por uma
expressão válida ou descartado explicitamente mediante `_ :=
FunctionCall`. O símbolo `_` constitui um destino de descarte, não um
identificador de variável. Não possui armazenamento, não introduz
declaração, não participa da resolução de nomes e não está sujeito à
OPEN-4. A expressão à direita deverá constituir uma chamada de função ou
método que produza valor. Sua avaliação ocorrerá exatamente uma vez,
preservando os efeitos observáveis e todas as obrigações semânticas
aplicáveis. O descarte explícito não constitui leitura artificial de outra
variável para fins de satisfação da OPEN-4. Chamadas de procedimentos sem
retorno permanecem válidas como comandos, sem utilização de `_ :=`. Tipos
ou funções sujeitos a obrigações especiais de tratamento de resultados
poderão proibir o descarte, conforme contratos estabelecidos
separadamente."

Decision: "Decisão final: opção 2, com `_ :=`, restrita inicialmente a
chamadas que retornem valor. Manter E2 e OPEN-4 como erros de compilação.
Aprovaria essa alteração em PR separado, com testes cobrindo chamada
isolada, descarte explícito, variável não lida e descarte inválido."

Risk: low (a new statement form; programs without `_` get the same IR).

Status: **implemented** (canon r3.93).

## What Changes

1. **Lexer.** `_` alone is the token Underscore, never an identifier
   (`_x` and `A_B` are still names).
2. **Parser.** `_ := Expression` at the start of a statement is a discard
   statement. `_` anywhere else (an expression, a declaration, a parameter,
   a routine name, `_ +=`, a chain) is "'_' is not a name: it only discards
   the result of a call, _ := F(X) (r3.93)".
3. **Analyzer.** The expression must be a call of a function or a method
   with a result. A literal, a variable (`_ := N` would pass for a read of
   N), a field, an element, an operation, a conversion (`Integer(S)`), a
   construction, or a call without a result is rejected. The E2 message
   now shows both ways: `X := F(...)` and `_ := F(...)`.
4. **Emitter.** The call is emitted as any call whose value is used, once;
   an owned result (String, owner, struct with Strings) is released with
   the statement's temporaries.
5. **Found on the way.** A line at module level that is no declaration
   (`+ Integer : ... ;`, `_ Integer : ... ;`) was parsed as a raw
   declaration and ignored; it is now "expected a declaration: Use, Type,
   Const, State or a routine, Name :".

## Open point

The right of `_ :=` is restricted to calls, as approved; a conversion such
as `Integer(S)` is not a call, so validating a text by discarding its
conversion (`_ := Integer(S)` inside try) is rejected. It can be widened
later if needed.
