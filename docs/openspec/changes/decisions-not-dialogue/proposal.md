## Why

The records of Inox (the constitution, the specs, the OpenSpec changes, the
revision log, the history, the roadmap and code comments) quoted the
maintainer's chat messages word for word, in Portuguese, as proof of each
approval. Decided by Marcelo Fortes, 2026-10-29: everything is in English,
and a record states the technical decision, who approved it and when; it
never transcribes the conversation that led to it.

Risk: normal (documents and comments only; no compiler, runtime or language
change).

Status: **implemented** (canon r3.102).

## What Changes

1. **The rule.** G2 and G3 say that a record states the technical decision
   in English, who approved it and when, never a conversation; G7 and G8
   ask for the approval and its date, not a quotation (relocked).
   `docs/openspec/config.yaml`, AGENTS.md, `docs/openspec/README.md` and
   `tools/constitution_check.py` say the same.
2. **Living records.** Every quotation becomes the decision it carried, in
   English, with the maintainer's name and the date: the constitution, the
   capability specs, ROADMAP, OOP_COMPARISON, the open changes, and comments
   in the compiler and in a test. The manual's discard example uses English
   names.
3. **History.** The revision log, `docs/history/canon-changelog.md` and the
   archived changes get the same treatment; every decision and date is kept.

## Not changed

- No decision changes: each record keeps what was decided, by whom and
  when.
- Unicode test data stays (`"água😀"`, `"héllo wörld"`, an argument
  `"ção"`): it tests non-ASCII text and is not prose.
- Identifiers and strings in Portuguese inside test and example programs
  (`TConta`, `FSaldo`, `"sem nome"`) are a separate step, since their
  expected outputs and diagnostics change with them.
