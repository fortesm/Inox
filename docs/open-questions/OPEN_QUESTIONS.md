# Inox Open Questions

There are no blocking open language questions for the current 0.1 implementation path.

Deferred topics are not open implementation permission. They are intentionally postponed and must not be implemented ad hoc by agents:

- full module/import/export system;
- visibility keywords;
- arrays/vectors/sets concrete syntax;
- full UTF-8 string runtime and indexing semantics;
- struct embedding/composition promotion;
- variant structs;
- JSON/DB field tags;
- contracts/protocols/behaviors;
- final runtime ABI;
- exception lowering.

If work touches any deferred topic, ask for an explicit design decision first and update `docs/canonical/language-reference.md`, the topic-specific canonical document, and tests.
