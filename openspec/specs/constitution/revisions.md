<!-- SPDX-License-Identifier: MPL-2.0 -->
# Inox revisions

Language version: Inox 0.2.0 pre-alpha
Canon revision: r3.64
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

1. **`std-strings` design** (Risk: high): approve
   `openspec/changes/std-strings/design.md` — concatenation without repeated
   copies, the block header, positions and the trap "string index out of
   range", the Unicode tables — before any code is written.
2. **Calling a function and ignoring its result.** `Twice(2)` or `Value`
   (functions with a result) written as statements pass the analyzer and the
   backend reports "not yet implemented". Should Inox allow it (the result is
   thrown away, as in C and Delphi) or reject it, as Ada and standard Pascal
   do ("a function call is not a statement")?

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
