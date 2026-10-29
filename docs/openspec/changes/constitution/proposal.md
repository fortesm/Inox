## Why

`docs/INOX_CANONICAL.md` grew to 4,475 lines: a change log of 1,300 lines,
status sections that went stale (B-WORKS still said Strings were not in
arrays; E12 said the Windows exception gap EH-v3.16a was open, closed in
v3.41), several sections twice (AGENT RULES, DEV-2, GROUP FUTURE, B-WORKS,
B-PARSED, B-GAPS), two sections both called CANON-20, and a migration to
OpenSpec that had moved none of its law yet. `AGENTS.md` repeated the
language rules and had drifted from them (`Var` blocks, `[T]` generics). A
reader without the history of a conversation could not tell law from status
from history, and each session had to be told again what was decided.

Approval (Marcelo Fortes, 2026-10-12): "Ok, estamos combinados, o arquivo
canonica.md pode sobreviver minimizado e devidamente higienizado dentro de
`openspec/specs/constitution/`, Ele deve ser editado por você para refletir
as suas necessidades, e remover todas as incosistências, incoerências e
redundâncias desnecessárias, o motivo? essa janela de sessão pode expirar
você pode perder todo o contexto e precisarei explicar tudo novamente, a
ideia de usarmos o OpenSpec é justamente acabar com esse ciclo interminável
das LLMs perderem o contexto e simplesmente esquecerem tudo que decidimos."
It follows the proposal he agreed to earlier the same day ("estamos
combinados"): the permanent ("pétreo") core lives in OpenSpec, locked by a
hash file and a CI check, changeable only by his approval.

Risk: normal (documentation, tools and CI; no compiler behavior changes).

## What Changes

- New `openspec/specs/constitution/`:
  - `spec.md` — the law, minimized: governance G1–G8, then CANON-1 to
    CANON-23, VERSIONING and FUTURE as requirements with test-named
    scenarios. The anchors keep the canon's identifiers, so every code
    comment that cites CANON-N stays valid (the LLVM-backend section, the
    second CANON-20, becomes CANON-23).
  - `engineering.md` — the engineering directive E1–E20 plus the module
    architecture (E21), backend strategy (E22) and examples, releases and
    documentation (E23) directives.
  - `revisions.md` — current version and revision, the open questions for
    the maintainer, and the revision log from r3.61 on.
  - `constitution.lock` — the hash and approving change of every unit of
    `spec.md` and `engineering.md`.
- `tools/constitution_check.py` (`check`, `relock --approval`, `list`): CI
  fails when locked text changes without an approved relock, and when any
  spec names a test file that does not exist (G5).
- `docs/INOX_CANONICAL.md` is retired: its change log is frozen verbatim in
  `docs/history/canon-changelog.md`, its last text is the git tag
  `canon-final` (to be pushed on the merged commit), and
  `docs/history/CANON_MAP.md` records where each of its 268 units went.
  `tools/canon_trace.py` and its CI step are removed with it.
- Status leaves the law: `docs/STATUS.md` (rewritten against the code of
  today), `docs/ROADMAP.md`; toolchain, packages and the release checklist
  join `BUILD_AND_TEST_INSTRUCTIONS.md`.
- The capability specs in `openspec/specs/language/` stop calling
  themselves mirrors: they are law under the constitution.
- `AGENTS.md` keeps only how to work in this repository and points to the
  constitution for the language; `README.md`, `openspec/README.md`,
  `openspec/config.yaml`, `CHANGELOG.md`, the manual and the grammar header
  point to the constitution.
- Version output and the suite read `revisions.md`; canon revision r3.61.
- Found while reading: the release scripts copied a manual that no longer
  exists and did not ship `libinoxrt`, so packaging failed; fixed.
- Three diagnostics tests for rules that had none: `mut-parameter-reserved`,
  `do-reserved` and `double-is-not-inox`.

Out of scope: any change to the language. Behavior that must not change:
every program (the compiler is not modified, except the canon revision it
prints).
