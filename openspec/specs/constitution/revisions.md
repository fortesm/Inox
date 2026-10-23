<!-- SPDX-License-Identifier: MPL-2.0 -->
# Inox revisions

Language version: Inox 0.2.0 pre-alpha
Canon revision: r3.93
Maintainer and sole design authority: Marcelo Fortes
Repository: github.com/fortesm/Inox — license MPL-2.0

The two lines above are what `inox --version` must print (with its BUILD);
the suite checks them. VERSIONING in `spec.md` defines both numbers. This
page is not locked: it is the living record of the constitution. Revision
lines are append-only (G3); open questions leave the list when the
maintainer answers them, and the answer becomes a revision.

## Open questions awaiting the maintainer

None of these is permission to choose (G4); until answered, the current text
stands.

1. **`binary-streams` design** (Risk: high):
   `openspec/changes/binary-streams/design.md`, six decisions for binary
   files (a `BinaryFile` type, little-endian, `Write`/`Read` sized by the
   type, Strings in binary files, reading past the end, 0-based byte
   offsets); streams come after `generics`.
2. **`| |`, `{ }` and annotations** (brainstorm, 2026-10-16):
   `openspec/changes/delimiters-and-annotations/proposal.md` — `| |` as a
   capture clause (`Name`, `Move(X)`, `Link(X)`), `{ }` as an effect or a
   requirement clause, `@` annotations; no String interpolation and `{ }`
   reserved (decided, r3.74).

## Revision log

Entries v1.0 to r3.60 are in `docs/history/canon-changelog.md` (frozen).
Each new line: revision — date — what changed — approval — OpenSpec change.

- r3.61 — 2026-10-12 — The canon becomes this constitution:
  `docs/INOX_CANONICAL.md` is retired (last text: git tag `canon-final`;
  change log in `docs/history/`), its law minimized here (`spec.md`,
  `engineering.md`), locked by `constitution.lock` and
  `tools/constitution_check.py`; the capability specs become law under it;
  stale status, duplicates and superseded text dropped (list in the
  change). Marcelo Fortes, 2026-10-12: "o arquivo canonica.md pode
  sobreviver minimizado e devidamente higienizado dentro de
  `openspec/specs/constitution/`, Ele deve ser editado por você para
  refletir as suas necessidades, e remover todas as incosistências,
  incoerências e redundâncias desnecessárias". Change `constitution`.
