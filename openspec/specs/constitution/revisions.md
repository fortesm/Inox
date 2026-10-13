<!-- SPDX-License-Identifier: MPL-2.0 -->
# Inox revisions

Language version: Inox 0.2.0 pre-alpha
Canon revision: r3.63
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

1. **`std-strings`** (open change). Decided on 2026-10-13: concatenation
   with `+`, without Delphi's copy on every step (a StringBuilder like
   Delphi's if the compiler cannot avoid it); positions start at 1 and count
   code points; `Substr()` as in Java instead of `Copy`; `Trim`, `LTrim`,
   `RTrim`; Delphi-style names for the rest. Open: which functions belong to
   `Std.Strings` and which to `Std.StrUtils` (the maintainer wants usable
   functions in `Std.Strings`, not only definitions), and the remaining
   names.

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
