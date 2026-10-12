## ADR-0005 — Consolidated 0.1 Language Decisions  (Status: Accepted)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1805-1828) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Decision

Context: early decisions were clarified incrementally; consolidated to prevent
drift between human intent, Codex prompts, and implementation.
Non-negotiable rules:
- Inox is post-object-oriented.
- No classes, inheritance, Java-style interfaces, mixins, or duck typing.
- `;` closes blocks and is not a statement terminator.
- `if`/`elif`/`else` do not use `then` or `:` and are closed by one final `;`.
- `Type` is a section/declarator, not a block; no `:` and no closing `;`.
- `TName Struct ... ;` is the canonical struct form.
- Structs declare fields only. Methods are associated outside structs.
- Associated methods use explicit receiver parameters and call-site sugar.
- `repeat` is a general loop; `until Condition` is an internal conditional-exit.
- Compiler stays portable C++20 across Windows/MSVC and Linux/GCC/Clang, future
  Unix portability a goal.
