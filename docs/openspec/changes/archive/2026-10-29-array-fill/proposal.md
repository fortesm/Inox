## Why

Open question 2 of `revisions.md` (change `enum-range-set`, r3.80): a struct
field of an enum type without a default, and an array element of an enum
type, started at the first literal, an implicit value that the strict rule
for enum variables (CANON-5) forbids everywhere else. Answering it, the
maintainer chose the strict rule for every array, with a minimal operation
to fill one, and renamed the set functions `With`/`Without`, since `with`
is a reserved word.

Approval (Marcelo Fortes, 2026-10-18):

- On enums in aggregates: "Entre as três alternativas originais, eu
  escolheria a estrita como regra definitiva, acompanhada de um mecanismo
  simples para inicialização de arrays." The mechanism: "uma operação mínima
  de preenchimento uniforme, inspirada no (others => valor) de Ada e no
  [valor; tamanho] de Rust, sem precisar implementar imediatamente todos os
  literais de arrays."
- The law, in his words: "Todos os arrays da linguagem Inox, independentemente
  do tipo de seus elementos, exigem inicialização explícita na declaração. A
  expressão Fill(Value) inicializa integralmente o array de destino com o
  valor especificado, observando as regras de tipos, cópia e construção. A
  inicialização implícita com zero, falso, primeiro literal de enum ou
  defaults dos componentes não é permitida para o array como um todo."
- On Fill: "Recomendo que Fill(X) seja uma expressão de inicialização
  contextual, cujo tipo e dimensões sejam determinados pelo array de
  destino"; "Fill(X) deve produzir um array completamente inicializado, e
  não depender de um array preexistente sem valor definido"; "A regra também
  deve funcionar para arrays multidimensionais e tipos compostos, respeitando
  as regras de cópia e construção dos elementos."
- On structs: "Eu manteria separada a regra já existente para inicialização
  de estruturas por defaults de seus campos. Isso permite preservar: P
  TPoint".
- On the set functions: "A função with deveria mudar de nome"; "Minha
  preferência seria Include(S, X), acompanhado de Exclude(S, X),
  substituindo o par atual With/Without." With a reservation: "em
  Pascal/Delphi, Include e Exclude modificam diretamente o conjunto passado
  como argumento. Se, na Inox, forem funções puras que retornam um novo
  valor, essa diferença deverá ficar expressa no contrato." The contract is
  expressed by the compiler: a call that drops the new set is an error that
  shows `S := Include(S, X)` (proposed in the same conversation, not
  objected to).

Risk: normal (an initialization rule, a new expression lowered with the
existing element loop, and a rename; no exception lowering, runtime ABI or
memory ownership change).

Status: **implemented** (canon r3.81 and r3.82, one PR).

Approval of r3.82 (Marcelo Fortes, 2026-10-18), answering the question this
change first left open: "Eu fecharia as duas questões aplicando um princípio
único à Inox: a inicialização explícita não pode ser contornada pelo fato de
um valor estar dentro de uma Struct, de um Array ou de um tipo genérico."
For array fields: "exigir := Fill(...) no campo ou o fornecimento explícito
do array durante a construção"; for type parameters: "um campo cujo tipo é
um parâmetro genérico T não possui default implícito, independentemente do
tipo concreto utilizado na instanciação"; and "Estruturas aninhadas somente
admitem construção por defaults quando todos os seus componentes puderem ser
inicializados conforme as regras aplicáveis. A ausência de um campo
obrigatório constitui erro de compilação. Nenhuma especialização genérica
pode introduzir silenciosamente um valor padrão que não seja garantido pelo
contrato do tipo genérico." Decision: "aprovar ambas as correções no mesmo
PR, incluindo os testes de estruturas aninhadas e especializações
genéricas." His examples write `TBox<TStatus>(FValue := Pending)`; type
arguments are never written in an expression (r3.77), so the tests write
`A TBox<TStatus> := TBox(FValue := Pending)`.

## What Changes

- **Every local and State array is initialized where it is declared**,
  whatever its element type. `A Array[1..3] Integer` is "an array needs an
  initializer (CANON-5, r3.81): A Array[1..3] Integer := Fill(0)"; the
  message suggests the Fill for the element type (`Fill(Club)`,
  `Fill(False)`, `Fill("")`, `Fill([])`, `Fill(None)`).
- **`Fill(Value)`** is an array whose type and bounds come from its target:
  a declaration, a State initializer, an assignment, an argument or a
  returned value. Without one (`X := Fill(1)`) it is an error. Value is evaluated once and copied into
  every element: a Range element checks it ("value out of range"), a String
  element holds its own reference, a struct element is a copy of a struct
  value (`Fill(Origin)` with `Origin TPoint`, or `Fill(TPoint(...))`). In a
  multi-dimensional array Value is an element (`Fill(0)`) or a whole row
  (`Fill(Row)`). An array of owners is filled only with None. `Fill` is a
  predefined name (CANON-2), like IIf: it cannot be declared.
- **Initialization is transitive (r3.82).** A struct field of an enum,
  array or type-parameter type has no implicit value: it has a default
  (`FSuit TSuit := Club`, `FData Array[1..4] Byte := Fill(0)`, a literal of
  the innermost element, `None` or `[]`) or is given in the construction.
  A struct takes its type defaults (`P TPoint`, State, `New(TName)`) only
  when every component has one, nested structs and parts included;
  otherwise it is an error naming the field. A field whose type uses a type
  parameter (`FValue T`, `FItems Array[1..3] T`) never has a default, in
  any instance (`D TBox<Integer>` is an error). Scalar, String, Set and
  pointer fields without default keep their zero, "", empty and None.
- **`Include(S, X)` and `Exclude(S, X)`** replace `With` and `Without`.
  `With(S, X)` and `Without(S, X)` are rejected with the new names. The six
  set functions never change their arguments; calling one as a statement is
  an error ("Include returns a new set and does not change its argument;
  write S := Include(S, ...)").

Migrated: every program of the suite and the examples (arrays filled; three
test routines named `Fill` renamed).

Both questions this change first left open (array fields of structs, and
fields of a type parameter instantiated with an enum) are answered by r3.82.

## Capabilities

### Modified Capabilities
- `language/fixed-arrays`: arrays are initialized where they are declared;
  Fill.
- `language/enums`: enums in aggregates.
- `language/sets`: Include and Exclude; set functions as statements.
- `language/module-state`: State arrays are initialized.
- `language/structs`: type defaults only when complete; required fields.
- `language/generics`: a field of a type parameter has no default.

## Impact

- Lexer: `Fill` is a predefined name.
- Parser: `with(` in an expression is the old set function, rejected with
  the new name.
- Parser: an array field default `:= Fill(Literal)`.
- Analyzer: `SemanticArrays.cpp` (Fill, the array diagnostic); the
  initialization rule for locals and State; required fields
  (`requiredFieldWithoutDefault`, used by locals, State, New and
  construction); fields of a type parameter in generic instances; the set
  functions renamed and rejected as statements.
- Emitter: `FunctionEmitter::emitFill` (FunctionEmitterArrays.cpp) and
  `emitArrayFillDefault` (FunctionEmitterAggregates.cpp).
- Constitution (CANON-2, CANON-5, CANON-13), the four capability specs,
  grammar.ebnf, docs/site/index.html, CHANGELOG.md, docs/STATUS.md,
  `tools/string_leak_check.py` (array-fill tests).