- r3.62 — 2026-10-12 — Three decisions. (1) `State` has no `:` and keeps
  its closing `;`; the forms `State :`, `Name : Type` and `Const Name : Type`
  are rejected; the constitution records the idea of the syntax and of `:`
  in the maintainer's words (Marcelo Fortes: "O State não deve usar :"; "A
  ideia de Inox é escrever menos. Fazer mais."; 2026-10-13: "State fecha com
  ;"). Change `state-without-colon`.
  (2) `Length` of a String and `Ord` of a Char are rejected, as the law
  already said (Marcelo Fortes: "Excelente não pode ter isso para char
  mesmo!"). Change `char-not-ordinal`. (3) The roadmap is renumbered for
  the version scheme and E22 names no versions (Marcelo Fortes: "Sim esse
  roadmap é antigo pode renumerar"). Change `roadmap-renumber`.
- r3.63 — 2026-10-13 — The three open questions answered (Marcelo Fortes:
  "Concordo com 1 , 2. 3 escolho a alternativa A"). (1) The driver has
  subcommands: `inox file.inox` and `inox run` build and run, `inox build`,
  `inox check`, `inox emit-llvm`, `inox help`, `inox version`; the `--`
  switches stay as aliases. Change `driver-commands`. (2) An exception
  binding is exempt from the unread-local rule only until exceptions have
  readable members. Change `driver-commands` (law text only). (3)
  `Std.Debug.Assert(Condition)` and `Assert(Condition, Message)` exist; a
  failed Assert is a trap, "assertion failed", exit 70, never compiled out.
  Change `debug-assert`.
- r3.64 — 2026-10-13 — Module pairs: `Std.X` holds a subject's types, data
  and fundamental operations, `Std.XUtils` the conveniences built on them
  (and gives everything of `Std.X`). Std.Files holds the File type and the
  basic operations; Std.IOUtils keeps Exists, DeleteFile, RenameFile.
  Std.Strings decisions: `+` concatenates without repeated copies,
  positions count code points from 1, `Length(S)`, `Substr`, `Trim`,
  `LTrim`, `RTrim`, Delphi-style names (Marcelo Fortes: "Length(S) ewm vez
  de charlenghth (aprovado)"; "Concatenação com + (aprovado)"; "Arquivos
  muda isso também o básico fica em Std.Files e as comodidades para
  Std,IOUtils"). Changes `module-pairs` (implemented) and `std-strings`
  (design awaiting approval).
- r3.65 — 2026-10-13 — Two answers and three designs. (1) The `std-strings`
  design is approved (Marcelo Fortes: "Std.Strings / Std.StrUtils: o design
  espera a sua aprovação (aprovado)"); its implementation may start.
  Change `std-strings`. (2) A function called as a statement (its result
  thrown away) stays as it is: accepted by the analyzer and reported by the
  backend as "not yet implemented" (a backend gap, `docs/BACKEND_GAPS.md`);
  the law adds no rule (Marcelo Fortes: "Deixe como está pois ele diz que
  esse método não foi implementado ainda"). (3) Designs proposed, awaiting
  decisions: changes `generics`, `composition`, `safe-pointers`, and
  `binary-streams` (binary files and streams, which CANON-17 leaves to a
  later change).
- r3.66 — 2026-10-13 — Method reuse. `TName Struct (TPart, ...)` reuses
  structs: each part is a field named after its type and brings its fields
  and methods, at every level; a reused method runs on its part; a name
  reached by two paths, or hidden, is an error at the declaration; a method
  replaces a reused one only with `Override`; no subtyping. Built-in value
  types accept methods (`String.UpdateStr(Self mut, ...)`), and a field of
  `Self mut` may be passed as `ref mut`. CANON-1, CANON-9 and CANON-10 say
  so; FUTURE no longer plans nominal protocols or traits. Conformance fixes:
  `Self` and value parameters cannot be written, `with` follows its target,
  a `Self mut` method needs a writable receiver. The `composition` design
  (embedding, protocols) is replaced. Marcelo Fortes: "troque herança por
  reaproveitamento de métodos isso já satifaz!"; "Aprovo tudo como você
  sugeriu 1a, 2a, 3b e 4a+b". Change `method-reuse`.
- r3.67 — 2026-10-14 — Design by contract, as in Eiffel (CANON-24): `pre`
  and `pos` on the first lines of a routine (`old X`, `Result`), `invariant`
  in a Struct (with the invariants of the structs it reuses), `pre else` and
  `pos then` in an Override, `invariant` and `variant` in loops. A broken
  contract is a CANON-19 trap; contracts are always checked and only read.
  Differences from Eiffel recorded in the change (trap instead of exception,
  no monitoring levels, invariants after outside writes, messages instead of
  tags). FUTURE now names generics and static proof. Marcelo Fortes:
  "implemente design by contract exatamente igual eiffel a diferença é que
  as palavras reservadas pre para precondições e pos para poscondições".
  Change `design-by-contract`.
- r3.68 — 2026-10-14 — Contracts are checked in every build through 1.0,
  with no switch that turns them off (CANON-24); Eiffel-style monitoring
  levels chosen per module are FUTURE work after 1.0, next to static proof.
  Marcelo Fortes: "Adicionar estilo de níveis de monitoramento Eiffel-style
  [...] per módulo mais tarde (implementação futura) e manter os checks
  always on [...] para a versão 1.0". Change `design-by-contract`.
- r3.69 — 2026-10-14 — Variant structs (a value that is one of several
  structs, examined with `case`) are recorded in FUTURE as the way a
  collection mixes kinds without subtyping; behavior stays written once per
  type through method reuse (`examples/reuse-army.inox`). Marcelo Fortes:
  "Sim, registre isso". Change `variant-structs` (direction; design after
  `enum-range-set`).
- r3.70 — 2026-10-14 — New FUTURE "Encapsulation, routine types and
  lifecycle": encapsulation from a design close to Object Pascal and Rust
  ("algo bem semelhante a do ObjectPascal + Rust mas precisamos desenhar
  ainda"); routine types (Pascal procedural types) recorded with the
  recommendation of named routines first and closures after safe pointers;
  constructors and destructors left to the maintainer's analysis
  ("preciso analisar o caso dos construtores e destrutores, e se
  precisaremos"). Marcelo Fortes: "Sim registre no future para não nos
  perdermos a questão do encapsulamento. Registre tudo isso para eu não me
  perder ou esquecer". Changes `encapsulation`, `routine-types`,
  `lifecycle` (directions); `docs/OOP_COMPARISON.md`.
- r3.71 — 2026-10-15 — Safe pointers without reference counting, garbage
  collection or arenas: every heap object has one owner (`Own<T>`, created
  by `New`, freed at the end of its block or by `Free`, never copied, given
  away with `Move`), and every other reference is a `Handle<T>` checked by a
  generation on each use (`Alive` tests it; a dead Handle traps; a Handle
  is born empty). Arenas may come later as an optional optimization.
  Marcelo Fortes: "se usarmos contagem por referência, ficamos igual a
  Delphi e isso tem problemas. então essas estariam descartadas"; "não
  quero ter de forma alguma, garbage collection"; "depois podemos ter arena
  no futuro opcionalmente"; "Acatado: um `Handle` pode simplesmente nascer
  vazio". Change `safe-pointers` (design awaiting approval, no code).
- r3.72 — 2026-10-16 — Safe pointers, completed: a non-owning reference
  may point to any variable, field or array element, not only to an owned
  object ("1 - Com Handle"); it may go anywhere, and one that outlives its
  variable is detected, never dangling ("2 - permissivo com detecção"); it
  is named `Link<T>`, not Handle ("Trocar para Link e não confundir com o
  Handle do Delphi ou do Windows"); `Value`, `New` and `Free` keep their
  names; an absent owner is `Option<Own<TNo>>` ("acatado o dono com
  Option<Own<TNo>>"). Marcelo Fortes. Change `safe-pointers` (design
  awaiting approval, no code).
- r3.73 — 2026-10-16 — Safe pointers become law (CANON-25: Owners, Moves
  are checked, Links, Access through pointers, Owners in routines and
  contracts; CANON-19 points to the trap categories of CANON-24 and
  CANON-25; FUTURE Ownership updated) and are implemented: `Own<T>`,
  `Option<Own<T>>`, `Link<T>`, `None`, `New`, `Free`, `Move`, `Value`,
  `Link`, `Alive`, `Same`, a definite-move analysis, generation-checked
  Links to owned objects, locals, fields, elements and State, and the
  runtime traps "use of a freed object or an empty Link" and "use of an
  owner after Move or Free". Approval: Marcelo Fortes, 2026-10-16: "Tudo
  aprovado, prossiga como o usual". Change `safe-pointers`.
- r3.74 — 2026-10-16 — Fewer reserved words (50 → 41): the last arm of a
  `case` is `else`, not `otherwise`; `True`, `False`, `Exit`, `bitand`,
  `bitor`, `bitxor` and `bitnot` are predefined names that keep their
  meaning and can never be declared (free as field and method names);
  `Main` is an ordinary name; `step`, `on` and `mut` stay reserved. `{` and
  `}` are reserved until generics and concurrency show their use. `div` and
  `/` stay as they are; `Float(Soma / N)` as the exact quotient is a FUTURE
  direction. Marcelo Fortes, 2026-10-16. Change `keyword-review`.
- r3.75 — 2026-10-16 — CANON-11 says what `with` does with its target: a
  variable or a value, never a type, evaluated once on entry; `.Member`
  acts on its storage and, through a pointer, is checked at each use. Any
  place is now lowered as a target, and a type as target gets a message
  naming the problem. Marcelo Fortes, 2026-10-16: "concordo Implemtne".
  Change `with-places`.
- r3.76 — 2026-10-16 — Second syntax review: no `Module` header, a file is
  a module named after the file (CANON-3); the conditional expression is
  `IIf(C, A, B)`, only the chosen branch evaluated, replacing `if C then A
  else B` (CANON-12, ADR-0013 superseded); `leave`, `continue` and `IIf` are
  predefined names; `Var`, `Module` and `then` are no longer reserved
  (`then` keeps its meaning in `pos then`); `pre`, `pos`, `old`,
  `invariant`, `variant` and `Override` are reserved (CANON-2). 42 reserved
  words, 10 predefined names. Marcelo Fortes, 2026-10-16. Change
  `syntax-review`.
- r3.77 — 2026-10-17 — Generics in their simplest form (CANON-8, FUTURE):
  a routine or a struct may declare type parameters (`Max<T>(A T, B T) T
  :`, `TPair<K, V> Struct`); each use with its types makes an instance,
  checked and compiled like ordinary code, one copy per list of types. The
  three decisions: no constraints for now (`{ }` stays reserved), type
  arguments inferred from the arguments or the target and never written in
  an expression, and T may be any type, also one that is only moved, with
  the error at the instance ("in Max<TPoint>, used in Main"). Methods of
  generic structs, and invariants and reused parts in them, come later.
  Marcelo Fortes, 2026-10-17: "Acato suas recomendações. Para as três
  perguntas." Change `generics`.
- r3.78 — 2026-10-17 — Std.Strings and Std.StrUtils implemented (CANON-14,
  CANON-18, CANON-19): `+` joins Strings and `+=` appends, in place when
  the String alone holds its block; no implicit conversion to String.
  Std.Strings gives Length, ByteLength, Substr, Position, UpperCase and
  LowerCase; Std.StrUtils adds Trim, LTrim, RTrim, Left, Right, PadLeft,
  PadRight, Replicate, Replace, Contains, StartsWith, EndsWith, SameText and
  CompareText. Positions count code points from 1; case and white space
  follow Unicode. A position outside a String traps with "string index out
  of range" (fault kind 15). `S[I]` and `GraphemeLength` stay reserved.
  `Pos` is a reserved word since r3.76, so the search function is
  `Position`: Marcelo Fortes, 2026-10-17: "Position(Sub, S) (Recommended)".
  Design approved in r3.65. Change `std-strings`.
- r3.79 — 2026-10-18 — UInt64 implemented (CANON-8): UInt64/UInteger holds
  0..2^64-1 everywhere an integer lives; arithmetic, power and shifts trap on
  overflow and below 0, `div`, `mod`, `shr` and comparisons are unsigned,
  `case` orders its choices unsigned (with `else`) and `for` reaches 2^64-1;
  conversions into and out of UInt64 are checked (kind 8); Get, PutLn and
  files print and read it unsigned. Q1 to Q3 as decided in r3.50: a literal
  up to 2^64-1 only in a UInt64 context, no unary minus, no mixing without a
  written conversion. A UInt64 Const is written `Const K := UInt64(...)`.
  Marcelo Fortes, 2026-10-12: "Todas as tomadas de decisão dos pontos 1 a 5
  foram verificadas e eleitas para serem acatadas." Change `uint64`.
- r3.80 — 2026-10-18 — Enum, Range and Set implemented (CANON-5, CANON-13,
  CANON-19): enums are nominal ordinals with Ord, checked TEnum(I), Low,
  High, checked Succ/Pred, comparison, enum `for` and `case` coverage, and
  print their literal's name; a Range of Integer is a subtype checked when a
  value is stored ("value out of range", kind 11) and computes as Integer;
  Set<T> over an Enum or Range of at most 256 values with `in`, =, #, <=,
  >=, the six functions and the literal `[A, B]`. The decisions of
  2026-10-12 (Q1–Q7, r3.50 and r3.51). Enums in aggregates start at the
  first literal (open question 2). Marcelo Fortes, 2026-10-12: "Todas as
  tomadas de decisão dos pontos 1 a 5 foram verificadas e eleitas para
  serem acatadas." Change `enum-range-set`.
- r3.81 — 2026-10-18 — Open question 2 answered: the strict rule for every
  array. Every local and State array, whatever its element type, is
  initialized where it is declared; `Fill(Value)` (a predefined name, like
  IIf) is the array its target expects, every element or row a copy of
  Value, evaluated once, following the rules of types (a Range checks it),
  copy and construction; an array of owners is filled with None. An enum
  struct field needs a default; structs keep their field defaults (`P
  TPoint`). The set functions `With`/`Without` become `Include`/`Exclude`
  (the old names are rejected with the new ones); the six set functions
  never change their arguments, so a call as a statement is an error.
  Marcelo Fortes, 2026-10-18: "Entre as três alternativas originais, eu
  escolheria a estrita como regra definitiva, acompanhada de um mecanismo
  simples para inicialização de arrays"; "Todos os arrays da linguagem
  Inox, independentemente do tipo de seus elementos, exigem inicialização
  explícita na declaração. A expressão Fill(Value) inicializa integralmente
  o array de destino com o valor especificado, observando as regras de
  tipos, cópia e construção"; "Eu manteria separada a regra já existente
  para inicialização de estruturas por defaults de seus campos"; "Minha
  preferência seria Include(S, X), acompanhado de Exclude(S, X),
  substituindo o par atual With/Without." Array fields of structs stay open
  (question 3). Change `array-fill`.
- r3.82 — 2026-10-18 — Open question 3 answered: initialization is
  transitive over aggregates and type parameters (CANON-5, CANON-9). A
  struct field of an enum, array or type-parameter type has no implicit
  value: it has a default (a literal, or `Fill(Literal)` for an array
  field) or is given in the construction, whose omission is an error. A
  struct takes its type defaults (a declaration without `:=`, State,
  `New(TName)`) only when every component has one, nested structs and
  parts included. A field whose type uses a type parameter never has a
  default, whatever the instance (`D TBox<Integer>` is an error). Scalar,
  String, Set and pointer fields keep their zero, "", empty and None.
  Marcelo Fortes, 2026-10-18: "a inicialização explícita não pode ser
  contornada pelo fato de um valor estar dentro de uma Struct, de um Array
  ou de um tipo genérico"; "Nenhuma especialização genérica pode introduzir
  silenciosamente um valor padrão que não seja garantido pelo contrato do
  tipo genérico"; "aprovar ambas as correções no mesmo PR, incluindo os
  testes de estruturas aninhadas e especializações genéricas." Change
  `array-fill`.
- r3.83 — 2026-10-19 — Char values (CANON-14, CANON-5): a Char literal
  takes the escapes of Strings and its quote (`'\n'`, `'\''`), never a
  surrogate; `Char(I)` is checked (kind 8), `Integer(C)` gives the code
  point and `String(C)` the one-character String, with no implicit
  conversion either way; Chars compare by code point and form ranges for
  `for`, `case` and `in`; a Char Range holds Chars (checked stores, kind 11,
  Low and High, sets over it); no arithmetic, `Ord`, `Succ` or `Pred`;
  `CharAt(S, I)` in Std.Strings (kind 15 outside); Put, PutLn and WriteLn
  write the character in UTF-8; a Char field has a default or is given in
  the construction (r3.82); Chars live in locals, parameters, results,
  fields, State, arrays (stored in 32 bits), Consts and generic instances.
  `Get(C)` comes later. Marcelo Fortes, 2026-10-19, on the eight
  recommendations: "Ok, então, neste caso, está tudo acatado. Prossiga com
  nosso padrão." Change `char-values`.
- r3.84 — 2026-10-21 — Float conversions and Float Consts (CANON-8, CANON-5):
  `Float(I)` and `Float32(I)` round an integer (a UInt64 read unsigned);
  `Float(G)` widens a Float32 exactly and `Float32(F)` rounds, trapping
  ("value out of range", kind 11) when a finite F is beyond Float32;
  `T(F)` into an integer type truncates toward zero and traps (kind 8) for
  NaN, the infinities and values outside T, as the law already said; a
  conversion into a number takes a number (Bool, String and enum rejected
  with the way to write them); a real literal is a Float32 where a Float32
  is expected; real Consts are folded at compile time, and Std.Math's Pi,
  Tau and E become Consts. Marcelo Fortes, 2026-10-19, on the plan to do
  "as conversões de Float e as Consts de Float" next: "Conbinado,
  prossiga". The choices marked in the change's proposal await his
  confirmation (open question 3). Change `float-conversions`.
- r3.85 — 2026-10-21 — `case` on a String (CANON-12, ADR-0011): the
  selector may be a String; each choice is a String literal or a String
  Const, compared byte for byte as `=` compares; no ranges; the same String
  twice is an overlap; `else` (Ada's `others`) is always required; a Float
  is never a selector. Marcelo Fortes, 2026-10-19, on the proposal (String
  yes with these rules, Float no): "Sim acatado !"; on `others`: `else` is
  that arm (r3.74). Change `case-strings`.
- r3.86 — 2026-10-21 — Open question 3 answered, the floating-point policy
  (CANON-8): `Float32(F)` rounds as IEEE 754 (ties to even), a loss of
  precision being no error; a finite F whose rounding overflows stops the
  program (kind 11), never an infinity or a saturation; a constant beyond
  Float32 is a compile error; NaN and the infinities keep their kind, and
  underflow gives a subnormal or a signed zero without error; Float32
  expressions are computed in Float32. A real literal is a Float32 where a
  Float32 is expected, rounded once from its decimal text, never through a
  Float64; a typed value never changes precision implicitly. A Const whose
  value is a real literal (or its negation) is universal, as Ada's named
  numbers and Go's untyped constants; Std.Math's Pi, Tau and E are such
  Consts, written with more digits than either type holds. An unchecked
  conversion to an infinity (`Float32Unchecked`) is an idea, not approved.
  Marcelo Fortes, 2026-10-21: "Eu adotaria um modelo verificado para
  Float32(F), inspirado em Ada, mas com regras mais determinísticas";
  "Eu confirmaria essa decisão sem alterações" (literals); "Prefiro
  claramente o modelo Ada/Go" (constants); "Não faria Float32(F)
  simplesmente saturar". Change `float-policy`.
- r3.87 — 2026-10-21 — `!=` is inequality, the same operator as `#`, both
  valid; a lone `!` is not an operator, negation is `not` (CANON-2).
  Marcelo Fortes, 2026-10-21: "Deve ser adotado o operador != paralelo ao
  operador #, ambos são válidos"; "if ! (a = b) é inválido deve ser o
  operador not". Change `not-equal-operator`.
- r3.88 — 2026-10-21 — The console and Std.Terminal (0.3). E1: the console
  belongs to libinoxrt and every program links it; Put, PutLn, Get, GetLn
  and Std.Terminal reach it only through the runtime; on Windows the
  console is UTF-8 with VT sequences while the program runs (CANON-17,
  CANON-19). E2: a function or method with a result is not a statement, as
  in Ada, with no way to drop a result (CANON-7). F1: `Std.Terminal` —
  RawMode, ReadKey (TKey: Kind, Ch, Ctrl, Alt), ScreenSize, MoveTo,
  ClearScreen, ClearLine, SetColor/ResetColor (16 colors), ShowCursor,
  Flush; ReadKey past the end of input traps "end of input" (CANON-18,
  CANON-19). F2: the terminal is given back however the program ends.
  Marcelo Fortes, 2026-10-21: "E1 Sim / E2 como em Ada / F1 Sim para tudo
  Std.Terminal é o nome padrão / F2 Sim / F3 Sim / Faça em Pr Serparados
  para ser mais fácil de testar". Change `console-abi-terminal`.
- r3.89 — 2026-10-21 — Reading characters and reals, Char classes, real
  exponents (0.3). B5: a real literal may end with an exponent after its
  fraction (`1.5e3`, `2.0E-7`); `1e3` is an error; a real literal beyond
  Float64 is a compile error (CANON-2). C1: `Get(C)` reads the next
  character exactly, spaces and line ends included; `EndOfInput`; past the
  end, "end of input" (CANON-17). C2: `Get` reads Float and Float32
  strictly; "invalid number input" is a new trap category (CANON-17,
  CANON-19); Currency and Crypto follow with their values. C3: Std.Strings
  classifies a Char by Unicode (IsLetter, IsDigit, IsSpace, IsUpper,
  IsLower, IsPunctuation) and maps its case (CANON-14). Marcelo Fortes,
  2026-10-21: "A Acatado todos / B Acatado todos / C Acatado todos".
  Change `input-chars-exponents`.
- r3.90 — 2026-10-21 — Numbers and text (0.3). B1: `String(N)` and
  `String(F)` write a number as Put does; `FormatFloat(F, Decimals)`
  (Std.Strings). B2: `Integer(S)`, every integer type, `Float(S)` and
  `Float32(S)` read the whole text; other text raises the new exception
  `ConversionError` (CANON-8, CANON-15). B3: a real is written as the
  shortest text that reads back as the same value of its type (`0.1`,
  `2.5`, `1500.0`, `1.0e16`), everywhere a real is written (CANON-17). B4:
  Split and Join move to 0.5 with the collections; `FieldCount(S, Sep)` and
  `Field(S, Sep, I)` (Std.StrUtils) now (CANON-14). Marcelo Fortes,
  2026-10-21: "B Acatado todos". Change `numbers-text`.
- r3.91 — 2026-10-22 — Currency and Crypto have values (0.3, CANON-8). A1:
  `*` and `/` of decimals are exact and then rounded half to even to the
  type's places. A2: a literal next to a decimal takes its type (`Price *
  3`); an Integer variable needs `Currency(N)`. A3: `Integer(C)` truncates
  toward zero; Std.Math gives `Round` (half to even) and `Trunc`. A4:
  `Crypto(C)` is exact; `Currency(X)` of a Crypto rounds half to even,
  without error. A5: a decimal is written with at least two decimals and no
  trailing zeros after them. `Get`, `String(C)` and `Currency(S)` read and
  write decimals (B1, B2, C2); "decimal overflow" is a new trap category
  (CANON-19). Marcelo Fortes, 2026-10-21 (repeated 2026-10-22): "A Acatado
  todos". Change `decimals`.
- r3.92 — 2026-10-22 — Dates, the system and the micro-editor (0.3). D1: a
  TDateTime is the microseconds since 0001-01-01 (proleptic Gregorian),
  with TDate and TDuration. D2: two zones, UTC and the system's. D3:
  Std.DateTime (EncodeDate/EncodeDateTime checked, DecodeDate, Year .. Second,
  DayOfWeek, AddDays, AddSeconds, DaysBetween, IsLeapYear, comparison) and
  Std.DateUtils (ISO 8601, FormatDateTime with Delphi's codes). D4:
  Std.SystemUtils (ParamCount, ParamStr, GetEnv, EnvExists, Halt, Sleep,
  CurrentDir, PlatformName); running other programs after 1.0; `inox run`
  gives the words after the file to the program and keeps its exit status
  (CANON-18, CANON-23). F3: `examples/edit.inox` accepts 0.3. Marcelo
  Fortes, 2026-10-21 (repeated 2026-10-22): "D1 Acatado / D2 Acatado / D3
  Acatados todos / D4 R: `ParamCount`, `ParamStr(I)`, `GetEnv(Name)`,
  `EnvExists(Name)`, `Halt(Code)` para sair com um código, `Sleep(Ms)`,
  `CurrentDir` e `PlatformName` / [...] / F3 Sim". Change
  `datetime-systemutils`.
- r3.93 (2026-10-22) — CANON-7 Explicit discard: `_ := Call` drops the
  result of a function or method call on purpose; E2 and OPEN-4 stay
  compile errors. `_` is a discard target without storage, declaration or
  name; only a call with a result may be discarded, never a literal, a
  variable, a field, an operation, a conversion or a construction. A line at
  module level that is no declaration is now an error (it was ignored).
  Marcelo Fortes, 2026-10-22: "Decisão final: opção 2, com `_ :=`,
  restrita inicialmente a chamadas que retornem valor. Manter E2 e OPEN-4
  como erros de compilação. Aprovaria essa alteração em PR separado, com
  testes cobrindo chamada isolada, descarte explícito, variável não lida e
  descarte inválido." Change `discard-results`.
