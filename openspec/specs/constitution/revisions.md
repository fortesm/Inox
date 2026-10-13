<!-- SPDX-License-Identifier: MPL-2.0 -->
# Inox revisions

Language version: Inox 0.2.0 pre-alpha
Canon revision: r3.61
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

1. **`State :` and the colon rule.** CANON-4 says `:` only declares a
   routine, yet the State section is written `State :` … `;` (the canon
   always listed it so, and every test uses it). Keep `State :` as the one
   other use of `:`, or write the section like `Type` (no `:`)?
2. **Roadmap numbering.** `docs/ROADMAP.md` keeps the 0.1.x–0.9.x plan of
   2026-06, written before the version scheme of r3.60; with Inox already
   at 0.2.0 and arrays done, should the milestones be renumbered?
3. **Driver commands.** CANON-23 says the driver should converge on `inox
   run/build/check/emit-llvm/help/version`; today it has `--build`, `--run`
   and the other switches. Is the subcommand form still the goal, and when?
4. **Exception bindings and unread locals.** `On E T` bindings are exempt
   from the unread-local rule until exceptions have readable members
   (pending since v3.32). Decide with the first readable member.
5. **`Std.Debug.Assert`.** It waits for "canonical trap/abort behavior";
   DECISION P-A defines traps since v3.18. Should `Assert` be a trap
   ("Inox runtime error: assertion failed"), an exception, or wait for
   contracts?
6. **`std-strings`** (open change): function names, the concatenation
   operator and the remaining module family names.

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
