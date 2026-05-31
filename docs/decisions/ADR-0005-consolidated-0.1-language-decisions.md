# ADR-0005: Consolidated Inox 0.1 Language Decisions

Status: Accepted

## Context

During early compiler construction, many syntax and semantic decisions were clarified incrementally. To prevent drift between human intent, Codex prompts, and implementation, these decisions are consolidated in `docs/canonical/language-reference.md` and mirrored by the topic-specific canonical documents.

## Decision

Inox 0.1 follows these non-negotiable rules:

- Inox is post-object-oriented.
- Inox has no classes, inheritance, Java-style interfaces, mixins, or duck typing.
- `;` closes blocks and is not a statement terminator.
- `if`/`elif`/`else` do not use `then` or `:` and are closed by one final `;`.
- `Type` is a section/declarator, not a block; it has no `:` and no closing `;`.
- `TName Struct ... ;` is the canonical struct form.
- Structs declare fields only. Methods are associated outside structs.
- Associated methods use explicit receiver parameters and call-site sugar.
- `repeat` is a general loop; `until Condition` is an internal conditional-exit statement.
- The compiler implementation must remain portable C++20 across Windows/MSVC and Linux/GCC/Clang, with future Unix portability as a design goal.

## Consequences

Agents must update `AGENTS.md`, canonical docs, grammar, examples, and tests whenever a language decision changes. Implementation must not drift toward ObjectPascal, Java, Go, Rust, Python, or C++ defaults unless those defaults have explicitly been accepted for Inox.
