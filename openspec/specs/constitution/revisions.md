<!-- SPDX-License-Identifier: MPL-2.0 -->
# Inox revisions

Language version: Inox 0.2.0 pre-alpha
Canon revision: r3.80
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
2. **Enums in aggregates** (change `enum-range-set`, r3.80): a struct field
   of an enum type without a default, and an array element of an enum
   type, start at the first literal (implemented, as every aggregate starts
   at its type's defaults); the alternative is to require a default for
   enum fields and to forbid enum arrays until array literals exist.
3. **`| |`, `{ }` and annotations** (brainstorm, 2026-10-16):
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
