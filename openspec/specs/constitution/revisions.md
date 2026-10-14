<!-- SPDX-License-Identifier: MPL-2.0 -->
# Inox revisions

Language version: Inox 0.2.0 pre-alpha
Canon revision: r3.70
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

1. **`generics` design** (Risk: high): `openspec/changes/generics/design.md`,
   rewritten in its simplest form (Marcelo Fortes, 2026-10-13: "da forma
   mais simples possível e depois podemos evoluir"): no constraints or
   protocols, inference only, one copy per type; three decisions.
2. **`safe-pointers` design** (Risk: high):
   `openspec/changes/safe-pointers/design.md`, five decisions (counted
   `Ref<T>`, mutation through a shared Ref, names, arenas in phase 2, the
   unsafe layer later). Depends on `generics`.
3. **`binary-streams` design** (Risk: high):
   `openspec/changes/binary-streams/design.md`, six decisions for binary
   files (a `BinaryFile` type, little-endian, `Write`/`Read` sized by the
   type, Strings in binary files, reading past the end, 0-based byte
   offsets); streams come after `generics`.

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
